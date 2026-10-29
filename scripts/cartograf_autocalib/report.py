"""
Модуль формирования протокола калибровки и графиков верификации зонда «Картограф».

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

import os
from typing import List, Optional
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

from .calibration_core import FullCalResult, MIRROR_PAIR_INDICES
from .verifier import FreqVerificationReport
from .segmentation import HeightSegment


REPORT_HEADER = """=======================================================
  ОТЧЕТ ПО АВТОМАТИЧЕСКОЙ КАЛИБРОВКЕ ЗОНДА «КАРТОГРАФ»
=======================================================
Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

# Цвета баз, соответствующие принятым в программе:
# 0.96 м: зелёный, 1.68 м: синий, 2.28 м: шоколадный/коричневый, 3.00 м: красный
BASE_COLORS = {
    0.96: "#2E7D32",   # Зеленый
    1.68: "#1565C0",   # Синий
    2.28: "#8D6E63",   # Шоколадный
    3.00: "#C62828",   # Красный
}


def generate_text_report(
    full_cal: FullCalResult,
    v400: FreqVerificationReport,
    v2000: FreqVerificationReport,
    segments: List[HeightSegment],
    input_source_desc: str,
    out_txt_path: Optional[str] = None,
) -> str:
    lines = [REPORT_HEADER]
    lines.append(f"Источник данных: {input_source_desc}")
    heights_str = ", ".join(f"{s.height_m:.3f}" for s in segments)
    lines.append(f"Количество высот: {len(segments)} ({heights_str} м)\n")

    lines.append("Статистика выборки по высотам:")
    lines.append(f"{'Высота, м':>10s} {'Кадры (всего)':>14s} {'Кадры (в расчёте)':>18s} {'Доля,%':>8s}")
    lines.append("-" * 54)
    for s in segments:
        pct = (s.n_used_frames / s.n_total_frames * 100.0) if s.n_total_frames > 0 else 0.0
        lines.append(f"{s.height_m:10.3f} {s.n_total_frames:14d} {s.n_used_frames:18d} {pct:8.1f}%")

    for freq_idx, (f_cal, f_ver) in enumerate([(full_cal.f400, v400), (full_cal.f2000, v2000)]):
        freq_khz = f_cal.freq_khz
        lines.append("\n" + "=" * 55)
        lines.append(f"--- Результаты калибровки и верификации ({freq_khz} кГц) ---")
        lines.append("=" * 55)

        lines.append("\n1. Калибровочные коэффициенты каналов:")
        lines.append(f"{'ch':>3s} {'Канал':>12s} {'L, м':>6s} {'beta, град':>11s} {'c_j (C/Vzz)':>14s} {'C_j (абс.)':>12s} {'d_ph, град':>11s}")
        lines.append("-" * 75)
        for ch in f_cal.channels:
            lines.append(
                f"{ch.ch_idx:>3d} {ch.name:>12s} {ch.l_m:>6.2f} {ch.beta_deg:>11.2f} "
                f"{ch.c_ratio:>14.4e} {ch.C:>12.4e} {ch.mean_dph_deg:>11.2f}"
            )

        lines.append("\n2. Симметрия зеркальных пар (разность углов наклона Delta beta):")
        lines.append(f"{'База L, м':>10s} {'Пара каналов':>22s} {'beta_L, град':>13s} {'beta_R, град':>13s} {'|Delta beta|':>12s} {'Статус':>8s}")
        lines.append("-" * 82)
        for j1, j2, L_pair in MIRROR_PAIR_INDICES:
            ch1 = f_cal.channels[j1]
            ch2 = f_cal.channels[j2]
            diff = abs(ch1.beta_deg - ch2.beta_deg)
            st = "OK" if diff <= 1.2 else ("WARN" if diff <= 2.0 else "FAIL")
            lines.append(
                f"{L_pair:10.2f} {ch1.name + ' - ' + ch2.name:>22s} {ch1.beta_deg:13.2f} {ch2.beta_deg:13.2f} "
                f"{diff:12.2f}° {st:>8s}"
            )

        lines.append("\n3. Сходимость геосигнала G = (1 - R) / (1 + R) с аналитической моделью:")
        lines.append(
            f"{'ch':>3s} {'Пара':>8s} {'h, м':>7s} {'h/L':>6s} {'G theor':>10s} {'G meas':>10s} "
            f"{'Невязка':>10s} {'Отн.ош,%':>9s} {'Статус':>7s}"
        )
        lines.append("-" * 75)
        for ch_ver in f_ver.channels:
            for pt in ch_ver.points:
                lines.append(
                    f"{ch_ver.ch_idx:>3d} {ch_ver.name:>8s} {pt.h_m:7.3f} {pt.h_div_l:6.2f} "
                    f"{pt.g_theor:10.4f} {pt.g_meas:10.4f} {pt.abs_err:10.4f} {pt.rel_err_pct:9.2f}% "
                    f"{ch_ver.status:>7s}"
                )

        lines.append(f"\nИтоги верификации {freq_khz} кГц:")
        lines.append(f"  Средняя погрешность геосигнала: {f_ver.total_mean_rel_err_pct:.2f}%")
        lines.append(f"  Максимальная погрешность:       {f_ver.total_max_rel_err_pct:.2f}%")
        lines.append(f"  Общий статус частоты:           {f_ver.overall_status}")

    report_text = "\n".join(lines) + "\n"

    if out_txt_path:
        out_dir = os.path.dirname(out_txt_path)
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
        with open(out_txt_path, 'w', encoding='utf-8') as f:
            f.write(report_text)

    return report_text


def generate_geosignal_table_txt(
    v400: FreqVerificationReport,
    v2000: FreqVerificationReport,
    out_txt_path: str,
) -> str:
    """
    Генерация отдельного удобного текстового файла с компактной таблицей невязок геосигнала (%).
    """
    lines = [
        "==========================================================================================",
        "  ТАБЛИЦА НЕВЯЗОК ГЕОСИГНАЛА ЗОНДА «КАРТОГРАФ» (ОТНОСИТЕЛЬНАЯ ОШИБКА, %)",
        "==========================================================================================",
        "Организация: НПФ «АМК Горизонт» | Разработал: зам начальника ОП Смирнов С. Г.\n",
    ]

    for v_rep in [v400, v2000]:
        lines.append(f"--- ЧАСТОТА {v_rep.freq_khz} кГц (Средняя невязка: {v_rep.total_mean_rel_err_pct:.2f}%, Макс: {v_rep.total_max_rel_err_pct:.2f}%) ---")
        
        # Получаем список высот
        heights = [p.h_m for p in v_rep.channels[0].points]
        
        header = f"{'h, м':>7s} | " + " | ".join(f"{ch.name:>7s}" for ch in v_rep.channels)
        lines.append(header)
        lines.append("-" * len(header))

        for h_idx, h_val in enumerate(heights):
            row_vals = [f"{h_val:7.3f}"]
            for ch in v_rep.channels:
                err_pct = ch.points[h_idx].rel_err_pct
                row_vals.append(f"{err_pct:+6.2f}%")
            lines.append(" | ".join(row_vals))

        lines.append("\nСводка по зеркальным парам:")
        lines.append(f"{'База L, м':>10s} {'Пара каналов':>22s} {'Mean ошибка Left':>18s} {'Mean ошибка Right':>18s} {'Симметрия |dL|':>15s}")
        lines.append("-" * 88)
        for j1, j2, L_pair in MIRROR_PAIR_INDICES:
            ch1 = v_rep.channels[j1]
            ch2 = v_rep.channels[j2]
            diff = abs(ch1.beta_deg - ch2.beta_deg)
            lines.append(
                f"{L_pair:10.2f} {ch1.name + ' - ' + ch2.name:>22s} "
                f"{ch1.mean_rel_err_pct:17.2f}% {ch2.mean_rel_err_pct:17.2f}% {diff:14.2f}°"
            )
        lines.append("\n" + "=" * 90 + "\n")

    res_text = "\n".join(lines)
    out_dir = os.path.dirname(out_txt_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    with open(out_txt_path, 'w', encoding='utf-8') as f:
        f.write(res_text)

    return res_text


def generate_geosignal_verification_image(
    v400: FreqVerificationReport,
    v2000: FreqVerificationReport,
    out_png_path: str,
) -> None:
    """
    Генерация единого графического отчета, разделенного по частотам и приемникам (Left / Right):
      1. 400 кГц,  приёмник Left  (Rzx1) — график G(h) + таблица невязок %
      2. 400 кГц,  приёмник Right (Rzx2) — график G(h) + таблица невязок %
      3. 2000 кГц, приёмник Left  (Rzx1) — график G(h) + таблица невязок %
      4. 2000 кГц, приёмник Right (Rzx2) — график G(h) + таблица невязок %
    """
    fig = plt.figure(figsize=(19, 16))
    fig.suptitle(
        'КОНТРОЛЬ КАЧЕСТВА ГЕОСИГНАЛА ЗОНДА «КАРТОГРАФ» (G_изм vs G_теор и невязки по приёмникам)\n'
        'Организация: НПФ «АМК Горизонт» | Разработал: зам начальника ОП Смирнов С. Г.',
        fontsize=13, fontweight='bold', y=0.985
    )

    # 4 строки, 2 колонки: слева график, справа таблица
    gs = gridspec.GridSpec(4, 2, figure=fig, width_ratios=[1.15, 1.0], left=0.06, right=0.98, top=0.925, bottom=0.04, hspace=0.35, wspace=0.14)

    tasks = [
        (v400, 'L', '400 кГц — Приёмник Left (Rzx1)', 0),
        (v400, 'R', '400 кГц — Приёмник Right (Rzx2)', 1),
        (v2000, 'L', '2000 кГц — Приёмник Left (Rzx1)', 2),
        (v2000, 'R', '2000 кГц — Приёмник Right (Rzx2)', 3),
    ]

    for v_rep, rx_target, title_text, row_idx in tasks:
        ax_plot = fig.add_subplot(gs[row_idx, 0])
        ax_table = fig.add_subplot(gs[row_idx, 1])
        ax_table.axis('off')

        target_channels = [ch for ch in v_rep.channels if ch.rx_letter == rx_target]
        heights = [p.h_m for p in target_channels[0].points]

        # Отрисовка графиков для 4 передатчиков выбранного приемника
        for ch in target_channels:
            color = BASE_COLORS.get(ch.l_m, '#333333')
            marker = 'o' if rx_target == 'L' else 's'
            lbl = f"{ch.name} (L={ch.l_m:.2f}м, beta={ch.beta_deg:.1f}°)"

            g_meas = [p.g_meas for p in ch.points]
            g_theor = [p.g_theor for p in ch.points]

            # Измеренные точки
            ax_plot.plot(heights, g_meas, marker=marker, linestyle='-', color=color,
                         label=lbl, linewidth=1.8, markersize=5, alpha=0.9)
            # Теоретическая кривая пунктиром
            ax_plot.plot(heights, g_theor, linestyle=':', color=color, linewidth=1.2, alpha=0.55)

        ax_plot.set_title(f"Геосигнал G(h) над проводящим листом — {title_text}", fontsize=10, fontweight='bold')
        ax_plot.set_xlabel('Высота над листом D, м', fontsize=9)
        ax_plot.set_ylabel('Геосигнал G', fontsize=9)
        ax_plot.grid(True, linestyle='--', alpha=0.55)
        ax_plot.legend(fontsize=8, loc='best', ncol=2)

        # Формирование таблицы справа для 4 каналов
        col_labels = ['Высота, м'] + [f"{ch.name} ({ch.l_m:.2f}м)" for ch in target_channels]
        table_data = []
        cell_colors = []

        for h_idx, h_val in enumerate(heights):
            row_text = [f"{h_val:.3f}"]
            row_colors = ['#F5F5F5']

            for ch in target_channels:
                err_pct = ch.points[h_idx].rel_err_pct
                row_text.append(f"{err_pct:+5.2f}%")

                abs_err = abs(err_pct)
                if abs_err <= 3.5:
                    c = '#C8E6C9'  # Зеленый (допуск)
                elif abs_err <= 6.0:
                    c = '#FFF59D'  # Желтый (предупреждение)
                else:
                    c = '#FFCDD2'  # Красный (превышение)
                row_colors.append(c)

            table_data.append(row_text)
            cell_colors.append(row_colors)

        # Строка средней ошибки
        mean_row = ['Средняя']
        mean_colors = ['#E0E0E0']
        for ch in target_channels:
            mean_err = ch.mean_rel_err_pct
            mean_row.append(f"{mean_err:5.2f}%")
            c = '#C8E6C9' if mean_err <= 3.5 else ('#FFF59D' if mean_err <= 6.0 else '#FFCDD2')
            mean_colors.append(c)
        table_data.append(mean_row)
        cell_colors.append(mean_colors)

        t = ax_table.table(
            cellText=table_data,
            cellColours=cell_colors,
            colLabels=col_labels,
            colColours=['#BBDEFB'] * len(col_labels),
            loc='center',
            cellLoc='center',
        )
        t.auto_set_font_size(False)
        t.set_fontsize(8.5)
        t.scale(1.0, 1.35)

        ax_table.set_title(
            f"Невязки (G_теор - G_изм) / G_изм * 100% — {title_text}\n"
            f"(Зелёный: допуск ≤3.5%, Жёлтый: ≤6.0%, Красный: >6.0%)",
            fontsize=9.5, fontweight='bold', pad=8
        )

    out_dir = os.path.dirname(out_png_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)
    fig.savefig(out_png_path, dpi=180)
    plt.close(fig)


def generate_calibration_plots(
    v400: FreqVerificationReport,
    v2000: FreqVerificationReport,
    out_png_path: str,
) -> None:
    """Совместимость со старым вызовом: перенаправляет на единый отчет с графиком и таблицей."""
    generate_geosignal_verification_image(v400, v2000, out_png_path)
