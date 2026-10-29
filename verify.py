#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.abspath(__file__))
APP_NAME = "ELECTROMAGNETIC_TOOLS_3.0.exe"


def build(config):
    result = subprocess.run(
        [sys.executable, os.path.join(ROOT, "build.py"), "--config", config],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="utf-8",
        errors="replace",
    )
    lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if result.returncode == 0:
        print(lines[0] if lines else f"[BUILD SUCCESS] {config}|x64")
        return True
    print(f"[BUILD FAILED] {config}|x64")
    for line in lines[-15:]:
        print(f"  {line}")
    return False


def smoke_run(config, seconds, data_file):
    exe = os.path.join(ROOT, "x64", config, APP_NAME)
    if not os.path.isfile(exe):
        print(f"[RUN FAILED] Не найден: {exe}")
        return False

    command = [exe]
    if data_file:
        command.append(data_file)

    process = subprocess.Popen(
        command,
        cwd=ROOT,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    try:
        return_code = process.wait(timeout=seconds)
        print(f"[RUN FAILED] {config}: приложение завершилось раньше времени, код {return_code}")
        return False
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        suffix = f", файл: {os.path.basename(data_file)}" if data_file else ""
        print(f"[RUN SUCCESS] {config}: раннего сбоя нет{suffix}")
        return True


def main():
    parser = argparse.ArgumentParser(description="Компактная сборка и smoke-проверка Electromagnetic Tools 3.0")
    parser.add_argument("--config", choices=["Release", "Debug"], default="Release")
    parser.add_argument("--full", action="store_true", help="Проверить Release и Debug")
    parser.add_argument("--build-only", action="store_true", help="Не запускать приложение")
    parser.add_argument("--run", action="store_true", help="Запустить без сборки")
    parser.add_argument("--data", help="Открыть указанный BIN/BIV/DEV-файл при smoke-проверке")
    parser.add_argument("--smoke-seconds", type=float, default=3.0, help="Длительность smoke-проверки")
    parser.add_argument("--csv-check", action="store_true", help="Проверить полный CSV-экспорт на синтетических и стендовых данных")
    parser.add_argument("--update-check", action="store_true", help="Локальная проверка updater, отката и упаковки без FTP")
    parser.add_argument("--python-check", action="store_true", help="Локальная проверка отдельной поставки Python Runtime без FTP")
    args = parser.parse_args()

    if args.data:
        args.data = os.path.abspath(args.data)
        if not os.path.isfile(args.data):
            print(f"[VERIFY FAILED] Файл данных не найден: {args.data}")
            return 2
    if args.smoke_seconds <= 0:
        print("[VERIFY FAILED] --smoke-seconds должен быть больше нуля")
        return 2

    configs = ["Release", "Debug"] if args.full else [args.config]
    checks = 0
    passed = 0

    for config in configs:
        if not args.run:
            checks += 1
            if not build(config):
                continue
            passed += 1
        for enabled, script in ((args.csv_check, "test_csv.py"), (args.update_check, "test_update.py"), (args.python_check, "test_python_runtime.py")):
            if enabled:
                checks += 1
                result = subprocess.run(
                    [sys.executable, os.path.join(ROOT, script), "--config", config],
                    cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                    text=True, encoding="utf-8", errors="replace",
                )
                for line in result.stdout.splitlines()[-15:]:
                    print(line)
                if result.returncode == 0:
                    passed += 1
        if not args.build_only:
            checks += 1
            if smoke_run(config, args.smoke_seconds, args.data):
                passed += 1

    if checks > 0 and passed == checks:
        print(f"[VERIFY SUCCESS] {passed}/{checks}")
        return 0
    print(f"[VERIFY FAILED] {passed}/{checks}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
