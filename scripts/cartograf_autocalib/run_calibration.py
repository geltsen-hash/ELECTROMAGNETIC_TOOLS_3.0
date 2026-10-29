"""
Главная точка входа для консольного запуска автоматической калибровки зонда «Картограф».

Использование:
  1. По единому склеенному файлу:
     python -m cartograf_autocalib.run_calibration --united-file "C:/EXP/rot_005.bin" --heights 0.44,0.68,0.82,1.115,1.33,1.53,1.83

  2. По папке с отдельными файлами высот:
     python -m cartograf_autocalib.run_calibration --folder "C:/EXP/Cart_005_2025_ROT"

  3. По готовому CSV-файлу:
     python -m cartograf_autocalib.run_calibration --csv "C:/EXP/Cart_005_2025_ROT/B_z.csv"

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

import os
import sys
import glob
import math
import argparse
from typing import List, Optional
import numpy as np
import pandas as pd

from .bin_parser import read_cartograf_bin
from .segmentation import (
    process_frames_interval,
    segment_united_file,
    segments_to_dataframe,
    extract_height_from_filename,
    HeightSegment,
)
from .calibration_core import run_full_calibration
from .verifier import verify_full_calibration
from .metro_writer import save_metrology_bin
from .report import (
    generate_text_report,
    generate_calibration_plots,
    generate_geosignal_table_txt,
    generate_geosignal_verification_image,
)
from .fourier_analysis import (
    generate_hardware_and_noise_report,
    generate_fourier_noise_image,
    SectorHarmonics,
    ProcessorComparison,
)


def run_pipeline(
    folder_path: Optional[str] = None,
    united_file_path: Optional[str] = None,
    file_list: Optional[List[str]] = None,
    heights_list: Optional[List[float]] = None,
    csv_path: Optional[str] = None,
    output_dir: Optional[str] = None,
    template_metro_path: Optional[str] = None,
    min_wg_rpm: float = 45.0,
    serial_number: Optional[int] = None,
) -> None:
    if not output_dir or output_dir in ('.', './calibration_output', './calib_output'):
        if folder_path and os.path.isdir(folder_path):
            output_dir = os.path.abspath(folder_path)
        elif united_file_path and os.path.exists(united_file_path):
            output_dir = os.path.abspath(os.path.dirname(united_file_path))
        elif file_list and len(file_list) > 0 and os.path.exists(file_list[0]):
            output_dir = os.path.abspath(os.path.dirname(file_list[0]))
        elif csv_path and os.path.exists(csv_path):
            output_dir = os.path.abspath(os.path.dirname(csv_path))
        else:
            output_dir = os.path.abspath(output_dir or '.')

    os.makedirs(output_dir, exist_ok=True)
    segments: List[HeightSegment] = []
    desc = ""

    # Сценарий 1: Прямой вход из CSV
    if csv_path and os.path.exists(csv_path):
        desc = f"CSV-файл: {csv_path}"
        print(f"Загрузка калибровочных данных из CSV: {csv_path}")
        df = pd.read_csv(csv_path)
        # Приведение имён колонок
        df.columns = [c.strip() for c in df.columns]

    # Сценарий 2: Единый склеенный файл
    elif united_file_path and os.path.exists(united_file_path):
        if not heights_list:
            raise ValueError("Для единого файла необходимо указать список высот через --heights")
        desc = f"Единый BIN-файл: {united_file_path}"
        print(f"Парсинг единого файла {united_file_path}...")
        records = read_cartograf_bin(united_file_path)
        print(f"Прочитано {len(records)} кадров. Выполняется автоматическая сегментация...")
        segments = segment_united_file(records, heights_list, min_wg_rpm=min_wg_rpm)
        df = segments_to_dataframe(segments)
        # Сохранение сгенерированного CSV
        auto_csv_path = os.path.join(output_dir, "B_z_auto.csv")
        df.to_csv(auto_csv_path, index=False)
        print(f"Автоматически сформированный калибровочный файл сохранён: {auto_csv_path}")

    # Сценарий 3: Папка с отдельными файлами высот
    elif folder_path and os.path.isdir(folder_path):
        desc = f"Папка: {folder_path}"
        all_bins = glob.glob(os.path.join(folder_path, "*.bin"))
        # Фильтруем файлы с указанием высоты
        file_height_pairs = []
        for b in all_bins:
            fname = os.path.basename(b)
            if "unated" in fname.lower() or "metro" in fname.lower():
                continue
            h = extract_height_from_filename(fname)
            if h is not None:
                file_height_pairs.append((h, b))

        if not file_height_pairs:
            raise ValueError(f"В папке {folder_path} не найдено файлов с распознаваемыми высотами (например *440_mm.bin)")

        file_height_pairs.sort(key=lambda x: x[0])
        print(f"Найдено {len(file_height_pairs)} файлов высот:")
        for h, b in file_height_pairs:
            print(f"  {h:.3f} м -> {os.path.basename(b)}")

        for h, b in file_height_pairs:
            records = read_cartograf_bin(b)
            seg = process_frames_interval(records, h, min_wg_rpm=min_wg_rpm)
            segments.append(seg)

        df = segments_to_dataframe(segments)
        auto_csv_path = os.path.join(output_dir, "B_z_auto.csv")
        df.to_csv(auto_csv_path, index=False)
        print(f"Сформирован калибровочный профиль: {auto_csv_path}")

    # Сценарий 4: Явный список файлов
    elif file_list:
        desc = f"Список файлов ({len(file_list)} шт.)"
        file_height_pairs = []
        for b in file_list:
            h = extract_height_from_filename(os.path.basename(b))
            if h is not None:
                file_height_pairs.append((h, b))
        file_height_pairs.sort(key=lambda x: x[0])
        for h, b in file_height_pairs:
            records = read_cartograf_bin(b)
            seg = process_frames_interval(records, h, min_wg_rpm=min_wg_rpm)
            segments.append(seg)
        df = segments_to_dataframe(segments)
        auto_csv_path = os.path.join(output_dir, "B_z_auto.csv")
        df.to_csv(auto_csv_path, index=False)
    else:
        raise ValueError("Не указан ни один источник данных (--folder, --united-file, --files или --csv)")

    print("\n--- Выполняется двухшаговый МНК-расчёт калибровки ---")
    full_cal = run_full_calibration(df)
    print("Калибровка успешно рассчитана.")

    print("\n--- Выполняется автоматическая сквозная верификация геосигнала ---")
    v400, v2000 = verify_full_calibration(df, full_cal)
    print(f"Статус 400 кГц:  {v400.overall_status} (средняя невязка {v400.total_mean_rel_err_pct:.2f}%)")
    print(f"Статус 2000 кГц: {v2000.overall_status} (средняя невязка {v2000.total_mean_rel_err_pct:.2f}%)")

    # Генерация текстовой таблицы невязок
    geo_table_path = os.path.join(output_dir, "geosignal_table.txt")
    generate_geosignal_table_txt(v400, v2000, geo_table_path)
    print(f"Таблица невязок сохранена: {geo_table_path}")

    # Генерация единого графического отчета (график G(h) + цветная таблица)
    geo_img_path = os.path.join(output_dir, "geosignal_verification.png")
    generate_geosignal_verification_image(v400, v2000, geo_img_path)
    print(f"Единый график с таблицей сохранён: {geo_img_path}")

    # Сохранение бинарного файла метрологии
    metro_bin_path = os.path.join(output_dir, "metrology_cartograf_calibrated.bin")
    save_metrology_bin(
        full_cal,
        metro_bin_path,
        template_bin_path=template_metro_path,
        serial_number=serial_number,
    )
    print(f"\nФайл метрологии сохранён: {metro_bin_path} ({os.path.getsize(metro_bin_path)} байт)")

    # Анализ шумов, второй гармоники и сверка процессора
    noise_summary = []
    if segments:
        for seg in segments:
            for ch in seg.channels:
                ch_name = f"{ch.rx}_Зонд{ch.tx}"
                h1_val = ch.raw_vzx
                h2_val = ch.raw_vxx_h2
                h1_div_h2 = (h1_val / h2_val) if h2_val > 1e-6 else (h1_val / 1e-6)
                noise_h3_h7 = (ch.thd_pct / 100.0) * h1_val
                harm_obj = SectorHarmonics(
                    h0_vzz=ch.raw_vzz,
                    h1_vzx=h1_val,
                    h2_vxx=h2_val,
                    dph_rad=ch.raw_dph_rad,
                    dph_deg=math.degrees(ch.raw_dph_rad),
                    higher_harmonics=np.zeros(5, dtype=np.float32),
                    residual_noise_dv=ch.residual_noise_dv,
                    thd_pct=ch.thd_pct,
                    h2_div_h1_pct=ch.h2_div_h1_pct,
                    snr_db=ch.snr_db,
                    h1_div_h2=h1_div_h2,
                    noise_h3_h7_mv=noise_h3_h7,
                )
                comp_obj = ProcessorComparison(
                    vzz_direct=ch.vzz_mean,
                    vzz_calc=ch.raw_vzz,
                    vzz_diff_pct=ch.processor_vzz_diff_pct,
                    vzx_direct=ch.vzx_mean,
                    vzx_calc=ch.raw_vzx,
                    vzx_diff_pct=ch.processor_vzx_diff_pct,
                    dph_direct_deg=math.degrees(ch.dph_rad_mean),
                    dph_calc_deg=math.degrees(ch.raw_dph_rad),
                    dph_diff_deg=ch.processor_dph_diff_deg,
                    status=ch.processor_status,
                )
                noise_summary.append({
                    'height': seg.height_m,
                    'freq': ch.freq,
                    'channel': ch_name,
                    'harmonics': harm_obj,
                    'comparison': comp_obj,
                })
    elif 'amp_V_xx' in df.columns and 'residual_noise_dv' in df.columns:
        for _, row in df.iterrows():
            ch_name = f"{row['Rx']}_Зонд{int(row['Tx'])}"
            h1_val = float(row.get('raw_vzx', row['amp_V_zx']))
            h2_val = float(row['amp_V_xx'])
            thd_val = float(row.get('thd_pct', 0.0))
            h1_div_h2 = (h1_val / h2_val) if h2_val > 1e-6 else (h1_val / 1e-6)
            noise_h3_h7 = float(row.get('noise_h3_h7_mv', (thd_val / 100.0) * h1_val))
            harm_obj = SectorHarmonics(
                h0_vzz=float(row.get('raw_vzz', row['amp_V_zz'])),
                h1_vzx=h1_val,
                h2_vxx=h2_val,
                dph_rad=float(row.get('d_ph_Vzx_Vzz', 0.0)),
                dph_deg=math.degrees(float(row.get('d_ph_Vzx_Vzz', 0.0))),
                higher_harmonics=np.zeros(5, dtype=np.float32),
                residual_noise_dv=float(row.get('residual_noise_dv', 0.0)),
                thd_pct=thd_val,
                h2_div_h1_pct=float(row.get('h2_div_h1_pct', (h2_val / h1_val * 100.0) if h1_val > 1e-6 else 0.0)),
                snr_db=float(row.get('snr_db', 0.0)),
                h1_div_h2=h1_div_h2,
                noise_h3_h7_mv=noise_h3_h7,
            )
            comp_obj = ProcessorComparison(
                vzz_direct=float(row['amp_V_zz']),
                vzz_calc=float(row.get('raw_vzz', row['amp_V_zz'])),
                vzz_diff_pct=float(row.get('proc_vzz_diff_pct', 0.0)),
                vzx_direct=float(row['amp_V_zx']),
                vzx_calc=h1_val,
                vzx_diff_pct=float(row.get('proc_vzx_diff_pct', 0.0)),
                dph_direct_deg=float(row.get('dph_std_deg', 0.0)),
                dph_calc_deg=0.0,
                dph_diff_deg=0.0,
                status=str(row.get('proc_status', 'OK')),
            )
            noise_summary.append({
                'height': float(row['D_m']),
                'freq': int(row['freq']),
                'channel': ch_name,
                'harmonics': harm_obj,
                'comparison': comp_obj,
            })

    if noise_summary:
        hw_report_path = os.path.join(output_dir, "hardware_and_noise_report.txt")
        generate_hardware_and_noise_report(noise_summary, hw_report_path)
        print(f"Протокол сверки процессора и анализа шумов сохранён: {hw_report_path}")

        noise_img_path = os.path.join(output_dir, "fourier_noise_analysis.png")
        generate_fourier_noise_image(noise_summary, noise_img_path)
        print(f"Графики мусора Фурье и 2-й гармоники сохранены: {noise_img_path}")

    # Генерация полного текстового протокола
    report_path = os.path.join(output_dir, "calibration_report.txt")
    generate_text_report(
        full_cal=full_cal,
        v400=v400,
        v2000=v2000,
        segments=segments,
        input_source_desc=desc,
        out_txt_path=report_path,
    )
    print(f"Полный метрологический протокол сохранён: {report_path}")

    print("\n=======================================================")
    print("  КАЛИБРОВКА И ВЕРИФИКАЦИЯ УСПЕШНО ЗАВЕРШЕНЫ")
    print("=======================================================")


def main():
    parser = argparse.ArgumentParser(
        description="Автоматическая калибровка и верификация зонда «Картограф». НПФ «АМК Горизонт»."
    )
    parser.add_argument("--folder", type=str, help="Папка с отдельными файлами высот (*_mm.bin)")
    parser.add_argument("--united-file", type=str, help="Единый склеенный BIN-файл всех высот")
    parser.add_argument("--files", nargs="+", help="Список BIN-файлов высот")
    parser.add_argument("--heights", type=str, help="Список высот через запятую в метрах, например: 0.44,0.68,0.82,1.115,1.33,1.53,1.83")
    parser.add_argument("--csv", type=str, help="Путь к готовому файлу калибровочных данных B_z.csv")
    parser.add_argument("--out-dir", type=str, default=None, help="Папка для сохранения результатов (по умолчанию: папка источника данных)")
    parser.add_argument("--template-metro", type=str, help="Шаблон файла метрологии .bin для сохранения общих параметров")
    parser.add_argument("--min-wg", type=float, default=45.0, help="Минимальная скорость вращения для отбора кадров (об/мин)")
    parser.add_argument("--serial", type=int, help="Заводской номер прибора")

    args = parser.parse_args()

    heights_list = None
    if args.heights:
        heights_list = [float(h.strip()) for h in args.heights.split(",") if h.strip()]

    run_pipeline(
        folder_path=args.folder,
        united_file_path=args.united_file,
        file_list=args.files,
        heights_list=heights_list,
        csv_path=args.csv,
        output_dir=args.out_dir,
        template_metro_path=args.template_metro,
        min_wg_rpm=args.min_wg,
        serial_number=args.serial,
    )


if __name__ == "__main__":
    main()
