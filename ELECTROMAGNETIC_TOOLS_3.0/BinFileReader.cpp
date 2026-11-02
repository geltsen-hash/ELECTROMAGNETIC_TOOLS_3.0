#include "BinFileReader.h"
#include "SondeDataTypes.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include <iomanip>
#include <sstream>
#include <locale>
#include <limits>
#include <utility>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>

namespace {
    LogSessionData g_session;

    // Метрология прибора (аналог metrology_GP_struct / metrology_cartograf_struct)
    METROLOGY_GP g_metroGP = {};
    METROLOGY_CARTOGRAPH g_metroCart = {};
    bool g_metroLoaded = false;
    bool g_metroCartograph = false;

    std::complex<float> PcSignal(const METROLOGY_GP& metro, int tx, int freq, float rho) {
        float l1 = metro.L1[tx] / 1000.0f;
        float l2 = metro.L2[tx] / 1000.0f;
        std::complex<float> imaginary(0.0f, 1.0f);
        auto ik = imaginary * std::sqrt(imaginary * (0.0078957f * metro.F[freq] / rho));
        return std::exp(ik * (l2 - l1)) * ((1.0f - ik * l2) / (1.0f - ik * l1));
    }

    double PcCurveValue(int type, const GP_DATA& gp, int freq, int tx) {
        const double missing = std::numeric_limits<double>::quiet_NaN();
        if (!g_metroLoaded) return missing;
        const METROLOGY_GP& metro = g_metroGP;
        float sign = tx % 2 == 0 ? 1.0f : -1.0f;
        float phase = sign * (gp.DELTA_PH[freq][tx] - metro.air_ph[freq][tx] / 57297.0f);
        if (type == SondeData::CURVE_PHASE_SMT_PC)
            return std::isfinite(phase) ? phase * MG_SCALE : missing;
        bool amplitude = type != SondeData::CURVE_RHO_PHASE_PC;
        float target = phase;
        if (amplitude) {
            float am1 = gp.AM_RX_1[freq][tx], am2 = gp.AM_RX_2[freq][tx];
            if (!(am1 > 0.0f) || !(am2 > 0.0f)) return missing;
            float att = sign * (20.0f * std::log10(am2 / am1)) - metro.air_att_dB[freq][tx];
            if (type == SondeData::CURVE_ATT_SMT_PC)
                return std::isfinite(att) ? att : missing;
            target = std::pow(10.0f, att / 20.0f);
        }
        if (!std::isfinite(target) || !(metro.F[freq] > 0) ||
            !(metro.L1[tx] > 0) || !(metro.L2[tx] > 0)) return missing;
        auto response = [&](float rho) {
            auto signal = PcSignal(metro, tx, freq, rho);
            return amplitude ? std::abs(signal) : std::arg(signal);
        };
        float left = 0.01f, right = amplitude ? 1000.0f : 7000.0f;
        float minimum = response(left), maximum = response(right);
        if (!std::isfinite(minimum) || !std::isfinite(maximum)) return missing;
        if (amplitude ? target >= maximum : target <= maximum)
            return amplitude ? 1200.0 : 7200.0;
        if (amplitude ? target <= minimum : target >= minimum) return 0.01f;
        for (int iteration = 0; iteration < 60; ++iteration) {
            float mid = 0.5f * (left + right);
            float diff = response(mid) - target;
            if (!std::isfinite(diff)) return missing;
            if (std::fabs(diff) < 1e-5f || right - left < 1e-4f * mid) return mid;
            if (amplitude ? diff < 0.0f : diff > 0.0f) left = mid;
            else right = mid;
        }
        return 0.5f * (left + right);
    }

    // Буферы онлайн-сборки кадров «Картограф» (половинки DATA+RAW по частотам)
    ALL_DATA g_pendingAllData = {};
    bool g_livePending400 = false;
    bool g_livePending2000 = false;
    std::vector<uint8_t> g_lastRecord;

    static_assert(sizeof(GP_DATA) == 320, "GP_DATA layout changed");
    static_assert(sizeof(ALL_DATA) == 3072, "ALL_DATA layout changed");
    static_assert(sizeof(std::complex<float>) == 8, "Complex layout changed");

    enum class CsvKind { Float, U32, U16, U8 };

    struct CsvColumn {
        std::string name;
        size_t offset;
        size_t size;
        CsvKind kind;
        bool available;
    };

    struct CsvLayout {
        const char* base;
        std::vector<CsvColumn> columns;

        template<class T>
        void Add(const std::string& name, const T& value, CsvKind kind, bool available = true) {
            columns.push_back({name, static_cast<size_t>(reinterpret_cast<const char*>(&value) - base), sizeof(T), kind, available});
        }

        void Float(const std::string& name, const float& value, bool available = true) {
            Add(name, value, CsvKind::Float, available);
        }

        void Complex(const std::string& name, const std::complex<float>& value) {
            size_t offset = static_cast<size_t>(reinterpret_cast<const char*>(&value) - base);
            columns.push_back({name + "_re", offset, 4, CsvKind::Float, true});
            columns.push_back({name + "_im", offset + 4, 4, CsvKind::Float, true});
        }
    };

    CsvLayout GpCsvLayout(const GP_DATA& gp) {
        CsvLayout layout = {reinterpret_cast<const char*>(&gp), {}};
        layout.Add("signature", gp.signature, CsvKind::U32);
        layout.Add("condition", gp.condition, CsvKind::U32);
        layout.Add("frame", gp.frame, CsvKind::U32);
        layout.Float("temperature_c", gp.temperature);
        for (int f = 0; f < 2; ++f) {
            std::string prefix = f == 0 ? "f400_" : "f2000_";
            layout.Float(prefix + "ZERO_AM_RX_1", gp.ZERO_AM_RX_1[f]);
            layout.Float(prefix + "ZERO_AM_RX_2", gp.ZERO_AM_RX_2[f]);
            layout.Float(prefix + "ZERO_dPH", gp.ZERO_dPH[f]);
            for (int tx = 0; tx < 5; ++tx) {
                std::string p = prefix + "tx" + std::to_string(tx + 1) + "_";
                bool available = tx < static_cast<int>(g_session.N_Tx);
                layout.Float(p + "rho_ph_smt", gp.rho_ph_smt[f][tx], available);
                layout.Float(p + "phase_smt", gp.phase_smt[f][tx], available);
                layout.Float(p + "AM_RX_1", gp.AM_RX_1[f][tx], available);
                layout.Float(p + "AM_RX_2", gp.AM_RX_2[f][tx], available);
                layout.Float(p + "DELTA_PH", gp.DELTA_PH[f][tx], available);
                layout.Float(p + "rho_att_smt", gp.rho_att_smt[f][tx], available);
                layout.Float(p + "att_dB_smt", gp.att_dB_smt[f][tx], available);
            }
        }
        return layout;
    }

    CsvLayout CartographCsvLayout(const ALL_DATA& all) {
        CsvLayout layout = {reinterpret_cast<const char*>(&all), {}};
        for (int f = 0; f < 2; ++f) {
            const DATA& d = all.data[f];
            std::string p = f == 0 ? "f400_" : "f2000_";
            layout.Add(p + "signature", d.signature, CsvKind::U32);
            layout.Add(p + "frame", d.frame, CsvKind::U32);
            layout.Add(p + "dds_freq_khz", d.dds_freq, CsvKind::U32);
            for (int tx = 0; tx < 4; ++tx) {
                std::string t = p + "tx" + std::to_string(tx + 1) + "_";
                layout.Float(t + "ATT_dB_geo_signal_smt", d.ATT_dB_geo_signal_smt[tx]);
                layout.Float(t + "PH_deg_geo_signal_smt", d.PH_deg_geo_signal_smt[tx]);
                layout.Float(t + "rho_ph_smt", d.rho_ph_smt[tx]);
                layout.Float(t + "phase_smt", d.phase_smt[tx]);
                layout.Float(t + "rho_att_smt", d.rho_att_smt[tx]);
                layout.Float(t + "att_dB_smt", d.att_dB_smt[tx]);
            }
            for (int i = 0; i < 32; ++i)
                layout.Add(p + "out_arr_" + std::to_string(i), d.out_arr[i], CsvKind::U8);
            layout.Add(p + "all_bit_cntr", d.all_bit_cntr, CsvKind::U32);
            layout.Add(p + "depth", d.depth, CsvKind::U32);
            for (int rx = 0; rx < 2; ++rx) {
                const direct_RX& r = d.R_zx[rx];
                std::string rprefix = p + (rx == 0 ? "rx_l_" : "rx_r_");
                layout.Complex(rprefix + "Tx_0", r.Tx_0);
                layout.Float(rprefix + "temperature_c", r.temperature);
                layout.Add(rprefix + "condition", r.condition, CsvKind::U32);
                for (int tx = 0; tx < 4; ++tx) {
                    std::string t = rprefix + "tx" + std::to_string(tx + 1) + "_";
                    layout.Complex(t + "Geo", r.Geo[tx]);
                    layout.Float(t + "amp_Vzz", r.amp_Vzz[tx]);
                    layout.Float(t + "amp_Vzx", r.amp_Vzx[tx]);
                    layout.Float(t + "ph_Vzx_Vzz", r.ph_Vzx_Vzz[tx]);
                    layout.Float(t + "dv", r.dv[tx]);
                    layout.Float(t + "border_angle", r.border_angle[tx]);
                    for (int component = 0; component < 2; ++component)
                        for (int s = 0; s < 16; ++s)
                            layout.Float(t + "sector_" + std::to_string(s) + (component == 0 ? "_re" : "_im"),
                                all.raw_data[f].R_zx[rx][tx].sector[component][s]);
                }
            }
            const undirect_RX& r = d.R_zz;
            for (int i = 0; i < 2; ++i)
                layout.Complex(p + "rzz_Tx_0_" + std::to_string(i), r.Tx_0[i]);
            for (int tx = 0; tx < 4; ++tx) {
                std::string t = p + "tx" + std::to_string(tx + 1) + "_";
                layout.Complex(t + "Rzz1", r.Rzz1[tx]);
                layout.Complex(t + "Rzz2", r.Rzz2[tx]);
            }
            layout.Float(p + "rzz_temperature_c", r.temperature);
            layout.Add(p + "rzz_condition", r.condition, CsvKind::U32);
            layout.Float(p + "INC_ABS_deg", d.INC.INC_ABS);
            layout.Float(p + "INC_ZEN_deg", d.INC.INC_ZEN);
            layout.Float(p + "INC_AZM_deg", d.INC.INC_AZM);
            layout.Add(p + "start_sector", d.start_sector, CsvKind::U16);
            layout.Add(p + "DataValid", d.DataValid, CsvKind::U16);
            layout.Float(p + "Wg_rpm", d.Wg);
        }
        return layout;
    }

    template<class T>
    T CsvValue(const char* data) {
        T value;
        std::memcpy(&value, data, sizeof(T));
        return value;
    }

    void WriteCsvValue(std::ostream& out, const CsvColumn& column, const char* frame, size_t frameSize) {
        if (!column.available || column.offset + column.size > frameSize) return;
        const char* data = frame + column.offset;
        switch (column.kind) {
        case CsvKind::Float: {
            float value = CsvValue<float>(data);
            if (std::isnan(value)) out << "nan";
            else if (std::isinf(value)) out << (value < 0 ? "-inf" : "inf");
            else out << value;
            break;
        }
        case CsvKind::U32: out << CsvValue<uint32_t>(data); break;
        case CsvKind::U16: out << CsvValue<uint16_t>(data); break;
        case CsvKind::U8: out << static_cast<unsigned>(CsvValue<uint8_t>(data)); break;
        }
    }

    std::string GetFileExtension(const std::string& path) {
        size_t dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos) return "";
        std::string ext = path.substr(dotPos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    }

    std::string GetFileNameOnly(const std::string& path) {
        size_t slashPos = path.find_last_of("/\\");
        if (slashPos == std::string::npos) return path;
        return path.substr(slashPos + 1);
    }

    GP_DATA TransformAllDataToGpData(const ALL_DATA& all) {
        GP_DATA gp = {};
        gp.signature = all.data[0].signature;
        gp.condition = all.data[0].R_zz.condition;
        gp.frame = all.data[0].frame;
        gp.temperature = all.data[0].R_zz.temperature;

        const int N_Tx = 4;
        for (int freq = 0; freq < 2; ++freq) {
            gp.ZERO_AM_RX_1[freq] = std::abs(all.data[freq].R_zz.Tx_0[0]);
            gp.ZERO_AM_RX_2[freq] = std::abs(all.data[freq].R_zz.Tx_0[1]);
            for (int tx = 0; tx < N_Tx; ++tx) {
                gp.phase_smt[freq][tx] = all.data[freq].phase_smt[tx];
                gp.rho_ph_smt[freq][tx] = all.data[freq].rho_ph_smt[tx];
                gp.AM_RX_1[freq][tx] = std::abs(all.data[freq].R_zz.Rzz1[tx]);
                gp.AM_RX_2[freq][tx] = std::abs(all.data[freq].R_zz.Rzz2[tx]);
                gp.att_dB_smt[freq][tx] = all.data[freq].att_dB_smt[tx];
                gp.rho_att_smt[freq][tx] = all.data[freq].rho_att_smt[tx];

                float signal = std::arg(all.data[freq].R_zz.Rzz2[tx]) - std::arg(all.data[freq].R_zz.Rzz1[tx]);
                if (std::fabs(signal) < 40.0f) {
                    while (signal >= PI_F) signal -= 2.0f * PI_F;
                    while (signal <= -PI_F) signal += 2.0f * PI_F;
                }
                else {
                    signal = 0.0f;
                }
                gp.DELTA_PH[freq][tx] = signal;
            }
        }
        return gp;
    }
}

namespace SondeData {

    bool LoadFile(const char* filePath, char* errorBuf, size_t errorBufLen) {
        if (!filePath) return false;
        LogSessionData session;

        std::string path(filePath);
        std::ifstream fin(path, std::ios::binary);
        if (!fin.is_open()) {
            if (errorBuf && errorBufLen > 0) {
                strncpy_s(errorBuf, errorBufLen, ("Не удалось открыть файл: " + path).c_str(), _TRUNCATE);
            }
            return false;
        }

        std::string ext = GetFileExtension(path);
        int pre = (ext == ".dev") ? 11 : 0;

        fin.seekg(pre, std::ios::beg);
        uint32_t signature = 0;
        fin.read(reinterpret_cast<char*>(&signature), sizeof(uint32_t));
        if (!fin) {
            if (errorBuf && errorBufLen > 0) {
                strncpy_s(errorBuf, errorBufLen, "Ошибка чтения сигнатуры файла (файл слишком мал).", _TRUNCATE);
            }
            return false;
        }

        fin.seekg(0, std::ios::end);
        std::streampos fileSize = fin.tellg();
        if (fileSize <= 0) {
            if (errorBuf && errorBufLen > 0) {
                strncpy_s(errorBuf, errorBufLen, "Файл пуст.", _TRUNCATE);
            }
            return false;
        }

        SondeID id = SondeID::FromSignature(signature);
        if (!is_cartograph(id.type) && (id.struct_size < 16 || id.struct_size > sizeof(GP_DATA)
            || id.struct_size % sizeof(float) != 0)) {
            if (errorBuf && errorBufLen)
                strncpy_s(errorBuf, errorBufLen, "Unsupported GP frame size (expected 16..320 bytes, multiple of 4).", _TRUNCATE);
            return false;
        }

        const bool cartograph = is_cartograph(id.type);
        const size_t recordSize = cartograph ? sizeof(ALL_DATA) + 2 * pre : id.struct_size + pre;
        const size_t bytes = static_cast<size_t>(fileSize);
        auto fail = [&](const std::string& message) {
            if (errorBuf && errorBufLen) strncpy_s(errorBuf, errorBufLen, message.c_str(), _TRUNCATE);
            return false;
        };
        if (bytes % recordSize != 0)
            return fail("Неполный кадр или изменившийся размер структуры: размер файла " + std::to_string(bytes)
                + " байт не кратен размеру записи " + std::to_string(recordSize) + " байт.");
        for (size_t n = 0; n < bytes / recordSize; ++n) {
            for (int frequency = 0; frequency < (cartograph ? 2 : 1); ++frequency) {
                size_t offset = n * recordSize + pre;
                if (frequency == 1) offset += pre ? sizeof(CARTOGRAF_DATA_1FREQ) + pre : sizeof(DATA);
                fin.seekg(offset, std::ios::beg);
                uint32_t current = 0;
                fin.read(reinterpret_cast<char*>(&current), sizeof(current));
                if (!fin) return fail("Ошибка чтения Signature в кадре " + std::to_string(n) + ". Файл не открыт.");
                if (current != signature) {
                    std::ostringstream message;
                    message << "Изменение Signature или размера структуры. Кадр (индекс с 0): " << n;
                    if (cartograph) message << ", блок " << (frequency == 0 ? "400" : "2000") << " кГц";
                    message << ", смещение " << offset << " байт.\nОжидалась Signature " << signature
                        << ", получена " << current << ".\nРазмер структуры по сигнатуре: ожидалось "
                        << id.struct_size << ", получено " << SondeID::FromSignature(current).struct_size
                        << " байт. Файл не открыт.";
                    return fail(message.str());
                }
            }
        }
        fin.clear();
        auto readFailure = [&]() {
            return fail("Ошибка чтения кадра после проверки целостности. Файл не открыт.");
        };

        session.sonde_type = id.type;
        session.signature = signature;
        session.struct_size = id.struct_size;
        session.N_Tx = id.N_Tx;
        session.sonde_name = id.GetTypeName();
        session.file_path = path;
        session.file_name = GetFileNameOnly(path);

        // 1. «Картограф» (RAW / LWD)
        if (id.type == CARTOGRAPH_RAW_ || id.type == CARTOGRAPH_) {
            if (ext == ".dev") {
                const size_t single_freq_size = sizeof(CARTOGRAF_DATA_1FREQ) + pre;
                size_t numFrames = static_cast<size_t>(fileSize) / (2 * single_freq_size);
                session.all_data.reserve(numFrames);
                session.gp_data.reserve(numFrames);

                std::vector<uint8_t> preBuf(pre > 0 ? pre : 1);
                for (size_t n = 0; n < numFrames; ++n) {
                    fin.seekg(n * 2 * single_freq_size, std::ios::beg);
                    if (pre > 0) fin.read(reinterpret_cast<char*>(preBuf.data()), pre);
                    CARTOGRAF_DATA_1FREQ c400;
                    fin.read(reinterpret_cast<char*>(&c400), sizeof(CARTOGRAF_DATA_1FREQ));

                    if (pre > 0) fin.read(reinterpret_cast<char*>(preBuf.data()), pre);
                    CARTOGRAF_DATA_1FREQ c2000;
                    fin.read(reinterpret_cast<char*>(&c2000), sizeof(CARTOGRAF_DATA_1FREQ));

                    if (!fin) return readFailure();

                    ALL_DATA all;
                    all.data[0] = c400.data;
                    all.raw_data[0] = c400.raw_data;
                    all.data[1] = c2000.data;
                    all.raw_data[1] = c2000.raw_data;

                    session.all_data.push_back(all);
                    session.gp_data.push_back(TransformAllDataToGpData(all));
                }
            }
            else {
                const size_t frameSize = sizeof(ALL_DATA); // 3072
                size_t numFrames = static_cast<size_t>(fileSize) / frameSize;
                session.all_data.reserve(numFrames);
                session.gp_data.reserve(numFrames);

                fin.seekg(0, std::ios::beg);
                for (size_t n = 0; n < numFrames; ++n) {
                    ALL_DATA all;
                    fin.read(reinterpret_cast<char*>(&all), sizeof(ALL_DATA));
                    if (!fin) return readFailure();

                    session.all_data.push_back(all);
                    session.gp_data.push_back(TransformAllDataToGpData(all));
                }
            }
        }
        // 2. 5Tx и автономные зонды
        else if (is_gp_sonde(id.type)) {
            if (ext == ".dev") {
                const size_t frameSize = id.struct_size + pre;
                size_t numFrames = static_cast<size_t>(fileSize) / frameSize;
                session.gp_data.reserve(numFrames);

                std::vector<uint8_t> preBuf(pre > 0 ? pre : 1);
                for (size_t n = 0; n < numFrames; ++n) {
                    fin.seekg(n * frameSize, std::ios::beg);
                    if (pre > 0) fin.read(reinterpret_cast<char*>(preBuf.data()), pre);
                    GP_DATA gp = {};
                    fin.read(reinterpret_cast<char*>(&gp), id.struct_size);
                    if (!fin) return readFailure();

                    session.gp_data.push_back(gp);
                }
            }
            else {
                const size_t frameSize = id.struct_size;
                size_t numFrames = static_cast<size_t>(fileSize) / frameSize;
                session.gp_data.reserve(numFrames);

                fin.seekg(0, std::ios::beg);
                for (size_t n = 0; n < numFrames; ++n) {
                    GP_DATA gp = {};
                    fin.read(reinterpret_cast<char*>(&gp), id.struct_size);
                    if (!fin) return readFailure();

                    session.gp_data.push_back(gp);
                }
            }
        }
        else {
            const size_t frameSize = (id.struct_size > 0) ? id.struct_size : sizeof(GP_DATA);
            size_t numFrames = static_cast<size_t>(fileSize) / frameSize;
            session.gp_data.reserve(numFrames);

            fin.seekg(0, std::ios::beg);
            for (size_t n = 0; n < numFrames; ++n) {
                GP_DATA gp = {};
                fin.read(reinterpret_cast<char*>(&gp), frameSize);
                if (!fin) return readFailure();

                session.gp_data.push_back(gp);
            }
        }

        if (session.gp_data.empty()) {
            if (errorBuf && errorBufLen > 0) {
                strncpy_s(errorBuf, errorBufLen, "Не удалось прочитать ни одного кадра из файла.", _TRUNCATE);
            }
            return false;
        }

        g_session = std::move(session);
        return true;
    }

    const char* GetCsvSchema() {
        return g_session.all_data.empty() ? "em_gp_v1" : "em_cartograph_v1";
    }

    bool ExportCsv(const wchar_t* path, const CsvSelection& selection,
        size_t& exportedFrames, char* errorBuf, size_t errorBufLen) {
        exportedFrames = 0;
        auto fail = [&](const char* message) {
            if (errorBuf && errorBufLen) strncpy_s(errorBuf, errorBufLen, message, _TRUNCATE);
            return false;
        };
        if (!IsLoaded()) return fail("No data loaded.");
        if (!path || !*path) return fail("CSV path is empty.");
        if (selection.enabled && (!std::isfinite(selection.minimum) || !std::isfinite(selection.maximum)
            || selection.minimum > selection.maximum)) return fail("Invalid Min-Max range.");
        if (selection.enabled && !selection.temperature && (selection.minimum < 0
            || selection.maximum >= static_cast<double>(GetFrameCount())
            || std::floor(selection.minimum) != selection.minimum
            || std::floor(selection.maximum) != selection.maximum)) return fail("Frame range must contain valid integer indices.");

        std::vector<size_t> indices;
        for (size_t i = 0; i < GetFrameCount(); ++i) {
            double value = selection.temperature ? GetTemperature(i) : static_cast<double>(i);
            if (!selection.enabled || (std::isfinite(value) && value >= selection.minimum && value <= selection.maximum))
                indices.push_back(i);
        }
        if (indices.empty()) return fail("No frames in the selected range.");

        bool cartograph = !g_session.all_data.empty();
        CsvLayout layout = cartograph ? CartographCsvLayout(g_session.all_data[0]) : GpCsvLayout(g_session.gp_data[0]);
        size_t recordBytes = cartograph ? sizeof(ALL_DATA) : g_session.struct_size;
        int fd = -1;
        if (_wsopen_s(&fd, path, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY,
            _SH_DENYRW, _S_IREAD | _S_IWRITE) != 0)
            return fail("Cannot create CSV. Choose a new filename; existing files are not overwritten.");
        FILE* file = _fdopen(fd, "wb");
        if (!file) {
            _close(fd);
            _wremove(path);
            return fail("Cannot open CSV output stream.");
        }
        std::ostringstream row;
        row.imbue(std::locale::classic());
        row << std::setprecision(std::numeric_limits<float>::max_digits10);
        row << "schema,row_index,sonde_type,sonde_number,tx_count,record_bytes";
        for (const auto& column : layout.columns) row << ',' << column.name;
        row << '\n';
        auto writeRow = [&]() {
            std::string text = row.str();
            bool ok = std::fwrite(text.data(), 1, text.size(), file) == text.size();
            row.str("");
            row.clear();
            return ok;
        };
        bool ok = writeRow();
        for (size_t i : indices) {
            if (!ok) break;
            const char* frame = cartograph ? reinterpret_cast<const char*>(&g_session.all_data[i])
                : reinterpret_cast<const char*>(&g_session.gp_data[i]);
            row << GetCsvSchema() << ',' << i << ',' << g_session.sonde_type << ','
                << (g_session.signature % 1000) << ',' << g_session.N_Tx << ',' << recordBytes;
            for (const auto& column : layout.columns) {
                row << ',';
                WriteCsvValue(row, column, frame, recordBytes);
            }
            row << '\n';
            ok = writeRow();
            if (ok) ++exportedFrames;
        }
        if (std::fclose(file) != 0) ok = false;
        if (!ok) {
            _wremove(path);
            exportedFrames = 0;
            return fail("CSV write failed. Partial output removed.");
        }
        return true;
    }

    bool IsLoaded() {
        return !g_session.gp_data.empty();
    }

    void CloseFile() {
        g_session.Clear();
        g_livePending400 = false;
        g_livePending2000 = false;
        g_lastRecord.clear();
    }

    // --- Онлайн-телеметрия -------------------------------------------------

    void BeginLiveSession() {
        g_session.Clear();
        g_session.sonde_name = "ОНЛАЙН";
        g_session.file_name = "ONLINE";
        g_livePending400 = false;
        g_livePending2000 = false;
        g_lastRecord.clear();
    }

    size_t AppendLiveRecord(const uint8_t* data, size_t len) {
        if (!data || len < 4) return 0;

        uint32_t signature = 0;
        memcpy(&signature, data, sizeof(signature));
        SondeID id = SondeID::FromSignature(signature);

        // Инициализация параметров сессии по первому кадру
        if (g_session.sonde_type == 0) {
            g_session.sonde_type = id.type;
            g_session.signature = signature;
            g_session.struct_size = id.struct_size;
            g_session.N_Tx = id.N_Tx;
            g_session.sonde_name = id.GetTypeName() + " [ONLINE]";
        }

        size_t added = 0;

        if (is_cartograph(id.type)) {
            // Полный ALL_DATA целиком (резервный путь)
            if (len >= sizeof(ALL_DATA)) {
                ALL_DATA all;
                memcpy(&all, data, sizeof(ALL_DATA));
                g_session.all_data.push_back(all);
                g_session.gp_data.push_back(TransformAllDataToGpData(all));
                g_lastRecord.assign(data, data + sizeof(ALL_DATA));
                added = 1;
            }
            // Одиночный частотный блок DATA+RAW (1536 Б) — штатный путь прибора
            else if (len >= sizeof(CARTOGRAF_DATA_1FREQ)) {
                CARTOGRAF_DATA_1FREQ one;
                memcpy(&one, data, sizeof(CARTOGRAF_DATA_1FREQ));
                int freqIdx = (one.data.dds_freq == 401) ? 0 : 1;
                g_pendingAllData.data[freqIdx] = one.data;
                g_pendingAllData.raw_data[freqIdx] = one.raw_data;
                if (freqIdx == 0) g_livePending400 = true; else g_livePending2000 = true;

                // Собрана пара 400+2000 → полный кадр
                if (g_livePending400 && g_livePending2000) {
                    g_session.all_data.push_back(g_pendingAllData);
                    g_session.gp_data.push_back(TransformAllDataToGpData(g_pendingAllData));
                    g_lastRecord.assign(reinterpret_cast<const uint8_t*>(&g_pendingAllData),
                                        reinterpret_cast<const uint8_t*>(&g_pendingAllData) + sizeof(ALL_DATA));
                    added = 1;
                }
            }
        }
        else {
            // GP-прибор: запись по размеру из сигнатуры (240..320)
            size_t rec = id.struct_size;
            if (rec > sizeof(GP_DATA)) rec = sizeof(GP_DATA);
            if (len >= rec && rec >= 16) {
                GP_DATA gp;
                memset(&gp, 0, sizeof(GP_DATA));
                memcpy(&gp, data, rec);
                g_session.gp_data.push_back(gp);
                g_lastRecord.assign(data, data + rec);
                added = 1;
            }
        }
        return added;
    }

    bool GetLastRecordBytes(const uint8_t** data, size_t* len) {
        if (g_lastRecord.empty()) { *data = nullptr; *len = 0; return false; }
        *data = g_lastRecord.data();
        *len = g_lastRecord.size();
        return true;
    }

    size_t GetFrameCount() {
        return g_session.gp_data.size();
    }

    int GetSondeType() {
        return static_cast<int>(g_session.sonde_type);
    }

    int GetTxCount() {
        return static_cast<int>(g_session.N_Tx);
    }

    uint32_t GetSignature() {
        return g_session.signature;
    }

    const char* GetSondeName() {
        return g_session.sonde_name.c_str();
    }

    const char* GetFileName() {
        return g_session.file_name.c_str();
    }

    double GetTemperature(size_t frameIdx) {
        if (frameIdx < g_session.all_data.size()) {
            float t = g_session.all_data[frameIdx].data[0].R_zx[0].temperature;
            if (t > -50.0f && t < 250.0f) return static_cast<double>(t);
        }
        if (frameIdx < g_session.gp_data.size()) {
            float t = g_session.gp_data[frameIdx].temperature;
            if (t > -50.0f && t < 250.0f) return static_cast<double>(t);
        }
        return 0.0;
    }

    double GetCurveValue(int curveType, int freqIdx, int txIdx, size_t frameIdx, int rxIdx) {
        if (frameIdx >= g_session.gp_data.size()) return 0.0;
        if (freqIdx < 0 || freqIdx > 1) freqIdx = 0;
        if (txIdx < 0 || txIdx > 4) txIdx = 0;
        if (rxIdx < 0 || rxIdx > 1) rxIdx = 0;

        const GP_DATA& gp = g_session.gp_data[frameIdx];
        bool hasAllData = (frameIdx < g_session.all_data.size());

        switch (curveType) {
        case CURVE_PHASE_SMT_PC:
        case CURVE_RHO_PHASE_PC:
        case CURVE_ATT_SMT_PC:
        case CURVE_RHO_ATT_PC:
            return PcCurveValue(curveType, gp, freqIdx, txIdx);
        // --- Ненаправленные кривые (UNDIRECT) ---
        case CURVE_PHASE_DIFF:
            // «PH Z2-Z1» в старой программе — миллиградусы (mG)
            return static_cast<double>(gp.DELTA_PH[freqIdx][txIdx] * MG_SCALE);

        case CURVE_AMPLITUDE_RX1:
            return static_cast<double>(gp.AM_RX_1[freqIdx][txIdx] * mVscale(g_session.sonde_type));

        case CURVE_AMPLITUDE_RX2:
            return static_cast<double>(gp.AM_RX_2[freqIdx][txIdx] * mVscale(g_session.sonde_type));

        case CURVE_RHO_PHASE:
            return static_cast<double>(gp.rho_ph_smt[freqIdx][txIdx]);

        case CURVE_ATTENUATION: {
            // «AM Z2/Z1» в старой программе — Gain_dB(AM2, AM1), знак чередуется +/− по Tx
            double am1 = gp.AM_RX_1[freqIdx][txIdx];
            double am2 = gp.AM_RX_2[freqIdx][txIdx];
            if (am1 <= 0.0 || am2 <= 0.0) return 0.0;
            double db = 20.0 * std::log10(am2 / am1);
            return (txIdx % 2 == 0) ? db : -db;
        }

        case CURVE_RHO_ATT:
            return static_cast<double>(gp.rho_att_smt[freqIdx][txIdx]);

        case CURVE_PHASE_SMT:
            // Симметризованная фаза в старой программе — тоже миллиградусы
            return static_cast<double>(gp.phase_smt[freqIdx][txIdx] * MG_SCALE);

        case CURVE_ZERO_AM_ZZ:
            if (txIdx == 0) return static_cast<double>(gp.ZERO_AM_RX_1[freqIdx] * mVscale(g_session.sonde_type) * 1000.0f);
            if (txIdx == 1) return static_cast<double>(gp.ZERO_AM_RX_2[freqIdx] * mVscale(g_session.sonde_type) * 1000.0f);
            return 0.0;

        case CURVE_TEMPERATURE:
            return GetTemperature(frameIdx);

        case CURVE_FRAME_NUM:
            return static_cast<double>(gp.frame);

        case CURVE_ACCEL:
            return 0.0;

        case CURVE_ATT_SMT:
            // «ATT SMT» — затухание, посчитанное прибором (att_dB_smt)
            return static_cast<double>(gp.att_dB_smt[freqIdx][txIdx]);

        case CURVE_ERROR_RHO:
            // «ε RO» — невязка УЭС прибор/компьютер: реализуется вместе с
            // PC-пересчётом через метрологию (следующий этап)
            return 0.0;

        // --- Направленные кривые (DIRECT) ---
        case CURVE_GEO_AM:
            // «AM GEO» — модуль гео-комплекса прибора (как abs(Geo) у старой)
            if (hasAllData && txIdx < 4) {
                return static_cast<double>(std::abs(g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].Geo[txIdx]));
            }
            return 0.0;

        case CURVE_GEO_PH:
            // «φ GEO» — аргумент гео-комплекса прибора в миллиградусах
            if (hasAllData && txIdx < 4) {
                return static_cast<double>(std::arg(g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].Geo[txIdx]) * MG_SCALE);
            }
            return 0.0;

        case CURVE_GEO_AM_SMT:
            if (hasAllData && txIdx < 4) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].ATT_dB_geo_signal_smt[txIdx]);
            }
            return 0.0;

        case CURVE_GEO_PH_SMT:
            if (hasAllData && txIdx < 4) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].PH_deg_geo_signal_smt[txIdx]);
            }
            return 0.0;

        case CURVE_V_ZX:
            if (hasAllData && txIdx < 4) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].amp_Vzx[txIdx]);
            }
            return 0.0;

        case CURVE_V_ZZ:
            if (hasAllData && txIdx < 4) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].amp_Vzz[txIdx]);
            }
            return 0.0;

        case CURVE_BETTA_Z:
            if (hasAllData && txIdx < 4) {
                double vzz = g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].amp_Vzz[txIdx];
                double vzx = g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].amp_Vzx[txIdx];
                return (vzz > 1e-12) ? (vzx / vzz) : 0.0;
            }
            return 0.0;

        case CURVE_RATIO_ZX_ZZ:
            if (hasAllData && txIdx < 4) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].ph_Vzx_Vzz[txIdx] * MG_SCALE);
            }
            return 0.0;

        case CURVE_V_XX:
            return 0.0;

        case CURVE_DV_NOISE:
            if (hasAllData && txIdx < 4) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].dv[txIdx]);
            }
            return 0.0;

        case CURVE_SNR:
            if (hasAllData && txIdx < 4) {
                double vzx = g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].amp_Vzx[txIdx];
                double dv = g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].dv[txIdx];
                return (dv > 1e-12) ? (vzx / std::sqrt(dv)) : 0.0;
            }
            return 0.0;

        case CURVE_ZERO_AM_ZX:
            if (hasAllData) {
                return static_cast<double>(std::abs(g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].Tx_0) * mVscale(g_session.sonde_type) * 1000.0f);
            }
            return 0.0;

        case CURVE_BORDER_ANGLE:
            if (hasAllData && txIdx < 4) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].border_angle[txIdx] * GRAD_SCALE);
            }
            return 0.0;

        case CURVE_INC_ABS:
            if (hasAllData) {
                const Inclinometer& inc = g_session.all_data[frameIdx].data[freqIdx].INC;
                if (txIdx == 0) return static_cast<double>(inc.INC_ABS);
                if (txIdx == 1) return static_cast<double>(inc.INC_ZEN);
                if (txIdx == 2) return static_cast<double>(inc.INC_AZM);
            }
            return 0.0;

        case CURVE_ANGULAR_VELOCITY:
            if (hasAllData) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].Wg);
            }
            return 0.0;

        case CURVE_START_SECTOR:
            if (hasAllData) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].start_sector);
            }
            return 0.0;

        case CURVE_TEMP_RZX:
            if (hasAllData) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].R_zx[rxIdx].temperature);
            }
            return 0.0;

        case CURVE_DEPTH:
            if (hasAllData) {
                return static_cast<double>(g_session.all_data[frameIdx].data[freqIdx].depth);
            }
            return 0.0;

        default:
            return 0.0;
        }
    }

    bool GetSectorData(int rxIdx, int freqIdx, int txIdx, size_t frameIdx, SectorAnalysis& analysis) {
        std::memset(&analysis, 0, sizeof(analysis));

        if (frameIdx >= g_session.all_data.size()) {
            return false;
        }
        if (rxIdx < 0 || rxIdx > 1) rxIdx = 0;
        if (freqIdx < 0 || freqIdx > 1) freqIdx = 0;
        if (txIdx < 0 || txIdx > 3) txIdx = 0;

        const RAW_DATA& raw = g_session.all_data[frameIdx].raw_data[freqIdx];
        const _16_sector_data& secData = raw.R_zx[rxIdx][txIdx];

        double sumY = 0.0;
        double a1 = 0.0, b1 = 0.0;
        double a2 = 0.0, b2 = 0.0;

        for (int k = 0; k < 16; ++k) {
            double re = secData.sector[0][k] * mVscale(g_session.sonde_type);
            double im = secData.sector[1][k] * mVscale(g_session.sonde_type);
            double amp = std::sqrt(re * re + im * im);
            analysis.sectors[k] = amp;
            sumY += amp;

            double angle1 = k * (2.0 * 3.14159265358979323846 / 16.0);
            a1 += amp * std::cos(angle1);
            b1 += amp * std::sin(angle1);

            double angle2 = 2.0 * angle1;
            a2 += amp * std::cos(angle2);
            b2 += amp * std::sin(angle2);
        }

        analysis.h0 = sumY / 16.0;
        a1 = (2.0 / 16.0) * a1;
        b1 = (2.0 / 16.0) * b1;
        analysis.h1 = std::sqrt(a1 * a1 + b1 * b1);

        a2 = (2.0 / 16.0) * a2;
        b2 = (2.0 / 16.0) * b2;
        analysis.h2 = std::sqrt(a2 * a2 + b2 * b2);

        double ph = std::atan2(b1, a1) * (180.0 / 3.14159265358979323846);
        if (ph < 0.0) ph += 360.0;
        analysis.phaseDeg = ph;

        analysis.h1_h2_ratio = (analysis.h2 > 1e-4) ? (analysis.h1 / analysis.h2) : 99.9;

        // Расчет невязки (шум dV и SNR)
        double sumResSq = 0.0;
        for (int k = 0; k < 16; ++k) {
            double angle1 = k * (2.0 * 3.14159265358979323846 / 16.0);
            double fit = analysis.h0 + a1 * std::cos(angle1) + b1 * std::sin(angle1);
            double diff = analysis.sectors[k] - fit;
            sumResSq += diff * diff;
        }
        analysis.dv = std::sqrt(sumResSq / 16.0);
        analysis.snr = (analysis.dv > 1e-4) ? (analysis.h1 / analysis.dv) : 99.9;

        return true;
    }

    // --- Метрология прибора (сессия) ---

    bool SetMetrology(const uint8_t* data, size_t len) {
        if (data == nullptr || len < sizeof(METROLOGY_GP)) return false;
        uint32_t signature = *reinterpret_cast<const uint32_t*>(data);
        SondeID id = SondeID::FromSignature(signature);
        if (is_cartograph(id.type) && len >= sizeof(METROLOGY_CARTOGRAPH)) {
            memcpy(&g_metroCart, data, sizeof(METROLOGY_CARTOGRAPH));
            memcpy(&g_metroGP, data, sizeof(METROLOGY_GP)); // общая часть совпадает
            g_metroCartograph = true;
        }
        else {
            memcpy(&g_metroGP, data, sizeof(METROLOGY_GP));
            g_metroCartograph = false;
        }
        g_metroLoaded = true;
        return true;
    }

    void SetMetrologyGP(const METROLOGY_GP* metro) {
        if (metro == nullptr) return;
        g_metroGP = *metro;
        g_metroCartograph = false;
        g_metroLoaded = true;
    }

    void SetMetrologyCartograph(const METROLOGY_CARTOGRAPH* metro) {
        if (metro == nullptr) return;
        g_metroCart = *metro;
        memcpy(&g_metroGP, metro, sizeof(METROLOGY_GP));
        g_metroCartograph = true;
        g_metroLoaded = true;
    }

    void ClearMetrology() {
        g_metroLoaded = false;
        g_metroCartograph = false;
        memset(&g_metroGP, 0, sizeof(g_metroGP));
        memset(&g_metroCart, 0, sizeof(g_metroCart));
    }

    bool HasMetrology() { return g_metroLoaded; }
    bool IsMetrologyCartograph() { return g_metroLoaded && g_metroCartograph; }

    const METROLOGY_GP* GetMetrologyGP() {
        return g_metroLoaded ? &g_metroGP : nullptr;
    }

    const METROLOGY_CARTOGRAPH* GetMetrologyCartograph() {
        return (g_metroLoaded && g_metroCartograph) ? &g_metroCart : nullptr;
    }

    uint32_t GetMetrologySignature() {
        return g_metroLoaded ? g_metroGP.signature : 0;
    }
}
