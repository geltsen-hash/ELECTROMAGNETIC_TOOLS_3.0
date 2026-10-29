#pragma once

#include <cstddef>

namespace SondeData {

    enum CurveType {
        // Ненаправленные кривые (UNDIRECT)
        CURVE_PHASE_DIFF = 0,         // Разность фаз Δφ (PH Z2-Z1)
        CURVE_AMPLITUDE_RX1 = 1,      // Амплитуда Rx1 / Z1
        CURVE_AMPLITUDE_RX2 = 2,      // Амплитуда Rx2 / Z2
        CURVE_RHO_PHASE = 3,          // УЭС по фазе (ρ_ph)
        CURVE_ATTENUATION = 4,        // Затухание Att (AM Z2/Z1)
        CURVE_RHO_ATT = 5,            // УЭС по затуханию (ρ_att)
        CURVE_PHASE_SMT = 6,          // Симметризованная фаза (PH_SMT)
        CURVE_ZERO_AM_ZZ = 7,         // Амплитуда нуля ZZ
        CURVE_TEMPERATURE = 8,        // Температура RZZ
        CURVE_FRAME_NUM = 9,          // Кадр (FRAME)
        CURVE_ACCEL = 10,             // Акселерометр

        // Направленные кривые (DIRECT)
        CURVE_GEO_AM = 11,            // Гео-Амплитуда (AM_GEO)
        CURVE_GEO_PH = 12,            // Гео-Фаза (PH_GEO)
        CURVE_V_ZX = 13,              // Напряжение V_ZX
        CURVE_V_ZZ = 14,              // Напряжение V_ZZ
        CURVE_BETTA_Z = 15,           // Коэффициент betta_z
        CURVE_RATIO_ZX_ZZ = 16,       // Отношение V_ZX / V_ZZ
        CURVE_V_XX = 17,              // Напряжение V_XX
        CURVE_DV_NOISE = 18,          // Шум dV (мусор)
        CURVE_SNR = 19,               // Сигнал/шум (SNR)
        CURVE_ZERO_AM_ZX = 20,        // Амплитуда нуля ZX
        CURVE_BORDER_ANGLE = 21,      // Угол границы (BORDER_ANGLE)
        CURVE_INC_ABS = 22,           // Инклинометр (INC Abs/Zen/Azm)
        CURVE_ANGULAR_VELOCITY = 23,  // Угловая скорость (ANGULAR_VELOCITY)
        CURVE_START_SECTOR = 24,      // Стартовый сектор (START_SECTOR)
        CURVE_TEMP_RZX = 25,          // Температура RZX
        CURVE_DEPTH = 26              // Глубина (DEPTH)
    };

    struct SectorAnalysis {
        double sectors[16];   // 16 секторов амплитуды в мВ
        double h0;            // постоянная составляющая (среднее)
        double h1;            // амплитуда 1-й гармоники
        double phaseDeg;      // угол 1-й гармоники в градусах (0..360)
        double h2;            // амплитуда 2-й гармоники
        double h1_h2_ratio;   // соотношение H1 / H2
        double snr;           // отношение сигнал/шум
        double dv;            // размах шума
    };

    // Загрузка файла данных (.bin, .biv, .DEV)
    bool LoadFile(const char* filePath, char* errorBuf, size_t errorBufLen);

    // Проверка наличия загруженных данных
    bool IsLoaded();

    // Очистка сессии
    void CloseFile();

    // Параметры загруженного зонда
    size_t GetFrameCount();
    int GetSondeType();
    int GetTxCount();
    const char* GetSondeName();
    const char* GetFileName();

    struct CsvSelection {
        bool enabled = false;
        bool temperature = false;
        double minimum = 0;
        double maximum = 0;
    };

    const char* GetCsvSchema();
    bool ExportCsv(const wchar_t* path, const CsvSelection& selection,
        size_t& exportedFrames, char* errorBuf, size_t errorBufLen);

    // Значение кривой для кадра frameIdx (кадр 0..N-1, частота 0=400, 1=2000, передатчик 0..Tx-1)
    double GetCurveValue(int curveType, int freqIdx, int txIdx, size_t frameIdx);

    // Получение температуры кадра
    double GetTemperature(size_t frameIdx);

    // Получение 16 секторов и спектрального анализа первой гармоники
    bool GetSectorData(int rxIdx, int freqIdx, int txIdx, size_t frameIdx, SectorAnalysis& analysis);
}
