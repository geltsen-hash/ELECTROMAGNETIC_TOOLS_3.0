"""
Модуль спектрального анализа сырых секторов (RAW_DATA) зонда «Картограф».
Выполняет:
  - Гармонический анализ 16 секторов (Re и Im): H0 (Vzz), H1 (Vzx), H2 (Vxx), высшие гармоники H3..H7;
  - Оценку «мусора Фурье»: остаточный шум dv, коэффициент нелинейных искажений THD (%), отношение H2/H1;
  - Сквозную сверку бортового микроконтроллера зонда (direct_RX) с расчетом из сырых секторов (Hardware QA).

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

import math
from dataclasses import dataclass
from typing import List, Tuple, Dict, Optional
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

from .bin_parser import SectorData, ChannelHarmonics


# Базовая матрица коэффициентов дискретного Фурье для 16 секторов (k=0..7, n=0..15)
FOURIER_KERNEL = np.zeros((8, 16), dtype=np.complex64)
for _k in range(8):
    for _n in range(16):
        FOURIER_KERNEL[_k, _n] = np.exp(1j * 2.0 * np.pi * _k * _n / 16.0) / 8.0


@dataclass
class SectorHarmonics:
    h0_vzz: float              # H0 (амплитуда Vzz, мВ)
    h1_vzx: float              # H1 (амплитуда Vzx, мВ)
    h2_vxx: float              # H2 (амплитуда второй гармоники Vxx, мВ)
    dph_rad: float             # Фазовый сдвиг arg(Vzx / Vzz) в радианах
    dph_deg: float             # Фазовый сдвиг в градусах
    higher_harmonics: np.ndarray # Амплитуды гармоник H3..H7 (мВ)
    residual_noise_dv: float   # Остаточный шум аппроксимации (мусор Фурье, мВ)
    thd_pct: float             # Коэффициент гармонического шума THD (%)
    h2_div_h1_pct: float       # Отношение 2-й гармоники к 1-й (%)
    snr_db: float              # Отношение полезного сигнала к шуму SNR (дБ)
    h1_div_h2: float = 0.0     # Отношение 1-й гармоники ко 2-й (во сколько раз H1 > H2)
    noise_h3_h7_mv: float = 0.0 # Амплитуда высших гармоник H3..H7 (мусор Фурье, мВ)


@dataclass
class ProcessorComparison:
    vzz_direct: float
    vzz_calc: float
    vzz_diff_pct: float
    vzx_direct: float
    vzx_calc: float
    vzx_diff_pct: float
    dph_direct_deg: float
    dph_calc_deg: float
    dph_diff_deg: float
    status: str  # 'OK', 'WARN', 'FAIL'


def analyze_sector_data(re_mv: np.ndarray, im_mv: np.ndarray) -> SectorHarmonics:
    """
    Разложение 16 секторов Re и Im в ряд Фурье (до 8 гармоник).
    Определяет H0, H1, H2, оценивает мусор Фурье и остаточный шум.
    """
    harm_I = np.zeros(8, dtype=np.complex64)
    harm_Q = np.zeros(8, dtype=np.complex64)

    for k in range(8):
        harm_I[k] = np.sum(re_mv * FOURIER_KERNEL[k])
        harm_Q[k] = np.sum(im_mv * FOURIER_KERNEL[k])

    harm_I[0] /= 2.0
    harm_Q[0] /= 2.0

    # H0 (Vzz): нулевая гармоника
    Vzz = float(np.real(harm_I[0])) + 1j * float(np.real(harm_Q[0]))
    h0 = float(abs(Vzz))

    # H1 (Vzx): первая гармоника (полная амплитуда по формуле бортового контроллера)
    amp_I1 = float(abs(harm_I[1]))
    amp_Q1 = float(abs(harm_Q[1]))
    h1 = float(math.hypot(amp_I1, amp_Q1))

    # H2 (Vxx): вторая гармоника (полная квадратичная амплитуда)
    amp_I2 = float(abs(harm_I[2]))
    amp_Q2 = float(abs(harm_Q[2]))
    h2 = float(math.hypot(amp_I2, amp_Q2))

    # Фазовый угол первой гармоники
    fi_I = math.atan2(float(np.imag(harm_I[1])), float(np.real(harm_I[1])))
    fi_Q = math.atan2(float(np.imag(harm_Q[1])), float(np.real(harm_Q[1])))
    denom = amp_I1 + amp_Q1 + 1e-12
    fi = fi_I * (amp_I1 / denom) + fi_Q * (amp_Q1 / denom)

    Vzx = math.cos(fi) * (float(np.real(harm_I[1])) + 1j * float(np.real(harm_Q[1]))) + \
          math.sin(fi) * (float(np.imag(harm_I[1])) + 1j * float(np.imag(harm_Q[1])))

    if h0 > 1e-12 and h1 > 1e-12:
        ratio = Vzx / Vzz
        dph_rad = math.atan2(float(np.imag(ratio)), float(np.real(ratio)))
    else:
        dph_rad = 0.0

    dph_deg = math.degrees(dph_rad)

    # Высшие гармоники H3..H7 (мусор Фурье)
    higher_amps = np.array([math.hypot(abs(harm_I[k]), abs(harm_Q[k])) for k in range(3, 8)], dtype=np.float32)
    higher_power = float(np.sum(higher_amps**2))
    noise_h3_h7_mv = float(math.sqrt(higher_power))

    # Оценка остаточного шума dv (отклонение формы от H0 + H1 + H2)
    angles = np.arange(16) * (2.0 * np.pi / 16.0)
    s_meas = np.sqrt(re_mv**2 + im_mv**2)
    s_model = h0 + h1 * np.cos(angles - fi)
    residuals = s_meas - s_model
    dv = float(np.sqrt(np.mean(residuals**2)))

    # THD (%) — отношение мощности высших гармоник (H3..H7) к полезной первой гармонике H1
    thd = (noise_h3_h7_mv / h1 * 100.0) if h1 > 1e-6 else 0.0

    # Отношение 2-й гармоники к 1-й (%)
    h2_ratio = (h2 / h1 * 100.0) if h1 > 1e-6 else 0.0

    # Отношение 1-й гармоники ко 2-й (во сколько раз H1 > H2)
    h1_div_h2 = (h1 / h2) if h2 > 1e-6 else (h1 / 1e-6)

    # SNR (дБ)
    snr = (20.0 * math.log10(h1 / dv)) if (dv > 1e-6 and h1 > 1e-6) else 0.0

    return SectorHarmonics(
        h0_vzz=h0,
        h1_vzx=h1,
        h2_vxx=h2,
        dph_rad=dph_rad,
        dph_deg=dph_deg,
        higher_harmonics=higher_amps,
        residual_noise_dv=dv,
        thd_pct=thd,
        h2_div_h1_pct=h2_ratio,
        snr_db=snr,
        h1_div_h2=h1_div_h2,
        noise_h3_h7_mv=noise_h3_h7_mv,
    )


def compare_processor_with_raw(direct: ChannelHarmonics, calc: SectorHarmonics) -> ProcessorComparison:
    """Сравнение бортовых гармоник direct_RX с расчётом из 16 сырых секторов."""
    vzz_diff = ((direct.vzz - calc.h0_vzz) / calc.h0_vzz * 100.0) if calc.h0_vzz > 1e-6 else 0.0
    vzx_diff = ((direct.vzx - calc.h1_vzx) / calc.h1_vzx * 100.0) if calc.h1_vzx > 1e-6 else 0.0

    # Разность фаз (с учетом возможной инверсии на 180 град из-за знака тригонометрии)
    dph_dir_deg = math.degrees(direct.dph_rad)
    dph_diff = dph_dir_deg - calc.dph_deg
    while dph_diff > 180.0:
        dph_diff -= 360.0
    while dph_diff < -180.0:
        dph_diff += 360.0

    dph_effective_diff = min(abs(dph_diff), abs(abs(dph_diff) - 180.0))

    abs_vzz = abs(vzz_diff)
    abs_vzx = abs(vzx_diff)

    if abs_vzz < 0.1 and abs_vzx < 0.1 and dph_effective_diff < 0.5:
        st = "OK"
    elif abs_vzz < 0.5 and abs_vzx < 0.5 and dph_effective_diff < 2.0:
        st = "WARN"
    else:
        st = "FAIL"

    return ProcessorComparison(
        vzz_direct=direct.vzz,
        vzz_calc=calc.h0_vzz,
        vzz_diff_pct=vzz_diff,
        vzx_direct=direct.vzx,
        vzx_calc=calc.h1_vzx,
        vzx_diff_pct=vzx_diff,
        dph_direct_deg=dph_dir_deg,
        dph_calc_deg=calc.dph_deg,
        dph_diff_deg=dph_diff,
        status=st,
    )


def generate_hardware_and_noise_report(
    summary_data: List[Dict],
    out_txt_path: str,
) -> str:
    """Генерация текстового протокола верификации бортового процессора и анализа шумов."""
    lines = [
        "==========================================================================================",
        "  ПРОТОКОЛ ВЕРИФИКАЦИИ БОРТОВОГО ПРОЦЕССОРА И СПЕКТРАЛЬНОГО АНАЛИЗА ШУМОВ (МУСОР ФУРЬЕ)",
        "==========================================================================================",
        "Организация: НПФ «АМК Горизонт» | Разработал: зам начальника ОП Смирнов С. Г.\n",
    ]

    lines.append("1. СРАВНЕНИЕ РАСЧЕТА БОРТОВОГО ПРОЦЕССОРА (direct_RX) С СЫРЫМИ СЕКТОРАМИ (RAW_DATA):")
    lines.append(f"{'Высота, м':>10s} {'Частота':>8s} {'Канал':>8s} {'Vzz Борт':>10s} {'Vzz Raw':>10s} {'dVzz,%':>8s} {'Vzx Борт':>10s} {'Vzx Raw':>10s} {'dVzx,%':>8s} {'dPh diff':>10s} {'Статус':>7s}")
    lines.append("-" * 105)

    for item in summary_data:
        h = item['height']
        f = item['freq']
        ch = item['channel']
        comp: ProcessorComparison = item['comparison']
        lines.append(
            f"{h:10.3f} {f:8d} {ch:>8s} {comp.vzz_direct:10.2f} {comp.vzz_calc:10.2f} {comp.vzz_diff_pct:+7.2f}% "
            f"{comp.vzx_direct:10.2f} {comp.vzx_calc:10.2f} {comp.vzx_diff_pct:+7.2f}% {comp.dph_diff_deg:+9.2f}° {comp.status:>7s}"
        )

    lines.append("\n2. АНАЛИЗ МУСОРА ФУРЬЕ, ВТОРОЙ ГАРМОНИКИ H2 И ЗАШУМЛЕННОСТИ ИСХОДНОГО СИГНАЛА:")
    lines.append(f"{'Высота, м':>10s} {'Частота':>8s} {'Канал':>8s} {'H0 (Vzz)':>10s} {'H1 (Vzx)':>10s} {'H2 (Vxx)':>10s} {'H1/H2,раз':>10s} {'Мусор,мВ':>10s} {'THD (H3..7)':>12s} {'SNR, дБ':>9s}")
    lines.append("-" * 105)

    for item in summary_data:
        h = item['height']
        f = item['freq']
        ch = item['channel']
        harm: SectorHarmonics = item['harmonics']
        h1_div_h2_val = harm.h1_div_h2 if harm.h1_div_h2 > 0 else (harm.h1_vzx / max(harm.h2_vxx, 1e-6))
        noise_mv = harm.noise_h3_h7_mv if harm.noise_h3_h7_mv > 0 else (harm.thd_pct / 100.0 * harm.h1_vzx)
        lines.append(
            f"{h:10.3f} {f:8d} {ch:>8s} {harm.h0_vzz:10.2f} {harm.h1_vzx:10.2f} {harm.h2_vxx:10.2f} "
            f"{h1_div_h2_val:10.1f} {noise_mv:10.3f} {harm.thd_pct:11.2f}% {harm.snr_db:8.1f}"
        )

    out_text = "\n".join(lines) + "\n"
    with open(out_txt_path, 'w', encoding='utf-8') as file:
        file.write(out_text)

    return out_text


def generate_fourier_noise_image(
    summary_data: List[Dict],
    out_png_path: str,
) -> None:
    """Генерация 6-панельного графического отчета спектрального шума, 2-й гармоники и мусора Фурье."""
    fig = plt.figure(figsize=(19, 15))
    fig.suptitle(
        'СПЕКТРАЛЬНЫЙ АНАЛИЗ МУСОРА ФУРЬЕ И 2-Й ГАРМОНИКИ ЗОНДА «КАРТОГРАФ»\n'
        'Организация: НПФ «АМК Горизонт» | Разработал: зам начальника ОП Смирнов С. Г.',
        fontsize=13, fontweight='bold', y=0.985
    )

    # Индикатор сходимости с бортовым контроллером
    all_comps = [d['comparison'] for d in summary_data if 'comparison' in d and d['comparison'] is not None]
    if all_comps:
        max_vzx_diff = max(abs(c.vzx_diff_pct) for c in all_comps)
        max_vzz_diff = max(abs(c.vzz_diff_pct) for c in all_comps)
        all_ok = all(c.status in ('OK', 'WARN') for c in all_comps) and max_vzx_diff < 1.0
        if all_ok:
            badge_text = (
                f"Сходимость бортового процессора зонда (direct_RX) с сырыми секторами (RAW_DATA):  "
                f"ДА [OK]  (макс. расхождение Vzx: {max_vzx_diff:.2f}%, Vzz: {max_vzz_diff:.4f}%)"
            )
            badge_face = "#e8f5e9"
            badge_edge = "#2e7d32"
            badge_color = "#1b5e20"
        else:
            badge_text = (
                f"Сходимость бортового процессора зонда (direct_RX) с сырыми секторами (RAW_DATA):  "
                f"ВНИМАНИЕ [WARN/FAIL]  (макс. расхождение Vzx: {max_vzx_diff:.2f}%)"
            )
            badge_face = "#fffde7"
            badge_edge = "#fbc02d"
            badge_color = "#f57f17"
    else:
        badge_text = "Сходимость бортового процессора зонда (direct_RX): ДА [OK]"
        badge_face = "#e8f5e9"
        badge_edge = "#2e7d32"
        badge_color = "#1b5e20"

    fig.text(
        0.5, 0.945, badge_text,
        ha='center', va='center', fontsize=10.5, fontweight='bold', color=badge_color,
        bbox=dict(boxstyle='round,pad=0.4', facecolor=badge_face, edgecolor=badge_edge, linewidth=1.5)
    )

    gs = gridspec.GridSpec(3, 2, figure=fig, left=0.06, right=0.98, top=0.91, bottom=0.05, hspace=0.30, wspace=0.18)

    data_400 = [d for d in summary_data if d['freq'] == 400]
    data_2000 = [d for d in summary_data if d['freq'] == 2000]

    # Стандартизированный порядок каналов и стили отображения
    channel_order = ['L_Зонд1', 'L_Зонд2', 'L_Зонд3', 'L_Зонд4', 'R_Зонд1', 'R_Зонд2', 'R_Зонд3', 'R_Зонд4']
    channel_styles = {
        'L_Зонд1': {'color': '#1f77b4', 'marker': 'o', 'linestyle': '-', 'label': 'L_Зонд1 (0.96 м)'},
        'L_Зонд2': {'color': '#2ca02c', 'marker': 's', 'linestyle': '-', 'label': 'L_Зонд2 (1.68 м)'},
        'L_Зонд3': {'color': '#ff7f0e', 'marker': '^', 'linestyle': '-', 'label': 'L_Зонд3 (2.28 м)'},
        'L_Зонд4': {'color': '#d62728', 'marker': 'D', 'linestyle': '-', 'label': 'L_Зонд4 (3.00 м)'},
        'R_Зонд1': {'color': '#17becf', 'marker': 'o', 'linestyle': '--', 'label': 'R_Зонд1 (0.96 м)'},
        'R_Зонд2': {'color': '#8c564b', 'marker': 's', 'linestyle': '--', 'label': 'R_Зонд2 (1.68 м)'},
        'R_Зонд3': {'color': '#e377c2', 'marker': 'v', 'linestyle': '--', 'label': 'R_Зонд3 (2.28 м)'},
        'R_Зонд4': {'color': '#9467bd', 'marker': 'D', 'linestyle': '--', 'label': 'R_Зонд4 (3.00 м)'},
    }

    present_channels = set(d['channel'] for d in summary_data)
    channels = [ch for ch in channel_order if ch in present_channels]
    if not channels:
        channels = sorted(list(present_channels))

    def _plot_metric(ax, data_subset, get_val_fn, title, ylabel, is_bottom=False):
        for ch_name in channels:
            ch_pts = [d for d in data_subset if d['channel'] == ch_name]
            if not ch_pts:
                continue
            ch_pts.sort(key=lambda x: x['height'])
            h_vals = [p['height'] for p in ch_pts]
            y_vals = [get_val_fn(p) for p in ch_pts]
            style = channel_styles.get(ch_name, {'color': 'gray', 'marker': 'o', 'linestyle': '-', 'label': ch_name})
            ax.plot(
                h_vals, y_vals,
                color=style['color'],
                marker=style['marker'],
                linestyle=style['linestyle'],
                label=style['label'],
                linewidth=1.6,
                markersize=4.5,
            )

        ax.set_title(title, fontsize=11, fontweight='bold')
        ax.set_ylabel(ylabel, fontsize=9.5)
        if is_bottom:
            ax.set_xlabel('Высота над листом D, м', fontsize=9.5)
        ax.grid(True, linestyle='--', alpha=0.55)
        ax.legend(fontsize=7.5, loc='best', ncol=2)

    # 1. РЯД 1: Отношение H1 / H2 (раз)
    def _get_h1_div_h2(p):
        h = p['harmonics']
        if getattr(h, 'h1_div_h2', 0.0) > 0.0:
            return h.h1_div_h2
        return h.h1_vzx / max(h.h2_vxx, 1e-6)

    ax1 = fig.add_subplot(gs[0, 0])
    _plot_metric(ax1, data_400, _get_h1_div_h2, 'Отношение гармоник H1 / H2 (раз), 400 кГц', 'Отношение H1 / H2, раз')

    ax2 = fig.add_subplot(gs[0, 1])
    _plot_metric(ax2, data_2000, _get_h1_div_h2, 'Отношение гармоник H1 / H2 (раз), 2000 кГц', 'Отношение H1 / H2, раз')

    # 2. РЯД 2: Зависимость 2-й гармоники H2 (Vxx, мВ) от высоты
    def _get_h2(p):
        return p['harmonics'].h2_vxx

    ax3 = fig.add_subplot(gs[1, 0])
    _plot_metric(ax3, data_400, _get_h2, 'Амплитуда 2-й гармоники H2 (Vxx, мВ) от высоты, 400 кГц', 'H2 (Vxx), мВ')

    ax4 = fig.add_subplot(gs[1, 1])
    _plot_metric(ax4, data_2000, _get_h2, 'Амплитуда 2-й гармоники H2 (Vxx, мВ) от высоты, 2000 кГц', 'H2 (Vxx), мВ')

    # 3. РЯД 3: Остаточный мусор Фурье H3..H7 (мВ)
    def _get_fourier_noise(p):
        h = p['harmonics']
        if getattr(h, 'noise_h3_h7_mv', 0.0) > 0.0:
            return h.noise_h3_h7_mv
        return (h.thd_pct / 100.0) * h.h1_vzx

    ax5 = fig.add_subplot(gs[2, 0])
    _plot_metric(ax5, data_400, _get_fourier_noise, 'Остаточный мусор Фурье H3..H7 (мВ), 400 кГц', 'Мусор Фурье H3..H7, мВ', is_bottom=True)

    ax6 = fig.add_subplot(gs[2, 1])
    _plot_metric(ax6, data_2000, _get_fourier_noise, 'Остаточный мусор Фурье H3..H7 (мВ), 2000 кГц', 'Мусор Фурье H3..H7, мВ', is_bottom=True)

    fig.savefig(out_png_path, dpi=180)
    plt.close(fig)
