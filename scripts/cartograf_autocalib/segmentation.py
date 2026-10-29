"""
Модуль автоматической сегментации и фильтрации сырых данных зонда «Картограф».
Выделяет участки квазистационарного вращения (Wg >= W_min),
отсекает переходные процессы раскрутки оператором и выбросы шума,
вычисляет средние гармоники Vzz, Vzx и круговую фазу dph.

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

import math
import re
from dataclasses import dataclass
from typing import List, Optional, Tuple, Dict
import numpy as np
import pandas as pd

from .bin_parser import FrameRecord, read_cartograf_bin, ChannelHarmonics
from .fourier_analysis import analyze_sector_data, compare_processor_with_raw, SectorHarmonics, ProcessorComparison


# Аппаратные слоты raw->R_zx и direct_RX для каждого зонда 1..4 по стандарту предприятия:
# Left (Rzx1):  Зонд 1 (0.96м)->слот 0, Зонд 2 (1.68м)->слот 3, Зонд 3 (2.28м)->слот 1, Зонд 4 (3.00м)->слот 2
# Right (Rzx2): Зонд 1 (0.96м)->слот 1, Зонд 2 (1.68м)->слот 3, Зонд 3 (2.28м)->слот 0, Зонд 4 (3.00м)->слот 2
RAW_SLOT_MAP = {
    'L': {1: 0, 2: 3, 3: 1, 4: 2},
    'R': {1: 1, 2: 3, 3: 0, 4: 2},
}

# Базовые расстояния зондов L_m (в метрах) строго по возрастанию длины
BASE_LENGTHS = {
    'L': {1: 0.96, 2: 1.68, 3: 2.28, 4: 3.00},
    'R': {1: 0.96, 2: 1.68, 3: 2.28, 4: 3.00},
}

# Зеркальные пары по стандарту предприятия (Зонды 1..4):
MIRROR_PAIRS = [
    (('L', 1), ('R', 1), 0.96),
    (('L', 2), ('R', 2), 1.68),
    (('L', 3), ('R', 3), 2.28),
    (('L', 4), ('R', 4), 3.00),
]


@dataclass
class ChannelAverage:
    rx: str
    freq: int
    tx: int
    l_m: float
    # Бортовые гармоники (direct_RX)
    vzz_mean: float
    vzz_std: float
    vzx_mean: float
    vzx_std: float
    dph_rad_mean: float
    dph_rad_std: float
    dv_mean: float
    n_frames: int
    # Сырые секторные данные (RAW_DATA) и мусор Фурье
    raw_vzz: float
    raw_vzx: float
    raw_vxx_h2: float
    raw_dph_rad: float
    residual_noise_dv: float
    thd_pct: float
    h2_div_h1_pct: float
    snr_db: float
    processor_vzz_diff_pct: float
    processor_vzx_diff_pct: float
    processor_dph_diff_deg: float
    processor_status: str


@dataclass
class HeightSegment:
    height_m: float
    frame_start: int
    frame_end: int
    n_total_frames: int
    n_used_frames: int
    channels: List[ChannelAverage]


def extract_height_from_filename(filename: str) -> Optional[float]:
    """
    Извлечение высоты из имени файла, например:
    'rot_005_2025_440_mm.bin' -> 0.440 м
    'height_1.33m.bin' -> 1.33 м
    """
    m_mm = re.search(r'(\d+)\s*(?:_|-)?mm', filename, re.IGNORECASE)
    if m_mm:
        return float(m_mm.group(1)) / 1000.0

    m_cm = re.search(r'(\d+)\s*(?:_|-)?cm', filename, re.IGNORECASE)
    if m_cm:
        return float(m_cm.group(1)) / 100.0

    m_m = re.search(r'(\d+(?:\.\d+)?)\s*(?:_|-)?m(?:\.|$|_)', filename, re.IGNORECASE)
    if m_m:
        return float(m_m.group(1))

    return None


def circular_mean_and_std(angles_rad: np.ndarray) -> Tuple[float, float]:
    """Вычисление кругового среднего и СКО фазовых углов."""
    if len(angles_rad) == 0:
        return 0.0, 0.0
    sin_sum = np.sum(np.sin(angles_rad))
    cos_sum = np.sum(np.cos(angles_rad))
    mean_angle = math.atan2(sin_sum, cos_sum)
    # R_bar = sqrt(sin_sum^2 + cos_sum^2) / N
    r_bar = math.sqrt(sin_sum**2 + cos_sum**2) / len(angles_rad)
    r_bar = min(1.0, max(0.0, r_bar))
    std_angle = math.sqrt(max(0.0, -2.0 * math.log(r_bar))) if r_bar > 1e-6 else math.pi
    return mean_angle, std_angle


def process_frames_interval(
    records: List[FrameRecord],
    height_m: float,
    min_wg_rpm: float = 45.0,
    skip_start_frames: int = 10,
    max_dv_sigma: float = 3.5,
) -> HeightSegment:
    """
    Фильтрация и усреднение кадров одного интервала высоты.
    """
    n_total = len(records)
    # Отбор кадров с достаточной скоростью вращения
    valid_records: List[FrameRecord] = []
    spinup_done = False
    consecutive_fast = 0

    for r in records:
        # Проверяем Wg по обеим частотам (кадр валиден только при устойчивом вращении на 400 и 2000 кГц)
        wg = min(r.f400.wg_rpm, r.f2000.wg_rpm)
        if wg >= min_wg_rpm:
            consecutive_fast += 1
            if consecutive_fast > skip_start_frames:
                spinup_done = True
                valid_records.append(r)
        else:
            consecutive_fast = 0

    # Если после жесткого фильтра осталось слишком мало, берем мягче (без skip_start_frames)
    if len(valid_records) < 15:
        valid_records = [r for r in records if min(r.f400.wg_rpm, r.f2000.wg_rpm) >= (min_wg_rpm * 0.7)]

    # Если совсем мало, берем все кадры, где Wg > 20
    if len(valid_records) < 10:
        valid_records = [r for r in records if min(r.f400.wg_rpm, r.f2000.wg_rpm) > 20.0]

    if not valid_records:
        raise ValueError(f"На высоте {height_m:.3f} м нет кадров с вращением зонда (Wg >= 20 об/мин)")

    ch_averages: List[ChannelAverage] = []

    for freq, freq_attr in [(400, 'f400'), (2000, 'f2000')]:
        for rx_letter, rx_attr in [('L', 'left'), ('R', 'right')]:
            for sonde_idx in range(4):
                sonde_num = sonde_idx + 1
                raw_slot = RAW_SLOT_MAP[rx_letter][sonde_num]
                l_m = BASE_LENGTHS[rx_letter][sonde_num]

                vzz_vals = np.array([getattr(getattr(r, freq_attr), rx_attr)[raw_slot].vzz for r in valid_records], dtype=np.float64)
                vzx_vals = np.array([getattr(getattr(r, freq_attr), rx_attr)[raw_slot].vzx for r in valid_records], dtype=np.float64)
                dph_vals = np.array([getattr(getattr(r, freq_attr), rx_attr)[raw_slot].dph_rad for r in valid_records], dtype=np.float64)
                dv_vals  = np.array([getattr(getattr(r, freq_attr), rx_attr)[raw_slot].dv for r in valid_records], dtype=np.float64)

                # Фильтр по выбросам dv (3.5 sigma)
                dv_mean = float(np.mean(dv_vals))
                dv_std = float(np.std(dv_vals))
                if dv_std > 1e-6:
                    mask = (dv_vals <= dv_mean + max_dv_sigma * dv_std)
                    if np.sum(mask) >= 10:
                        vzz_vals = vzz_vals[mask]
                        vzx_vals = vzx_vals[mask]
                        dph_vals = dph_vals[mask]
                        dv_vals = dv_vals[mask]

                mean_dph, std_dph = circular_mean_and_std(dph_vals)

                # Анализ сырых 16 секторов (RAW_DATA)
                raw_attr = 'raw_' + rx_attr  # raw_left или raw_right
                re_arr = np.array([getattr(getattr(r, freq_attr), raw_attr)[raw_slot].re_mv for r in valid_records], dtype=np.float32)
                im_arr = np.array([getattr(getattr(r, freq_attr), raw_attr)[raw_slot].im_mv for r in valid_records], dtype=np.float32)
                if dv_std > 1e-6 and np.sum(mask) >= 10:
                    re_arr = re_arr[mask]
                    im_arr = im_arr[mask]
                re_mean = np.mean(re_arr, axis=0)
                im_mean = np.mean(im_arr, axis=0)

                sector_res = analyze_sector_data(re_mean, im_mean)
                direct_harm = ChannelHarmonics(
                    vzz=float(np.mean(vzz_vals)),
                    vzx=float(np.mean(vzx_vals)),
                    dph_rad=mean_dph,
                    dv=float(np.mean(dv_vals)),
                )
                comp_res = compare_processor_with_raw(direct_harm, sector_res)

                ch_averages.append(ChannelAverage(
                    rx=rx_letter,
                    freq=freq,
                    tx=sonde_num,
                    l_m=l_m,
                    vzz_mean=float(np.mean(vzz_vals)),
                    vzz_std=float(np.std(vzz_vals)),
                    vzx_mean=float(np.mean(vzx_vals)),
                    vzx_std=float(np.std(vzx_vals)),
                    dph_rad_mean=mean_dph,
                    dph_rad_std=std_dph,
                    dv_mean=float(np.mean(dv_vals)),
                    n_frames=len(vzz_vals),
                    raw_vzz=sector_res.h0_vzz,
                    raw_vzx=sector_res.h1_vzx,
                    raw_vxx_h2=sector_res.h2_vxx,
                    raw_dph_rad=sector_res.dph_rad,
                    residual_noise_dv=sector_res.residual_noise_dv,
                    thd_pct=sector_res.thd_pct,
                    h2_div_h1_pct=sector_res.h2_div_h1_pct,
                    snr_db=sector_res.snr_db,
                    processor_vzz_diff_pct=comp_res.vzz_diff_pct,
                    processor_vzx_diff_pct=comp_res.vzx_diff_pct,
                    processor_dph_diff_deg=comp_res.dph_diff_deg,
                    processor_status=comp_res.status,
                ))

    return HeightSegment(
        height_m=height_m,
        frame_start=records[0].frame_idx,
        frame_end=records[-1].frame_idx,
        n_total_frames=n_total,
        n_used_frames=len(valid_records),
        channels=ch_averages,
    )


def segment_united_file(
    records: List[FrameRecord],
    heights_m: List[float],
    min_wg_rpm: float = 45.0,
    skip_start_frames: int = 10,
) -> List[HeightSegment]:
    """
    Автоматическое разбиение единого склеенного файла на сегменты высот.
    Сегментация выполняется по сбросам номера кадра прибора и паузам вращения (Wg < 40).
    """
    n_heights = len(heights_m)
    if n_heights <= 1:
        return [process_frames_interval(records, heights_m[0], min_wg_rpm, skip_start_frames)]

    # Ищем границы перехода
    boundaries = [0]
    for i in range(1, len(records)):
        prev_f = records[i - 1].f400.frame_num
        curr_f = records[i].f400.frame_num
        prev_wg = min(records[i - 1].f400.wg_rpm, records[i - 1].f2000.wg_rpm)
        curr_wg = min(records[i].f400.wg_rpm, records[i].f2000.wg_rpm)

        # Сброс счетчика кадров в приборе или резкий скачок назад
        is_frame_reset = (curr_f < prev_f and (prev_f - curr_f) > 5)
        # Либо провал вращения (останов оператором для перестановки высоты)
        is_stop_and_start = (prev_wg < 20.0 and curr_wg >= min_wg_rpm and (i - boundaries[-1]) >= 40)

        if (is_frame_reset or is_stop_and_start) and (i - boundaries[-1]) >= 40:
            boundaries.append(i)

    # Если автоматические границы нашли ровно n_heights:
    if len(boundaries) == n_heights:
        boundaries.append(len(records))
    elif len(boundaries) != n_heights + 1:
        # Если автоматика не нашла точные границы, разбиваем равномерно
        # или ищем локальные минимумы Wg
        step = len(records) // n_heights
        boundaries = [i * step for i in range(n_heights)] + [len(records)]

    segments: List[HeightSegment] = []
    for h_idx in range(n_heights):
        h_val = heights_m[h_idx]
        b_start = boundaries[h_idx]
        b_end = boundaries[h_idx + 1]
        seg_records = records[b_start:b_end]
        seg = process_frames_interval(seg_records, h_val, min_wg_rpm, skip_start_frames)
        segments.append(seg)

    return segments


def segments_to_dataframe(segments: List[HeightSegment]) -> pd.DataFrame:
    """
    Преобразование сегментов в стандартный DataFrame калибровки (формат B_z.csv).
    Колонки: D_m, Rx, freq, Tx, L_m, amp_V_zz, amp_V_zx, amp_V_xx, d_ph_Vzx_Vzz
    """
    rows = []
    for seg in segments:
        for ch in seg.channels:
            rows.append({
                'D_m': seg.height_m,
                'Rx': ch.rx,
                'freq': ch.freq,
                'Tx': ch.tx,
                'L_m': ch.l_m,
                'amp_V_zz': ch.vzz_mean,
                'amp_V_zx': ch.vzx_mean,
                'amp_V_xx': ch.raw_vxx_h2,
                'd_ph_Vzx_Vzz': ch.dph_rad_mean,
                'vzz_std': ch.vzz_std,
                'vzx_std': ch.vzx_std,
                'dph_std_deg': math.degrees(ch.dph_rad_std),
                'n_frames': ch.n_frames,
                'raw_vzz': ch.raw_vzz,
                'raw_vzx': ch.raw_vzx,
                'h2_div_h1_pct': ch.h2_div_h1_pct,
                'residual_noise_dv': ch.residual_noise_dv,
                'thd_pct': ch.thd_pct,
                'snr_db': ch.snr_db,
                'proc_vzz_diff_pct': ch.processor_vzz_diff_pct,
                'proc_vzx_diff_pct': ch.processor_vzx_diff_pct,
                'proc_status': ch.processor_status,
            })
    return pd.DataFrame(rows)
