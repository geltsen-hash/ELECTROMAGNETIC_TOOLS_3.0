import argparse
import csv
import math
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent


def gp_columns(tx_count):
    columns = []
    def add(name, offset, fmt="f", available=True):
        columns.append((name, offset, fmt, available))
    for name, offset, fmt in [("signature", 0, "I"), ("condition", 4, "I"), ("frame", 8, "I"), ("temperature_c", 12, "f")]:
        add(name, offset, fmt)
    arrays = [("rho_ph_smt", 16), ("phase_smt", 56), ("AM_RX_1", 96),
              ("AM_RX_2", 144), ("DELTA_PH", 192), ("rho_att_smt", 240), ("att_dB_smt", 280)]
    for f, freq in enumerate((400, 2000)):
        p = f"f{freq}_"
        for name, offset in [("ZERO_AM_RX_1", 136), ("ZERO_AM_RX_2", 184), ("ZERO_dPH", 232)]:
            add(p + name, offset + f * 4)
        for tx in range(5):
            for name, offset in arrays:
                add(f"{p}tx{tx + 1}_{name}", offset + (f * 5 + tx) * 4, available=tx < tx_count)
    return columns


def cartograph_columns():
    columns = []
    def add(name, offset, fmt="f"):
        columns.append((name, offset, fmt, True))
    def complex_value(name, offset):
        add(name + "_re", offset)
        add(name + "_im", offset + 4)
    for f, freq in enumerate((400, 2000)):
        p, base = f"f{freq}_", f * 512
        for name, offset in [("signature", 0), ("frame", 4), ("dds_freq_khz", 8)]:
            add(p + name, base + offset, "I")
        for tx in range(4):
            t = f"{p}tx{tx + 1}_"
            for name, offset in [("ATT_dB_geo_signal_smt", 12), ("PH_deg_geo_signal_smt", 28),
                                 ("rho_ph_smt", 44), ("phase_smt", 60), ("rho_att_smt", 480), ("att_dB_smt", 496)]:
                add(t + name, base + offset + tx * 4)
        for i in range(32):
            add(p + f"out_arr_{i}", base + 76 + i, "B")
        add(p + "all_bit_cntr", base + 108, "I")
        add(p + "depth", base + 112, "I")
        for rx, rx_name in enumerate(("rx_l_", "rx_r_")):
            r, rb = p + rx_name, base + 116 + rx * 128
            complex_value(r + "Tx_0", rb)
            add(r + "temperature_c", rb + 120)
            add(r + "condition", rb + 124, "I")
            for tx in range(4):
                t = r + f"tx{tx + 1}_"
                complex_value(t + "Geo", rb + 8 + tx * 8)
                for name, offset in [("amp_Vzz", 40), ("amp_Vzx", 56), ("ph_Vzx_Vzz", 72), ("dv", 88), ("border_angle", 104)]:
                    add(t + name, rb + offset + tx * 4)
                for component, suffix in enumerate(("re", "im")):
                    for sector in range(16):
                        add(t + f"sector_{sector}_{suffix}", 1024 + f * 1024 + rx * 512 + tx * 128 + component * 64 + sector * 4)
        for i in range(2):
            complex_value(p + f"rzz_Tx_0_{i}", base + 372 + i * 8)
        for tx in range(4):
            complex_value(p + f"tx{tx + 1}_Rzz1", base + 388 + tx * 8)
            complex_value(p + f"tx{tx + 1}_Rzz2", base + 420 + tx * 8)
        add(p + "rzz_temperature_c", base + 452)
        add(p + "rzz_condition", base + 456, "I")
        for i, name in enumerate(("INC_ABS_deg", "INC_ZEN_deg", "INC_AZM_deg")):
            add(p + name, base + 460 + i * 4)
        add(p + "start_sector", base + 472, "H")
        add(p + "DataValid", base + 474, "H")
        add(p + "Wg_rpm", base + 476)
    return columns


def fixture(columns, size, sonde_type, index):
    data = bytearray(size)
    signature = size * 1000000 + sonde_type * 1000 + 17
    for name, offset, fmt, _ in columns:
        if offset + struct.calcsize(fmt) > size:
            continue
        value = (-1 if offset % 8 else 1) * (offset + index + 0.125) if fmt == "f" else (offset + index) % (1 << (8 * struct.calcsize(fmt)))
        if name.endswith("signature"):
            value = signature
        if name.endswith("temperature_c"):
            value = (10, 30, 10)[index % 3]
        if name.endswith("frame"):
            value = 100 + index
        if name.endswith("condition"):
            value = 0xF0000000 + index
        struct.pack_into("<" + fmt, data, offset, value)
    return bytes(data)


def check_rows(path, raw_frames, columns, indices, sonde_type, tx_count, cartograph):
    size = len(raw_frames[0])
    covered = [b for _, offset, fmt, _ in columns for b in range(offset, offset + struct.calcsize(fmt))]
    assert len(covered) == len(set(covered)), "Duplicate binary field"
    assert set(covered) == set(range(3072 if cartograph else 320)), "Unexported binary field"
    with path.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream)
        expected_header = ["schema", "row_index", "sonde_type", "sonde_number", "tx_count", "record_bytes"] + [c[0] for c in columns]
        assert reader.fieldnames == expected_header, "CSV header mismatch"
        rows = list(reader)
    assert len(rows) == len(indices), (len(rows), len(indices))
    for row, index in zip(rows, indices):
        assert row["schema"] == ("em_cartograph_v1" if cartograph else "em_gp_v1")
        assert int(row["row_index"]) == index
        assert int(row["sonde_type"]) == sonde_type
        assert int(row["sonde_number"]) == struct.unpack_from("<I", raw_frames[index])[0] % 1000
        assert int(row["tx_count"]) == tx_count
        assert int(row["record_bytes"]) == size
        for name, offset, fmt, available in columns:
            if not available or offset + struct.calcsize(fmt) > size:
                assert row[name] == "", name
                continue
            expected = struct.unpack_from("<" + fmt, raw_frames[index], offset)[0]
            if fmt == "f":
                actual = float(row[name])
                assert math.isnan(actual) if math.isnan(expected) else struct.pack("<f", actual) == struct.pack("<f", expected), name
            else:
                assert int(row[name]) == expected, name


def main():
    parser = argparse.ArgumentParser(description="CSV regression checks against binary field offsets")
    parser.add_argument("--config", choices=("Release", "Debug"), default="Release")
    args = parser.parse_args()
    exe = ROOT / "x64" / args.config / "ELECTROMAGNETIC_TOOLS_3.0.exe"
    checks = 0
    with tempfile.TemporaryDirectory(prefix="em_csv_") as directory:
        directory = Path(directory)
        def run(source, destination, *options, success=True):
            result = subprocess.run([str(exe), "--export-csv", str(source), str(destination), *map(str, options)],
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
            output = result.stdout.decode("utf-8", errors="replace")
            assert (result.returncode == 0) == success, output
            return output
        for sonde_type, tx_count, size in [(151, 5, 320), (152, 5, 320), (349, 4, 320), (242, 4, 320), (231, 3, 320), (151, 5, 240), (351, 4, 3072), (352, 4, 3072)]:
            cartograph = size == 3072
            columns = cartograph_columns() if cartograph else gp_columns(tx_count)
            frames = [fixture(columns, size, sonde_type, i) for i in range(3)]
            source = directory / f"{sonde_type}_{size}.bin"
            source.write_bytes(b"".join(frames))
            for label, options, indices in [("all", (), [0, 1, 2]), ("range", ("--frames", 1, 1), [1]), ("temperature", ("--temperature", 9, 11), [0, 2])]:
                destination = directory / f"{source.stem}_{label}_данные.csv"
                run(source, destination, *options)
                check_rows(destination, frames, columns, indices, sonde_type, tx_count, cartograph)
                checks += 1
            existing = directory / f"{source.stem}_all_данные.csv"
            before = existing.read_bytes()
            run(source, existing, success=False)
            assert existing.read_bytes() == before
            run(source, source, success=False)
            assert source.read_bytes() == b"".join(frames)
            for n, options in enumerate([("--frames", -1, 1), ("--frames", 2, 1), ("--frames", 0, 3), ("--frames", 0.5, 1), ("--temperature", 80, 90), ("--temperature", "NaN", 30)]):
                destination = directory / f"bad_{sonde_type}_{size}_{n}.csv"
                run(source, destination, *options, success=False)
                assert not destination.exists()
                checks += 1
            dev = directory / f"{source.stem}.DEV"
            if cartograph:
                dev.write_bytes(b"".join(b"\0" * 11 + frame[:512] + frame[1024:2048] + b"\0" * 11 + frame[512:1024] + frame[2048:] for frame in frames))
            else:
                dev.write_bytes(b"".join(b"\0" * 11 + frame for frame in frames))
            destination = dev.with_suffix(".csv")
            run(dev, destination)
            check_rows(destination, frames, columns, [0, 1, 2], sonde_type, tx_count, cartograph)
            checks += 3
            for extension, valid_data in (("bin", source.read_bytes()), ("DEV", dev.read_bytes())):
                stride = size if extension == "bin" else (3094 if cartograph else size + 11)
                signature_offset = stride + (11 if extension == "DEV" else 0)
                signature = struct.unpack_from("<I", valid_data, signature_offset)[0]
                mutations = []
                for increment in (1, 1000000):
                    damaged = bytearray(valid_data)
                    struct.pack_into("<I", damaged, signature_offset, signature + increment)
                    mutations.append(damaged)
                if cartograph:
                    damaged = bytearray(valid_data)
                    second_frequency = 512 if extension == "bin" else 1558
                    struct.pack_into("<I", damaged, second_frequency, signature + 1)
                    mutations.append(damaged)
                mutations.extend((valid_data[:-1], valid_data + b"\0"))
                for i, damaged in enumerate(mutations):
                    bad_source = directory / f"mixed_{sonde_type}_{size}_{i}.{extension}"
                    bad_destination = directory / f"mixed_{sonde_type}_{size}_{i}_{extension}.csv"
                    bad_source.write_bytes(damaged)
                    output = run(bad_source, bad_destination, success=False)
                    if i < len(mutations) - 2:
                        assert "Signature" in output and "смещение" in output and str(signature) in output, output
                    else:
                        assert "не кратен размеру записи" in output, output
                    assert not bad_destination.exists()
                    checks += 1
        for sonde_type, tx_count in [(151, 5), (152, 5), (349, 4), (242, 4), (231, 3)]:
            columns = gp_columns(tx_count)
            frames = [struct.pack("<I", sonde_type * 1000 + 17) + fixture(columns, 240, sonde_type, i)[4:] for i in range(3)]
            for extension in ("bin", "DEV"):
                source = directory / f"legacy_{sonde_type}.{extension}"
                source.write_bytes(b"".join((b"\0" * 11 if extension == "DEV" else b"") + frame for frame in frames))
                destination = directory / f"legacy_{sonde_type}_{extension}.csv"
                run(source, destination)
                check_rows(destination, frames, columns, [0, 1, 2], sonde_type, tx_count, False)
                checks += 1
        for n, value in enumerate((float("nan"), float("inf"), -float("inf"))):
            columns = gp_columns(5)
            frame = bytearray(fixture(columns, 320, 151, 0))
            struct.pack_into("<f", frame, 16, value)
            source, destination = directory / f"nonfinite_{n}.bin", directory / f"nonfinite_{n}.csv"
            source.write_bytes(frame)
            run(source, destination)
            check_rows(destination, [frame], columns, [0], 151, 5, False)
            checks += 1
        for n, payload in enumerate((b"", b"\0", struct.pack("<I", 324151017) + b"\0" * 320)):
            source, destination = directory / f"invalid_{n}.bin", directory / f"invalid_{n}.csv"
            source.write_bytes(payload)
            run(source, destination, success=False)
            assert not destination.exists()
            checks += 1
        for n, path in enumerate([Path(r"C:\EXP\CARTOGRAF\Dataout_cartograf_2_03_12_25.bin"), Path(r"C:\EXP\AUTONOM_5Tx_TEST\Dataout_AUTONOM_5Tx.bin")]):
            if not path.is_file():
                print(f"[CSV SKIP] Test data unavailable: {path}")
                continue
            raw = path.read_bytes()
            signature = struct.unpack_from("<I", raw)[0]
            sonde_type = signature % 1000000 // 1000
            cartograph = sonde_type in (351, 352)
            size = 3072 if cartograph else (signature // 1000000 or 240)
            tx_count = 4 if cartograph or sonde_type in (349, 242) else 3 if sonde_type == 231 else 5
            frames = [raw[i:i + size] for i in range(0, len(raw) - size + 1, size)]
            assert len(frames) == (3212 if n == 0 else 10004), "Unexpected reference frame count"
            columns = cartograph_columns() if cartograph else gp_columns(tx_count)
            destination = directory / f"real_{n}.csv"
            run(path, destination)
            check_rows(destination, frames, columns, list(range(len(frames))), sonde_type, tx_count, cartograph)
            print(f"[CSV DATA OK] {path.name}: {len(frames)} frames, {len(columns) + 6} columns")
            checks += 1
    print(f"[CSV TEST SUCCESS] {args.config}: {checks} checks, all binary fields verified")


if __name__ == "__main__":
    main()
