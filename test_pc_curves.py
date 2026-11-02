from pathlib import Path
import subprocess
import struct
import re
from unittest.mock import patch

from scripts import em_probe as probe
import tempfile

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "ELECTROMAGNETIC_TOOLS_3.0"

HARNESS = r'''
#include "BinFileReader.cpp"
#include <cassert>
#include <iostream>
namespace reference {
using namespace std;
using namespace std::complex_literals;
__LEGACY__
__CARTOGRAPH__
}

int main(int argc, char** argv) {
    using namespace SondeData;
    if (argc == 3) {
        std::ifstream input(argv[2], std::ios::binary);
        METROLOGY_CARTOGRAPH metro;
        ALL_DATA all;
        size_t frames = 0;
        while (input.read(reinterpret_cast<char*>(&metro), sizeof(metro))) {
            assert(input.read(reinterpret_cast<char*>(&all), sizeof(all)));
            SetMetrologyCartograph(&metro);
            GP_DATA gp = TransformAllDataToGpData(all);
            ALL_DATA expected = all;
            for (int f = 0; f < 2; ++f) {
                reference::calc_geo_signal_smt(&metro, &expected.data[f], f);
                for (int tx = 0; tx < 4; ++tx) {
                    float phase = static_cast<float>(PcCurveValue(CURVE_PHASE_SMT_PC, gp, f, tx) / MG_SCALE);
                    float att = static_cast<float>(PcCurveValue(CURVE_ATT_SMT_PC, gp, f, tx));
                    float rhoPh = static_cast<float>(PcCurveValue(CURVE_RHO_PHASE_PC, gp, f, tx));
                    float rhoAtt = static_cast<float>(PcCurveValue(CURVE_RHO_ATT_PC, gp, f, tx));
                    assert(std::abs(all.data[f].phase_smt[tx] - phase) < 1e-6);
                    assert(std::abs(all.data[f].att_dB_smt[tx] - att) < 0.0001);
                    assert(std::abs(all.data[f].rho_ph_smt[tx] - rhoPh) < std::max(0.05f, std::abs(rhoPh) * 0.005f));
                    assert(std::abs(all.data[f].rho_att_smt[tx] - rhoAtt) < std::max(0.05f, std::abs(rhoAtt) * 0.005f));
                    for (int rx = 0; rx < 2; ++rx)
                        assert(std::abs(all.data[f].R_zx[rx].Geo[tx] - expected.data[f].R_zx[rx].Geo[tx]) < 0.0001f);
                    assert(std::abs(all.data[f].ATT_dB_geo_signal_smt[tx] - expected.data[f].ATT_dB_geo_signal_smt[tx]) < 0.001f);
                    assert(std::abs(all.data[f].PH_deg_geo_signal_smt[tx] - expected.data[f].PH_deg_geo_signal_smt[tx]) < 0.001f);
                    expected.data[f].rho_ph_smt[tx] = rhoPh;
                }
                expected.data[f].all_bit_cntr = 0;
            }
            reference::OutCompressedData(expected.data, &metro);
            for (int f = 0; f < 2; ++f) {
                if (std::memcmp(all.data[f].out_arr, expected.data[f].out_arr, 32) != 0) {
                    std::cerr << "Packed mismatch frame=" << frames << " frequency=" << f << std::endl;
                    return 1;
                }
                assert(all.data[f].all_bit_cntr == expected.data[f].all_bit_cntr);
            }
            ++frames;
        }
        assert(frames > 0);
        std::cout << "Cartograph vs native PC/firmware: " << frames << " frames PASS\n";
        return 0;
    }
    if (argc == 2) {
        std::ifstream input(argv[1], std::ios::binary);
        METROLOGY_GP metro;
        GP_DATA gp;
        size_t frames = 0;
        while (input.read(reinterpret_cast<char*>(&metro), sizeof(metro))) {
            assert(input.read(reinterpret_cast<char*>(&gp), sizeof(gp)));
            SetMetrologyGP(&metro);
            SondeID id = SondeID::FromSignature(gp.signature);
            for (int f = 0; f < 2; ++f) {
                for (int tx = 0; tx < static_cast<int>(id.N_Tx); ++tx) {
                    for (int type = CURVE_PHASE_SMT_PC; type <= CURVE_RHO_ATT_PC; ++type) {
                        if (id.struct_size == 240 && type >= CURVE_ATT_SMT_PC) continue;
                        double expected = PcCurveValue(type, gp, f, tx);
                        double actual = type == CURVE_PHASE_SMT_PC ? gp.phase_smt[f][tx] * MG_SCALE :
                            type == CURVE_RHO_PHASE_PC ? gp.rho_ph_smt[f][tx] :
                            type == CURVE_ATT_SMT_PC ? gp.att_dB_smt[f][tx] : gp.rho_att_smt[f][tx];
                        double tolerance = type == CURVE_PHASE_SMT_PC ? 0.01 :
                            type == CURVE_ATT_SMT_PC ? 0.0001 : std::max(0.05, std::abs(expected) * 0.005);
                        if (std::isfinite(expected)) {
                            if (std::abs(actual - expected) > tolerance) {
                                std::cerr << "Mismatch frame=" << frames << " f=" << f << " tx=" << tx
                                    << " type=" << type << " expected=" << expected << " actual=" << actual << std::endl;
                                return 1;
                            }
                        } else assert(actual == 0.0);
                    }
                }
            }
            ++frames;
        }
        assert(frames > 0);
        std::cout << "Emulator vs native PC: " << frames << " frames PASS\n";
        return 0;
    }
    for (int sonde : {231, 242, 151}) {
        BeginLiveSession();
        GP_DATA gp = {};
        gp.signature = 320000000 + sonde * 1000 + 1;
        METROLOGY_GP metro = {};
        metro.signature = gp.signature;
        metro.F[0] = 400;
        metro.F[1] = 2000;
        for (int tx = 0; tx < 5; ++tx) {
            metro.L1[tx] = 50 + tx * 10;
            metro.L2[tx] = 100 + tx * 10;
            for (int f = 0; f < 2; ++f) {
                metro.air_ph[f][tx] = 120 + tx * 10;
                metro.air_att_dB[f][tx] = 0.15f;
                float sign = tx % 2 == 0 ? 1.0f : -1.0f;
                auto signal = PcSignal(metro, tx, f, 20.0f);
                gp.DELTA_PH[f][tx] = sign * std::arg(signal) + metro.air_ph[f][tx] / 57297.0f;
                gp.AM_RX_1[f][tx] = 100.0f;
                gp.AM_RX_2[f][tx] = 100.0f * std::pow(10.0f,
                    sign * (20.0f * std::log10(std::abs(signal)) + metro.air_att_dB[f][tx]) / 20.0f);
                gp.phase_smt[f][tx] = 0.123f;
                gp.att_dB_smt[f][tx] = 3.5f;
                gp.rho_ph_smt[f][tx] = 123.0f;
                gp.rho_att_smt[f][tx] = 456.0f;
            }
        }
        assert(AppendLiveRecord(reinterpret_cast<const uint8_t*>(&gp), sizeof(gp)) == 1);
        ClearMetrology();
        for (int type = CURVE_PHASE_SMT_PC; type <= CURVE_RHO_ATT_PC; ++type)
            assert(std::isnan(GetCurveValue(type, 0, 0, 0)));
        assert(GetCurveValue(CURVE_RHO_PHASE, 0, 0, 0) == 123.0);
        assert(GetCurveValue(CURVE_RHO_ATT, 0, 0, 0) == 456.0);
        SetMetrologyGP(&metro);
        for (int f = 0; f < 2; ++f) {
            for (int tx = 0; tx < GetTxCount(); ++tx) {
                auto signal = PcSignal(metro, tx, f, 20.0f);
                assert(std::abs(GetCurveValue(CURVE_PHASE_SMT_PC, f, tx, 0) - std::arg(signal) * MG_SCALE) < 0.01);
                assert(std::abs(GetCurveValue(CURVE_ATT_SMT_PC, f, tx, 0) - 20.0f * std::log10(std::abs(signal))) < 0.0001);
                GP_DATA expected = gp;
                float sign = tx % 2 == 0 ? 1.0f : -1.0f;
                expected.phase_smt[f][tx] = sign * (gp.DELTA_PH[f][tx] - metro.air_ph[f][tx] / 57297.0f);
                expected.att_dB_smt[f][tx] = sign * (20.0f * std::log10(gp.AM_RX_2[f][tx] / gp.AM_RX_1[f][tx])) - metro.air_att_dB[f][tx];
                reference::RO_ARG(&metro, &expected, tx, f);
                reference::RO_AMP(&metro, &expected, tx, f);
                assert(std::abs(GetCurveValue(CURVE_RHO_PHASE_PC, f, tx, 0) - expected.rho_ph_smt[f][tx]) < 0.001);
                assert(std::abs(GetCurveValue(CURVE_RHO_ATT_PC, f, tx, 0) - expected.rho_att_smt[f][tx]) < 0.001);
                assert(GetCurveValue(CURVE_RHO_PHASE, f, tx, 0) == 123.0);
                assert(GetCurveValue(CURVE_RHO_ATT, f, tx, 0) == 456.0);
                assert(GetCurveValue(CURVE_ATT_SMT, f, tx, 0) == 3.5);
            }
        }
        for (float rho : {0.01f, 0.1f, 1.0f, 20.0f, 500.0f, 1000.0f, 7000.0f, 10000.0f}) {
            GP_DATA sample = gp;
            auto signal = reference::SIGNAL(&metro, 0, 1, rho);
            sample.DELTA_PH[1][0] = std::arg(signal) + metro.air_ph[1][0] / 57297.0f;
            sample.AM_RX_2[1][0] = sample.AM_RX_1[1][0] * std::pow(10.0f,
                (20.0f * std::log10(std::abs(signal)) + metro.air_att_dB[1][0]) / 20.0f);
            sample.phase_smt[1][0] = sample.DELTA_PH[1][0] - metro.air_ph[1][0] / 57297.0f;
            sample.att_dB_smt[1][0] = 20.0f * std::log10(sample.AM_RX_2[1][0] / sample.AM_RX_1[1][0]) - metro.air_att_dB[1][0];
            reference::RO_ARG(&metro, &sample, 0, 1);
            reference::RO_AMP(&metro, &sample, 0, 1);
            assert(std::abs(PcCurveValue(CURVE_RHO_PHASE_PC, sample, 1, 0) - sample.rho_ph_smt[1][0]) < 0.001);
            assert(std::abs(PcCurveValue(CURVE_RHO_ATT_PC, sample, 1, 0) - sample.rho_att_smt[1][0]) < 0.001);
        }
        GP_DATA invalid = gp;
        invalid.AM_RX_1[0][0] = 0;
        assert(std::isnan(PcCurveValue(CURVE_ATT_SMT_PC, invalid, 0, 0)));
        assert(std::isnan(PcCurveValue(CURVE_RHO_ATT_PC, invalid, 0, 0)));
        const uint8_t* bytes = nullptr;
        size_t length = 0;
        assert(GetLastRecordBytes(&bytes, &length));
        assert(length == sizeof(gp) && std::memcmp(bytes, &gp, sizeof(gp)) == 0);
        metro.air_ph[0][0] += 100;
        SetMetrologyGP(&metro);
        double updated = GetCurveValue(CURVE_PHASE_SMT_PC, 0, 0, 0);
        assert(std::abs(updated - (gp.DELTA_PH[0][0] - metro.air_ph[0][0] / 57297.0f) * MG_SCALE) < 0.01);
        metro.F[0] = 0;
        SetMetrologyGP(&metro);
        assert(std::isnan(GetCurveValue(CURVE_RHO_PHASE_PC, 0, 0, 0)));
        ClearMetrology();
        assert(std::isnan(GetCurveValue(CURVE_PHASE_SMT_PC, 0, 0, 0)));
    }
    BeginLiveSession();
    ALL_DATA cart = {};
    for (int f = 0; f < 2; ++f) {
        DATA& data = cart.data[f];
        data.signature = 1536351001;
        data.dds_freq = f == 0 ? 401 : 2001;
        data.frame = 1;
        for (int tx = 0; tx < 4; ++tx) {
            data.ATT_dB_geo_signal_smt[tx] = -3.0f - tx;
            data.PH_deg_geo_signal_smt[tx] = 2.0f + tx;
            for (int rx = 0; rx < 2; ++rx) {
                direct_RX& receiver = data.R_zx[rx];
                receiver.Tx_0 = std::complex<float>(10.0f + rx * 10, 0);
                receiver.Geo[tx] = std::complex<float>(1.0f + rx + tx, 0.5f + f);
                receiver.amp_Vzx[tx] = 4.0f + rx * 3 + tx;
                receiver.amp_Vzz[tx] = 10.0f + rx * 20 + tx;
                receiver.ph_Vzx_Vzz[tx] = 0.1f + rx * 0.2f + tx * 0.01f;
                receiver.dv[tx] = 4.0f + rx;
                receiver.border_angle[tx] = 0.3f + rx * 0.2f;
            }
        }
        CARTOGRAF_DATA_1FREQ block = {};
        block.data = data;
        assert(AppendLiveRecord(reinterpret_cast<const uint8_t*>(&block), sizeof(block)) == static_cast<size_t>(f));
    }
    for (int f = 0; f < 2; ++f) {
        for (int tx = 0; tx < 4; ++tx) {
            for (int rx = 0; rx < 2; ++rx) {
                const direct_RX& receiver = cart.data[f].R_zx[rx];
                assert(GetCurveValue(CURVE_GEO_AM, f, tx, 0, rx) == std::abs(receiver.Geo[tx]));
                assert(GetCurveValue(CURVE_GEO_PH, f, tx, 0, rx) == std::arg(receiver.Geo[tx]) * MG_SCALE);
                assert(GetCurveValue(CURVE_V_ZX, f, tx, 0, rx) == receiver.amp_Vzx[tx]);
                assert(GetCurveValue(CURVE_V_ZZ, f, tx, 0, rx) == receiver.amp_Vzz[tx]);
                assert(GetCurveValue(CURVE_RATIO_ZX_ZZ, f, tx, 0, rx) == receiver.ph_Vzx_Vzz[tx] * MG_SCALE);
                assert(GetCurveValue(CURVE_DV_NOISE, f, tx, 0, rx) == receiver.dv[tx]);
                assert(GetCurveValue(CURVE_BORDER_ANGLE, f, tx, 0, rx) == receiver.border_angle[tx] * GRAD_SCALE);
                assert(GetCurveValue(CURVE_GEO_AM_SMT, f, tx, 0, rx) == cart.data[f].ATT_dB_geo_signal_smt[tx]);
                assert(GetCurveValue(CURVE_GEO_PH_SMT, f, tx, 0, rx) == cart.data[f].PH_deg_geo_signal_smt[tx]);
            }
            assert(GetCurveValue(CURVE_V_ZX, f, tx, 0) == GetCurveValue(CURVE_V_ZX, f, tx, 0, 0));
            assert(GetCurveValue(CURVE_V_ZX, f, tx, 0, 2) == GetCurveValue(CURVE_V_ZX, f, tx, 0, 0));
        }
    }
    std::cout << "Receiver selection: both frequencies, L/R fields, shared SMT, phase field PASS\n";
    std::cout << "PC curves: 3Tx/4Tx/5Tx, both frequencies, Sonde isolation, missing metrology: PASS\n";
}
'''


def emulator_samples(work):
    sources = [probe.pack_gp_data(320151502, 1, 25.0, 5, 0.1)]
    for path in sorted((ROOT / "scripts").glob("model_autonom_5Tx*.bin")):
        frames, _ = probe.load_model_file(str(path))
        sources.extend(frames[index] for index in (0, len(frames) // 2, len(frames) - 1))
    samples = bytearray()
    with patch.object(probe, "SCRIPT_DIR", str(work)):
        model = probe.ProbeModel()
        for changed in (False, True, None):
            metro = bytearray(probe.pack_metrology_gp(320151502, 502))
            local_metro = ROOT / "scripts" / "metro_autonom_5Tx.bin"
            if changed is None and local_metro.is_file():
                metro = bytearray(local_metro.read_bytes())
                assert len(metro) == 240
            for tx in (range(5) if changed is not None else ()):
                struct.pack_into("<H", metro, 8 + tx * 2, 50 + tx * 10)
                struct.pack_into("<H", metro, 18 + tx * 2, 100 + tx * 10)
                for f in range(2):
                    struct.pack_into("<h", metro, 32 + (f * 5 + tx) * 2, 500 if changed else 0)
                    struct.pack_into("<f", metro, 84 + (f * 5 + tx) * 4, 0.5 if changed else 0.0)
            for sonde in (231, 242, 349, 151, 152):
                for size in (240, 320):
                    model.configure(sonde, 502, size)
                    model.save_metrology(metro)
                    for raw in sources:
                        frame = model._gp_frame(raw)
                        samples.extend(model.metrology_blob())
                        samples.extend(frame.ljust(320, b"\0"))
    path = work / "emulator_samples.bin"
    path.write_bytes(samples)
    return path


def cartograph_reference():
    firmware = ROOT.parent / "TEST_SONDE_KARTOGRAF_2.0" / "CARTOGRAF_FIRMWARE"
    rho = (firmware / "RHO_calc.c").read_text(encoding="cp1251")
    geo = rho[rho.index("void calc_geo_signal_smt("):]
    geo = geo.replace("complex float", "std::complex<float>").replace("struct ALLDATA", "struct DATA")
    geo = geo.replace("cabsf(", "std::abs(").replace("cargf(", "std::arg(")
    geo = re.sub(r"\bI\b", "std::complex<float>(0.0f, 1.0f)", geo)
    source = (firmware / "data_compress.c").read_text(encoding="cp1251")
    ro = source[source.index("uint16_t ro_to_bits_code("):source.index("/*void set_mean_bits")]
    compression = source[source.index("void compress("):source.index("void transform_data(")]
    compression = compression.replace("struct ALLDATA", "struct DATA")
    compression = compression.replace("memset((uint16_t*)&data[0].out_arr[0], 0x00, 16)", "memset(data[0].out_arr, 0, 32)")
    compression = compression.replace("memset((uint16_t*)&data[1].out_arr[0], 0x00, 16)", "memset(data[1].out_arr, 0, 32)")
    compression = compression.replace("sizeof(struct COMPRESSED_1freq) - 4", "sizeof(struct COMPRESSED_1freq) - 8")
    compression = compression.replace("data[write_arr_freq_idx].out_arr[byte_cntr]", "reinterpret_cast<uint16_t*>(data[write_arr_freq_idx].out_arr)[byte_cntr]")
    declarations = "struct PACKED { uint16_t data; uint16_t bits; };\n"
    declarations += "struct COMPRESSED_1freq { uint32_t frame; uint32_t dds_freq; PACKED GA[4], GP[4], Ro[4]; };\n"
    declarations += "struct TO_PACK { PACKED G[24]; };\n"
    declarations += "enum { WORK_400=24, WORK_2000=25, _400_kGz=0, _2000_kGz=1 };\n"
    return declarations + geo + ro + compression


def cartograph_samples(work):
    raw = probe.pack_all_data(1536351502, 1, 1, 25, 15, 0.3, 0, 45, 90, 2, 1)
    sources = [[raw[:512] + raw[1024:2048], raw[512:1024] + raw[2048:]]]
    model_path = ROOT / "scripts" / "model_cart_raw.bin"
    if model_path.is_file():
        frames, _ = probe.load_model_file(str(model_path))
        for index in (0, len(frames) // 4 * 2, len(frames) - 2):
            sources.append(frames[index:index + 2])
    samples = bytearray()
    with patch.object(probe, "SCRIPT_DIR", str(work)):
        model = probe.ProbeModel(351, 502)
        model.model_frames = sources[0]
        model.model_sig = 1536351502
        for changed in (False, True):
            metro = bytearray(probe.pack_metrology_cartograph(model.signature(), 502))
            if changed:
                for rx in range(2):
                    for freq in range(2):
                        for tx in range(4):
                            index = (rx * 2 + freq) * 4 + tx
                            struct.pack_into("<f", metro, 860 + index * 4, 40 + rx + tx)
                            struct.pack_into("<f", metro, 924 + index * 4, 0.02 + rx * 0.01)
                            struct.pack_into("<f", metro, 988 + index * 4, 300 + rx * 100)
                struct.pack_into("<h", metro, 32, 1000)
            for key in (0x3ffffff, 0x1000fff, 0x2fff000):
                struct.pack_into("<I", metro, 856, key)
                model.save_metrology(metro)
                for source in sources:
                    for sonde_type in (351, 352):
                        model.configure(sonde_type, 502, 320)
                        model.save_metrology(metro)
                        blocks = model._cartograph_pair(source)
                        samples.extend(model.metrology_blob())
                        samples.extend(blocks[0][:512] + blocks[1][:512] + blocks[0][512:] + blocks[1][512:])
    path = work / "cartograph_samples.bin"
    path.write_bytes(samples)
    return path


def main():
    devcmd = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat")
    if not devcmd.is_file():
        raise SystemExit("Visual Studio developer tools not found")
    with tempfile.TemporaryDirectory(prefix="em_pc_curves_", ignore_cleanup_errors=True) as directory:
        work = Path(directory)
        legacy_path = ROOT.parent / "TEST_SONDE_KARTOGRAF_2.0" / "AIR_TEMP_1.0" / "variable.cpp"
        legacy = legacy_path.read_text(encoding="utf-8-sig")
        start = legacy.index("complex<float> SIGNAL(")
        end = legacy.index("// определяет работоспособность", start)
        harness = HARNESS.replace("__LEGACY__", legacy[start:end]).replace("__CARTOGRAPH__", cartograph_reference())
        (work / "test.cpp").write_text(harness, encoding="utf-8")
        command = f'call "{devcmd}" -arch=x64 >nul && cl /nologo /EHsc /std:c++17 /utf-8 /I"{SOURCE}" test.cpp /Fe:test.exe'
        build = subprocess.run(command, shell=True, cwd=work, capture_output=True)
        if build.returncode:
            print(build.stdout.decode(errors="replace"))
            print(build.stderr.decode(errors="replace"))
            raise SystemExit(build.returncode)
        subprocess.run([str(work / "test.exe")], cwd=work, check=True)
        subprocess.run([str(work / "test.exe"), str(emulator_samples(work))], cwd=work, check=True)
        subprocess.run([str(work / "test.exe"), "--cartograph", str(cartograph_samples(work))], cwd=work, check=True)


if __name__ == "__main__":
    main()
