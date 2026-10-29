"""
Графический интерфейс (GUI) мастера калибровки АМК-120 «Картограф».
Позволяет в один клик выполнить полный цикл: парсинг, фильтрацию, МНК-расчет,
сквозную верификацию геосигнала, запись файла метрологии и генерацию протоколов.

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

import os
import sys
import threading
import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
from typing import Optional, List

from .run_calibration import run_pipeline


class CalibrationGui(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Мастер калибровки АМК-120 «Картограф»")
        self.geometry("920x760")
        self.minsize(820, 680)

        # Переменные формы
        self.mode_var = tk.StringVar(value="folder")
        init_src = ""
        for cand in ["C:/EXP/Cart_005_2025_ROT_13_okt", "C:/EXP/Cart_005_2025_ROT", "C:/EXP/Cart_005_2025_ROT_5_okt"]:
            if os.path.exists(cand):
                init_src = cand
                break
        self.source_path_var = tk.StringVar(value=init_src)
        self.heights_var = tk.StringVar(value="0.440, 0.680, 0.820, 1.115, 1.330, 1.530, 1.830")
        self.min_wg_var = tk.StringVar(value="45.0")
        self.serial_var = tk.StringVar(value="5")
        self.out_dir_var = tk.StringVar(value=init_src)
        self.template_metro_var = tk.StringVar(value="")

        self.source_path_var.trace_add("write", self._on_source_path_changed)

        # Пути к готовым артефактам
        self.last_geo_img_path = ""
        self.last_geo_txt_path = ""
        self.last_noise_img_path = ""
        self.last_hw_report_path = ""
        self.last_report_path = ""
        self.last_plots_path = ""
        self.last_metro_bin_path = ""

        self._build_ui()

    def _build_ui(self):
        # Верхний заголовок
        header_frame = tk.Frame(self, bg="#1E395B", padx=15, pady=10)
        header_frame.pack(fill=tk.X)

        title_lbl = tk.Label(
            header_frame,
            text="МАСТЕР КАЛИБРОВКИ АМК-120 «КАРТОГРАФ»",
            font=("Arial", 13, "bold"),
            fg="white",
            bg="#1E395B",
        )
        title_lbl.pack(anchor=tk.W)

        subtitle_lbl = tk.Label(
            header_frame,
            text="Организация: НПФ «АМК Горизонт» | Разработал: зам начальника ОП Смирнов С. Г.",
            font=("Arial", 9),
            fg="#D0E0FF",
            bg="#1E395B",
        )
        subtitle_lbl.pack(anchor=tk.W)

        main_frame = ttk.Frame(self, padding=12)
        main_frame.pack(fill=tk.BOTH, expand=True)

        # 1. Группа источника данных
        src_group = ttk.LabelFrame(main_frame, text=" 1. Источник данных для калибровки ", padding=8)
        src_group.pack(fill=tk.X, pady=(0, 6))

        mode_row = ttk.Frame(src_group)
        mode_row.pack(fill=tk.X, pady=(0, 4))

        ttk.Radiobutton(
            mode_row, text="Папка с файлами высот (*_mm.bin)",
            variable=self.mode_var, value="folder", command=self._on_mode_changed
        ).pack(side=tk.LEFT, padx=(0, 15))

        ttk.Radiobutton(
            mode_row, text="Единый склеенный BIN-файл",
            variable=self.mode_var, value="united", command=self._on_mode_changed
        ).pack(side=tk.LEFT, padx=(0, 15))

        ttk.Radiobutton(
            mode_row, text="Готовый CSV-файл (B_z.csv)",
            variable=self.mode_var, value="csv", command=self._on_mode_changed
        ).pack(side=tk.LEFT)

        path_row = ttk.Frame(src_group)
        path_row.pack(fill=tk.X)

        self.path_entry = ttk.Entry(path_row, textvariable=self.source_path_var, font=("Consolas", 9))
        self.path_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 6))

        self.browse_btn = ttk.Button(path_row, text="Обзор...", command=self._on_browse_source)
        self.browse_btn.pack(side=tk.RIGHT)

        # 2. Группа параметров
        param_group = ttk.LabelFrame(main_frame, text=" 2. Параметры стенда и фильтрации ", padding=8)
        param_group.pack(fill=tk.X, pady=(0, 6))

        # Высоты
        h_row = ttk.Frame(param_group)
        h_row.pack(fill=tk.X, pady=(0, 4))
        ttk.Label(h_row, text="Высоты над листом (м):", width=24).pack(side=tk.LEFT)
        ttk.Entry(h_row, textvariable=self.heights_var, font=("Consolas", 9)).pack(side=tk.LEFT, fill=tk.X, expand=True)

        # Wg и номер прибора
        sub_row = ttk.Frame(param_group)
        sub_row.pack(fill=tk.X, pady=(0, 4))
        ttk.Label(sub_row, text="Мин. скорость Wg (об/мин):", width=24).pack(side=tk.LEFT)
        ttk.Entry(sub_row, textvariable=self.min_wg_var, width=10).pack(side=tk.LEFT, padx=(0, 20))

        ttk.Label(sub_row, text="Заводской № зонда:").pack(side=tk.LEFT, padx=(0, 6))
        ttk.Entry(sub_row, textvariable=self.serial_var, width=12).pack(side=tk.LEFT)

        # Папка вывода
        out_row = ttk.Frame(param_group)
        out_row.pack(fill=tk.X, pady=(0, 4))
        ttk.Label(out_row, text="Папка сохранения:", width=24).pack(side=tk.LEFT)
        ttk.Entry(out_row, textvariable=self.out_dir_var, font=("Consolas", 9)).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 6))
        ttk.Button(out_row, text="Обзор...", command=self._on_browse_out_dir).pack(side=tk.RIGHT)

        # Шаблон метрологии
        tmpl_row = ttk.Frame(param_group)
        tmpl_row.pack(fill=tk.X)
        ttk.Label(tmpl_row, text="Шаблон метрологии (.bin):", width=24).pack(side=tk.LEFT)
        ttk.Entry(tmpl_row, textvariable=self.template_metro_var, font=("Consolas", 9)).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 6))
        ttk.Button(tmpl_row, text="Выбрать...", command=self._on_browse_template).pack(side=tk.RIGHT)

        # 3. Кнопка запуска (ВСЕГДА НА ВИДУ В ВЕРХНЕЙ ЧАСТИ)
        btn_frame = ttk.Frame(main_frame)
        btn_frame.pack(fill=tk.X, pady=(6, 4))

        self.run_btn = tk.Button(
            btn_frame,
            text="▶   ВЫПОЛНИТЬ АВТОКАЛИБРОВКУ И ВЕРИФИКАЦИЮ",
            font=("Arial", 11, "bold"),
            bg="#2E7D32",
            fg="white",
            activebackground="#1B5E20",
            activeforeground="white",
            padx=15,
            pady=8,
            cursor="hand2",
            command=self._start_calibration,
        )
        self.run_btn.pack(fill=tk.X)

        # Индикатор статуса
        self.status_lbl = tk.Label(
            main_frame,
            text="Готов к работе. Выберите источник данных и нажмите кнопку расчёта.",
            font=("Arial", 9, "bold"),
            bg="#E0E0E0",
            fg="#333333",
            pady=5,
            relief=tk.RIDGE,
        )
        self.status_lbl.pack(fill=tk.X, pady=(0, 6))

        # 4. Панель действий с результатами (ВСЕГДА НА ВИДУ НАД ЛОГОМ)
        action_row = tk.Frame(main_frame, bg="#F0F4F8", padx=8, pady=6, relief=tk.GROOVE, bd=1)
        action_row.pack(fill=tk.X, pady=(0, 8))

        lbl_act = tk.Label(action_row, text="Открыть результаты:", font=("Arial", 9, "bold"), bg="#F0F4F8", fg="black")
        lbl_act.pack(side=tk.LEFT, padx=(0, 6))

        self.btn_open_geo_img = tk.Button(
            action_row,
            text="График и таблица геосигнала (PNG)",
            font=("Arial", 9, "bold"),
            bg="#E8E8E8",
            fg="black",
            activebackground="#D0D0D0",
            activeforeground="black",
            padx=8,
            pady=4,
            cursor="hand2",
            state=tk.DISABLED,
            command=self._open_geo_img,
        )
        self.btn_open_geo_img.pack(side=tk.LEFT, padx=(0, 5))

        self.btn_open_noise_img = tk.Button(
            action_row,
            text="Анализ шумов и H2 (PNG)",
            font=("Arial", 9, "bold"),
            bg="#E8E8E8",
            fg="black",
            activebackground="#D0D0D0",
            activeforeground="black",
            padx=8,
            pady=4,
            cursor="hand2",
            state=tk.DISABLED,
            command=self._open_noise_img,
        )
        self.btn_open_noise_img.pack(side=tk.LEFT, padx=(0, 5))

        self.btn_open_hw_report = tk.Button(
            action_row,
            text="Сверка процессора (TXT)",
            font=("Arial", 9, "bold"),
            bg="#E8E8E8",
            fg="black",
            activebackground="#D0D0D0",
            activeforeground="black",
            padx=8,
            pady=4,
            cursor="hand2",
            state=tk.DISABLED,
            command=self._open_hw_report,
        )
        self.btn_open_hw_report.pack(side=tk.LEFT, padx=(0, 5))

        self.btn_open_geo_txt = tk.Button(
            action_row,
            text="Таблица невязок (TXT)",
            font=("Arial", 9, "bold"),
            bg="#E8E8E8",
            fg="black",
            activebackground="#D0D0D0",
            activeforeground="black",
            padx=8,
            pady=4,
            cursor="hand2",
            state=tk.DISABLED,
            command=self._open_geo_txt,
        )
        self.btn_open_geo_txt.pack(side=tk.LEFT, padx=(0, 5))

        self.btn_open_report = tk.Button(
            action_row,
            text="Полный протокол (.txt)",
            font=("Arial", 9, "bold"),
            bg="#E8E8E8",
            fg="black",
            activebackground="#D0D0D0",
            activeforeground="black",
            padx=8,
            pady=4,
            cursor="hand2",
            state=tk.DISABLED,
            command=self._open_report,
        )
        self.btn_open_report.pack(side=tk.LEFT, padx=(0, 5))

        self.btn_open_dir = tk.Button(
            action_row,
            text="Папка с файлами",
            font=("Arial", 9, "bold"),
            bg="#E8E8E8",
            fg="black",
            activebackground="#D0D0D0",
            activeforeground="black",
            padx=8,
            pady=4,
            cursor="hand2",
            state=tk.DISABLED,
            command=self._open_output_dir,
        )
        self.btn_open_dir.pack(side=tk.LEFT)

        # 5. Окно лога (занимает оставшееся место внизу)
        log_group = ttk.LabelFrame(main_frame, text=" Протокол расчёта и таблица невязок ", padding=6)
        log_group.pack(fill=tk.BOTH, expand=True)

        self.log_text = scrolledtext.ScrolledText(log_group, wrap=tk.NONE, font=("Consolas", 9))
        self.log_text.pack(fill=tk.BOTH, expand=True)

    def _compute_default_out_dir(self, src: str) -> str:
        s = src.strip()
        if not s:
            return ""
        if os.path.isdir(s):
            return os.path.abspath(s)
        elif os.path.isfile(s):
            return os.path.abspath(os.path.dirname(s))
        return os.path.abspath(s)

    def _on_source_path_changed(self, *args):
        src = self.source_path_var.get().strip()
        if src:
            default_out = self._compute_default_out_dir(src)
            if default_out:
                self.out_dir_var.set(default_out)

    def _on_mode_changed(self):
        mode = self.mode_var.get()
        cur = self.source_path_var.get()
        if mode == "folder":
            if os.path.isfile(cur):
                self.source_path_var.set(os.path.dirname(cur))
        elif mode in ("united", "csv"):
            if os.path.isdir(cur):
                self.source_path_var.set("")

    def _on_browse_source(self):
        mode = self.mode_var.get()
        if mode == "folder":
            d = filedialog.askdirectory(title="Выберите папку с файлами высот (*_mm.bin)")
            if d:
                self.source_path_var.set(d)
                self.out_dir_var.set(os.path.abspath(d))
        elif mode == "united":
            f = filedialog.askopenfilename(
                title="Выберите единый BIN-файл всех высот",
                filetypes=[("BIN files", "*.bin"), ("All files", "*.*")]
            )
            if f:
                self.source_path_var.set(f)
                self.out_dir_var.set(os.path.abspath(os.path.dirname(f)))
        elif mode == "csv":
            f = filedialog.askopenfilename(
                title="Выберите готовый CSV-файл",
                filetypes=[("CSV files", "*.csv"), ("All files", "*.*")]
            )
            if f:
                self.source_path_var.set(f)
                self.out_dir_var.set(os.path.abspath(os.path.dirname(f)))

    def _on_browse_out_dir(self):
        d = filedialog.askdirectory(title="Выберите папку для сохранения результатов")
        if d:
            self.out_dir_var.set(d)

    def _on_browse_template(self):
        f = filedialog.askopenfilename(
            title="Выберите шаблон файла метрологии (.bin)",
            filetypes=[("BIN files", "*.bin"), ("All files", "*.*")]
        )
        if f:
            self.template_metro_var.set(f)

    def _log(self, text: str):
        self.log_text.insert(tk.END, text + "\n")
        self.log_text.see(tk.END)

    def _set_status(self, text: str, state: str = "info"):
        colors = {
            "info": ("#E0E0E0", "#333333"),
            "busy": ("#FFF9C4", "#5D4037"),
            "success": ("#C8E6C9", "#1B5E20"),
            "warn": ("#FFE082", "#E65100"),
            "error": ("#FFCDD2", "#B71C1C"),
        }
        bg, fg = colors.get(state, ("#E0E0E0", "#333333"))
        self.status_lbl.config(text=text, bg=bg, fg=fg)

    def _start_calibration(self):
        src = self.source_path_var.get().strip()
        if not src or not os.path.exists(src):
            messagebox.showerror("Ошибка", f"Указанный источник данных не существует:\n{src}")
            return

        try:
            min_wg = float(self.min_wg_var.get().strip())
        except ValueError:
            messagebox.showerror("Ошибка", "Некорректное значение скорости вращения Wg.")
            return

        serial = None
        if self.serial_var.get().strip():
            try:
                serial = int(self.serial_var.get().strip())
            except ValueError:
                pass

        heights = None
        if self.mode_var.get() == "united":
            try:
                heights = [float(h.strip()) for h in self.heights_var.get().split(",") if h.strip()]
            except ValueError:
                messagebox.showerror("Ошибка", "Некорректный список высот (укажите через запятую в метрах).")
                return

        self.run_btn.config(state=tk.DISABLED)
        self.btn_open_geo_img.config(state=tk.DISABLED)
        self.btn_open_noise_img.config(state=tk.DISABLED)
        self.btn_open_hw_report.config(state=tk.DISABLED)
        self.btn_open_geo_txt.config(state=tk.DISABLED)
        self.btn_open_report.config(state=tk.DISABLED)
        self.btn_open_dir.config(state=tk.DISABLED)

        self.log_text.delete("1.0", tk.END)
        self._set_status("Выполняется калибровка и верификация геосигнала...", "busy")

        out_dir = self.out_dir_var.get().strip()
        if not out_dir:
            out_dir = self._compute_default_out_dir(src)
            self.out_dir_var.set(out_dir)
        tmpl = self.template_metro_var.get().strip()
        if not tmpl:
            tmpl = None

        thread = threading.Thread(
            target=self._worker,
            args=(self.mode_var.get(), src, heights, min_wg, serial, out_dir, tmpl),
            daemon=True,
        )
        thread.start()

    def _worker(self, mode: str, src: str, heights: Optional[List[float]], min_wg: float, serial: Optional[int], out_dir: str, tmpl: Optional[str]):
        class StdoutRedirect:
            def __init__(self, callback):
                self.callback = callback
            def write(self, s):
                if s:
                    self.callback(s)
            def flush(self):
                pass

        old_stdout = sys.stdout
        sys.stdout = StdoutRedirect(lambda s: self.after(0, self._log_chunk, s))

        try:
            folder_p = src if mode == "folder" else None
            united_p = src if mode == "united" else None
            csv_p = src if mode == "csv" else None

            run_pipeline(
                folder_path=folder_p,
                united_file_path=united_p,
                heights_list=heights,
                csv_path=csv_p,
                output_dir=out_dir,
                template_metro_path=tmpl,
                min_wg_rpm=min_wg,
                serial_number=serial,
            )

            geo_img_f = os.path.join(out_dir, "geosignal_verification.png")
            noise_img_f = os.path.join(out_dir, "fourier_noise_analysis.png")
            hw_report_f = os.path.join(out_dir, "hardware_and_noise_report.txt")
            geo_txt_f = os.path.join(out_dir, "geosignal_table.txt")
            report_f = os.path.join(out_dir, "calibration_report.txt")
            metro_f = os.path.join(out_dir, "metrology_cartograf_calibrated.bin")

            self.after(0, self._on_success, geo_img_f, noise_img_f, hw_report_f, geo_txt_f, report_f, metro_f)

        except Exception as e:
            self.after(0, self._on_error, str(e))
        finally:
            sys.stdout = old_stdout

    def _log_chunk(self, s: str):
        self.log_text.insert(tk.END, s)
        self.log_text.see(tk.END)

    def _on_success(self, geo_img_path: str, noise_img_path: str, hw_report_path: str, geo_txt_path: str, report_path: str, metro_path: str):
        self.last_geo_img_path = geo_img_path
        self.last_noise_img_path = noise_img_path
        self.last_hw_report_path = hw_report_path
        self.last_geo_txt_path = geo_txt_path
        self.last_report_path = report_path
        self.last_metro_bin_path = metro_path

        self.run_btn.config(state=tk.NORMAL)
        self.btn_open_geo_img.config(state=tk.NORMAL)
        if os.path.exists(noise_img_path):
            self.btn_open_noise_img.config(state=tk.NORMAL)
        if os.path.exists(hw_report_path):
            self.btn_open_hw_report.config(state=tk.NORMAL)
        self.btn_open_geo_txt.config(state=tk.NORMAL)
        self.btn_open_report.config(state=tk.NORMAL)
        self.btn_open_dir.config(state=tk.NORMAL)

        self._set_status("Калибровка и верификация успешно завершены!", "success")
        messagebox.showinfo(
            "Калибровка завершена",
            "Расчёт калибровки, верификация геосигнала и анализ шумов выполнены успешно!\n\n"
            f"• Файл метрологии: {os.path.basename(metro_path)}\n"
            f"• График и таблица невязок: {os.path.basename(geo_img_path)}\n"
            f"• Анализ шумов и 2-й гармоники: {os.path.basename(noise_img_path)}\n"
            f"• Сверка процессора: {os.path.basename(hw_report_path)}"
        )

    def _on_error(self, err_msg: str):
        self.run_btn.config(state=tk.NORMAL)
        self._set_status(f"Ошибка калибровки: {err_msg}", "error")
        messagebox.showerror("Ошибка калибровки", f"Произошла ошибка при расчёте:\n\n{err_msg}")

    def _open_geo_img(self):
        if self.last_geo_img_path and os.path.exists(self.last_geo_img_path):
            os.startfile(self.last_geo_img_path)

    def _open_noise_img(self):
        if self.last_noise_img_path and os.path.exists(self.last_noise_img_path):
            os.startfile(self.last_noise_img_path)

    def _open_hw_report(self):
        if self.last_hw_report_path and os.path.exists(self.last_hw_report_path):
            os.startfile(self.last_hw_report_path)

    def _open_geo_txt(self):
        if self.last_geo_txt_path and os.path.exists(self.last_geo_txt_path):
            os.startfile(self.last_geo_txt_path)

    def _open_report(self):
        if self.last_report_path and os.path.exists(self.last_report_path):
            os.startfile(self.last_report_path)

    def _open_output_dir(self):
        out_dir = self.out_dir_var.get().strip()
        if os.path.exists(out_dir):
            os.startfile(out_dir)


def main():
    app = CalibrationGui()
    app.mainloop()


if __name__ == "__main__":
    main()
