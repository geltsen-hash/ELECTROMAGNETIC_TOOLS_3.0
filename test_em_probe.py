import http.client
import json
import math
from http.server import HTTPServer
from pathlib import Path
import struct
import tempfile
import threading
import unittest
from unittest.mock import patch
from urllib.parse import urlencode

from scripts import em_probe as probe


class ProbeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="em_probe_test_")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.patcher = patch.object(probe, "SCRIPT_DIR", str(self.root))
        self.patcher.start()
        self.addCleanup(self.patcher.stop)
        self.model = probe.ProbeModel()
        self.source = bytearray(probe.pack_gp_data(320151502, 99, 42.5, 5, 0.6))
        struct.pack_into("<I", self.source, 4, 0x1f1f)
        self.path = self.root / "model_autonom_5Tx.bin"
        self.path.write_bytes(self.source)

    def request(self, command):
        response = probe.handle_command(self.model, probe.build_packet(command, b""))
        parsed = probe.parse_packet(response)
        self.assertEqual(parsed[0], command)
        return parsed[1]

    def check_gp(self, sonde_type, size, serial=17):
        self.model.configure(sonde_type, serial, size)
        signature = size * 1000000 + sonde_type * 1000 + serial
        self.assertEqual(struct.unpack("<I", self.request(probe.CMD_GET_SIGNATURE))[0], signature)
        data = self.request(probe.CMD_GET_DATA)
        self.assertEqual(len(data), size)
        self.assertEqual(struct.unpack_from("<I", data)[0], signature)
        metro = self.request(probe.CMD_GET_METROLOGY)
        self.assertEqual(len(metro), 240)
        self.assertEqual(struct.unpack_from("<II", metro), (signature, serial))
        tx_count = probe.SONDE_TYPES[sonde_type][2]
        for base in (16, 56, 96, 144, 192, 240, 280):
            if base >= size:
                continue
            for frequency in range(2):
                for tx in range(5):
                    offset = base + (frequency * 5 + tx) * 4
                    if tx >= tx_count:
                        self.assertEqual(data[offset:offset + 4], bytes(4))
                    elif base in (96, 144, 192):
                        self.assertEqual(data[offset:offset + 4], self.source[offset:offset + 4])
                    else:
                        value = struct.unpack_from("<f", data, offset)[0]
                        self.assertTrue(math.isfinite(value))
                        if base in (16, 240):
                            self.assertGreaterEqual(value, 0.0)
                        if base == 56:
                            raw = struct.unpack_from("<f", self.source, 192 + (frequency * 5 + tx) * 4)[0]
                            air = struct.unpack_from("<h", metro, 32 + (frequency * 5 + tx) * 2)[0]
                            self.assertAlmostEqual(value, (-1 if tx % 2 else 1) * (raw - air / 57297.0), delta=1e-6)
        mask = (1 << tx_count) - 1
        source_condition = struct.unpack_from("<I", self.source, 4)[0]
        self.assertEqual(struct.unpack_from("<I", data, 4)[0], source_condition & (0xffff0000 | mask | (mask << 8)))
        for offset in (12, 136, 184, 232):
            self.assertEqual(data[offset:offset + 4], self.source[offset:offset + 4])
        self.assertEqual(self.model.model_frames[0], self.source)

    def test_all_gp_profiles(self):
        self.model.load_model(str(self.path))
        for sonde_type in (231, 242, 349, 151, 152):
            for size in (240, 320):
                with self.subTest(sonde_type=sonde_type, size=size):
                    self.check_gp(sonde_type, size)
        self.assertEqual(self.path.read_bytes(), self.source)

    def test_real_models_when_available(self):
        directory = Path(__file__).resolve().parent / "scripts"
        paths = sorted(directory.glob("model_autonom_5Tx*.bin"))
        if not paths:
            self.skipTest("local AUTONOM models not available")
        for path in paths:
            self.model.load_model(str(path))
            self.source = self.model.model_frames[0]
            for sonde_type in (231, 242, 349, 151, 152):
                for size in (240, 320):
                    with self.subTest(model=path.name, sonde_type=sonde_type, size=size):
                        self.check_gp(sonde_type, size)

    def test_loading_preserves_profile_and_wraps(self):
        self.model.configure(231, 999, 240)
        self.model.load_model(str(self.path))
        self.assertEqual((self.model.sonde_type, self.model.serial, self.model.struct_size), (231, 999, 240))
        self.check_gp(231, 240, 999)
        first = self.model.telemetry_frame()
        second = self.model.telemetry_frame()
        self.assertEqual(struct.unpack_from("<I", second, 8)[0], struct.unpack_from("<I", first, 8)[0] + 1)
        self.model.configure(231, 999, 320)
        self.check_gp(231, 320, 999)

    def test_synthetic_profiles(self):
        for sonde_type in (231, 242, 349, 151, 152):
            for size in (240, 320):
                self.model.configure(sonde_type, 4, size)
                frame = self.model.telemetry_frame()
                self.assertEqual(len(frame), size)
                for base in (16, 56, 96, 144, 192, 240, 280):
                    if base >= size:
                        continue
                    for frequency in range(2):
                        for tx in range(self.model.n_tx, 5):
                            self.assertEqual(frame[base + (frequency * 5 + tx) * 4:base + (frequency * 5 + tx) * 4 + 4], bytes(4))

    def test_metrology_change_recalculates_next_frame(self):
        self.model.load_model(str(self.path))
        first = self.request(probe.CMD_GET_DATA)
        metro = bytearray(self.request(probe.CMD_GET_METROLOGY))
        struct.pack_into("<h", metro, 32, 1000)
        struct.pack_into("<f", metro, 84, 2.0)
        response = probe.handle_command(self.model, probe.build_packet(probe.CMD_SET_METROLOGY, metro))
        self.assertEqual(probe.parse_packet(response)[0], probe.CMD_SET_METROLOGY)
        second = self.request(probe.CMD_GET_DATA)
        self.assertNotEqual(first[56:60], second[56:60])
        self.assertNotEqual(first[280:284], second[280:284])
        self.assertAlmostEqual(struct.unpack_from("<f", second, 56)[0],
                               struct.unpack_from("<f", self.source, 192)[0] - 1000 / 57297.0, places=6)
        self.assertEqual(second[96:136], first[96:136])
        self.assertEqual(second[144:184], first[144:184])
        self.assertEqual(second[192:232], first[192:232])
        self.assertEqual(self.model.model_frames[0], self.source)
        self.assertEqual(self.path.read_bytes(), self.source)

    def test_invalid_inputs_return_zero(self):
        for invalid in (0.0, -1.0, float("nan"), float("inf")):
            with self.subTest(amplitude=invalid):
                raw = bytearray(self.source)
                struct.pack_into("<f", raw, 96, invalid)
                frame = self.model._gp_frame(raw)
                self.assertEqual(frame[240:244], bytes(4))
                self.assertEqual(frame[280:284], bytes(4))
        for invalid in (float("nan"), float("inf")):
            raw = bytearray(self.source)
            struct.pack_into("<f", raw, 192, invalid)
            frame = self.model._gp_frame(raw)
            self.assertEqual(frame[16:20], bytes(4))
            self.assertEqual(frame[56:60], bytes(4))
        metro = bytearray(self.model.metrology)
        struct.pack_into("<H", metro, 28, 0)
        self.model.metrology = bytes(metro)
        frame = self.model._gp_frame(self.source)
        self.assertEqual(frame[16:20], bytes(4))
        self.assertEqual(frame[240:244], bytes(4))
        self.model.metrology = None
        frame = self.model._gp_frame(self.source)
        for offset in (16, 56, 240, 280):
            self.assertEqual(frame[offset:offset + 40], bytes(40))

    def test_invalid_configuration_and_model(self):
        original = self.model.signature()
        for profile in ((999, 1, 320), (151, -1, 320), (151, 1000, 320), (151, 1, 241)):
            with self.assertRaises(ValueError):
                self.model.configure(*profile)
            self.assertEqual(self.model.signature(), original)
        self.model.load_model(str(self.path))
        invalid = self.root / "invalid.bin"
        invalid.write_bytes(self.source[:-1])
        with self.assertRaises(ValueError):
            self.model.load_model(str(invalid))
        self.assertEqual(self.model.model_path, str(self.path))

    def test_cartograph_model_blocks(self):
        signature = 3072351502
        raw = probe.pack_all_data(signature, 10, 20, 25.0, 15, 0.3, 0, 45, 90, 2, 1)
        path = self.root / "cartograph.bin"
        path.write_bytes(raw)
        frames, sig = probe.load_model_file(str(path))
        self.assertEqual(sig, signature)
        self.assertEqual(frames, [raw[:512] + raw[1024:2048], raw[512:1024] + raw[2048:]])

    def test_real_cartograph_model(self):
        path = Path(__file__).resolve().parent / "scripts" / "model_cart_raw.bin"
        if not path.is_file():
            self.skipTest("local Cartograph model not available")
        raw = path.read_bytes()
        frames, sig = probe.load_model_file(str(path))
        print("Cartograph model: signature=%d, record_size=%d, bytes=%d" % (sig, sig // 1000000, len(raw)))
        self.assertEqual(len(frames), len(raw) // 3072 * 2)
        self.assertEqual(frames[0], raw[:512] + raw[1024:2048])
        self.assertEqual(frames[1], raw[512:1024] + raw[2048:3072])
        self.model.load_model(str(path))
        first = self.model.telemetry_frame()
        second = self.model.telemetry_frame()
        self.assertEqual(len(first), 1536)
        self.assertEqual(len(second), 1536)
        self.assertEqual(struct.unpack_from("<I", first, 8)[0], 401)
        self.assertEqual(struct.unpack_from("<I", second, 8)[0], 2001)
        self.assertEqual(first[512:], frames[0][512:])
        self.assertEqual(second[512:], frames[1][512:])
        for index in range(0, len(frames), 2):
            calculated = self.model._cartograph_pair(frames[index:index + 2])
            for frequency, block in enumerate(calculated):
                self.assertEqual(block[512:], frames[index + frequency][512:])
                for offset in (12, 28, 44, 60, 480, 496):
                    self.assertTrue(all(math.isfinite(value) for value in struct.unpack_from("<4f", block, offset)))
                self.assertLessEqual(struct.unpack_from("<I", block, 108)[0], 256)
        self.assertEqual(path.read_bytes(), raw)

    def test_cartograph_recalculation_and_packing(self):
        raw = probe.pack_all_data(1536351502, 10, 20, 25.0, 15, 0.3, 0, 45, 90, 2, 1)
        path = self.root / "cartograph.bin"
        path.write_bytes(raw)
        self.model.load_model(str(path))
        before = self.model._cartograph_pair(self.model.model_frames)
        metro = bytearray(self.model.metrology_blob())
        self.assertEqual(len(metro), 1536)
        self.assertEqual(struct.unpack_from("<I", before[0], 108)[0], 0)
        self.assertEqual(struct.unpack_from("<I", before[1], 108)[0], 191)
        self.assertNotEqual(before[1][76:108], bytes(32))
        struct.pack_into("<f", metro, 860, 40.0)
        struct.pack_into("<f", metro, 924, 0.1)
        struct.pack_into("<f", metro, 988, 500.0)
        struct.pack_into("<h", metro, 32, 1000)
        response = probe.handle_command(self.model, probe.build_packet(probe.CMD_SET_METROLOGY, metro))
        self.assertEqual(probe.parse_packet(response)[0], probe.CMD_SET_METROLOGY)
        first = self.model.telemetry_frame()
        second = self.model.telemetry_frame()
        self.assertNotEqual(first[60:64], before[0][60:64])
        self.assertNotEqual(first[124:132], before[0][124:132])
        self.assertNotEqual(first[12:16], before[0][12:16])
        self.assertNotEqual(second[76:108], before[1][76:108])
        for f, frame in enumerate((first, second)):
            source = self.model.model_frames[f]
            for rx in range(2):
                offset = 116 + rx * 128
                self.assertEqual(frame[offset + 40:offset + 128], source[offset + 40:offset + 128])
            self.assertEqual(frame[372:480], source[372:480])
            self.assertEqual(frame[512:], source[512:])
        self.assertEqual(path.read_bytes(), raw)
        for key, expected in ((0x1000fff, (92, 0)), (0x2fff000, (0, 99)), (0, (0, 0))):
            struct.pack_into("<I", metro, 856, key)
            self.model.save_metrology(metro)
            blocks = self.model._cartograph_pair(self.model.model_frames)
            self.assertEqual(tuple(struct.unpack_from("<I", block, 108)[0] for block in blocks), expected)
        self.model.configure(352, 37, 320)
        frame = self.model.telemetry_frame()
        sig = self.model.signature()
        self.assertEqual(sig, 1536352037)
        self.assertEqual(struct.unpack_from("<I", frame)[0], sig)
        self.assertEqual(struct.unpack_from("<II", self.model.metrology_blob()), (sig, 37))

    def test_cartograph_invalid_calibration(self):
        self.model.configure(351, 1, 320)
        raw = [probe.pack_data_block(self.model.signature(), 1, freq, 25, 15, 0.3, 0, 45, 90, 2, 1) + probe.pack_raw_block(0.3)
               for freq in (401, 2001)]
        for value in (0.0, float("nan"), float("inf")):
            metro = bytearray(self.model.metrology_blob())
            struct.pack_into("<f", metro, 860, value)
            self.model.save_metrology(metro)
            frame = self.model._cartograph_pair(raw)[0]
            self.assertEqual(frame[124:132], bytes(8))
            self.assertEqual(frame[12:16], bytes(4))
            self.assertEqual(frame[28:32], bytes(4))
        self.model.metrology = None
        blocks = self.model._cartograph_pair(raw)
        for block in blocks:
            for offset, size in ((12, 64), (76, 36), (124, 32), (252, 32), (480, 32)):
                self.assertEqual(block[offset:offset + size], bytes(size))

    def test_cartograph_dev_and_rejected_models(self):
        signature = 1536351502
        raw = probe.pack_all_data(signature, 10, 20, 25.0, 15, 0.3, 0, 45, 90, 2, 1)
        blocks = [raw[:512] + raw[1024:2048], raw[512:1024] + raw[2048:]]
        path = self.root / "cartograph.dev"
        path.write_bytes(b"p" * 11 + blocks[0] + b"p" * 11 + blocks[1])
        self.assertEqual(probe.load_model_file(str(path))[0], blocks)
        invalid = self.root / "invalid_cart.bin"
        invalid.write_bytes(raw[:-1])
        with self.assertRaises(ValueError):
            probe.load_model_file(str(invalid))
        broken = bytearray(raw)
        struct.pack_into("<I", broken, 520, 401)
        invalid.write_bytes(broken)
        with self.assertRaises(ValueError):
            probe.load_model_file(str(invalid))

    def test_cartograph_synthetic(self):
        for sonde_type in (351, 352):
            self.model.configure(sonde_type, 5, 240)
            data = self.model.telemetry_frame()
            self.assertEqual(len(data), 1536)
            self.assertEqual(struct.unpack_from("<I", data)[0], 3072000000 + sonde_type * 1000 + 5)
        self.model.load_model(str(self.path))
        self.model.configure(351, 5, 320)
        self.assertEqual(len(self.model.telemetry_frame()), 1536)

    def test_http_controls(self):
        self.model.load_model(str(self.path))
        server = HTTPServer(("127.0.0.1", 0), probe.make_remote_handler(self.model))
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            connection = http.client.HTTPConnection("127.0.0.1", server.server_port, timeout=5)
            connection.request("GET", "/")
            response = connection.getresponse()
            page = response.read().decode("utf-8")
            self.assertEqual(response.status, 200)
            self.assertIn('name="struct_size"', page)
            self.assertIn('value="240"', page)
            self.assertIn('value="320" selected', page)
            self.assertNotIn('http-equiv="refresh"', page)
            self.assertNotIn('stat-azimuth', page)
            self.assertNotIn('Азимут:', page)
            self.assertIn("fetch('/stats'", page)
            self.assertIn(".textContent = stats[key]", page)
            connection.request("POST", "/set", urlencode({"sonde_type": 242, "serial": 37, "struct_size": 240}), {"Content-Type": "application/x-www-form-urlencoded"})
            response = connection.getresponse()
            response.read()
            self.assertEqual(response.status, 303)
            self.check_gp(242, 240, 37)
            connection.request("GET", "/stats")
            response = connection.getresponse()
            stats = json.loads(response.read())
            self.assertEqual(response.status, 200)
            self.assertEqual(stats["frame"], self.model.frame400)
            self.assertEqual(stats["frame2000"], self.model.frame2000)
            self.assertIn("log", stats)
            self.assertEqual(stats["power_state"], "питание OK")
            self.assertEqual((self.model.sonde_type, self.model.serial, self.model.struct_size), (242, 37, 240))
            connection.close()
        finally:
            server.shutdown()
            server.server_close()
            thread.join()


if __name__ == "__main__":
    unittest.main()
