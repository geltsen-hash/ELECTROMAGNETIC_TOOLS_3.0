"""
Модуль формирования и записи бинарного файла метрологии зонда «Картограф» (.bin).
Размер файла: ровно 1536 байт (структура METROLOGY_CARTOGRAPH).
Записывает калибровочные массивы с точным аппаратным маппингом каналов:
  - Смещение 860: betta_Z_deg[2][2][4]     (16 floats, 64 байта)
  - Смещение 924: V_zx_colar_add[2][2][4]  (16 floats, 64 байта)
  - Смещение 988: d_ph_Vzx_Vzz_mG[2][2][4] (16 floats, 64 байта)

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

import os
import struct
from typing import Optional
import numpy as np

from .calibration_core import FullCalResult


METROLOGY_FILE_SIZE = 1536

# Смещения массивов калибровки внутри структуры METROLOGY_CARTOGRAPH
OFFSET_BETTA = 860
OFFSET_COLAR = 924
OFFSET_DPH   = 988


def save_metrology_bin(
    full_cal: FullCalResult,
    output_path: str,
    template_bin_path: Optional[str] = None,
    serial_number: Optional[int] = None,
) -> None:
    """
    Запись бинарного файла метрологии.
    Если указан template_bin_path, берёт базовые параметры (сигнатуру, базы ненаправленных антенн,
    температурные коэффициенты) из него, и перезаписывает калибровочные массивы направленных антенн.
    """
    if template_bin_path and os.path.exists(template_bin_path):
        with open(template_bin_path, 'rb') as f:
            buf = bytearray(f.read())
        if len(buf) != METROLOGY_FILE_SIZE:
            # Если размер отличается, дополняем или обрезаем до 1536
            buf = buf[:METROLOGY_FILE_SIZE]
            if len(buf) < METROLOGY_FILE_SIZE:
                buf.extend(b'\x00' * (METROLOGY_FILE_SIZE - len(buf)))
    else:
        # Создаем чистый буфер размером 1536 байт с базовой сигнатурой Cartograf (512351111)
        buf = bytearray(METROLOGY_FILE_SIZE)
        struct.pack_into('<I', buf, 0, 512351111)  # Cartograf signature
        struct.pack_into('<5H', buf, 8, 358, 720, 1078, 1437, 0)   # L1
        struct.pack_into('<5H', buf, 18, 598, 960, 1318, 1677, 0)  # L2
        struct.pack_into('<2H', buf, 28, 401, 2000)                # F

    if serial_number is not None:
        struct.pack_into('<I', buf, 4, serial_number)

    # 1. Запись betta_Z_deg [2][2][4]
    # shape (2, 2, 4): lr in 0..1, freq in 0..1, tx in 0..3
    betta_bytes = full_cal.betta_zx_deg.astype('<f4').tobytes()
    buf[OFFSET_BETTA : OFFSET_BETTA + 64] = betta_bytes

    # 2. Запись V_zx_colar_add [2][2][4]
    colar_bytes = full_cal.v_zx_colar_add.astype('<f4').tobytes()
    buf[OFFSET_COLAR : OFFSET_COLAR + 64] = colar_bytes

    # 3. Запись d_ph_Vzx_Vzz_mG [2][2][4]
    dph_bytes = full_cal.d_ph_mg.astype('<f4').tobytes()
    buf[OFFSET_DPH : OFFSET_DPH + 64] = dph_bytes

    out_dir = os.path.dirname(output_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(output_path, 'wb') as f:
        f.write(buf)
