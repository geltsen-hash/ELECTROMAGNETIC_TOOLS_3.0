#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build.py - Компактный скрипт сборки проекта ELECTROMAGNETIC_TOOLS_3.0.sln через MSBuild.
Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.

Подавляет избыточный шум компилятора, экономя токены контекста.
При успехе выводит ровно 3 строки. При ошибке - только строки ошибок.
"""

import os
import sys
import time
import argparse
import subprocess

MSBUILD_CANDIDATES = [
    r"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe",
    r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe",
    r"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe",
]

def find_msbuild():
    for path in MSBUILD_CANDIDATES:
        if os.path.isfile(path):
            return path
    vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if os.path.isfile(vswhere):
        try:
            res = subprocess.check_output([
                vswhere, "-latest", "-requires", "Microsoft.Component.MSBuild",
                "-find", r"MSBuild\**\Bin\MSBuild.exe"
            ], text=True).strip().splitlines()
            if res and os.path.isfile(res[0]):
                return res[0]
        except Exception:
            pass
    return "MSBuild.exe"

def main():
    parser = argparse.ArgumentParser(description="Сборка проекта Electromagnetic Tools 3.0 с фильтрацией вывода.")
    parser.add_argument("--config", default="Release", choices=["Release", "Debug"], help="Конфигурация сборки (по умолчанию Release)")
    parser.add_argument("--platform", default="x64", choices=["x64"], help="Платформа сборки (по умолчанию x64)")
    parser.add_argument("--rebuild", action="store_true", help="Полная пересборка (Rebuild)")
    parser.add_argument("--show-warnings", action="store_true", help="Не подавлять вывод предупреждений")
    args = parser.parse_args()

    project_dir = os.path.dirname(os.path.abspath(__file__))
    sln_path = os.path.join(project_dir, "ELECTROMAGNETIC_TOOLS_3.0.sln")
    if not os.path.isfile(sln_path):
        print(f"[BUILD ERROR] Файл решения не найден: {sln_path}")
        sys.exit(1)

    msbuild_exe = find_msbuild()
    cmd = [
        msbuild_exe,
        sln_path,
        f"/p:Configuration={args.config}",
        f"/p:Platform={args.platform}",
        "/nologo",
        "/m",
        "/clp:ErrorsOnly" if not args.show_warnings else "/clp:Summary"
    ]
    if args.rebuild:
        cmd.append("/t:Rebuild")

    start_time = time.time()
    proc = subprocess.Popen(
        cmd,
        cwd=project_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace"
    )

    error_lines = []
    warning_count = 0
    all_lines = []

    for line in proc.stdout:
        all_lines.append(line)
        line_clean = line.strip()
        if ": error " in line_clean or line_clean.startswith("fatal error"):
            error_lines.append(line_clean)
        elif ": warning " in line_clean:
            warning_count += 1
            if args.show_warnings:
                print(f"  [WARN] {line_clean}")

    proc.wait()
    elapsed = time.time() - start_time
    exe_target = os.path.join(project_dir, args.platform, args.config, "ELECTROMAGNETIC_TOOLS_3.0.exe")

    if proc.returncode == 0:
        exe_status = f"{exe_target} (существует)" if os.path.isfile(exe_target) else "цель скомпилирована"
        print(f"[BUILD SUCCESS] {args.config}|{args.platform} за {elapsed:.1f}с")
        print(f"  Выходной файл: {exe_status}")
        print(f"  Ошибок: 0, Предупреждений: {warning_count} (подавлены)")
        sys.exit(0)
    else:
        print(f"[BUILD FAILED] {args.config}|{args.platform} (код завершения: {proc.returncode}, {elapsed:.1f}с)")
        if error_lines:
            print(f"  Найдено ошибок ({len(error_lines)}):")
            for err in error_lines:
                print(f"    - {err}")
        else:
            print("  Хвост лога сборки:")
            for l in all_lines[-15:]:
                if l.strip():
                    print(f"    {l.strip()}")
        sys.exit(proc.returncode)

if __name__ == "__main__":
    main()
