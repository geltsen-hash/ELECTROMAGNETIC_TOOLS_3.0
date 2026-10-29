"""
Парсер сырых бинарных файлов зонда «Картограф».
Формат кадра: 3072 байта (структура ALL_DATA).
  - data[0]: 512 байт (частота 400 кГц, бортовые гармоники direct_RX)
  - data[1]: 512 байт (частота 2000 кГц, бортовые гармоники direct_RX)
  - raw_data[0]: 1024 байт (истинные 16 секторов Re/Im для 400 кГц)
  - raw_data[1]: 1024 байт (истинные 16 секторов Re/Im для 2000 кГц)

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

import os
import struct
from dataclasses import dataclass
from typing import List, Optional, Tuple
import numpy as np


FRAME_SIZE_BYTES = 3072
ADC_TO_MV = 0.00023842  # Коэффициент перевода отсчетов АЦП секторов в милливольты (Constants.h)

# Типы приборов семейства «Картограф» в сигнатуре DATA-блока (variable.h):
# CARTOGRAPH_LWD_4Tx_=349, CARTOGRAPH_RAW_=351, CARTOGRAPH_=352.
# Сигнатура decimal-кодирована: type = (signature % 1_000_000) // 1000.
CARTOGRAPH_TYPES = {349, 351, 352}

# Смещения внутри каждого блока DATA (512 байт)
OFFSET_SIG = 0
OFFSET_FRAME = 4
OFFSET_FREQ = 8
OFFSET_RX_LEFT = 116
OFFSET_RX_RIGHT = 244
OFFSET_INC_AZM = 460
OFFSET_INC_ZEN = 464
OFFSET_INC_APS = 468
OFFSET_START_SEC = 472
OFFSET_DATA_VALID = 474
OFFSET_WG = 476

# Смещения внутри direct_RX (128 байт)
RX_OFFSET_VZZ = 40
RX_OFFSET_VZX = 56
RX_OFFSET_DPH = 72
RX_OFFSET_DV = 88


@dataclass
class ChannelHarmonics:
    """Гармоники, вычисленные бортовым процессором зонда."""
    vzz: float
    vzx: float
    dph_rad: float
    dv: float


@dataclass
class SectorData:
    """Истинные секторные данные 16 секторов (Re и Im в мВ)."""
    re_mv: np.ndarray  # shape (16,) float32
    im_mv: np.ndarray  # shape (16,) float32


@dataclass
class FrameData1Freq:
    signature: int
    frame_num: int
    dds_freq_khz: int
    wg_rpm: float
    zen_deg: float
    azm_deg: float
    aps_deg: float
    left: List[ChannelHarmonics]        # 4 передатчика Tx1..Tx4 (бортовые гармоники)
    right: List[ChannelHarmonics]       # 4 передатчика Tx1..Tx4 (бортовые гармоники)
    raw_left: List[SectorData]          # 4 передатчика Tx1..Tx4 (сырые 16 секторов Re/Im)
    raw_right: List[SectorData]         # 4 передатчика Tx1..Tx4 (сырые 16 секторов Re/Im)


@dataclass
class FrameRecord:
    frame_idx: int
    f400: FrameData1Freq
    f2000: FrameData1Freq


def _parse_data_block(buf: bytes, raw_buf: Optional[bytes] = None) -> FrameData1Freq:
    sig, frm, freq = struct.unpack_from('<III', buf, OFFSET_SIG)
    wg = struct.unpack_from('<f', buf, OFFSET_WG)[0]
    azm, zen, aps = struct.unpack_from('<fff', buf, OFFSET_INC_AZM)

    def _parse_rx_direct(base_offset: int) -> List[ChannelHarmonics]:
        vzz = struct.unpack_from('<4f', buf, base_offset + RX_OFFSET_VZZ)
        vzx = struct.unpack_from('<4f', buf, base_offset + RX_OFFSET_VZX)
        dph = struct.unpack_from('<4f', buf, base_offset + RX_OFFSET_DPH)
        dv = struct.unpack_from('<4f', buf, base_offset + RX_OFFSET_DV)
        return [
            ChannelHarmonics(vzz=vzz[i], vzx=vzx[i], dph_rad=dph[i], dv=dv[i])
            for i in range(4)
        ]

    left_rx = _parse_rx_direct(OFFSET_RX_LEFT)
    right_rx = _parse_rx_direct(OFFSET_RX_RIGHT)

    # Разбор сырых 16 секторов (RAW_DATA: struct _16_sector_data R_zx[2][4])
    # 2 Rx * 4 Tx * 128 байт = 1024 байта
    raw_left: List[SectorData] = []
    raw_right: List[SectorData] = []

    if raw_buf and len(raw_buf) >= 1024:
        for rx_idx, target_list in [(0, raw_left), (1, raw_right)]:
            for tx_idx in range(4):
                base = rx_idx * 512 + tx_idx * 128
                re_floats = np.array(struct.unpack_from('<16f', raw_buf, base), dtype=np.float32) * ADC_TO_MV
                im_floats = np.array(struct.unpack_from('<16f', raw_buf, base + 64), dtype=np.float32) * ADC_TO_MV
                target_list.append(SectorData(re_mv=re_floats, im_mv=im_floats))
    else:
        # Заглушки если сырых секторов нет
        dummy_re = np.zeros(16, dtype=np.float32)
        dummy_im = np.zeros(16, dtype=np.float32)
        raw_left = [SectorData(re_mv=dummy_re, im_mv=dummy_im) for _ in range(4)]
        raw_right = [SectorData(re_mv=dummy_re, im_mv=dummy_im) for _ in range(4)]

    return FrameData1Freq(
        signature=sig,
        frame_num=frm,
        dds_freq_khz=freq,
        wg_rpm=wg,
        zen_deg=zen,
        azm_deg=azm,
        aps_deg=aps,
        left=left_rx,
        right=right_rx,
        raw_left=raw_left,
        raw_right=raw_right,
    )


def read_cartograf_bin(file_path: str) -> List[FrameRecord]:
    """
    Чтение бинарного файла Cartograf.
    Возвращает список FrameRecord для каждого кадра по 3072 байта.
    """
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"Файл не найден: {file_path}")

    file_size = os.path.getsize(file_path)
    if file_size < FRAME_SIZE_BYTES or file_size % FRAME_SIZE_BYTES != 0:
        n_frames = file_size // FRAME_SIZE_BYTES
        if n_frames == 0:
            raise ValueError(f"Файл слишком мал для формата Cartograf: {file_size} байт")
    else:
        n_frames = file_size // FRAME_SIZE_BYTES

    records: List[FrameRecord] = []
    with open(file_path, 'rb') as f:
        for idx in range(n_frames):
            raw = f.read(FRAME_SIZE_BYTES)
            if len(raw) < FRAME_SIZE_BYTES:
                break
            # Проверка сигнатур обоих частотных блоков: файл должен состоять
            # только из кадров ALL_DATA семейства «Картограф». Файлы иного
            # формата (например, дамп float32 без заголовков) отклоняются
            # сразу, а не дают мусорный результат калибровки.
            sig0, sig1 = struct.unpack_from('<II', raw, 0)[0], struct.unpack_from('<I', raw, 512)[0]
            for blk, sig in ((0, sig0), (1, sig1)):
                sonde_type = (sig % 1_000_000) // 1000
                if sonde_type not in CARTOGRAPH_TYPES:
                    raise ValueError(
                        f"Неверная сигнатура кадра {idx} (блок {blk}, частота "
                        f"{'400' if blk == 0 else '2000'} кГц): signature={sig} "
                        f"(type={sonde_type}), файл не является потоком ALL_DATA "
                        f"«Картограф»: {file_path}")
            d0 = _parse_data_block(raw[0:512], raw[1024:2048])
            d1 = _parse_data_block(raw[512:1024], raw[2048:3072])
            records.append(FrameRecord(frame_idx=idx, f400=d0, f2000=d1))

    return records
