#pragma once

#include <cstdint>
#include <complex>
#include <vector>
#include <string>
#include <cmath>

// Константы математические и физические
#ifndef PI_CONST_DEF
#define PI_CONST_DEF
const float PI_F = 3.141592741f;
const float EPS0_F = 8.85e-12f;
const float MU0_F = 4.0f * PI_F * 1e-7f;
const float MV_SCALE = 0.00023842f; // перевод кодов АЦП в милливольты
const float MG_SCALE = 1000.0f * 180.0f / PI_F; // миллиградусы
const float GRAD_SCALE = 180.0f / PI_F;
#endif

// Константы типов зондов
#define CARTOGRAPH_ 352
#define CARTOGRAPH_RAW_ 351
#define CARTOGRAPH_LWD_4Tx_ 349
#define AUTONOM_5Tx_ 151
#define AUTONOM_5Tx_SDR_ 152
#define LWD_4Tx_NEW_ 242
#define LWD_3Tx_ 231

inline bool is_cartograph(uint32_t type) {
    return (type == CARTOGRAPH_RAW_ || type == CARTOGRAPH_);
}

inline bool is_gp_sonde(uint32_t type) {
    return (type == AUTONOM_5Tx_ || type == CARTOGRAPH_LWD_4Tx_ || 
            type == LWD_4Tx_NEW_ || type == LWD_3Tx_ || type == AUTONOM_5Tx_SDR_);
}

#pragma pack(push, 1)

// Инклинометр (углы в градусах)
struct Inclinometer {
    float INC_ABS;
    float INC_ZEN;
    float INC_AZM;
};

// 16-секторные сырые данные (Re, Im)
struct _16_sector_data {
    float sector[2][16]; // [0] = Re, [1] = Im
};

// Данные от одного направленного приемника (Rx_Left или Rx_Right)
struct direct_RX {
    std::complex<float> Tx_0;
    std::complex<float> Geo[4];
    float amp_Vzz[4];
    float amp_Vzx[4];
    float ph_Vzx_Vzz[4];
    float dv[4];
    float border_angle[4];
    float temperature;
    uint32_t condition;
};

// Данные от ненаправленного приемника
struct undirect_RX {
    std::complex<float> Tx_0[2];
    std::complex<float> Rzz1[4];
    std::complex<float> Rzz2[4];
    float temperature;
    uint32_t condition;
};

// Блок потребительской и отладочной информации одной частоты (512 байт)
struct DATA {
    uint32_t signature;
    uint32_t frame;
    uint32_t dds_freq;
    float ATT_dB_geo_signal_smt[4];
    float PH_deg_geo_signal_smt[4];
    float rho_ph_smt[4];
    float phase_smt[4];
    uint8_t out_arr[32];
    uint32_t all_bit_cntr;
    uint32_t depth;
    direct_RX R_zx[2]; // [0] = Rx_Left, [1] = Rx_Right
    undirect_RX R_zz;
    Inclinometer INC;
    uint16_t start_sector;
    uint16_t DataValid;
    float Wg;
    float rho_att_smt[4];
    float att_dB_smt[4];
};

// Сырые 16 секторов по передатчикам и приемникам (1024 байта)
struct RAW_DATA {
    _16_sector_data R_zx[2][4]; // [0..1: Rx_L, Rx_R][0..3: Tx1..Tx4]
};

// Кадр одной частоты (1536 байт)
struct CARTOGRAF_DATA_1FREQ {
    DATA data;
    RAW_DATA raw_data;
};

// Полный сырой срез каротажа «Картограф» по двум частотам (3072 байта)
struct ALL_DATA {
    DATA data[2];       // [0] = 400 кГц, [1] = 2000 кГц
    RAW_DATA raw_data[2]; // [0] = 400 кГц, [1] = 2000 кГц
};

// Нормализованные геометрические параметры (240 байт для 5Tx и автономных зондов)
struct GP_DATA {
    uint32_t signature;
    uint32_t condition;
    uint32_t frame;
    float temperature;
    float rho_ph_smt[2][5];
    float phase_smt[2][5];
    float AM_RX_1[2][5];
    float ZERO_AM_RX_1[2];
    float AM_RX_2[2][5];
    float ZERO_AM_RX_2[2];
    float DELTA_PH[2][5];
    float ZERO_dPH[2];
    float rho_att_smt[2][5];
    float att_dB_smt[2][5];
};

// Преамбула для файлов дампов памяти .DEV
struct DEV_PREAMBLE {
    uint8_t raw[11];
};

#pragma pack(pop)

// Идентификатор зонда, разобранный из 32-битной сигнатуры
struct SondeID {
    uint32_t struct_size = 0;
    uint32_t type = 0;
    uint32_t type_ = 0;
    uint32_t N_Tx = 4;
    uint32_t mod = 0;
    uint32_t number = 0;

    static SondeID FromSignature(uint32_t signature) {
        SondeID id;
        id.struct_size = signature / 1000000;
        id.type = (signature % 1000000) / 1000;
        id.type_ = (signature % 1000000) / 100000;
        id.N_Tx = (signature % 100000) / 10000;
        id.mod = (signature % 10000) / 1000;
        id.number = (signature % 1000);

        if (id.struct_size == 0) {
            if (is_gp_sonde(id.type)) {
                id.struct_size = 240; // 240
            }
            else if (id.type == CARTOGRAPH_RAW_) {
                id.struct_size = sizeof(ALL_DATA); // 3072
            }
            else if (id.type == CARTOGRAPH_) {
                id.struct_size = sizeof(ALL_DATA);
            }
            else {
                id.struct_size = sizeof(GP_DATA);
            }
        }
        if (is_cartograph(id.type) || id.type == CARTOGRAPH_LWD_4Tx_ || id.type == LWD_4Tx_NEW_) {
            id.N_Tx = 4;
        }
        else if (id.type == LWD_3Tx_) {
            id.N_Tx = 3;
        }
        else if (id.type == AUTONOM_5Tx_ || id.type == AUTONOM_5Tx_SDR_) {
            id.N_Tx = 5;
        }
        else if (id.N_Tx == 0) {
            id.N_Tx = 4;
        }
        return id;
    }

    std::string GetTypeName() const {
        switch (type) {
        case CARTOGRAPH_RAW_: return "КАРТОГРАФ (Сырой)";
        case CARTOGRAPH_: return "КАРТОГРАФ";
        case CARTOGRAPH_LWD_4Tx_: return "КАРТОГРАФ LWD (4Tx)";
        case AUTONOM_5Tx_: return "АВТОНОМНЫЙ 5Tx";
        case AUTONOM_5Tx_SDR_: return "АВТОНОМНЫЙ 5Tx SDR";
        case LWD_4Tx_NEW_: return "LWD 4Tx NEW";
        case LWD_3Tx_: return "LWD 3Tx";
        default: return "ЗОНД ТИП " + std::to_string(type);
        }
    }
};

// Сессия каротажных данных в оперативной памяти
struct LogSessionData {
    uint32_t sonde_type = 0;
    uint32_t signature = 0;
    uint32_t struct_size = 0;
    uint32_t N_Tx = 4;
    std::string sonde_name;
    std::string file_path;
    std::string file_name;

    std::vector<ALL_DATA> all_data; // заполнено для «Картографа»
    std::vector<GP_DATA> gp_data;   // нормализованные данные для скоростных графиков

    void Clear() {
        sonde_type = 0;
        signature = 0;
        struct_size = 0;
        N_Tx = 4;
        sonde_name.clear();
        file_path.clear();
        file_name.clear();
        all_data.clear();
        gp_data.clear();
    }

    size_t GetFrameCount() const {
        return gp_data.size();
    }
};
