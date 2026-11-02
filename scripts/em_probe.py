#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
em_probe.py — эмулятор прибора для Electromagnetic Tools 3.0.

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.

Назначение:
  Имитация зонда/станции по UDP :5000 (и опционально COM через pyserial):
  - командный протокол [CMD][LEN16][DATA][CRC16] (как в AIR_TEMP_1.0/variable.cpp);
  - калибровочный протокол 0x14 (как в MAGNETOMETR_CAL/DeviceProtocol.h);
  - генерация кадров телеметрии ALL_DATA (Картограф, 3072 B) и GP_DATA (320 B);
  - генерация G/M/W векторов для инклинометрии;
  - веб-пульт управления на http://<host>:8080.

Использование:
  python em_probe.py --bind 0.0.0.0 --port 5000 --http 8080 --sonde 352
  python em_probe.py --serial COM12          # опционально COM-канал (нужен pyserial)

Зависимости: только стандартная библиотека Python 3.8+.
"""

import argparse
import cmath
import json
import math
import os
import socket
import struct
import sys
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

# ---------------------------------------------------------------------------
# Константы протокола (AIR_TEMP_1.0/Constants.h)
# ---------------------------------------------------------------------------
TOOL_KEY = 0x01 << 4
CMD_GET_DATA = 0x01 | TOOL_KEY
CMD_SET_METROLOGY = 0x02 | TOOL_KEY
CMD_GET_METROLOGY = 0x03 | TOOL_KEY
CMD_SET_PRESETS = 0x04 | TOOL_KEY
CMD_GET_PRESETS = 0x05 | TOOL_KEY
CMD_GET_SIGNATURE = 0x06 | TOOL_KEY
CMD_SET_TELESYSTEM = 0x07 | TOOL_KEY
CMD_SET_WORK_TYPE = 0x19 | TOOL_KEY
CMD_SET_AIR = 0x1A
CMD_TIME_SYNC = 0xF5
CMD_ERAM = 0x61
CMD_TURBO = 0xFD
CMD_WORK = 0x67
CMD_READ_EE = 0x65
CMD_WRITE_EE = 0x66

# Подкоманды калибровочного протокола 0x14 (MAGNETOMETR_CAL)
SUB_GMW = 0xBB
SUB_GET_CAL_M = 0xCC
SUB_GET_CAL_G = 0xCA
SUB_SET_CAL_M = 0xDD
SUB_SET_CAL_G = 0xDA

# Типы приборов
SONDE_TYPES = {
    352: ("КАРТОГРАФ", "cartograph", 4),
    351: ("КАРТОГРАФ RAW", "cartograph", 4),
    349: ("КАРТОГРАФ LWD 4Tx", "gp", 4),
    242: ("LWD 4Tx NEW", "gp", 4),
    231: ("LWD 3Tx", "gp", 3),
    151: ("AUTONOM 5Tx", "gp", 5),
    152: ("AUTONOM 5Tx SDR", "gp", 5),
}

# «Флэш» метрологии прибора — файл рядом со скриптом, имя по типу прибора
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
METRO_FILES = {
    352: "metro_cartograf.bin",
    351: "metro_cartograf_raw.bin",
    349: "metro_cartograf_lwd_4tx.bin",
    242: "metro_lwd_4tx_new.bin",
    231: "metro_lwd_3tx.bin",
    151: "metro_autonom_5Tx.bin",
    152: "metro_autonom_5Tx_sdr.bin",
}

def metro_path(sonde_type):
    return os.path.join(SCRIPT_DIR, METRO_FILES.get(sonde_type, "metro_%d.bin" % sonde_type))

# Таблица CRC16 (Modbus) — побайтовый перенос из variable.cpp
CRC_TABLE = (
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
)


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc = ((crc >> 8) ^ CRC_TABLE[(b ^ crc) & 0xFF]) & 0xFFFF
    return crc


# ---------------------------------------------------------------------------
# Сборка/разбор пакетов командного протокола
# ---------------------------------------------------------------------------
def build_packet(cmd: int, data: bytes) -> bytes:
    total = len(data) + 5
    pkt = bytes([cmd]) + struct.pack("<H", total) + data
    return pkt + struct.pack("<H", crc16(pkt))


def parse_packet(buf: bytes):
    """Разбор [CMD][LEN16][DATA][CRC16]. Возврат (cmd, data) или None."""
    if len(buf) < 5:
        return None
    total = struct.unpack("<H", buf[1:3])[0]
    if total != len(buf) or total < 5:
        return None
    if crc16(buf[:-2]) != struct.unpack("<H", buf[-2:])[0]:
        return ("CRC_ERR", b"")
    return (buf[0], buf[3:-2])


def build_packet14(payload5: bytes) -> bytes:
    """Пакет калибровочного протокола: [0x14][LEN][...][CRC16]"""
    total = len(payload5) + 2
    pkt = payload5 + struct.pack("<H", total)[1:]  # placeholder — собирается ниже
    # Реальная сборка: [0x14][total_len_lo][total_len_hi][data...][crc]
    body = bytes([0x14]) + struct.pack("<H", total) + payload5[3:]
    return body + struct.pack("<H", crc16(body))


# ---------------------------------------------------------------------------
# Структуры данных прибора
# ---------------------------------------------------------------------------
def pack_vec(v):
    return struct.pack("<fff", *v)


def pack_cal(offset=(0.0, 0.0, 0.0)):
    """Cal = Vec offset + MATRIX_3_3 sens (единичная) = 48 B"""
    sens = (1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0)
    return pack_vec(offset) + struct.pack("<fffffffff", *sens)


def pack_tgmw(g, m, w):
    return pack_vec(g) + pack_vec(m) + pack_vec(w)


def pack_direct_rx(temp, cond, rot_phase):
    """direct_RX = 128 B: Tx_0 cplx, Geo[4], amp_Vzz[4], amp_Vzx[4],
    ph_Vzx_Vzz[4], dv[4], border_angle[4], temperature, condition"""
    b = bytearray()
    # complex Tx_0 (Re, Im)
    b += struct.pack("<ff", 1000.0 * math.cos(rot_phase), 1000.0 * math.sin(rot_phase))
    for i in range(4):
        amp = 800.0 + 150.0 * i
        b += struct.pack("<ff", amp * math.cos(rot_phase + i), amp * math.sin(rot_phase + i))
    for i in range(4):
        b += struct.pack("<f", 800.0 + 150.0 * i)        # amp_Vzz
    for i in range(4):
        b += struct.pack("<f", 600.0 + 100.0 * i)        # amp_Vzx
    for i in range(4):
        b += struct.pack("<f", 0.5 + 0.1 * i)            # ph_Vzx_Vzz
    for i in range(4):
        b += struct.pack("<f", 2.0 + i * 0.5)            # dv %
    for i in range(4):
        b += struct.pack("<f", 45.0 + i * 30.0)          # border_angle
    b += struct.pack("<f", temp)
    b += struct.pack("<I", cond)
    return bytes(b)


def pack_undirect_rx(temp, cond, rot_phase):
    """undirect_RX = 80 B: Tx_0[2] cplx, Rzz1[4] cplx, Rzz2[4] cplx, temp, cond"""
    b = bytearray()
    for _ in range(2):
        b += struct.pack("<ff", 950.0 * math.cos(rot_phase), 950.0 * math.sin(rot_phase))
    for i in range(4):
        b += struct.pack("<ff", (700 + 120 * i) * math.cos(rot_phase), (700 + 120 * i) * math.sin(rot_phase))
    for i in range(4):
        b += struct.pack("<ff", (650 + 110 * i) * math.cos(rot_phase), (650 + 110 * i) * math.sin(rot_phase))
    b += struct.pack("<f", temp)
    b += struct.pack("<I", cond)
    return bytes(b)


def pack_data_block(signature, frame, dds_freq, temp, cond, rot_phase,
                    inc_abs, inc_zen, inc_azm, start_sector, wg):
    """DATA = 512 B (SondeDataTypes.h)"""
    b = bytearray()
    b += struct.pack("<III", signature, frame, dds_freq)
    for i in range(4):
        b += struct.pack("<f", -20.0 - i * 5.0)              # ATT_dB_geo_signal_smt
    for i in range(4):
        b += struct.pack("<f", (i * 10.0 + rot_phase * 57.3) % 360.0)  # PH_deg_geo_signal_smt
    for i in range(4):
        b += struct.pack("<f", 1.0 + 0.5 * i)                # rho_ph_smt
    for i in range(4):
        b += struct.pack("<f", (i * 5.0 + rot_phase) % 6.28)  # phase_smt
    b += bytes(32)                                           # out_arr[32]
    b += struct.pack("<II", frame * 16, 0)                   # all_bit_cntr, depth
    b += pack_direct_rx(temp, cond, rot_phase)               # R_zx[0] Rx_L
    b += pack_direct_rx(temp, cond, rot_phase + 0.3)         # R_zx[1] Rx_R
    b += pack_undirect_rx(temp, cond, rot_phase)             # R_zz
    b += struct.pack("<fff", inc_abs, inc_zen, inc_azm)      # INC
    b += struct.pack("<HH", start_sector & 0xFFFF, 1)        # start_sector, DataValid
    b += struct.pack("<f", wg)                               # Wg
    for i in range(4):
        b += struct.pack("<f", 0.8 + 0.2 * i)                # rho_att_smt
    for i in range(4):
        b += struct.pack("<f", -25.0 - i * 4.0)              # att_dB_smt
    assert len(b) == 512, len(b)
    return bytes(b)


def pack_raw_block(rot_phase):
    """RAW_DATA = 1024 B: R_zx[2][4] по 16 секторов Re/Im"""
    b = bytearray()
    for rx in range(2):
        for tx in range(4):
            for s in range(16):
                angle = rot_phase + s * (2.0 * math.pi / 16.0) + tx * 0.2 + rx * 0.1
                amp = 500.0 + 80.0 * tx + 40.0 * rx
                b += struct.pack("<f", amp * math.cos(angle))   # Re
            for s in range(16):
                angle = rot_phase + s * (2.0 * math.pi / 16.0) + tx * 0.2 + rx * 0.1
                amp = 500.0 + 80.0 * tx + 40.0 * rx
                b += struct.pack("<f", amp * math.sin(angle))   # Im
    assert len(b) == 1024, len(b)
    return bytes(b)


def pack_all_data(signature, frame400, frame2000, temp, cond, rot_phase,
                  inc_abs, inc_zen, inc_azm, start_sector, wg):
    """ALL_DATA = 3072 B: DATA+RAW для 400 и 2000 кГц"""
    b = bytearray()
    b += pack_data_block(signature, frame400, 400000, temp, cond, rot_phase,
                         inc_abs, inc_zen, inc_azm, start_sector, wg)
    b += pack_data_block(signature, frame2000, 2000000, temp, cond, rot_phase,
                         inc_abs, inc_zen, inc_azm, start_sector, wg)
    b += pack_raw_block(rot_phase)
    b += pack_raw_block(rot_phase + 0.15)
    assert len(b) == 3072, len(b)
    return bytes(b)


def pack_gp_data(signature, frame, temp, n_tx, rot_phase):
    """GP_DATA = 320 B (новый формат с размером в сигнатуре)"""
    b = bytearray()
    b += struct.pack("<III", signature, 0, frame)   # signature, condition, frame
    b += struct.pack("<f", temp)
    for f in range(2):
        for t in range(5):
            b += struct.pack("<f", 1.0 + 0.4 * t + (0.05 if f else 0.0))    # rho_ph_smt
    for f in range(2):
        for t in range(5):
            b += struct.pack("<f", (t * 0.3 + rot_phase) % 6.28)            # phase_smt
    for f in range(2):
        for t in range(5):
            b += struct.pack("<f", 900.0 + 130.0 * t if t < n_tx else 0.0)  # AM_RX_1
    for _ in range(2):
        b += struct.pack("<f", 5.0)                                          # ZERO_AM_RX_1
    for f in range(2):
        for t in range(5):
            b += struct.pack("<f", 850.0 + 120.0 * t if t < n_tx else 0.0)  # AM_RX_2
    for _ in range(2):
        b += struct.pack("<f", 4.0)                                          # ZERO_AM_RX_2
    for f in range(2):
        for t in range(5):
            b += struct.pack("<f", 0.02 * t + f * 0.01)                      # DELTA_PH
    for _ in range(2):
        b += struct.pack("<f", 0.5)                                          # ZERO_dPH
    for f in range(2):
        for t in range(5):
            b += struct.pack("<f", 0.9 + 0.15 * t)                           # rho_att_smt
    for f in range(2):
        for t in range(5):
            b += struct.pack("<f", -30.0 - 4.0 * t)                          # att_dB_smt
    assert len(b) == 320, len(b)
    return bytes(b)


def float32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def gp_resistivity(target, l1, l2, frequency, amplitude=False):
    if not all(math.isfinite(value) for value in (target, l1, l2, frequency)) or min(l1, l2, frequency) <= 0:
        return 0.0
    try:
        l1, l2 = float32(l1 / 1000.0), float32(l2 / 1000.0)
        def response(rho):
            sigma = float32(float32(float32(0.0078957) * frequency) / rho)
            ik = 1j * cmath.sqrt(1j * sigma)
            signal = cmath.exp(ik * (l2 - l1)) * (1.0 - ik * l2) / (1.0 - ik * l1)
            return float32(abs(signal) if amplitude else cmath.phase(signal))

        left, right = float32(0.01), 1000.0 if amplitude else 7000.0
        minimum, maximum = response(left), response(right)
        if not all(math.isfinite(value) for value in (minimum, maximum)):
            return 0.0
        if (target >= maximum) if amplitude else (target <= maximum):
            return 1200.0 if amplitude else 7200.0
        if (target <= minimum) if amplitude else (target >= minimum):
            return left
        for _ in range(60):
            mid = float32(0.5 * float32(left + right))
            diff = float32(response(mid) - target)
            if not math.isfinite(diff):
                return 0.0
            if abs(diff) < float32(1e-5) or float32(right - left) < float32(float32(1e-4) * mid):
                return mid
            if (diff < 0.0) if amplitude else (diff > 0.0):
                left = mid
            else:
                right = mid
        return float32(0.5 * float32(left + right))
    except (OverflowError, ValueError, ZeroDivisionError):
        return 0.0


def recalculate_gp(frame, metro, n_tx):
    for base in (16, 56, 240, 280):
        if base < len(frame):
            frame[base:base + 40] = bytes(40)
    if metro is None or len(metro) != 240:
        return
    for frequency in range(2):
        freq = struct.unpack_from("<H", metro, 28 + frequency * 2)[0]
        for tx in range(n_tx):
            index = frequency * 5 + tx
            sign = 1.0 if tx % 2 == 0 else -1.0
            l1 = struct.unpack_from("<H", metro, 8 + tx * 2)[0]
            l2 = struct.unpack_from("<H", metro, 18 + tx * 2)[0]
            raw_phase = struct.unpack_from("<f", frame, 192 + index * 4)[0]
            air_phase = struct.unpack_from("<h", metro, 32 + index * 2)[0]
            if math.isfinite(raw_phase):
                phase = float32(sign * float32(raw_phase - float32(air_phase / 57297.0)))
                struct.pack_into("<f", frame, 56 + index * 4, phase)
                struct.pack_into("<f", frame, 16 + index * 4, gp_resistivity(phase, l1, l2, freq))
            if len(frame) < 320:
                continue
            am1 = struct.unpack_from("<f", frame, 96 + index * 4)[0]
            am2 = struct.unpack_from("<f", frame, 144 + index * 4)[0]
            air_att = struct.unpack_from("<f", metro, 84 + index * 4)[0]
            if not all(math.isfinite(value) for value in (am1, am2, air_att)) or min(am1, am2) <= 0:
                continue
            try:
                ratio = float32(am2 / am1)
                att = float32(sign * float32(20.0 * float32(math.log10(ratio))) - air_att)
                if not math.isfinite(att):
                    continue
                struct.pack_into("<f", frame, 280 + index * 4, att)
                target = float32(10.0 ** float32(att / 20.0))
                struct.pack_into("<f", frame, 240 + index * 4, gp_resistivity(target, l1, l2, freq, True))
            except (OverflowError, ValueError):
                pass


def cartograph_frequency(block):
    value = struct.unpack_from("<I", block, 8)[0]
    if value in (400, 401, 400000):
        return 0
    if value in (2000, 2001, 2000000):
        return 1
    raise ValueError("неизвестная частота Картографа")


def recalculate_cartograph(block, metro):
    frequency = cartograph_frequency(block)
    gp = bytearray(320)
    for tx in range(4):
        index = frequency * 5 + tx
        first = complex(*struct.unpack_from("<ff", block, 388 + tx * 8))
        second = complex(*struct.unpack_from("<ff", block, 420 + tx * 8))
        try:
            phase = cmath.phase(second) - cmath.phase(first)
            if phase >= math.pi:
                phase -= 2 * math.pi
            if phase <= -math.pi:
                phase += 2 * math.pi
            struct.pack_into("<f", gp, 96 + index * 4, abs(first))
            struct.pack_into("<f", gp, 144 + index * 4, abs(second))
            struct.pack_into("<f", gp, 192 + index * 4, phase)
        except (OverflowError, ValueError):
            struct.pack_into("<f", gp, 192 + index * 4, float("nan"))
    valid_metro = metro is not None and len(metro) == 1536
    recalculate_gp(gp, metro[:240] if valid_metro else None, 4)
    for target, source in ((44, 16), (60, 56), (480, 240), (496, 280)):
        block[target:target + 16] = gp[source + frequency * 20:source + frequency * 20 + 16]
    block[12:44] = bytes(32)
    for rx in range(2):
        block[124 + rx * 128:156 + rx * 128] = bytes(32)
    if not valid_metro:
        return
    for tx in range(4):
        pure = []
        for rx in range(2):
            base = 116 + rx * 128
            index = (rx * 2 + frequency) * 4 + tx
            beta = struct.unpack_from("<f", metro, 860 + index * 4)[0]
            collar = struct.unpack_from("<f", metro, 924 + index * 4)[0]
            correction = struct.unpack_from("<f", metro, 988 + index * 4)[0]
            zz = struct.unpack_from("<f", block, base + 40 + tx * 4)[0]
            zx = struct.unpack_from("<f", block, base + 56 + tx * 4)[0]
            phase = struct.unpack_from("<f", block, base + 72 + tx * 4)[0]
            value = None
            if all(math.isfinite(v) for v in (beta, collar, correction, zz, zx, phase)) and zz > 0 and zx >= 0:
                try:
                    sine = math.sin(beta / 57.2958)
                    if abs(sine) > 1e-12:
                        raw = (zx / zz) * cmath.exp(1j * phase)
                        value = (raw * cmath.exp(-1j * correction / 57295.8) - collar) * (0.70710678 / sine)
                        geo = (1 - value) / (1 + value)
                        if not all(math.isfinite(v) for v in (value.real, value.imag, geo.real, geo.imag)):
                            raise ValueError("недопустимый геосигнал")
                        struct.pack_into("<ff", block, base + 8 + tx * 8, geo.real, geo.imag)
                except (OverflowError, ValueError, ZeroDivisionError):
                    block[base + 8 + tx * 8:base + 16 + tx * 8] = bytes(8)
                    value = None
            pure.append(value)
        if None in pure:
            continue
        try:
            boundary = 0.5 * (pure[0] - pure[1])
            geo = (1 - boundary) / (1 + boundary)
            attenuation = 20 * math.log10(abs(geo))
            phase = 57.296 * cmath.phase(geo)
            if all(math.isfinite(v) for v in (attenuation, phase)):
                struct.pack_into("<f", block, 12 + tx * 4, attenuation)
                struct.pack_into("<f", block, 28 + tx * 4, phase)
        except (OverflowError, ValueError, ZeroDivisionError):
            pass


def ro_to_bits_code(rho):
    if not math.isfinite(rho) or rho < 0.3:
        return 0
    result = 0
    for n in range(255):
        lower = 0.3 * (1.02 + 0.0000593 * n) ** n
        upper = 0.3 * (1.02 + 0.0000593 * (n + 1)) ** (n + 1)
        if lower <= rho <= upper:
            result = n if rho <= (lower + upper) / 2 else n + 1
        if n == 254 and rho >= upper:
            result = 255
    return result


def pack_cartograph_output(blocks, metro):
    for block in blocks:
        block[76:112] = bytes(36)
    if metro is None or len(metro) != 1536:
        return
    key = struct.unpack_from("<I", metro, 856)[0]
    enabled = [freq for freq in range(2) if key & (1 << (24 + freq))]
    if not enabled:
        return
    packed, count = 0, 0
    for freq in enabled:
        block = blocks[freq]
        for group in range(3):
            for tx in range(4):
                if not key & (1 << (freq * 12 + group * 4 + tx)):
                    continue
                value = struct.unpack_from("<f", block, (12, 28, 44)[group] + tx * 4)[0]
                if group == 2:
                    code, bits = ro_to_bits_code(value), 8
                else:
                    offset = 728 + group * 64 + (freq * 4 + tx) * 4
                    maximum = struct.unpack_from("<f", metro, offset)[0]
                    gradient = struct.unpack_from("<I", metro, offset + 32)[0]
                    if not math.isfinite(maximum) or maximum <= 0 or not gradient or not math.isfinite(value):
                        return
                    bits = int(math.log2(gradient + 1))
                    if not 1 <= bits <= 16:
                        return
                    try:
                        quantized = float32(float32(value + maximum) * float32(gradient / float32(2 * maximum)))
                        if not math.isfinite(quantized):
                            return
                        code = math.floor(quantized + 0.5) if quantized >= 0 else math.ceil(quantized - 0.5)
                    except (OverflowError, ValueError):
                        return
                if count + bits > 256:
                    return
                packed |= (code & 0xffff & ((1 << bits) - 1)) << count
                count += bits
    target = blocks[enabled[-1]]
    target[76:108] = packed.to_bytes(32, "little")
    struct.pack_into("<I", target, 108, count)


def pack_metrology_gp(signature, serial):
    """METROLOGY_GP = 240 B"""
    b = bytearray()
    b += struct.pack("<II", signature, serial)
    b += struct.pack("<5H", 360, 720, 1080, 1440, 1800)   # L1[5]
    b += struct.pack("<5H", 600, 960, 1320, 1680, 2040)   # L2[5]
    b += struct.pack("<2H", 400, 2000)                    # F[2], кГц
    for _ in range(10):
        b += struct.pack("<h", 0)                          # air_ph[2][5]
    for _ in range(10):
        b += struct.pack("<h", 50)                         # min_amp[2][5]
    b += struct.pack("<III", 120, 0, 0)                   # D_sonde_mm, work_type, Rx_Position
    for _ in range(10):
        b += struct.pack("<f", 0.0)                        # air_att_dB
    b += struct.pack("<q", int(time.time()))               # timestamp (time_t = 8 B)
    b += bytes(54 * 2)                                     # service[54]
    assert len(b) == 240, len(b)
    return bytes(b)


def pack_metrology_cartograph(signature, serial):
    """METROLOGY_CARTOGRAPH = 1000 B"""
    b = pack_metrology_gp(signature, serial)
    extra = bytearray()
    extra += struct.pack("<5I", 960, 1680, 2280, 3000, 0)  # L_geo_mm[5]
    # INC_SET = 48 B
    extra += struct.pack("<6I", 32, 32, 128, 128, 1, 0)
    extra += struct.pack("<ffffff", 3.14, 18.84, 0.5, math.pi / 360.0, 20.0 * math.pi / 180.0, 2.5)
    # TURN_PRESETS: Tx[6] + Tx_by_turn[3][5][6] + tx_work_time_units[7]
    extra += struct.pack("<6I", 0, 1, 2, 3, 4, 5)
    extra += bytes(3 * 5 * 6 * 4)
    extra += struct.pack("<7I", 5, 5, 6, 7, 8, 9, 0)
    extra += struct.pack("<q", int(time.time()))           # unix_epoh_data
    # REAP_CONST: ATT_max[2][4] f, ATT_grad[2][4] u32, PH_max f, PH_grad u32
    for value in (0.1, 0.42, 1.2, 2.7, 1.0, 4.1, 7.6, 10.0):
        extra += struct.pack("<f", value)
    for value in (31, 63, 255, 511, 63, 255, 511, 1024):
        extra += struct.pack("<I", value)
    for value in (4.0, 10.0, 18.0, 26.0, 18.0, 30.0, 42.0, 56.0):
        extra += struct.pack("<f", value)
    for value in (63, 255, 511, 511, 255, 255, 511, 511):
        extra += struct.pack("<I", value)
    extra += struct.pack("<I", 0x3FFFFFF)                   # select_key
    # betta_Z_deg[2][2][4], V_zx_colar_add[2][2][4], d_ph_Vzx_Vzz_mG[2][2][4]
    for _ in range(16):
        extra += struct.pack("<f", 45.0)
    for _ in range(16):
        extra += struct.pack("<f", 0.0)
    for _ in range(16):
        extra += struct.pack("<f", 0.0)
    extra += bytes(242 * 2)                                # service1[242]
    b += bytes(extra)
    return bytes(b)


# ---------------------------------------------------------------------------
# Файл-модель телеметрии: реальные записи .bin (голые структуры) и
# .dev (11-байтная преамбула перед каждой записью), проигрываются по кругу.
# Номер кадра подставляется свой — у прибора нет памяти измерений.
# ---------------------------------------------------------------------------
DEV_PREAMBLE = 11
CARTO_BLOCK = 1536  # CARTOGRAF_DATA_1FREQ


def struct_size_from_signature(sig):
    """Размер записи из Signature (как SondeID::FromSignature)."""
    size = sig // 1000000
    if size:
        return size
    stype = (sig % 1000000) // 1000
    if stype in (351, 352):
        return 3072
    return 240


def load_model_file(path):
    """Разобрать файл-модель → (frames, signature).
    frames — список полезных нагрузок GET_DATA:
    GP — целый GP_DATA; Картограф — блоки 1536 B, чередующиеся по частоте."""
    with open(path, "rb") as f:
        raw = f.read()
    pre = DEV_PREAMBLE if path.lower().endswith(".dev") else 0
    if len(raw) < pre + 4:
        raise ValueError("файл слишком мал")
    sig = struct.unpack_from("<I", raw, pre)[0]
    size = struct_size_from_signature(sig)
    stype = (sig % 1000000) // 1000
    frames = []
    if stype in (351, 352):
        if pre:
            rec = pre + min(size, CARTO_BLOCK)  # .dev — блоки по частоте
            if size < CARTO_BLOCK or len(raw) % rec:
                raise ValueError("неполная RAW-запись Картографа")
            for n in range(len(raw) // rec):
                frames.append(raw[n * rec + pre: n * rec + rec])
        else:
            if len(raw) % 3072:
                raise ValueError("неполный ALL_DATA Картографа")
            for n in range(len(raw) // 3072):
                blk = raw[n * 3072: n * 3072 + 3072]
                frames.append(blk[:512] + blk[1024:2048])
                frames.append(blk[512:1024] + blk[2048:3072])
        if len(frames) % 2:
            raise ValueError("нет пар частот Картографа")
        for index, frame in enumerate(frames):
            if struct.unpack_from("<I", frame)[0] != sig or cartograph_frequency(frame) != index % 2:
                raise ValueError("сигнатура или порядок частот Картографа изменены")
    else:
        if size not in (240, 320):
            raise ValueError("неподдерживаемый размер GP-модели")
        rec = pre + size
        if len(raw) % rec:
            raise ValueError("неполная запись GP-модели")
        for n in range(len(raw) // rec):
            frame = raw[n * rec + pre: n * rec + pre + size]
            if struct.unpack_from("<I", frame)[0] != sig:
                raise ValueError("изменение сигнатуры в GP-модели")
            frames.append(frame)
    if not frames:
        raise ValueError("нет целых записей")
    return frames, sig


def list_model_files():
    """Файлы-модели в папке скрипта (без «флэша» метрологии)."""
    out = []
    for name in sorted(os.listdir(SCRIPT_DIR)):
        low = name.lower()
        if (low.endswith(".bin") or low.endswith(".dev")) and not low.startswith("metro_"):
            out.append(name)
    return out


# ---------------------------------------------------------------------------
# Модель прибора
# ---------------------------------------------------------------------------
class ProbeModel:
    def __init__(self, sonde_type=151, serial=1):
        self.lock = threading.Lock()
        self.sonde_type = sonde_type
        self.serial = serial
        self.struct_size = 320
        self.kind = SONDE_TYPES[sonde_type][1]
        self.n_tx = SONDE_TYPES[sonde_type][2]
        # Параметры имитации (управляются с пульта)
        self.rpm = 60.0            # об/мин
        self.temperature = 25.0    # °C
        self.inc_zen = 45.0        # зенитный угол, град
        self.azimuth = 0.0         # азимут, град
        self.drop_packets = False  # терять ответы
        self.corrupt_crc = False   # портить CRC
        self.silent = False        # «прибор молчит» (полный облом связи)
        # Счётчики
        self.frame400 = 0
        self.frame2000 = 0
        self.rotation = 0.0        # рад
        self.rx_packets = 0
        self.tx_packets = 0
        self.crc_errors = 0
        self.dropped = 0
        self.cmd_log = []
        # EEPROM-эмуляция: метрология — «флэш» прибора, файл metro_<тип>.bin
        self.metrology = None
        self._load_metrology()
        self.cal_m = pack_cal()
        self.cal_g = pack_cal()
        self.presets = bytes(64)
        self.reboot_until = 0.0   # «потеря питания»: молчание до этого момента
        # Файл-модель телеметрии (реальные записи, выбираются на пульте)
        self.model_frames = None  # список кадров-байтов или None (синтез)
        self.model_pos = 0        # позиция проигрывания по кругу
        self.model_path = ""
        self.model_sig = 0        # Signature из файла — эмулируем именно этот прибор

    # Сигнатура: struct_size*1e6 + type*1e3 + number (SondeID::FromSignature)
    def signature(self):
        size = 3072 if self.kind == "cartograph" else self.struct_size
        if self.kind == "cartograph" and getattr(self, "model_frames", None) and self.model_sig % 1000000 // 1000 in (351, 352):
            size = self.model_sig // 1000000 or 1536
        return size * 1000000 + self.sonde_type * 1000 + self.serial

    def configure(self, sonde_type, serial, struct_size):
        if sonde_type not in SONDE_TYPES or not 0 <= serial <= 999 or struct_size not in (240, 320):
            raise ValueError("тип, номер 0..999 или размер 240/320 недопустимы")
        with self.lock:
            changed_type = sonde_type != self.sonde_type
            changed = changed_type or serial != self.serial or struct_size != self.struct_size
            self.sonde_type = sonde_type
            self.serial = serial
            self.struct_size = struct_size
            self.kind = SONDE_TYPES[sonde_type][1]
            self.n_tx = SONDE_TYPES[sonde_type][2]
            if changed:
                self.frame400 = self.frame2000 = self.model_pos = 0
                self._freq_flip = False
            if changed_type:
                self._load_metrology()

    def _gp_frame(self, data):
        frame = bytearray(data[:self.struct_size])
        if len(frame) != self.struct_size:
            raise ValueError("модель короче выбранного GP-кадра")
        struct.pack_into("<I", frame, 0, self.signature())
        condition = struct.unpack_from("<I", frame, 4)[0]
        mask = (1 << self.n_tx) - 1
        struct.pack_into("<I", frame, 4, condition & (0xffff0000 | (mask << 8) | mask))
        for offset in (16, 56, 96, 144, 192, 240, 280):
            if offset >= len(frame):
                continue
            for freq in range(2):
                for tx in range(self.n_tx, 5):
                    struct.pack_into("<f", frame, offset + (freq * 5 + tx) * 4, 0.0)
        recalculate_gp(frame, self.metrology, self.n_tx)
        return bytes(frame)

    def _cartograph_pair(self, source):
        blocks = [bytearray(block) for block in source]
        for frequency, block in enumerate(blocks):
            struct.pack_into("<I", block, 0, self.signature())
            struct.pack_into("<I", block, 8, (401, 2001)[frequency])
            recalculate_cartograph(block, self.metrology)
        pack_cartograph_output(blocks, self.metrology)
        return blocks

    def step(self, dt):
        """Обновить состояние модели (вращение, счётчики кадров)."""
        with self.lock:
            self.rotation += 2.0 * math.pi * (self.rpm / 60.0) * dt
            self.rotation %= 2.0 * math.pi
            self.azimuth = math.degrees(self.rotation) % 360.0

    def telemetry_frame(self):
        """Текущий кадр телеметрии по типу прибора.
        Картограф отдаёт один блок CARTOGRAF_DATA_1FREQ (1536 Б) на запрос —
        кадры чередуются по частоте, как у настоящего прибора (dds_freq 401/2001).
        GP-приборы отдают целый GP_DATA."""
        with self.lock:
            source_cartograph = self.model_sig % 1000000 // 1000 in (351, 352)
            if self.model_frames and source_cartograph == (self.kind == "cartograph"):
                # Проигрывание реальных записей по кругу; номер кадра — свой.
                model_index = self.model_pos
                blk = bytearray(self.model_frames[model_index])
                self.model_pos = (self.model_pos + 1) % len(self.model_frames)
                if self.kind == "cartograph":
                    # DATA: signature(4), frame(4), dds_freq(4) — счётчик по частоте
                    pair_start = model_index - model_index % 2
                    pair = self._cartograph_pair(self.model_frames[pair_start:pair_start + 2])
                    blk = pair[model_index % 2]
                    if cartograph_frequency(blk) == 0:
                        self.frame400 += 1
                        struct.pack_into("<I", blk, 4, self.frame400)
                    else:
                        self.frame2000 += 1
                        struct.pack_into("<I", blk, 4, self.frame2000)
                else:
                    # GP_DATA: signature(4), condition(4), frame(4)
                    self.frame400 += 1
                    struct.pack_into("<I", blk, 8, self.frame400)
                return bytes(blk) if self.kind == "cartograph" else self._gp_frame(blk)
            sig = self.signature()
            if self.kind == "cartograph":
                # Чередование частот: 400 → 2000 → 400 ...
                if not hasattr(self, "_freq_flip"):
                    self._freq_flip = False
                self._freq_flip = not self._freq_flip
                start_sector = int((self.rotation / (2 * math.pi)) * 16) % 16
                wg = self.rpm * 2.0 * math.pi / 60.0
                frequency = 0 if self._freq_flip else 1
                if frequency == 0:
                    self.frame400 += 1
                else:
                    self.frame2000 += 1
                pair = [pack_data_block(sig, counter, dds, self.temperature, 0,
                                        self.rotation, 0.0, self.inc_zen, self.azimuth,
                                        start_sector, wg) + pack_raw_block(self.rotation + freq * 0.15)
                        for freq, (counter, dds) in enumerate(((self.frame400, 401), (self.frame2000, 2001)))]
                return bytes(self._cartograph_pair(pair)[frequency])
            else:
                self.frame400 += 1
                return self._gp_frame(pack_gp_data(sig, self.frame400, self.temperature,
                                    self.n_tx, self.rotation))

    def gmw(self):
        """Векторы G/M/W для инклинометрии при текущем положении."""
        with self.lock:
            zr = math.radians(self.inc_zen)
            r = self.rotation
            # G: ось Z прибора; зенит zr; вращение вокруг Z
            g = (math.sin(zr) * math.cos(r), math.sin(zr) * math.sin(r), math.cos(zr))
            # M: магнитное поле ~0.5 Гс, склонение 60°, вращается с прибором
            incl = math.radians(60.0)
            m = (0.5 * (math.cos(incl) * math.cos(r) + math.sin(incl) * math.sin(zr) * math.sin(r)),
                 0.5 * (math.cos(incl) * math.sin(r) - math.sin(incl) * math.sin(zr) * math.cos(r)),
                 0.5 * (math.sin(incl) * math.cos(zr)))
            w = (0.0, 0.0, self.rpm * 2.0 * math.pi / 60.0)
            return g, m, w

    def _load_metrology(self):
        """Подгрузить метрологию из файла; при отсутствии — прошить дефолт и создать файл."""
        path = metro_path(self.sonde_type)
        try:
            with open(path, "rb") as f:
                self.metrology = f.read()
            if self.metrology:
                return
        except OSError:
            pass
        sig = self.signature()
        blob = (pack_metrology_cartograph(sig, self.serial)
                if self.kind == "cartograph" else pack_metrology_gp(sig, self.serial))
        try:
            with open(path, "wb") as f:
                f.write(blob)
        except OSError:
            pass
        self.metrology = blob

    def save_metrology(self, data):
        """SET_METROLOGY: записать во «флэш» — файл перезаписывается."""
        self.metrology = bytes(data)
        try:
            with open(metro_path(self.sonde_type), "wb") as f:
                f.write(self.metrology)
        except OSError:
            pass

    def metrology_blob(self):
        if self.metrology is None:
            self._load_metrology()
        blob = bytearray(self.metrology)
        expected = 240 if self.kind == "gp" else 1536
        if len(blob) != expected:
            raise ValueError("неверный размер метрологии")
        struct.pack_into("<II", blob, 0, self.signature(), self.serial)
        return bytes(blob)

    def load_model(self, path):
        """Загрузить файл-модель телеметрии; прибор = то, что записано в файле."""
        frames, sig = load_model_file(path)  # ошибки — наружу, ловит пульт
        stype = (sig % 1000000) // 1000
        if stype not in SONDE_TYPES:
            raise ValueError("неизвестный тип модели")
        source_kind = SONDE_TYPES[stype][1]
        if source_kind == "gp" and (SONDE_TYPES[stype][2] != 5 or any(len(frame) != 320 for frame in frames)):
            raise ValueError("для GP требуется модель AUTONOM 5Tx размером 320 байт")
        with self.lock:
            self.model_frames = frames
            self.model_pos = 0
            self.model_path = path
            self.model_sig = sig
            if source_kind != self.kind:
                self.sonde_type = stype
                self.kind = source_kind
                self.n_tx = SONDE_TYPES[stype][2]
                self.serial = sig % 1000
                self._load_metrology()
            self.log_cmd("Модель «%s»: %d записей, sig=%d"
                         % (os.path.basename(path), len(frames), sig))

    def clear_model(self):
        with self.lock:
            self.model_frames = None
            self.model_path = ""
            self.model_sig = 0
            self.model_pos = 0
            self.log_cmd("Модель выключена — синтез")

    def power_loss(self):
        """Потеря питания: счётчики кадров сбрасываются, ~2.5 с прибор молчит (перезагрузка)."""
        with self.lock:
            self.frame400 = 0
            self.frame2000 = 0
            self._freq_flip = False
            self.reboot_until = time.time() + 2.5
            self.log_cmd("ПОТЕРЯ ПИТАНИЯ — перезагрузка, счётчики кадров сброшены")

    def log_cmd(self, text):
        self.cmd_log.append("%s %s" % (time.strftime("%H:%M:%S"), text))
        if len(self.cmd_log) > 60:
            self.cmd_log = self.cmd_log[-60:]


# ---------------------------------------------------------------------------
# Обработчик команд
# ---------------------------------------------------------------------------
def handle_command(model: ProbeModel, buf: bytes):
    """Возвращает байты ответа или None (нет ответа)."""
    with model.lock:
        model.rx_packets += 1

    parsed = parse_packet(buf)
    if parsed is None:
        with model.lock:
            model.log_cmd("RX %d B — не пакет" % len(buf))
        return None

    cmd, data = parsed
    if cmd == "CRC_ERR":
        with model.lock:
            model.crc_errors += 1
            model.log_cmd("RX — ошибка CRC, игнорируем")
        return None

    # Калибровочный протокол 0x14
    if cmd == 0x14:
        return handle_cmd14(model, buf)

    # Лёгкий отклик для журнала
    names = {CMD_GET_DATA: "GET_DATA", CMD_GET_SIGNATURE: "GET_SIGNATURE",
             CMD_GET_METROLOGY: "GET_METROLOGY", CMD_SET_METROLOGY: "SET_METROLOGY",
             CMD_GET_PRESETS: "GET_PRESETS", CMD_SET_PRESETS: "SET_PRESETS",
             CMD_SET_TELESYSTEM: "SET_TELESYSTEM", CMD_SET_WORK_TYPE: "SET_WORK_TYPE",
             CMD_WORK: "WORK", CMD_TURBO: "TURBO", CMD_TIME_SYNC: "TIME_SYNC",
             CMD_READ_EE: "READ_EE", CMD_WRITE_EE: "WRITE_EE", CMD_ERAM: "ERAM"}
    name = names.get(cmd, "0x%02X" % cmd)

    if cmd == CMD_GET_SIGNATURE:
        resp = build_packet(cmd, struct.pack("<I", model.signature()))
    elif cmd == CMD_GET_DATA:
        resp = build_packet(cmd, model.telemetry_frame())
    elif cmd == CMD_GET_METROLOGY:
        resp = build_packet(cmd, model.metrology_blob())
    elif cmd == CMD_SET_METROLOGY:
        model.save_metrology(data)
        resp = build_packet(cmd, b"")
    elif cmd == CMD_GET_PRESETS:
        resp = build_packet(cmd, model.presets)
    elif cmd == CMD_SET_PRESETS:
        model.presets = data
        resp = build_packet(cmd, b"")
    else:
        # Общий ACK для остальных команд
        resp = build_packet(cmd, b"")

    with model.lock:
        model.log_cmd("RX %s (%d B) → TX %d B" % (name, len(buf), len(resp)))
    return resp


def handle_cmd14(model: ProbeModel, buf: bytes):
    """Калибровочный протокол (MAGNETOMETR_CAL): [0x14][LEN][0x00][0xAA][SUB][...]"""
    if len(buf) < 7:
        return None
    sub = buf[4]
    if sub == SUB_GMW:
        g, m, w = model.gmw()
        payload = bytes([0x14, 0x2B, 0x00, 0xAA, 0xBB]) + pack_tgmw(g, m, w)
        resp = payload + struct.pack("<H", crc16(payload))
    elif sub == SUB_GET_CAL_M:
        payload = bytes([0x14, 0x37, 0x00, 0xAA, 0xCC]) + model.cal_m
        resp = payload + struct.pack("<H", crc16(payload))
    elif sub == SUB_GET_CAL_G:
        payload = bytes([0x14, 0x37, 0x00, 0xAA, 0xCA]) + model.cal_g
        resp = payload + struct.pack("<H", crc16(payload))
    elif sub == SUB_SET_CAL_M and len(buf) >= 53:
        model.cal_m = buf[5:53]
        payload = bytes([0x14, 0x05, 0x00])
        resp = payload + struct.pack("<H", crc16(payload))
    elif sub == SUB_SET_CAL_G and len(buf) >= 53:
        model.cal_g = buf[5:53]
        payload = bytes([0x14, 0x05, 0x00])
        resp = payload + struct.pack("<H", crc16(payload))
    else:
        with model.lock:
            model.log_cmd("RX 0x14 sub=0x%02X — неизвестная подкоманда" % sub)
        return None

    with model.lock:
        model.log_cmd("RX 0x14 sub=0x%02X → TX %d B" % (sub, len(resp)))
    return resp


# ---------------------------------------------------------------------------
# UDP-сервер и опциональный COM
# ---------------------------------------------------------------------------
def udp_server(model: ProbeModel, bind: str, port: int, stop: threading.Event):
    srv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((bind, port))
    srv.settimeout(0.5)
    print("[UDP] Слушаю %s:%d" % (bind, port))
    while not stop.is_set():
        try:
            data, addr = srv.recvfrom(65535)
        except socket.timeout:
            continue
        except OSError:
            break
        resp = handle_command(model, data)
        if resp is None:
            continue
        with model.lock:
            if model.silent or time.time() < model.reboot_until:
                model.dropped += 1
                continue
            if model.drop_packets and (model.rx_packets % 3 == 0):
                model.dropped += 1
                continue
            if model.corrupt_crc:
                resp = resp[:-1] + bytes([resp[-1] ^ 0xFF])
            model.tx_packets += 1
        srv.sendto(resp, addr)


def serial_server(model: ProbeModel, port_name: str, baud: int, stop: threading.Event):
    try:
        import serial  # pyserial
    except ImportError:
        print("[COM] pyserial не установлен — COM-канал выключен")
        return
    try:
        port = serial.Serial(port_name, baudrate=baud, timeout=0.5)
    except Exception as e:
        print("[COM] %s не открыт: %s" % (port_name, e))
        return
    print("[COM] Открыт %s @ %d" % (port_name, baud))
    buf = bytearray()
    while not stop.is_set():
        try:
            chunk = port.read(4096)
        except Exception:
            break
        if not chunk:
            continue
        buf += chunk
        # Вычленяем целые пакеты по LEN16
        while len(buf) >= 5:
            total = struct.unpack("<H", bytes(buf[1:3]))[0]
            if total < 5 or total > 65535 or len(buf) < total:
                break
            pkt = bytes(buf[:total])
            del buf[:total]
            resp = handle_command(model, pkt)
            if resp is None:
                continue
            with model.lock:
                if model.silent or time.time() < model.reboot_until:
                    model.dropped += 1
                    continue
                if model.corrupt_crc:
                    resp = resp[:-1] + bytes([resp[-1] ^ 0xFF])
                model.tx_packets += 1
            port.write(resp)
    try:
        port.close()
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Веб-пульт :8080
# ---------------------------------------------------------------------------
REMOTE_HTML = """<!doctype html><html><head><meta charset="utf-8">
<title>EM Probe — пульт эмулятора</title>
<style>
body{{font-family:Segoe UI,Arial,sans-serif;margin:16px;background:#f4f5f7;color:#222;max-width:960px}}
h1{{font-size:20px}} h2{{font-size:15px;margin-top:18px}}
.card{{background:#fff;border:1px solid #ddd;border-radius:6px;padding:12px;margin:8px 0}}
label{{display:inline-block;width:150px}}
input[type=range]{{width:240px;vertical-align:middle}}
button{{padding:4px 14px}}
.log{{font-family:Consolas,monospace;font-size:12px;background:#fff;border:1px solid #ddd;
  border-radius:6px;padding:8px;height:220px;overflow-y:scroll;white-space:pre-wrap}}
.stat b{{color:#176}}
</style></head><body>
<h1>Эмулятор прибора — {sonde_name}</h1>
<form method="post" action="/set">
<div class="card"><h2>Прибор</h2>
<label>Тип:</label><select name="sonde_type">{sonde_opts}</select>
<label style="width:auto;margin-left:14px">№ серии:</label>
<input name="serial" type="number" min="0" max="999" size="4" value="{serial}">
<label style="width:auto;margin-left:14px">Размер GP:</label>
<select name="struct_size" {size_disabled}>{size_opts}</select>
<button type="submit">Применить</button></div>
<div class="card"><h2>Сбои связи</h2>
<label><input type="checkbox" name="silent" {silent}> Молчит (нет ответов)</label><br>
<label><input type="checkbox" name="drop_packets" {drop_packets}> Терять ответы (1/3)</label><br>
<label><input type="checkbox" name="corrupt_crc" {corrupt_crc}> Портить CRC</label>
<button type="submit">Применить</button></div>
</form>
<form method="post" action="/model">
<div class="card"><h2>Модель данных (реальные записи)</h2>
<label>Файл (.bin/.dev):</label><select name="model_file">{model_opts}</select>
<button type="submit" name="action" value="load">Загрузить</button>
<button type="submit" name="action" value="off">Синтез</button>
<p style="margin:8px 0 0">Текущая: <b>{model_name}</b> — {model_info}</p></div>
</form>
<form method="post" action="/power">
<div class="card"><h2>Питание</h2>
<button type="submit" name="power_loss" value="1">Потеря питания — перезагрузка прибора</button>
<span style="color:#a44;font-size:12px">Счётчики кадров сбрасываются, прибор ~2.5 с молчит</span></div>
</form>
<div class="card"><h2>Статистика канала</h2>
<p class="stat">Принято: <b id="stat-rx">{rx}</b> &nbsp; Отправлено: <b id="stat-tx">{tx}</b> &nbsp;
Потеряно: <b id="stat-dropped">{dropped}</b> &nbsp; Ошибок CRC: <b id="stat-crc_errors">{crc_errors}</b> &nbsp;
Кадры: <b id="stat-frames">{frame400}/{frame2000}</b> &nbsp; <span id="stat-power_state">{power_state}</span></p></div>
<div class="log" id="stat-log">{log}</div>
<script>
async function updateStats() {{
  try {{
    const response = await fetch('/stats', {{cache: 'no-store'}});
    if (!response.ok) return;
    const stats = await response.json();
    stats.frames = stats.frame + '/' + stats.frame2000;
    for (const key of ['rx', 'tx', 'dropped', 'crc_errors', 'frames', 'power_state', 'log']) {{
      document.getElementById('stat-' + key).textContent = stats[key];
    }}
  }} catch (error) {{
    document.getElementById('stat-power_state').textContent = 'Нет связи с пультом';
  }} finally {{
    setTimeout(updateStats, 5000);
  }}
}}
setTimeout(updateStats, 5000);
</script>
<p style="color:#888;font-size:12px">НПФ «АМК Горизонт» — эмулятор прибора em_probe.py</p>
</body></html>"""


def make_remote_handler(model: ProbeModel):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *args):
            pass

        def _opts(self):
            out = []
            for code, (name, kind, n) in SONDE_TYPES.items():
                sel = " selected" if code == model.sonde_type else ""
                out.append('<option value="%d"%s>%d — %s</option>' % (code, sel, code, name))
            return "".join(out)

        def _model_opts(self):
            out = []
            for name in list_model_files():
                sel = " selected" if name == os.path.basename(model.model_path) else ""
                out.append('<option value="%s"%s>%s</option>' % (name, sel, name))
            return "".join(out) or '<option value="">— файлов нет —</option>'

        def do_GET(self):
            if self.path == "/" or self.path.startswith("/?"):
                name = SONDE_TYPES[model.sonde_type][0]
                model_name = (os.path.basename(model.model_path)
                              if model.model_frames else "—")
                model_info = ("%d записей, кольцо" % len(model.model_frames)
                              if model.model_frames else "синтез")
                page = REMOTE_HTML.format(
                    sonde_name=name, sonde_opts=self._opts(), serial=model.serial,
                    size_opts="".join('<option value="%d"%s>%d байт</option>' %
                                      (size, " selected" if size == model.struct_size else "", size)
                                      for size in (240, 320)),
                    size_disabled="disabled" if model.kind == "cartograph" else "",
                    model_opts=self._model_opts(),
                    model_name=model_name, model_info=model_info,
                    rpm=int(model.rpm), temperature=model.temperature,
                    inc_zen=model.inc_zen,
                    silent="checked" if model.silent else "",
                    drop_packets="checked" if model.drop_packets else "",
                    corrupt_crc="checked" if model.corrupt_crc else "",
                    rx=model.rx_packets, tx=model.tx_packets, dropped=model.dropped,
                    crc_errors=model.crc_errors, frame400=model.frame400,
                    frame2000=model.frame2000,
                    power_state=("ПЕРЕЗАГРУЗКА…" if time.time() < model.reboot_until else "питание OK"),
                    azimuth=model.azimuth,
                    log="\n".join(reversed(model.cmd_log)))
                body = page.encode("utf-8")
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
            elif self.path == "/stats":
                body = json.dumps({
                    "rx": model.rx_packets, "tx": model.tx_packets,
                    "dropped": model.dropped, "crc_errors": model.crc_errors,
                    "frame": model.frame400, "frame2000": model.frame2000,
                    "azimuth": model.azimuth,
                    "power_state": "ПЕРЕЗАГРУЗКА…" if time.time() < model.reboot_until else "питание OK",
                    "log": "\n".join(reversed(model.cmd_log))}).encode()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
            else:
                self.send_response(404)
                self.end_headers()

        def do_POST(self):
            if self.path == "/power":
                model.power_loss()
                self.send_response(303)
                self.send_header("Location", "/")
                self.end_headers()
                return
            if self.path == "/model":
                ln = int(self.headers.get("Content-Length", 0))
                form = self.rfile.read(ln).decode("utf-8", "replace")
                params = {k: v[0] for k, v in urllib.parse.parse_qs(form).items()}
                if params.get("action") == "off":
                    model.clear_model()
                else:
                    path = params.get("model_path", "").strip()
                    if not path:
                        sel = params.get("model_file", "")
                        if sel:
                            path = os.path.join(SCRIPT_DIR, sel)
                    if path:
                        try:
                            model.load_model(path)
                        except (OSError, ValueError, struct.error) as e:
                            model.log_cmd("Модель не загружена: %s" % e)
                self.send_response(303)
                self.send_header("Location", "/")
                self.end_headers()
                return
            if self.path != "/set":
                self.send_response(404)
                self.end_headers()
                return
            ln = int(self.headers.get("Content-Length", 0))
            form = self.rfile.read(ln).decode("utf-8", "replace")
            params = {k: v[0] for k, v in urllib.parse.parse_qs(form).items()}
            try:
                # Другой прибор = другой «флэш» метрологии и свои счётчики
                model.configure(int(params.get("sonde_type", model.sonde_type)),
                                int(params.get("serial", model.serial)),
                                int(params.get("struct_size", model.struct_size)))
            except ValueError as error:
                self.send_error(400, str(error).encode("ascii", "replace").decode("ascii"))
                return
            with model.lock:
                for key, attr in (("rpm", "rpm"),
                                  ("temperature", "temperature"),
                                  ("inc_zen", "inc_zen")):
                    if key in params:
                        try:
                            setattr(model, attr,
                                    float(params[key]) if key != "serial" else int(params[key]))
                        except ValueError:
                            pass
                model.silent = "silent" in params
                model.drop_packets = "drop_packets" in params
                model.corrupt_crc = "corrupt_crc" in params
                model.log_cmd("Пульт: тип=%d rpm=%.0f T=%.1f zen=%.0f silent=%s drop=%s crc=%s" % (
                    model.sonde_type, model.rpm, model.temperature, model.inc_zen,
                    model.silent, model.drop_packets, model.corrupt_crc))
            self.send_response(303)
            self.send_header("Location", "/")
            self.end_headers()
    return Handler


# ---------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description="Эмулятор прибора EM Tools 3.0")
    ap.add_argument("--bind", default="0.0.0.0", help="IP прослушивания UDP (по умолчанию 0.0.0.0)")
    ap.add_argument("--port", type=int, default=5000, help="UDP-порт прибора (по умолчанию 5000)")
    ap.add_argument("--http", type=int, default=8080, help="Порт веб-пульта (по умолчанию 8080)")
    ap.add_argument("--sonde", type=int, default=151, choices=sorted(SONDE_TYPES),
                    help="Тип прибора (сигнатура), по умолчанию 151 — AUTONOM 5Tx")
    ap.add_argument("--serial", type=int, default=1, help="Номер прибора")
    ap.add_argument("--com", default=None, help="Имя COM-порта (опционально, нужен pyserial)")
    ap.add_argument("--baud", type=int, default=125000, help="Скорость COM (по умолчанию 125000)")
    args = ap.parse_args()

    model = ProbeModel(args.sonde, args.serial)
    stop = threading.Event()

    # Поток имитации движения
    def sim_loop():
        last = time.time()
        while not stop.is_set():
            now = time.time()
            model.step(now - last)
            last = now
            time.sleep(0.05)

    threading.Thread(target=sim_loop, daemon=True).start()
    threading.Thread(target=udp_server, args=(model, args.bind, args.port, stop), daemon=True).start()

    if args.com:
        threading.Thread(target=serial_server,
                         args=(model, args.com, args.baud, stop), daemon=True).start()

    http = ThreadingHTTPServer((args.bind, args.http), make_remote_handler(model))
    threading.Thread(target=http.serve_forever, daemon=True).start()

    name = SONDE_TYPES[args.sonde][0]
    print("=" * 60)
    print("  EM Probe — эмулятор прибора")
    print("  Прибор: %s (%d) №%d" % (name, args.sonde, args.serial))
    print("  UDP:   %s:%d" % (args.bind, args.port))
    print("  Пульт: http://localhost:%d" % args.http)
    print("=" * 60)

    try:
        while True:
            time.sleep(1.0)
    except KeyboardInterrupt:
        stop.set()
        http.shutdown()
        print("\nОстановлено.")


if __name__ == "__main__":
    main()
