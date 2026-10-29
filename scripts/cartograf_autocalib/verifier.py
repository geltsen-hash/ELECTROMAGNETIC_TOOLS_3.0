"""
Модуль верификации качества калибровки зонда «Картограф» (Auto-QA).
Сравнивает экспериментальный геосигнал G_meas с аналитической моделью
диполя над проводящим листом G_theor(h, L) и проверяет допуски метрологии.

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

import math
from dataclasses import dataclass
from typing import Dict, List, Tuple
import numpy as np
import pandas as pd

from .calibration_core import (
    CHANNEL_DEFINITIONS,
    MIRROR_PAIR_INDICES,
    FreqCalResult,
    FullCalResult,
    theoretical_harmonics,
)


NOMINAL_BETA_DEG = 45.0
TAN_NOMINAL_BETA = math.tan(math.radians(NOMINAL_BETA_DEG))  # 1.0


@dataclass
class GeoHeightPoint:
    h_m: float
    h_div_l: float
    g_theor: float
    g_meas: float
    abs_err: float
    rel_err_pct: float


@dataclass
class ChannelGeoVerification:
    ch_idx: int
    name: str
    rx_letter: str
    tx_num: int
    l_m: float
    beta_deg: float
    c_ratio: float
    mean_dph_deg: float
    points: List[GeoHeightPoint]
    mean_abs_err: float
    max_abs_err: float
    mean_rel_err_pct: float
    max_rel_err_pct: float
    status: str  # 'PASS', 'WARN', 'FAIL'


@dataclass
class FreqVerificationReport:
    freq_khz: int
    channels: List[ChannelGeoVerification]
    mirror_pairs_diff: Dict[float, float]
    total_mean_abs_err: float
    total_max_abs_err: float
    total_mean_rel_err_pct: float
    total_max_rel_err_pct: float
    overall_status: str  # 'PASS', 'WARN', 'FAIL'


def compute_theoretical_geo(h_m: float, l_m: float) -> float:
    """
    Расчёт теоретического геосигнала диполя над проводником:
    G_theor = (1 - R_theor) / (1 + R_theor), где R_theor = H1(45°) / H0(45°).
    """
    if h_m <= 1e-6 or l_m <= 1e-6:
        return 1.0
    h0, h1, _ = theoretical_harmonics(h_m, l_m, NOMINAL_BETA_DEG)
    if h0 < 1e-30:
        return 1.0
    r_theor = h1 / h0
    return (1.0 - r_theor) / (1.0 + r_theor)


def verify_frequency_geosignal(df_freq: pd.DataFrame, f_res: FreqCalResult) -> FreqVerificationReport:
    """
    Сквозная верификация геосигнала для одной частоты.
    """
    heights = np.array(sorted(df_freq['D_m'].unique()), dtype=np.float64)
    chan_verifications: List[ChannelGeoVerification] = []
    all_abs_errs: List[float] = []
    all_rel_errs: List[float] = []

    has_fail = False
    has_warn = False

    for ch in f_res.channels:
        l_m = ch.l_m
        beta_rad = math.radians(ch.beta_deg)
        rx_tilt = math.tan(beta_rad)
        mean_dph = ch.mean_dph_rad
        c_val = ch.c_ratio

        points: List[GeoHeightPoint] = []
        ch_abs_errs = []
        ch_rel_errs = []

        for h in heights:
            row = df_freq[(df_freq['D_m'] == h) & (df_freq['Rx'] == ch.rx_letter) & (df_freq['Tx'] == ch.tx_num)]
            vzz = row['amp_V_zz'].values[0]
            vzx = row['amp_V_zx'].values[0]
            dph = row['d_ph_Vzx_Vzz'].values[0]

            # Теория
            g_theor = compute_theoretical_geo(h, l_m)

            # Эксперимент с компенсацией фазы, корпуса и наклона
            r_raw = (vzx / vzz) if vzz > 1e-30 else 0.0
            r_complex = r_raw * np.exp(-1j * (dph - mean_dph))
            r_corr = float(np.real(r_complex)) - c_val
            r_pure = r_corr * (TAN_NOMINAL_BETA / rx_tilt) if abs(rx_tilt) > 1e-30 else r_corr
            g_meas = (1.0 - r_pure) / (1.0 + r_pure)

            ae = g_theor - g_meas
            re = (ae / g_meas * 100.0) if abs(g_meas) > 1e-30 else 0.0

            points.append(GeoHeightPoint(
                h_m=h,
                h_div_l=h / l_m,
                g_theor=g_theor,
                g_meas=g_meas,
                abs_err=ae,
                rel_err_pct=re,
            ))
            ch_abs_errs.append(abs(ae))
            ch_rel_errs.append(abs(re))

        all_abs_errs.extend(ch_abs_errs)
        all_rel_errs.extend(ch_rel_errs)

        max_re = float(np.max(ch_rel_errs))
        mean_re = float(np.mean(ch_rel_errs))

        # Критерии допуска по длине базы L_m
        if l_m <= 1.0:
            status = 'PASS' if max_re < 1.5 else ('WARN' if max_re < 3.0 else 'FAIL')
        elif l_m <= 1.8:
            status = 'PASS' if max_re < 4.0 else ('WARN' if max_re < 6.0 else 'FAIL')
        elif l_m <= 2.5:
            status = 'PASS' if max_re < 6.0 else ('WARN' if max_re < 8.0 else 'FAIL')
        else:
            status = 'PASS' if max_re < 7.0 else ('WARN' if max_re < 10.0 else 'FAIL')

        if status == 'FAIL':
            has_fail = True
        elif status == 'WARN':
            has_warn = True

        chan_verifications.append(ChannelGeoVerification(
            ch_idx=ch.ch_idx,
            name=ch.name,
            rx_letter=ch.rx_letter,
            tx_num=ch.tx_num,
            l_m=l_m,
            beta_deg=ch.beta_deg,
            c_ratio=ch.c_ratio,
            mean_dph_deg=ch.mean_dph_deg,
            points=points,
            mean_abs_err=float(np.mean(ch_abs_errs)),
            max_abs_err=float(np.max(ch_abs_errs)),
            mean_rel_err_pct=mean_re,
            max_rel_err_pct=max_re,
            status=status,
        ))

    overall = 'FAIL' if has_fail else ('WARN' if has_warn else 'PASS')

    return FreqVerificationReport(
        freq_khz=f_res.freq_khz,
        channels=chan_verifications,
        mirror_pairs_diff=f_res.mirror_diff_deg,
        total_mean_abs_err=float(np.mean(all_abs_errs)),
        total_max_abs_err=float(np.max(all_abs_errs)),
        total_mean_rel_err_pct=float(np.mean(all_rel_errs)),
        total_max_rel_err_pct=float(np.max(all_rel_errs)),
        overall_status=overall,
    )


def verify_full_calibration(df: pd.DataFrame, full_cal: FullCalResult) -> Tuple[FreqVerificationReport, FreqVerificationReport]:
    """Сквозная верификация для обеих частот 400 и 2000 кГц."""
    rep400 = verify_frequency_geosignal(df[df['freq'] == 400], full_cal.f400)
    rep2000 = verify_frequency_geosignal(df[df['freq'] == 2000], full_cal.f2000)
    return rep400, rep2000
