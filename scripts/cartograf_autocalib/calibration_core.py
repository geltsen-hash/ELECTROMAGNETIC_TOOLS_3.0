"""
Ядро двухшагового МНК-расчёта калибровки зонда «Картограф».
  - Шаг 1: Определение масштабного коэффициента A, корпусной добавки C и константы c = C/A
    по отклику отражённого поля air_h1(h, L).
  - Шаг 2: Высокоточный подбор эффективного угла наклона антенн beta_deg с вычитанием C
    по аналитической модели диполя над проводящим экраном.

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

import math
from dataclasses import dataclass
from typing import Dict, List, Tuple, Optional
import numpy as np
from scipy.optimize import minimize_scalar
import pandas as pd


# Определение 8 каналов калибровки по стандарту предприятия:
# Зонды 1..4 строго по возрастанию длины (0.96м, 1.68м, 2.28м, 3.00м)
# (имя, rx_name, L_m, rx_letter, tx_num)
CHANNEL_DEFINITIONS = [
    ('L_Зонд1', 'Rzx1', 0.96, 'L', 1),
    ('L_Зонд2', 'Rzx1', 1.68, 'L', 2),
    ('L_Зонд3', 'Rzx1', 2.28, 'L', 3),
    ('L_Зонд4', 'Rzx1', 3.00, 'L', 4),
    ('R_Зонд1', 'Rzx2', 0.96, 'R', 1),
    ('R_Зонд2', 'Rzx2', 1.68, 'R', 2),
    ('R_Зонд3', 'Rzx2', 2.28, 'R', 3),
    ('R_Зонд4', 'Rzx2', 3.00, 'R', 4),
]

# Зеркальные пары по индексам CHANNEL_DEFINITIONS:
# Зонд 1 (0.96 м): ch0 (L_Зонд1) - ch4 (R_Зонд1)
# Зонд 2 (1.68 м): ch1 (L_Зонд2) - ch5 (R_Зонд2)
# Зонд 3 (2.28 м): ch2 (L_Зонд3) - ch6 (R_Зонд3)
# Зонд 4 (3.00 м): ch3 (L_Зонд4) - ch7 (R_Зонд4)
MIRROR_PAIR_INDICES = [
    (0, 4, 0.96),
    (1, 5, 1.68),
    (2, 6, 2.28),
    (3, 7, 3.00),
]


@dataclass
class ChannelCalResult:
    ch_idx: int
    name: str
    rx_name: str
    rx_letter: str
    tx_num: int
    l_m: float
    A: float
    C: float
    c_ratio: float          # c = C / Vzz_air (нормировка на SSI прямого сигнала)
    beta_deg: float
    mean_dph_rad: float
    mean_dph_deg: float
    mean_dph_mg: float      # миллиградусы (deg * 1000)
    h1_mean_abs_err_pct: float
    h1_max_abs_err_pct: float
    h1_mean_rel_err_pct: float
    h1_max_rel_err_pct: float


@dataclass
class FreqCalResult:
    freq_khz: int
    heights_m: np.ndarray
    channels: List[ChannelCalResult]  # 8 каналов
    # Зеркальные разности углов (beta_L - beta_R) для баз 0.96, 1.68, 2.28, 3.00
    mirror_diff_deg: Dict[float, float]
    total_mean_abs_err_pct: float
    total_max_abs_err_pct: float


@dataclass
class FullCalResult:
    f400: FreqCalResult
    f2000: FreqCalResult
    # 3D массивы коэффициентов [lr 0..1][freq 0..1][tx 0..3]
    # lr: 0=Left, 1=Right
    # freq: 0=400, 1=2000
    # tx: 0=Tx1, 1=Tx2, 2=Tx3, 3=Tx4
    betta_zx_deg: np.ndarray    # float32 shape (2, 2, 4)
    v_zx_colar_add: np.ndarray  # float32 shape (2, 2, 4)
    d_ph_mg: np.ndarray         # float32 shape (2, 2, 4)


def air_h1(h: float, L: float) -> float:
    """Геометрическая функция отклика гармоники H1 от бесконечного экрана."""
    denom = L * L + 4.0 * h * h
    return (h * L) / (denom ** 2.5)


def theoretical_harmonics(h: float, L_eff: float, beta_deg: float) -> Tuple[float, float, float]:
    """
    Аналитический расчёт амплитуд H0, H1, H2 диполя над проводящим листом.
    Векторная модель зеркального отражения диполя.
    """
    beta_rad = math.radians(beta_deg)
    sin_beta = math.sin(beta_rad)
    cos_beta = math.cos(beta_rad)

    r_img = math.sqrt(4.0 * h * h + L_eff * L_eff)
    r3 = r_img ** 3.0
    r5 = r_img ** 5.0

    term_dir = 2.0 / (L_eff ** 3.0)
    term_img = 3.0 * (L_eff * L_eff) / r5 - 1.0 / r3
    C0 = cos_beta * (term_dir + term_img)
    C1 = sin_beta * (6.0 * h * L_eff / r5)

    H0 = abs(C0)
    H1 = abs(C1)
    H2 = 0.0
    return H0, H1, H2


def compute_direct_vzz(heights: np.ndarray, H0_meas: np.ndarray, L: float) -> float:
    """
    Вычисление прямого сигнала Vzz в воздухе (невозмущенное поле диполя без экрана).
    H0(h) = H0_dir * (1 + term_img(h, L) / term_dir(L)).
    Прямой сигнал H0_dir пропорционален интенсивности излучения SSI.
    """
    factors = []
    term_dir = 2.0 / (L ** 3.0)
    for h in heights:
        r_img = math.sqrt(4.0 * h * h + L * L)
        term_img = 3.0 * (L * L) / (r_img ** 5.0) - 1.0 / (r_img ** 3.0)
        factors.append(1.0 + term_img / term_dir)
    geom_factors = np.array(factors, dtype=np.float64)
    denom = np.sum(geom_factors ** 2.0)
    if denom > 1e-15:
        return float(np.sum(H0_meas * geom_factors) / denom)
    return float(H0_meas[-1])


def fit_casing_offset_channel(heights: np.ndarray, H1_meas: np.ndarray, L: float) -> Tuple[float, float]:
    """
    Шаг 1: Определение A и корпусной добавки C (в мВ) через взвешенный МНК:
    Минимизация sum w_i * (H1_meas - A * X - C)^2, где w_i = 1 / H1_meas^2, X = air_h1(h, L).
    """
    X = np.array([air_h1(h, L) for h in heights], dtype=np.float64)
    w = 1.0 / (H1_meas ** 2.0)

    Sxx = np.sum(w * X * X)
    Sx  = np.sum(w * X)
    Sxy = np.sum(w * X * H1_meas)
    Sy  = np.sum(w * H1_meas)
    Sw  = np.sum(w)

    det = Sxx * Sw - Sx * Sx
    if abs(det) > 1e-15:
        A = (Sxy * Sw - Sx * Sy) / det
        C = (Sxx * Sy - Sx * Sxy) / det
    else:
        A = (Sxy / Sxx) if Sxx > 1e-15 else 0.0
        C = 0.0

    return float(A), float(C)


def fit_single_angle(heights: np.ndarray, H0_meas: np.ndarray, H1_meas: np.ndarray, L_eff: float, C_offset: float) -> float:
    """
    Шаг 2: Одномерная оптимизация эффективного угла beta_deg.
    Минимизация квадратичной невязки отношения H1/H0 со скорректированным H1.
    """
    def objective(angle_deg: float) -> float:
        sum_sq = 0.0
        for i in range(len(heights)):
            H0_t, H1_t, _ = theoretical_harmonics(heights[i], L_eff, angle_deg)
            ratio_t = (H1_t / H0_t) if H0_t > 1e-30 else 0.0
            ratio_m = ((H1_meas[i] - C_offset) / H0_meas[i]) if H0_meas[i] > 1e-30 else 0.0
            diff = ratio_t - ratio_m
            sum_sq += diff * diff
        return sum_sq

    res = minimize_scalar(objective, bounds=(10.0, 89.0), method='bounded')
    return float(res.x)


def calibrate_frequency(df_freq: pd.DataFrame, freq_khz: int) -> FreqCalResult:
    """
    Полный двухшаговый расчёт для одной частоты (400 или 2000 кГц).
    """
    heights = np.array(sorted(df_freq['D_m'].unique()), dtype=np.float64)
    n_heights = len(heights)

    # Заполняем матрицы гармоник: H0 (Vzz) и H1 (Vzx), а также dph
    arr_h0 = np.zeros((n_heights, 8), dtype=np.float64)
    arr_h1 = np.zeros((n_heights, 8), dtype=np.float64)
    arr_dph = np.zeros((n_heights, 8), dtype=np.float64)

    for i, h in enumerate(heights):
        for j, (name, rx_name, L_val, rx_letter, tx_num) in enumerate(CHANNEL_DEFINITIONS):
            row = df_freq[(df_freq['D_m'] == h) & (df_freq['Rx'] == rx_letter) & (df_freq['Tx'] == tx_num)]
            if len(row) == 0:
                raise ValueError(f"Нет данных для D_m={h}, Rx={rx_letter}, Tx={tx_num}, freq={freq_khz}")
            arr_h0[i, j] = row['amp_V_zz'].values[0]
            arr_h1[i, j] = row['amp_V_zx'].values[0]
            arr_dph[i, j] = row['d_ph_Vzx_Vzz'].values[0]

    channels_res: List[ChannelCalResult] = []
    all_abs_errs: List[float] = []

    for j, (name, rx_name, L_val, rx_letter, tx_num) in enumerate(CHANNEL_DEFINITIONS):
        # Шаг 1: корпусная добавка C (мВ) и масштабный множитель A
        A, C = fit_casing_offset_channel(heights, arr_h1[:, j], L_val)
        # Нормировка корпусной добавки C на прямой сигнал в воздухе Vzz_dir (SSI)
        Vzz_dir = compute_direct_vzz(heights, arr_h0[:, j], L_val)
        c_ratio = (C / Vzz_dir) if abs(Vzz_dir) > 1e-15 else 0.0
        # Шаг 2: подбор угла
        beta = fit_single_angle(heights, arr_h0[:, j], arr_h1[:, j], L_val, C)

        # Фаза: средняя круговая фаза
        sin_sum = np.sum(np.sin(arr_dph[:, j]))
        cos_sum = np.sum(np.cos(arr_dph[:, j]))
        mean_dph_rad = math.atan2(sin_sum, cos_sum)
        mean_dph_deg = math.degrees(mean_dph_rad)
        mean_dph_mg = mean_dph_deg * 1000.0

        # Статистика невязок H1/H0
        abs_errs = []
        rel_errs = []
        for i, h in enumerate(heights):
            H0_t, H1_t, _ = theoretical_harmonics(h, L_val, beta)
            ratio_t = (H1_t / H0_t) if H0_t > 1e-30 else 0.0
            ratio_m = ((arr_h1[i, j] - C) / arr_h0[i, j]) if arr_h0[i, j] > 1e-30 else 0.0
            ae = ratio_t - ratio_m
            re = (ae / ratio_m * 100.0) if abs(ratio_m) > 1e-30 else 0.0
            abs_errs.append(abs(ae))
            rel_errs.append(abs(re))

        all_abs_errs.extend(abs_errs)

        channels_res.append(ChannelCalResult(
            ch_idx=j,
            name=name,
            rx_name=rx_name,
            rx_letter=rx_letter,
            tx_num=tx_num,
            l_m=L_val,
            A=A,
            C=C,
            c_ratio=c_ratio,
            beta_deg=beta,
            mean_dph_rad=mean_dph_rad,
            mean_dph_deg=mean_dph_deg,
            mean_dph_mg=mean_dph_mg,
            h1_mean_abs_err_pct=float(np.mean(abs_errs) * 100.0),
            h1_max_abs_err_pct=float(np.max(abs_errs) * 100.0),
            h1_mean_rel_err_pct=float(np.mean(rel_errs)),
            h1_max_rel_err_pct=float(np.max(rel_errs)),
        ))

    # Зеркальные разности углов
    mirror_diff = {}
    for j1, j2, L_pair in MIRROR_PAIR_INDICES:
        diff = abs(channels_res[j1].beta_deg - channels_res[j2].beta_deg)
        mirror_diff[L_pair] = diff

    return FreqCalResult(
        freq_khz=freq_khz,
        heights_m=heights,
        channels=channels_res,
        mirror_diff_deg=mirror_diff,
        total_mean_abs_err_pct=float(np.mean(all_abs_errs) * 100.0),
        total_max_abs_err_pct=float(np.max(all_abs_errs) * 100.0),
    )


def run_full_calibration(df: pd.DataFrame) -> FullCalResult:
    """
    Полный сквозной расчёт калибровки для обеих частот (400 и 2000 кГц).
    Формирует результирующие 3D массивы с точным аппаратным маппингом:
    [lr: 0=Left, 1=Right][freq: 0=400, 1=2000][tx: 0=Tx1, 1=Tx2, 2=Tx3, 3=Tx4]
    """
    df_400 = df[df['freq'] == 400]
    df_2000 = df[df['freq'] == 2000]

    res_400 = calibrate_frequency(df_400, 400)
    res_2000 = calibrate_frequency(df_2000, 2000)

    betta_arr = np.zeros((2, 2, 4), dtype=np.float32)
    colar_arr = np.zeros((2, 2, 4), dtype=np.float32)
    dph_arr = np.zeros((2, 2, 4), dtype=np.float32)

    # Маппинг каналов:
    # Индексы CHANNEL_DEFINITIONS:
    # 0..3: Left  (Tx1, Tx2, Tx3, Tx4) -> lr=0, tx=0..3
    # 4..7: Right (Tx1, Tx2, Tx3, Tx4) -> lr=1, tx=0..3
    for f_idx, f_res in enumerate([res_400, res_2000]):
        for ch in f_res.channels:
            lr_idx = 0 if ch.rx_letter == 'L' else 1
            tx_idx = ch.tx_num - 1
            betta_arr[lr_idx, f_idx, tx_idx] = np.float32(ch.beta_deg)
            colar_arr[lr_idx, f_idx, tx_idx] = np.float32(ch.c_ratio)
            dph_arr[lr_idx, f_idx, tx_idx] = np.float32(ch.mean_dph_mg)

    return FullCalResult(
        f400=res_400,
        f2000=res_2000,
        betta_zx_deg=betta_arr,
        v_zx_colar_add=colar_arr,
        d_ph_mg=dph_arr,
    )
