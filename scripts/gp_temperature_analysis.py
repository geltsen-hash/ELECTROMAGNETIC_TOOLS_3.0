# -*- coding: utf-8 -*-
"""
Анализ температурного дрейфа и аппаратурных паразитных наводок индукционного зонда (GP_DATA).
Диагностический комплекс для схемотехников и разработчиков аппаратуры.
Поддерживает универсальную работу с приборами на 3, 4 и 5 передатчиков (AUTONOM_5Tx, CARTOGRAPH_LWD_4Tx, LWD_4Tx_NEW, LWD_3Tx).

Организация: НПФ «АМК Горизонт», разработал зам начальника ОП Смирнов С. Г.
"""

import sys
import os
import argparse
import subprocess
import threading
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


def parse_csv_metadata(csv_file):
    """Считывает закомментированные строки паспорта прибора (# Key: Value) из заголовка CSV."""
    metadata = {}
    try:
        with open(csv_file, 'r', encoding='utf-8', errors='replace') as f:
            for line in f:
                line = line.strip()
                if line.startswith('#'):
                    parts = line[1:].strip().split(':', 1)
                    if len(parts) == 2:
                        k = parts[0].strip()
                        v = parts[1].strip()
                        metadata[k] = v
                elif line:
                    break
    except Exception:
        pass
    return metadata


def fit_linear_trend(x, y):
    """Возвращает наклон (slope), смещение (intercept) и коэффициент детерминации R^2."""
    valid = ~(np.isnan(x) | np.isnan(y) | np.isinf(x) | np.isinf(y))
    if np.sum(valid) < 10:
        return 0.0, 0.0, 0.0
    xv = x[valid]
    yv = y[valid]
    p = np.polyfit(xv, yv, 1)
    y_pred = np.polyval(p, xv)
    ss_tot = np.sum((yv - np.mean(yv)) ** 2)
    ss_res = np.sum((yv - y_pred) ** 2)
    r2 = 1.0 - (ss_res / ss_tot) if ss_tot > 0 else 0.0
    return p[0], p[1], r2


def open_folder_in_explorer(folder_path):
    """Открывает папку в Проводнике Windows."""
    try:
        if os.name == 'nt' and os.path.isdir(folder_path):
            os.startfile(folder_path)
    except Exception as e:
        print(f"Не удалось открыть папку: {e}")


def adapt_em_gp_csv(df):
    """Приводит контракт em_gp_v1 (ELECTROMAGNETIC_TOOLS_3.0) к именам колонок старого экспорта AIR_TEMP_1.0."""
    if "schema" not in df.columns or not str(df["schema"].iloc[0]).startswith("em_gp"):
        return df, {}
    out = pd.DataFrame(index=df.index)
    out["frame"] = df["frame"]
    out["temperature"] = df["temperature_c"]

    def hex_mask(v):
        try:
            return format(int(v), "X")
        except (TypeError, ValueError):
            return ""
    out["condition"] = df["condition"].apply(hex_mask)
    for freq, prefix in ((400, "f400"), (2000, "f2000")):
        out[f"ZERO_AM1_{freq}"] = df[f"{prefix}_ZERO_AM_RX_1"]
        out[f"ZERO_AM2_{freq}"] = df[f"{prefix}_ZERO_AM_RX_2"]
        for tx in range(1, 6):
            src = f"{prefix}_tx{tx}_"
            if src + "AM_RX_1" not in df.columns:
                continue
            out[f"DPH_{freq}_T{tx}"] = df[src + "DELTA_PH"]
            out[f"PH_smt_{freq}_T{tx}"] = df[src + "phase_smt"]
            out[f"AM1_{freq}_T{tx}"] = df[src + "AM_RX_1"]
            out[f"AM2_{freq}_T{tx}"] = df[src + "AM_RX_2"]
            out[f"ATT_smt_{freq}_T{tx}"] = df[src + "att_dB_smt"]
    first = df.iloc[0]
    serial = first.get("sonde_number")
    metadata = {
        "N_Tx": str(int(first.get("tx_count", 5))),
        "SondeType": str(first.get("sonde_type", "GP_DATA")),
        "Serial": str(int(serial)) if pd.notna(serial) else "не указан",
        "Signature": hex_mask(first.get("signature")),
    }
    return out, metadata


def analyze_gp_data(csv_file, out_dir=None, log_cb=None):
    """
    Основная функция диагностического анализа данных GP_DATA.
    log_cb(str): опциональный коллбэк для вывода сообщений в консоль или GUI.
    """
    def log(msg):
        if log_cb:
            log_cb(msg)
        else:
            print(msg)

    csv_file = os.path.abspath(csv_file)
    if not os.path.exists(csv_file):
        raise FileNotFoundError(f"Файл {csv_file} не найден!")

    # Если выходная папка не задана, формируем подпапку рядом с CSV-файлом
    if out_dir is None or str(out_dir).strip() == "":
        csv_dir = os.path.dirname(csv_file)
        base_name = os.path.splitext(os.path.basename(csv_file))[0]
        out_dir = os.path.join(csv_dir, f"gp_analysis_{base_name}")

    out_dir = os.path.abspath(out_dir)
    os.makedirs(out_dir, exist_ok=True)

    # 1. Чтение паспорта прибора из заголовка CSV (если есть)
    metadata = parse_csv_metadata(csv_file)

    log(f"Загрузка данных из: {csv_file}...")
    df = pd.read_csv(csv_file, comment='#')
    df.columns = [c.strip().lstrip('\ufeff') for c in df.columns]
    if "schema" in df.columns:
        df, em_meta = adapt_em_gp_csv(df)
        metadata.update(em_meta)
    total_rows = len(df)
    log(f"Загружено строк: {total_rows}")

    # 2. Определение числа передатчиков N_Tx
    if "N_Tx" in metadata and metadata["N_Tx"].isdigit():
        n_tx = int(metadata["N_Tx"])
    else:
        # Автоопределение по наличию колонок DPH_400_Tx
        detected = 0
        for i in range(1, 6):
            if any(col.startswith(f"DPH_400_T{i}") or col.startswith(f"AM1_400_T{i}") for col in df.columns):
                detected = i
        n_tx = detected if detected >= 3 else 5

    tx_list = list(range(1, n_tx + 1))
    sonde_name = metadata.get("SondeType", f"Индукционный зонд ({n_tx} передатчиков)")
    serial_num = metadata.get("Serial", "не указан")
    signature_str = metadata.get("Signature", "—")

    log(f"Паспорт прибора: {sonde_name}, заводской №: {serial_num}, число передатчиков N_Tx = {n_tx}")

    # 3. Базы катушек L1 и L2
    L1 = None
    L2 = None
    if "L1_mm" in metadata and "L2_mm" in metadata:
        try:
            L1 = [float(x) / 1000.0 for x in metadata["L1_mm"].replace(',', ' ').split()][:n_tx]
            L2 = [float(x) / 1000.0 for x in metadata["L2_mm"].replace(',', ' ').split()][:n_tx]
            if len(L1) == n_tx and len(L2) == n_tx and all(x > 0 for x in L1) and all(x > 0 for x in L2):
                log(f"Использованы паспортные расстояния катушек из метрологии: L1={L1}, L2={L2}")
            else:
                L1 = None
                L2 = None
        except Exception:
            L1 = None
            L2 = None

    if L1 is None or L2 is None:
        if n_tx == 4:
            # Стандартная геометрия Картограф LWD / LWD-4Tx
            L1 = [0.358, 0.720, 1.078, 1.437]
            L2 = [0.598, 0.960, 1.318, 1.677]
            log(f"Установлены эталонные базы катушек 4-зондового прибора: L1={L1}, L2={L2}")
        elif n_tx == 3:
            # LWD-3Tx
            L1 = [0.358, 0.720, 1.078]
            L2 = [0.598, 0.960, 1.318]
            log(f"Установлены эталонные базы катушек 3-зондового прибора: L1={L1}, L2={L2}")
        else:
            # АВТОНОМ-5Tx
            L1 = [0.365, 0.710, 1.070, 1.415, 1.768]
            L2 = [0.635, 0.980, 1.340, 1.685, 2.038]
            log(f"Установлены эталонные базы катушек 5-зондового прибора: L1={L1}, L2={L2}")

    # Физическое определение ближнего и дальнего приёмника по реальным расстояниям
    L_near = [min(L1[i], L2[i]) for i in range(n_tx)]
    L_far  = [max(L1[i], L2[i]) for i in range(n_tx)]

    # Фильтрация по условию condition (все передатчики работают)
    expected_mask_str = {5: "1F1F", 4: "0F0F", 3: "0707"}.get(n_tx, "1F1F")
    valid_mask = df["condition"].astype(str).str.upper().str.contains(expected_mask_str)
    if not valid_mask.any():
        valid_mask = pd.Series(True, index=df.index)
    df_valid = df[valid_mask].copy()

    temp = df_valid["temperature"].values
    t_min = np.min(temp)
    t_max = np.max(temp)
    delta_t = t_max - t_min
    frames = df_valid["frame"].values

    log(f"Диапазон температур: {t_max:.2f} °C -> {t_min:.2f} °C (ΔT = {delta_t:.2f} °C), стабильных кадров: {len(df_valid)}")

    # 1. График кривой остывания
    log("Построение графика 1/7: кривая остывания...")
    fig, ax1 = plt.subplots(figsize=(10, 5))
    ax1.plot(frames, temp, color='tab:red', lw=1.5, label='Температура корпуса T (°C)')
    ax1.set_xlabel('Номер кадра (frame)', fontsize=11)
    ax1.set_ylabel('Температура, °C', color='tab:red', fontsize=11)
    ax1.tick_params(axis='y', labelcolor='tab:red')
    ax1.grid(True, linestyle='--', alpha=0.5)

    window = 100
    temp_smooth = pd.Series(temp).rolling(window, center=True).mean().bfill().ffill().values
    dt_df = np.gradient(temp_smooth) * 1000.0  # м°C на кадр

    ax2 = ax1.twinx()
    ax2.plot(frames, dt_df, color='tab:blue', lw=1.0, alpha=0.7, label='Скорость dT/d(frame), м°C/кадр')
    ax2.set_ylabel('Скорость охлаждения, м°C/кадр', color='tab:blue', fontsize=11)
    ax2.tick_params(axis='y', labelcolor='tab:blue')

    plt.title(f'Кривая остывания: {sonde_name} (Т: {t_max:.1f} °C → {t_min:.1f} °C, {len(df_valid)} кадров)', fontsize=12)
    fig.tight_layout()
    plot_cooling_path = os.path.join(out_dir, "gp_temp_cooling_curve.png")
    fig.savefig(plot_cooling_path, dpi=200)
    plt.close(fig)

    freqs = [400, 2000]
    palette = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b']
    colors = palette[:n_tx]

    results = []

    # 2. Графики фаз от температуры: Raw DPH и Phase SMT (2x2)
    log("Построение графика 2/7: разность фаз несимм. vs симм....")
    fig, axes = plt.subplots(2, 2, figsize=(14, 10), sharex=True)

    for f_idx, freq in enumerate(freqs):
        v_zero1 = df_valid[f"ZERO_AM1_{freq}"].mean() if f"ZERO_AM1_{freq}" in df_valid.columns else 0.05
        v_zero2 = df_valid[f"ZERO_AM2_{freq}"].mean() if f"ZERO_AM2_{freq}" in df_valid.columns else 0.05
        v_zero = 0.5 * (v_zero1 + v_zero2)

        for tx_idx, tx in enumerate(tx_list):
            raw_dph_col = f"DPH_{freq}_T{tx}"
            smt_ph_col = f"PH_smt_{freq}_T{tx}"
            am1_col = f"AM1_{freq}_T{tx}"
            am2_col = f"AM2_{freq}_T{tx}"
            att_smt_col = f"ATT_smt_{freq}_T{tx}"

            raw_dph = df_valid[raw_dph_col].values
            smt_ph = df_valid[smt_ph_col].values
            am1 = df_valid[am1_col].values
            am2 = df_valid[am2_col].values
            att_smt = df_valid[att_smt_col].values

            ln = L_near[tx - 1]
            lf = L_far[tx - 1]

            # Выбор ближнего/дальнего приемника:
            # Нечётные передатчики (T1, T3, T5) - сторона А: R1 ближний (AM1), R2 дальний (AM2)
            # Чётные передатчики (T2, T4) - сторона Б: R2 ближний (AM2), R1 дальний (AM1)
            if tx % 2 == 1:
                am_near_arr = am1
                am_far_arr = am2
                dph_fn = raw_dph
            else:
                am_near_arr = am2
                am_far_arr = am1
                dph_fn = -raw_dph

            mean_am1 = np.nanmean(am1)
            mean_am2 = np.nanmean(am2)
            am_near = np.nanmean(am_near_arr)
            am_far = np.nanmean(am_far_arr)

            # Несимметризованное затухание
            att_raw_norm = 20.0 * np.log10((np.maximum(am_near_arr, 1e-6) / np.maximum(am_far_arr, 1e-6)) * ((ln / lf) ** 3))

            rad_to_mgrad = (180.0 / np.pi) * 1000.0
            raw_dph_mdeg = raw_dph * rad_to_mgrad
            smt_ph_mdeg = smt_ph * rad_to_mgrad

            slope_raw, _, r2_raw = fit_linear_trend(temp, raw_dph_mdeg)
            slope_smt, _, r2_smt = fit_linear_trend(temp, smt_ph_mdeg)
            slope_raw_att, _, _ = fit_linear_trend(temp, att_raw_norm * 1000.0)  # мдБ/°C
            slope_att_smt, _, r2_att = fit_linear_trend(temp, att_smt * 1000.0)  # мдБ/°C

            # Магнитный момент передатчика: M_Tx = AM * L^3
            m_tx_near = am_near * (ln ** 3)
            m_tx_far = am_far * (lf ** 3)
            m_tx = 0.5 * (m_tx_near + m_tx_far)

            # Векторное разделение полезного поля и наводки (1/L^3)
            k_geom = (ln / lf) ** 3

            m_tx_series = am_near_arr * (ln ** 3)
            m_tx_hot = m_tx_series[0]      # ~120 °C
            m_tx_cold = m_tx_series[-1]    # ~25 °C
            m_tx_drop_pct = (m_tx_cold - m_tx_hot) / (m_tx_cold + 1e-9) * 100.0

            # Комплексный дифференциальный вектор паразитной наводки D_leak(T)
            re_leak = am_far_arr * np.cos(dph_fn) - k_geom * am_near_arr
            im_leak = am_far_arr * np.sin(dph_fn)
            mod_leak = np.sqrt(re_leak ** 2 + im_leak ** 2)

            mod_leak_hot = mod_leak[0]
            mod_leak_cold = mod_leak[-1]
            slope_mod_leak, _, _ = fit_linear_trend(temp, mod_leak * 1000.0)  # мкВ/°C
            slope_re_leak, _, _ = fit_linear_trend(temp, re_leak * 1000.0)
            slope_im_leak, _, _ = fit_linear_trend(temp, im_leak * 1000.0)

            # Размах фазового дрейфа на симметризованном канале (peak-to-peak)
            ph_span_rad = np.ptp(smt_ph)
            ph_span_deg = ph_span_rad * (180.0 / np.pi)

            # Абсолютная амплитуда наводки на ближнем приёмнике (мВ)
            v_leak = am_near * ph_span_rad

            # Превышение над шумом молчащих каналов
            k_excess = v_leak / (v_zero + 1e-9)
            k_excess_db = 20.0 * np.log10(np.maximum(k_excess, 1e-6))

            # Паразитная связь относительно мощности
            k_leak_abs = v_leak / (m_tx + 1e-9)
            leak_pct_near = (v_leak / (am_near + 1e-9)) * 100.0

            # Оценка полезного сигнала на дальнем приёмнике и доли наводки в нём
            v_useful_far = am_near * k_geom
            leak_pct_far = (mod_leak_cold / (v_useful_far + 1e-9)) * 100.0

            # Анализ фазового угла наводки и преобладающего характера
            delta_re = re_leak[0] - re_leak[-1]  # hot -> cold
            delta_im = im_leak[0] - im_leak[-1]
            theta_deg = np.degrees(np.arctan2(delta_im, delta_re))

            ratio_re_im = abs(delta_re) / (abs(delta_im) + 1e-9)
            if ratio_re_im > 2.0:
                nature = "Синфазная (активная, индуктивно-гальваническая связь, токи в корпусе/землях)"
                nature_short = "Синфазная (Re)"
            elif ratio_re_im < 0.5:
                nature = "Квадратурная (реактивная, паразитная ёмкостная связь, сдвиг фазы)"
                nature_short = "Квадратурная (Im)"
            else:
                nature = "Смешанная (индуктивно-ёмкостная связь, Re ~ Im)"
                nature_short = "Смешанная (Re+Im)"

            results.append({
                "freq": freq,
                "tx": f"T{tx}",
                "AM1_mV": mean_am1,
                "AM2_mV": mean_am2,
                "AM_near_mV": am_near,
                "AM_far_mV": am_far,
                "V_useful_far_mV": v_useful_far,
                "L1_m": L1[tx - 1],
                "L2_m": L2[tx - 1],
                "L_near_m": ln,
                "L_far_m": lf,
                "M_Tx": m_tx,
                "M_Tx_hot": m_tx_hot,
                "M_Tx_cold": m_tx_cold,
                "M_Tx_drop_pct": m_tx_drop_pct,
                "V_zero_mV": v_zero,
                "V_leak_mV": v_leak,
                "D_leak_hot": mod_leak_hot,
                "D_leak_cold": mod_leak_cold,
                "slope_mod_leak_uV_C": slope_mod_leak,
                "slope_re_leak_uV_C": slope_re_leak,
                "slope_im_leak_uV_C": slope_im_leak,
                "re_leak_hot": re_leak[0],
                "re_leak_cold": re_leak[-1],
                "im_leak_hot": im_leak[0],
                "im_leak_cold": im_leak[-1],
                "delta_re_mV": delta_re,
                "delta_im_mV": delta_im,
                "theta_deg": theta_deg,
                "nature": nature,
                "nature_short": nature_short,
                "mod_leak_series": mod_leak,
                "re_leak_series": re_leak,
                "im_leak_series": im_leak,
                "m_tx_series": m_tx_series,
                "K_excess": k_excess,
                "K_excess_dB": k_excess_db,
                "K_leak_abs": k_leak_abs,
                "leak_pct_near": leak_pct_near,
                "leak_pct_far": leak_pct_far,
                "ph_span_deg": ph_span_deg,
                "slope_raw_mdeg_C": slope_raw,
                "slope_smt_mdeg_C": slope_smt,
                "drift_reduction_ph": abs(slope_raw) / (abs(slope_smt) + 1e-6),
                "slope_raw_att_mdB_C": slope_raw_att,
                "slope_att_smt_mdB_C": slope_att_smt,
                "drift_reduction_att": abs(slope_raw_att) / (abs(slope_att_smt) + 1e-6),
                "r2_smt": r2_smt
            })

            ax_raw = axes[f_idx, 0]
            ax_smt = axes[f_idx, 1]

            step = 5
            ax_raw.plot(temp[::step], raw_dph_mdeg[::step], color=colors[tx_idx],
                        label=f'T{tx} ({slope_raw:+.1f} мград/°C)', alpha=0.8, lw=1.2)
            ax_smt.plot(temp[::step], smt_ph_mdeg[::step], color=colors[tx_idx],
                        label=f'T{tx} ({slope_smt:+.1f} мград/°C, R²={r2_smt:.2f})', alpha=0.8, lw=1.2)

        axes[f_idx, 0].set_title(f'{freq} кГц: Несимметризованная разность фаз ΔΦ(T)', fontsize=11)
        axes[f_idx, 0].set_ylabel('ΔΦ, миллиградусы', fontsize=10)
        axes[f_idx, 0].grid(True, linestyle='--', alpha=0.5)
        axes[f_idx, 0].legend(fontsize=9, loc='best')

        axes[f_idx, 1].set_title(f'{freq} кГц: Симметризованная разность фаз Φ_smt(T)', fontsize=11)
        axes[f_idx, 1].set_ylabel('Φ_smt, миллиградусы', fontsize=10)
        axes[f_idx, 1].grid(True, linestyle='--', alpha=0.5)
        axes[f_idx, 1].legend(fontsize=9, loc='best')

    axes[1, 0].set_xlabel('Температура, °C', fontsize=11)
    axes[1, 1].set_xlabel('Температура, °C', fontsize=11)

    fig.suptitle(f'{sonde_name}: Температурный дрейф фазы (Несимметризованный vs Симметризованный)', fontsize=13)
    fig.tight_layout()
    plot_phase_path = os.path.join(out_dir, "gp_phase_vs_temperature.png")
    fig.savefig(plot_phase_path, dpi=200)
    plt.close(fig)

    # 3. Графики несимметризованного и симметризованного затухания
    log("Построение графика 3/7: амплитудное затухание...")
    fig, axes = plt.subplots(2, 2, figsize=(14, 10), sharex=True)
    for f_idx, freq in enumerate(freqs):
        for tx_idx, tx in enumerate(tx_list):
            am1 = df_valid[f"AM1_{freq}_T{tx}"].values
            am2 = df_valid[f"AM2_{freq}_T{tx}"].values
            att_smt = df_valid[f"ATT_smt_{freq}_T{tx}"].values
            res_row = next(r for r in results if r["freq"] == freq and r["tx"] == f"T{tx}")

            ln = L_near[tx - 1]
            lf = L_far[tx - 1]
            if tx % 2 == 1:
                am_near_arr = am1
                am_far_arr = am2
            else:
                am_near_arr = am2
                am_far_arr = am1

            att_raw_norm = 20.0 * np.log10((np.maximum(am_near_arr, 1e-6) / np.maximum(am_far_arr, 1e-6)) * ((ln / lf) ** 3))

            step = 5
            axes[f_idx, 0].plot(temp[::step], att_raw_norm[::step], color=colors[tx_idx],
                                label=f'T{tx} ({res_row["slope_raw_att_mdB_C"]:+.2f} мдБ/°C)', alpha=0.8, lw=1.2)
            axes[f_idx, 1].plot(temp[::step], att_smt[::step], color=colors[tx_idx],
                                label=f'T{tx} ({res_row["slope_att_smt_mdB_C"]:+.2f} мдБ/°C)', alpha=0.8, lw=1.2)

        axes[f_idx, 0].set_title(f'{freq} кГц: Несимметризованное затухание Att_несм(T) (норм. на 1/L³)', fontsize=11)
        axes[f_idx, 0].set_ylabel('Att_несм, дБ', fontsize=10)
        axes[f_idx, 0].grid(True, linestyle='--', alpha=0.5)
        axes[f_idx, 0].legend(fontsize=9, loc='best')

        axes[f_idx, 1].set_title(f'{freq} кГц: Симметризованное затухание Att_smt(T)', fontsize=11)
        axes[f_idx, 1].set_ylabel('Att_smt, дБ', fontsize=10)
        axes[f_idx, 1].grid(True, linestyle='--', alpha=0.5)
        axes[f_idx, 1].legend(fontsize=9, loc='best')

    axes[1, 0].set_xlabel('Температура, °C', fontsize=11)
    axes[1, 1].set_xlabel('Температура, °C', fontsize=11)

    fig.suptitle(f'{sonde_name}: Температурное поведение амплитудного затухания', fontsize=13)
    fig.tight_layout()
    plot_att_path = os.path.join(out_dir, "gp_attenuation_vs_temperature.png")
    fig.savefig(plot_att_path, dpi=200)
    plt.close(fig)

    # 4. График подавления дрейфа (4 столбчатые диаграммы)
    log("Построение графика 4/7: эффективность подавления дрейфа...")
    res_df = pd.DataFrame(results)
    fig, axes = plt.subplots(2, 2, figsize=(15, 11))
    x_indices = np.arange(len(tx_list))
    width = 0.35

    for f_idx, freq in enumerate(freqs):
        ax = axes[0, f_idx]
        sub = res_df[res_df["freq"] == freq]

        ax.bar(x_indices - width/2, np.abs(sub["slope_raw_mdeg_C"]), width,
               label='|Несимметризованный дрейф ΔΦ|', color='lightcoral', alpha=0.85)
        ax.bar(x_indices + width/2, np.abs(sub["slope_smt_mdeg_C"]), width,
               label='|Симметризованный дрейф Φ_smt|', color='royalblue', alpha=0.85)

        ax.set_xticks(x_indices)
        ax.set_xticklabels([f"{tx}\n({m:.0f} мВ·м³)" for tx, m in zip(sub["tx"], sub["M_Tx"])])
        ax.set_xlabel('Передатчик (в скобках мощность M_Tx)', fontsize=10)
        ax.set_ylabel('|dΦ/dT|, мград/°C', fontsize=10)
        ax.set_title(f'{freq} кГц: Дрейф разности фаз (несимм. vs симм.)', fontsize=11, fontweight='bold')
        ax.grid(True, linestyle='--', alpha=0.5, axis='y')
        ax.legend(fontsize=9)

    for f_idx, freq in enumerate(freqs):
        ax = axes[1, f_idx]
        sub = res_df[res_df["freq"] == freq]

        ax.bar(x_indices - width/2, np.abs(sub["slope_raw_att_mdB_C"]), width,
               label='|Несимметризованный дрейф Att|', color='coral', alpha=0.85)
        ax.bar(x_indices + width/2, np.abs(sub["slope_att_smt_mdB_C"]), width,
               label='|Симметризованный дрейф Att_smt|', color='seagreen', alpha=0.85)

        ax.set_xticks(x_indices)
        ax.set_xticklabels([f"{tx}\n({m:.0f} мВ·м³)" for tx, m in zip(sub["tx"], sub["M_Tx"])])
        ax.set_xlabel('Передатчик (в скобках мощность M_Tx)', fontsize=10)
        ax.set_ylabel('|dAtt/dT|, мдБ/°C', fontsize=10)
        ax.set_title(f'{freq} кГц: Дрейф амплитудного затухания (несимм. vs симм.)', fontsize=11, fontweight='bold')
        ax.grid(True, linestyle='--', alpha=0.5, axis='y')
        ax.legend(fontsize=9)

    fig.suptitle(f'{sonde_name}: Подавление температурного дрейфа фазы и затухания', fontsize=13, fontweight='bold')
    fig.tight_layout()
    plot_sym_4charts = os.path.join(out_dir, "gp_symmetrization_summary_4charts.png")
    fig.savefig(plot_sym_4charts, dpi=200)
    plt.close(fig)

    # 5. Метрологический анализ мощности и наводки
    log("Построение графика 5/7: метрология мощности и наводки...")
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))

    # Панель 1: Абсолютная мощность M_Tx
    ax = axes[0, 0]
    for f_idx, freq in enumerate(freqs):
        sub = res_df[res_df["freq"] == freq]
        offset = -width/2 if f_idx == 0 else width/2
        color = 'steelblue' if f_idx == 0 else 'darkorange'
        ax.bar(x_indices + offset, sub["M_Tx"], width, label=f'{freq} кГц', color=color, alpha=0.85)
    ax.set_xticks(x_indices)
    ax.set_xticklabels([f"T{tx}" for tx in tx_list])
    ax.set_xlabel('Передатчик', fontsize=10)
    ax.set_ylabel('M_Tx = AM · L³, мВ·м³', fontsize=10)
    ax.set_title('Абсолютная мощность (магнитный момент) передатчиков', fontsize=11)
    ax.grid(True, linestyle='--', alpha=0.5, axis='y')
    ax.legend(fontsize=9)

    # Панель 2: Абсолютная наводка V_leak (мВ)
    ax = axes[0, 1]
    for f_idx, freq in enumerate(freqs):
        sub = res_df[res_df["freq"] == freq]
        offset = -width/2 if f_idx == 0 else width/2
        color = 'crimson' if f_idx == 0 else 'purple'
        ax.bar(x_indices + offset, sub["V_leak_mV"], width, label=f'{freq} кГц', color=color, alpha=0.85)
    ax.set_xticks(x_indices)
    ax.set_xticklabels([f"T{tx}" for tx in tx_list])
    ax.set_xlabel('Передатчик', fontsize=10)
    ax.set_ylabel('V_leak, мВ', fontsize=10)
    ax.set_title('Абсолютная паразитная наводка V_leak на входе приёмника', fontsize=11)
    ax.grid(True, linestyle='--', alpha=0.5, axis='y')
    ax.legend(fontsize=9)

    # Панель 3: Кратность относительно молчащих передатчиков
    ax = axes[1, 0]
    for f_idx, freq in enumerate(freqs):
        sub = res_df[res_df["freq"] == freq]
        offset = -width/2 if f_idx == 0 else width/2
        color = 'teal' if f_idx == 0 else 'goldenrod'
        bars = ax.bar(x_indices + offset, sub["K_excess"], width, label=f'{freq} кГц', color=color, alpha=0.85)
        for bar, db_val in zip(bars, sub["K_excess_dB"]):
            yval = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2.0, yval + 2, f"{db_val:.0f}дБ",
                    ha='center', va='bottom', fontsize=8, rotation=0)
    ax.set_xticks(x_indices)
    ax.set_xticklabels([f"T{tx}" for tx in tx_list])
    ax.set_xlabel('Передатчик', fontsize=10)
    ax.set_ylabel('V_leak / V_zero (раз)', fontsize=10)
    ax.set_title('Кратность наводки относительно молчащих каналов (V_leak / V_zero)', fontsize=11)
    ax.grid(True, linestyle='--', alpha=0.5, axis='y')
    ax.legend(fontsize=9)

    # Панель 4: Паразитная связь относительно мощности
    ax = axes[1, 1]
    for f_idx, freq in enumerate(freqs):
        sub = res_df[res_df["freq"] == freq]
        offset = -width/2 if f_idx == 0 else width/2
        color = 'navy' if f_idx == 0 else 'maroon'
        ax.bar(x_indices + offset, sub["K_leak_abs"] * 1000.0, width, label=f'{freq} кГц', color=color, alpha=0.85)
    ax.set_xticks(x_indices)
    ax.set_xticklabels([f"T{tx}" for tx in tx_list])
    ax.set_xlabel('Передатчик', fontsize=10)
    ax.set_ylabel('K_leak · 10³, м⁻³', fontsize=10)
    ax.set_title('Паразитная связь относительно абсолютной мощности (V_leak / M_Tx)', fontsize=11)
    ax.grid(True, linestyle='--', alpha=0.5, axis='y')
    ax.legend(fontsize=9)

    fig.suptitle(f'{sonde_name}: Мощность, Наводка, Превышение над шумом', fontsize=13)
    fig.tight_layout()
    plot_leakage_analysis = os.path.join(out_dir, "gp_leakage_and_power_analysis.png")
    fig.savefig(plot_leakage_analysis, dpi=200)
    plt.close(fig)

    # 6. Температурная эволюция наводки и полезного излучения
    log("Построение графика 6/7: температурная эволюция наводки и мощности...")
    fig, axes = plt.subplots(2, 2, figsize=(15, 11))

    # Панель 1: Модуль наводки |D_leak|(T)
    ax1 = axes[0, 0]
    step = 5
    for r in results:
        f = r["freq"]
        tx_str = r["tx"]
        tx_num = int(tx_str[1])
        c = colors[tx_num - 1]
        ls = '-' if f == 400 else '--'
        ax1.plot(temp[::step], r["mod_leak_series"][::step], color=c, linestyle=ls,
                 label=f"{tx_str} {f}k ({r['D_leak_hot']:.1f}→{r['D_leak_cold']:.1f} мВ)", lw=1.3, alpha=0.85)
    ax1.set_xlabel('Температура, °C', fontsize=10)
    ax1.set_ylabel('|D_leak|, мВ', fontsize=10)
    ax1.set_title('Модуль дифференциальной паразитной наводки |D_leak|(T)', fontsize=11, fontweight='bold')
    ax1.grid(True, linestyle='--', alpha=0.5)
    ax1.legend(fontsize=8, loc='best', ncol=2)

    # Панель 2: Квадратурная составляющая Im{D_leak}(T)
    ax2 = axes[0, 1]
    for r in results:
        f = r["freq"]
        tx_str = r["tx"]
        tx_num = int(tx_str[1])
        c = colors[tx_num - 1]
        ls = '-' if f == 400 else '--'
        ax2.plot(temp[::step], r["im_leak_series"][::step], color=c, linestyle=ls,
                 label=f"{tx_str} {f}k ({r['im_leak_hot']:+.1f}→{r['im_leak_cold']:+.1f} мВ)", lw=1.3, alpha=0.85)
    ax2.set_xlabel('Температура, °C', fontsize=10)
    ax2.set_ylabel('Im{D_leak}, мВ (квадратурная)', fontsize=10)
    ax2.set_title('Квадратурная наводка Im{D_leak}(T) (источник искажения фазы)', fontsize=11, fontweight='bold')
    ax2.grid(True, linestyle='--', alpha=0.5)
    ax2.legend(fontsize=8, loc='best', ncol=2)

    # Панель 3: Общий годограф
    ax3 = axes[1, 0]
    for r in results:
        f = r["freq"]
        tx_str = r["tx"]
        tx_num = int(tx_str[1])
        c = colors[tx_num - 1]
        ls = '-' if f == 400 else '--'
        re_s = r["re_leak_series"]
        im_s = r["im_leak_series"]
        ax3.plot(re_s[::step], im_s[::step], color=c, linestyle=ls, lw=1.3, alpha=0.8, label=f"{tx_str} {f}k")
        ax3.scatter(re_s[0], im_s[0], color=c, marker='s', s=35, zorder=4)
        ax3.scatter(re_s[-1], im_s[-1], color=c, marker='o', s=35, zorder=4)
    ax3.set_xlabel('Re{D_leak}, мВ (синфазная наводка)', fontsize=10)
    ax3.set_ylabel('Im{D_leak}, мВ (квадратурная наводка)', fontsize=10)
    ax3.set_title('Сводный годограф наводки (квадрат: 120°C, круг: 25°C)', fontsize=11, fontweight='bold')
    ax3.grid(True, linestyle='--', alpha=0.5)
    ax3.axhline(0, color='gray', lw=0.7, ls=':')
    ax3.axvline(0, color='gray', lw=0.7, ls=':')
    ax3.legend(fontsize=8, loc='best', ncol=2)

    # Панель 4: Полезная мощность передатчиков M_Tx(T)
    ax4 = axes[1, 1]
    for r in results:
        f = r["freq"]
        tx_str = r["tx"]
        tx_num = int(tx_str[1])
        c = colors[tx_num - 1]
        ls = '-' if f == 400 else '--'
        m_s = r["m_tx_series"]
        ax4.plot(temp[::step], m_s[::step], color=c, linestyle=ls,
                 label=f"{tx_str} {f}k (спад: {r['M_Tx_drop_pct']:.1f}%)", lw=1.3, alpha=0.85)
    ax4.set_xlabel('Температура, °C', fontsize=10)
    ax4.set_ylabel('M_Tx = AM · L³, мВ·м³', fontsize=10)
    ax4.set_title('Полезная мощность излучения M_Tx(T) (падение из-за нагрева контура)', fontsize=11, fontweight='bold')
    ax4.grid(True, linestyle='--', alpha=0.5)
    ax4.legend(fontsize=8, loc='best', ncol=2)

    fig.suptitle(f'{sonde_name}: Векторное разделение полезного сигнала и эволюции наводки', fontsize=13, fontweight='bold')
    fig.tight_layout()
    plot_leakage_evolution = os.path.join(out_dir, "gp_leakage_temperature_evolution.png")
    fig.savefig(plot_leakage_evolution, dpi=200)
    plt.close(fig)

    # 7. Новый детальный диагностический лист: поканальные масштабированные годографы (2 x N_Tx)
    log(f"Построение графика 7/7: поканальные годографы для схемотехников (2 x {n_tx})...")
    fig, axes = plt.subplots(2, n_tx, figsize=(max(4.0 * n_tx, 10), 8), sharex=False, sharey=False)

    for f_idx, freq in enumerate(freqs):
        for tx_idx, tx in enumerate(tx_list):
            ax = axes[f_idx, tx_idx]
            r = next(item for item in results if item["freq"] == freq and item["tx"] == f"T{tx}")

            re_s = r["re_leak_series"]
            im_s = r["im_leak_series"]
            c = colors[tx_idx]

            ax.plot(re_s[::step], im_s[::step], color=c, lw=1.8, alpha=0.9)
            ax.scatter(re_s[0], im_s[0], color='crimson', marker='s', s=55, zorder=5, label='120°C (старт)')
            ax.scatter(re_s[-1], im_s[-1], color='navy', marker='o', s=55, zorder=5, label='25°C (финиш)')

            # Стрелка направления движения при остывании
            mid_idx = len(re_s) // 2
            dx = re_s[-1] - re_s[0]
            dy = im_s[-1] - im_s[0]
            if np.hypot(dx, dy) > 0.05:
                ax.annotate('', xy=(re_s[mid_idx], im_s[mid_idx]),
                            xytext=(re_s[mid_idx] - dx * 0.15, im_s[mid_idx] - dy * 0.15),
                            arrowprops=dict(arrowstyle="->", color=c, lw=2.0))

            ax.grid(True, linestyle='--', alpha=0.6)
            ax.axhline(0, color='gray', lw=0.6, ls=':')
            ax.axvline(0, color='gray', lw=0.6, ls=':')

            dre = r["delta_re_mV"]
            dim = r["delta_im_mV"]
            title_text = f"T{tx} ({freq} кГц)\n{r['nature_short']}"
            ax.set_title(title_text, fontsize=10, fontweight='bold', color='darkred' if abs(dre) > 5.0 else 'black')
            ax.set_xlabel('Re (мВ)', fontsize=9)
            if tx_idx == 0:
                ax.set_ylabel(f'{freq} кГц\nIm (мВ)', fontsize=10, fontweight='bold')

            info = f"ΔRe={dre:+.2f}\nΔIm={dim:+.2f}\n{r['leak_pct_far']:.1f}% сигнала"
            ax.text(0.04, 0.06, info, transform=ax.transAxes, fontsize=8,
                    bbox=dict(boxstyle="round,pad=0.3", fc="white", ec="gray", alpha=0.85))

    fig.suptitle(f'{sonde_name}: Поканальные годографы наводки (индивидуальный масштаб для схемотехников)\n'
                 'Квадрат: 120°C (горячий прибор), Круг: 25°C (холодный прибор)', fontsize=13, fontweight='bold')
    fig.tight_layout()
    plot_hodographs_path = os.path.join(out_dir, "gp_hodograph_per_channel.png")
    fig.savefig(plot_hodographs_path, dpi=200)
    plt.close(fig)

    # 8. Формирование подробного инженерного протокола для схемотехников
    log("Формирование инженерного текстового протокола...")
    report_file = os.path.join(out_dir, "gp_hardware_leakage_report.txt")
    report_file_compat = os.path.join(out_dir, "gp_temperature_report.txt")

    risk_list = []
    for r in results:
        span_leak = np.hypot(r["delta_re_mV"], r["delta_im_mV"])
        pct_far = r["leak_pct_far"]
        risk_score = span_leak * (1.0 + pct_far / 10.0)
        risk_list.append((risk_score, r))
    risk_list.sort(key=lambda x: x[0], reverse=True)

    report_lines = []
    def pr(line=""):
        report_lines.append(line)

    pr("=========================================================================================================")
    pr("  ПРОТОКОЛ ДИАГНОСТИКИ АППАРАТУРНЫХ НАВОДОК И ТЕМПЕРАТУРНОГО ДРЕЙФА ЗОНДА GP_DATA")
    pr("  Организация: НПФ «АМК Горизонт», разработал зам начальника ОП Смирнов С. Г.")
    pr("=========================================================================================================\n")
    pr(f"Прибор (SondeType):   {sonde_name}")
    pr(f"Заводской номер (SN): {serial_num}")
    pr(f"Сигнатура метрологии: {signature_str}")
    pr(f"Число передатчиков:   {n_tx}")
    pr(f"Базы катушек L1 (м):  {', '.join(f'{x:.3f}' for x in L1)}")
    pr(f"Базы катушек L2 (м):  {', '.join(f'{x:.3f}' for x in L2)}")
    pr(f"Файл данных:          {os.path.abspath(csv_file)}")
    pr(f"Папка результатов:    {os.path.abspath(out_dir)}")
    pr(f"Всего строк (кадров): {total_rows}")
    pr(f"Отобрано стабильных:  {len(df_valid)}")
    pr(f"Диапазон температур:  {t_max:.2f} °C -> {t_min:.2f} °C (ΔT = {delta_t:.2f} °C)\n")

    pr("---------------------------------------------------------------------------------------------------------")
    pr("1. СВОДНАЯ ДИАГНОСТИЧЕСКАЯ КАРТА ДЛЯ СХЕМОТЕХНИКОВ (РАНЖИРОВАНИЕ ПО ОПАСНОСТИ НАВОДКИ)")
    pr("---------------------------------------------------------------------------------------------------------")
    pr(f"{'Ранг':<5} {'Freq':<6} {'Tx':<4} {'ΔRe (120→25)':<14} {'ΔIm (120→25)':<14} {'Угол θ':<8} {'Доля в сигнале':<16} {'Характер наводки'}")
    pr(f"{'':<5} {'(кГц)':<6} {'':<4} {'(мВ, активн.)':<14} {'(мВ, реакт.)':<14} {'(град)':<8} {'(на дальнем Rx)':<16} {''}")
    pr("-" * 115)

    for rank, (_, r) in enumerate(risk_list, 1):
        pr(f"#{rank:<4} {r['freq']:<6} {r['tx']:<4} {r['delta_re_mV']:<+14.2f} {r['delta_im_mV']:<+14.2f} "
           f"{r['theta_deg']:<+8.1f} {r['leak_pct_far']:<15.1f}% {r['nature']}")

    pr("\n---------------------------------------------------------------------------------------------------------")
    pr("2. ДЕТАЛЬНАЯ ДИАГНОСТИКА АКТИВНОЙ И РЕАКТИВНОЙ СОСТАВЛЯЮЩИХ ПАРАЗИТНОЙ СВЯЗИ")
    pr("---------------------------------------------------------------------------------------------------------")
    pr(f"{'Freq':<6} {'Tx':<4} {'|D|(120°C)':<11} {'|D|(25°C)':<10} {'d|D|/dT':<10} {'dRe/dT':<10} {'dIm/dT':<10} {'V_useful(Rx2)':<14} {'Доля наводки'}")
    pr(f"{'(кГц)':<6} {'':<4} {'(мВ)':<11} {'(мВ)':<10} {'(мкВ/°C)':<10} {'(мкВ/°C)':<10} {'(мкВ/°C)':<10} {'(мВ)':<14} {'(Rx2, %)'}")
    pr("-" * 105)

    for r in results:
        pr(f"{r['freq']:<6} {r['tx']:<4} {r['D_leak_hot']:<11.2f} {r['D_leak_cold']:<10.2f} "
           f"{r['slope_mod_leak_uV_C']:<+10.1f} {r['slope_re_leak_uV_C']:<+10.1f} {r['slope_im_leak_uV_C']:<+10.1f} "
           f"{r['V_useful_far_mV']:<14.2f} {r['leak_pct_far']:<10.1f}%")

    pr("\n---------------------------------------------------------------------------------------------------------")
    pr("3. ПАДЕНИЕ МОЩНОСТИ ИЗЛУЧЕНИЯ ПЕРЕДАТЧИКОВ (ДЕГРАДАЦИЯ ДОБРОТНОСТИ И НАГРЕВ КАТУШЕК)")
    pr("---------------------------------------------------------------------------------------------------------")
    pr(f"{'Freq':<6} {'Tx':<4} {'M_Tx(120°C)':<13} {'M_Tx(25°C)':<12} {'Спад мощности':<15} {'AM_near(120°C)':<15} {'AM_near(25°C)'}")
    pr(f"{'(кГц)':<6} {'':<4} {'(мВ·м³)':<13} {'(мВ·м³)':<12} {'(нагрев, %)':<15} {'(мВ)':<15} {'(мВ)'}")
    pr("-" * 105)

    for r in results:
        pr(f"{r['freq']:<6} {r['tx']:<4} {r['M_Tx_hot']:<13.1f} {r['M_Tx_cold']:<12.1f} "
           f"-{r['M_Tx_drop_pct']:<14.1f} {r['m_tx_series'][0]/(r['L_near_m']**3):<15.1f} "
           f"{r['m_tx_series'][-1]/(r['L_near_m']**3):.1f}")

    pr("\n---------------------------------------------------------------------------------------------------------")
    pr("4. ПОВЕДЕНИЕ ШТАТНОЙ СИММЕТРИЗАЦИИ (ОСТАТОЧНЫЙ ДРЕЙФ ПРИ БАЗОВЫХ КОЭФФИЦИЕНТАХ)")
    pr("---------------------------------------------------------------------------------------------------------")
    pr(f"{'Freq':<6} {'Tx':<4} {'Несимм. dΦ':<13} {'Симм. dΦ':<12} {'Подавл. Φ':<11} {'Несимм. dAtt':<13} {'Симм. dAtt':<12} {'Подавл. Att'}")
    pr(f"{'(кГц)':<6} {'':<4} {'(мград/°C)':<13} {'(мград/°C)':<12} {'(раз)':<11} {'(мдБ/°C)':<13} {'(мдБ/°C)':<12} {'(раз)'}")
    pr("-" * 105)

    for r in results:
        pr(f"{r['freq']:<6} {r['tx']:<4} {r['slope_raw_mdeg_C']:<+13.2f} {r['slope_smt_mdeg_C']:<+12.2f} "
           f"{r['drift_reduction_ph']:<11.1f} {r['slope_raw_att_mdB_C']:<+13.2f} {r['slope_att_smt_mdB_C']:<+12.2f} "
           f"{r['drift_reduction_att']:<6.1f}")

    pr("\n---------------------------------------------------------------------------------------------------------")
    pr("5. КОНКРЕТНЫЕ УКАЗАНИЯ И РЕКОМЕНДАЦИИ ДЛЯ СХЕМОТЕХНИКОВ И КОНСТРУКТОРОВ")
    pr("---------------------------------------------------------------------------------------------------------")
    pr("1. АНАЛИЗ СИНФАЗНОЙ (АКТИВНОЙ, Re) НАВОДКИ:")
    pr("   - Синфазная наводка (угол близкий к 0° или 180°) порождается прямым протеканием токов накачки")
    pr("     по металлоконструкциям корпуса, земляным шинам плат и оплеткам кабельных жгутов.")
    pr("   - Решение в «железе»:")
    pr("     а) Свить провод накачки каждого мощного передатчика в плотную витую пару с возвратным земляным проводом;")
    pr("     б) Устранить двойное заземление экранов кабелей (разорвать паразитные контуры по корпусу);")
    pr("     в) Установить симметрирующий балун / трансформатор на выходе генераторных ключей;")
    pr("     г) Установить синфазный ферритовый фильтр (common-mode choke) в цепи питания генераторов.\n")

    pr("2. АНАЛИЗ КВАДРАТУРНОЙ (РЕАКТИВНОЙ, Im) НАВОДКИ:")
    pr("   - Квадратурная наводка (угол близкий к ±90°) порождается паразитной ёмкостной связью монтажа,")
    pr("     горячих выводов катушек и проводников с приёмным трактом, а также фазовыми сдвигами в фильтрах.")
    pr("   - Решение: экранирование плат генераторов, минимизация площади ВЧ-полигонов, качественная изоляция.\n")

    pr("3. ВЛИЯНИЕ ДЛИНЫ ЗОНДОВ (1/L³):")
    pr("   - Сигнал на дальнем приёмнике длинных зондов ослаблен геометрией в десятки раз по сравнению с короткими.")
    pr("   - Даже небольшая аппаратная наводка в 3-10 мВ, незаметная на коротких зондах (<0.5%), составляет")
    pr("     критические 3-8% от полезного сигнала длинных зондов, приводя к искажению кажущегося сопротивления.\n")

    pr("4. НАГРЕВ КОНТУРОВ:")
    pr("   - Падение полезного магнитного момента M_Tx на 16% - 38% вызвано ростом активного сопротивления")
    pr("     меди обмоток катушек (температурный коэффициент меди +0.39%/°C).")
    pr("   - Рекомендуется стабилизация тока накачки (генератор тока) вместо простого ключа напряжения.")
    pr("=========================================================================================================\n")

    report_text = "\n".join(report_lines)
    with open(report_file, "w", encoding="utf-8") as f:
        f.write(report_text)
    with open(report_file_compat, "w", encoding="utf-8") as f:
        f.write(report_text)

    log(f"Сохранён инженерный отчёт: {report_file}")
    log(f"Все 7 диагностических графиков и протокол сохранены в:\n{out_dir}")

    return {
        "out_dir": out_dir,
        "report_file": report_file,
        "results": results,
        "n_tx": n_tx,
        "sonde_name": sonde_name,
        "serial_num": serial_num,
        "total_rows": total_rows,
        "valid_rows": len(df_valid),
        "t_min": t_min,
        "t_max": t_max,
        "risk_list": risk_list
    }


# =============================================================================
# ГРАФИЧЕСКАЯ ОБОЛОЧКА (Tkinter GUI)
# =============================================================================

def run_gui():
    import tkinter as tk
    from tkinter import ttk, filedialog, messagebox

    root = tk.Tk()
    root.title("Диагностика паразитных наводок зонда (GP_DATA)")
    root.geometry("860x650")
    root.minsize(720, 520)

    try:
        root.iconbitmap(default='')
    except Exception:
        pass

    last_dir = [os.getcwd()]

    header_frame = ttk.Frame(root, padding="10")
    header_frame.pack(fill=tk.X)

    title_label = ttk.Label(header_frame, text="Комплекс диагностики аппаратурных наводок индукционного зонда",
                            font=("Segoe UI", 12, "bold"))
    title_label.pack(anchor=tk.W)

    author_label = ttk.Label(header_frame,
                             text="Организация: НПФ «АМК Горизонт», разработал зам начальника ОП Смирнов С. Г.",
                             font=("Segoe UI", 9, "italic"), foreground="#444444")
    author_label.pack(anchor=tk.W, pady=(2, 0))

    separator = ttk.Separator(root, orient=tk.HORIZONTAL)
    separator.pack(fill=tk.X, padx=10, pady=5)

    file_frame = ttk.LabelFrame(root, text="Параметры данных и прибора", padding="10")
    file_frame.pack(fill=tk.X, padx=10, pady=5)

    csv_var = tk.StringVar()
    outdir_var = tk.StringVar()
    passport_var = tk.StringVar(value="Паспорт прибора: выберите файл данных для определения")
    open_after_var = tk.BooleanVar(value=True)

    row1 = ttk.Frame(file_frame)
    row1.pack(fill=tk.X, pady=3)
    ttk.Label(row1, text="CSV файл данных:", width=18).pack(side=tk.LEFT)
    csv_entry = ttk.Entry(row1, textvariable=csv_var)
    csv_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)

    def update_passport_display(fpath):
        if not fpath or not os.path.isfile(fpath):
            passport_var.set("Паспорт прибора: файл не выбран")
            return
        meta = parse_csv_metadata(fpath)
        sonde = meta.get("SondeType", "")
        sn = meta.get("Serial", "")
        ntx = meta.get("N_Tx", "")
        if sonde:
            txt = f"Прибор: {sonde} (SN: {sn}, N_Tx: {ntx})"
            if "L1_mm" in meta:
                txt += f", L1={meta['L1_mm']} мм"
            passport_var.set(txt)
        else:
            passport_var.set("Паспорт прибора: старый формат CSV (N_Tx будет определён автоматически)")

    def on_browse_csv():
        initial_dir = last_dir[0]
        if csv_var.get() and os.path.isfile(csv_var.get()):
            initial_dir = os.path.dirname(csv_var.get())
        fpath = filedialog.askopenfilename(
            title="Выберите CSV-файл ненаправленных измерений (GP_DATA)",
            initialdir=initial_dir,
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")]
        )
        if fpath:
            fpath = os.path.normpath(fpath)
            csv_var.set(fpath)
            last_dir[0] = os.path.dirname(fpath)
            stem = os.path.splitext(os.path.basename(fpath))[0]
            auto_out = os.path.join(os.path.dirname(fpath), f"gp_analysis_{stem}")
            outdir_var.set(auto_out)
            update_passport_display(fpath)

    ttk.Button(row1, text="Обзор...", width=10, command=on_browse_csv).pack(side=tk.RIGHT)

    # Строка паспорта
    row_pass = ttk.Frame(file_frame)
    row_pass.pack(fill=tk.X, pady=(2, 4))
    ttk.Label(row_pass, textvariable=passport_var, font=("Segoe UI", 8, "italic"), foreground="#004080").pack(anchor=tk.W, padx=(130, 0))

    # Строка Outdir
    row2 = ttk.Frame(file_frame)
    row2.pack(fill=tk.X, pady=3)
    ttk.Label(row2, text="Папка результатов:", width=18).pack(side=tk.LEFT)
    outdir_entry = ttk.Entry(row2, textvariable=outdir_var)
    outdir_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)

    def on_browse_outdir():
        initial_dir = outdir_var.get() if outdir_var.get() else last_dir[0]
        dpath = filedialog.askdirectory(
            title="Выберите папку для сохранения отчётов и графиков",
            initialdir=initial_dir
        )
        if dpath:
            outdir_var.set(os.path.normpath(dpath))

    ttk.Button(row2, text="Папка...", width=10, command=on_browse_outdir).pack(side=tk.RIGHT)

    # Опции
    row3 = ttk.Frame(file_frame)
    row3.pack(fill=tk.X, pady=(5, 0))
    ttk.Checkbutton(row3, text="Открыть папку с графиками и отчётом после завершения",
                    variable=open_after_var).pack(side=tk.LEFT)

    # Кнопки запуска
    btn_frame = ttk.Frame(root, padding="10")
    btn_frame.pack(fill=tk.X)

    btn_start = ttk.Button(btn_frame, text="▶  Запустить диагностику")
    btn_start.pack(side=tk.LEFT, padx=(0, 10))

    btn_open = ttk.Button(btn_frame, text="📁  Открыть папку с результатами", state=tk.DISABLED)
    btn_open.pack(side=tk.LEFT)

    progress_bar = ttk.Progressbar(btn_frame, mode='indeterminate')

    # Окно журнала
    log_frame = ttk.LabelFrame(root, text="Журнал и выводы диагностики", padding="10")
    log_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

    txt_log = tk.Text(log_frame, wrap=tk.WORD, font=("Consolas", 9), bg="#fcfcfc")
    txt_log.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    scrollbar = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=txt_log.yview)
    scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    txt_log.config(yscrollcommand=scrollbar.set)

    def log_to_text(msg):
        def _append():
            txt_log.insert(tk.END, msg + "\n")
            txt_log.see(tk.END)
        root.after(0, _append)

    last_out_dir = [None]

    def on_open_folder():
        if last_out_dir[0] and os.path.isdir(last_out_dir[0]):
            open_folder_in_explorer(last_out_dir[0])

    btn_open.config(command=on_open_folder)

    def worker():
        try:
            csv_path = csv_var.get().strip()
            out_path = outdir_var.get().strip()
            if not csv_path or not os.path.isfile(csv_path):
                root.after(0, lambda: messagebox.showerror("Ошибка", f"Файл не найден:\n{csv_path}"))
                return

            res = analyze_gp_data(csv_path, out_path if out_path else None, log_cb=log_to_text)
            last_out_dir[0] = res["out_dir"]

            def _on_success():
                btn_start.config(state=tk.NORMAL)
                btn_open.config(state=tk.NORMAL)
                progress_bar.stop()
                progress_bar.pack_forget()
                messagebox.showinfo(
                    "Успех",
                    f"Диагностика успешно завершена!\n\nПрибор: {res['sonde_name']} ({res['n_tx']} передатчиков)\nРезультаты сохранены в:\n{res['out_dir']}"
                )
                if open_after_var.get():
                    open_folder_in_explorer(res["out_dir"])

            root.after(0, _on_success)
        except Exception as ex:
            def _on_error(err=str(ex)):
                btn_start.config(state=tk.NORMAL)
                progress_bar.stop()
                progress_bar.pack_forget()
                txt_log.insert(tk.END, f"\n[ОШИБКА] {err}\n")
                messagebox.showerror("Ошибка диагностики", f"Произошла ошибка при анализе:\n{err}")
            root.after(0, _on_error)

    def on_start():
        csv_path = csv_var.get().strip()
        if not csv_path:
            messagebox.showwarning("Внимание", "Пожалуйста, выберите CSV-файл для анализа!")
            return
        if not os.path.isfile(csv_path):
            messagebox.showerror("Ошибка", f"Указанный файл не существует:\n{csv_path}")
            return

        txt_log.delete("1.0", tk.END)
        btn_start.config(state=tk.DISABLED)
        btn_open.config(state=tk.DISABLED)
        progress_bar.pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=10)
        progress_bar.start(10)

        t = threading.Thread(target=worker, daemon=True)
        t.start()

    btn_start.config(command=on_start)

    default_candidates = [
        "gp_data_4635_11895.csv",
        "gp_data_all.csv"
    ]
    for c in default_candidates:
        if os.path.isfile(c):
            full_c = os.path.abspath(c)
            csv_var.set(full_c)
            stem = os.path.splitext(os.path.basename(full_c))[0]
            outdir_var.set(os.path.join(os.path.dirname(full_c), f"gp_analysis_{stem}"))
            update_passport_display(full_c)
            break

    root.mainloop()


# =============================================================================
# CLI РЕЖИМ
# =============================================================================

def parse_args():
    parser = argparse.ArgumentParser(description="Анализ температурного дрейфа и аппаратурных паразитных наводок (GP_DATA)")
    parser.add_argument("--file", "-f", default=None, help="Путь к файлу GP CSV")
    parser.add_argument("--outdir", "-o", default=None, help="Папка для сохранения результатов (по умолчанию подпапка рядом с CSV)")
    parser.add_argument("--gui", action="store_true", help="Запустить графический интерфейс (GUI)")
    parser.add_argument("--no-gui", action="store_true", help="Принудительный запуск в консольном режиме")
    parser.add_argument("--open-folder", action="store_true", help="Открыть папку результатов в Проводнике по завершении")
    return parser.parse_args()


def main():
    args = parse_args()

    should_run_gui = args.gui or (args.file is None and not args.no_gui and len(sys.argv) <= 1)

    if should_run_gui:
        run_gui()
    else:
        csv_file = args.file if args.file else "gp_data_4635_11895.csv"
        try:
            res = analyze_gp_data(csv_file, args.outdir)
            if args.open_folder:
                open_folder_in_explorer(res["out_dir"])
        except Exception as e:
            print(f"[ERROR] Ошибка анализа: {e}", file=sys.stderr)
            sys.exit(1)


if __name__ == "__main__":
    main()
