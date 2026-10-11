#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
git_commit.py - Компактный Git-клиент для проекта ELECTROMAGNETIC_TOOLS_3.0.
Подавляет шум сотен строк предупреждений CRLF и промежуточных файлов,
выводит компактный статус и экономит до 95% токенов контекста.
"""

import sys
import subprocess
import argparse
import os

def run_cmd(cmd, input_text=None):
    """Выполняет команду git и возвращает stdout, stderr и код возврата."""
    res = subprocess.run(
        cmd,
        input=input_text,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        shell=False
    )
    return res.stdout.strip(), res.stderr.strip(), res.returncode

def get_current_branch():
    """Определяет текущую ветку git."""
    out, _, code = run_cmd(["git", "branch", "--show-current"])
    if code == 0 and out:
        return out
    return "main"

def get_compact_status():
    """Формирует ультра-компактный статус репозитория (1-3 строки)."""
    out, _, code = run_cmd(["git", "status", "--porcelain"])
    if code != 0 or not out:
        return "ℹ️ Рабочее дерево чистое, изменений нет."

    lines = out.splitlines()
    code_files = []
    proj_files = []
    doc_files = []
    other_files = []

    for line in lines:
        parts = line.strip().split(maxsplit=1)
        if len(parts) < 2:
            continue
        status, filepath = parts[0], parts[1]

        if filepath.endswith((".cpp", ".h", ".c")):
            code_files.append(os.path.basename(filepath))
        elif filepath.endswith((".vcxproj", ".vcxproj.filters", ".sln")):
            proj_files.append(os.path.basename(filepath))
        elif filepath.endswith((".md", ".txt", ".docx")):
            doc_files.append(os.path.basename(filepath))
        else:
            other_files.append(os.path.basename(filepath))

    summary = []
    if code_files:
        summary.append(f"Исходный код ({len(code_files)}): {', '.join(code_files[:6])}{'...' if len(code_files) > 6 else ''}")
    if proj_files:
        summary.append(f"Проектные файлы ({len(proj_files)}): {', '.join(proj_files)}")
    if doc_files:
        summary.append(f"Документы ({len(doc_files)}): {', '.join(doc_files[:5])}{'...' if len(doc_files) > 5 else ''}")
    if other_files:
        summary.append(f"Прочие ({len(other_files)}): {', '.join(other_files[:5])}{'...' if len(other_files) > 5 else ''}")

    return "\n".join(summary) if summary else f"Изменено файлов: {len(lines)}"

def do_commit(message, desc="", do_push=True):
    """Индексирует, коммитит и отправляет изменения (по умолчанию сразу в GitHub)."""
    out, _, _ = run_cmd(["git", "status", "--porcelain"])
    if not out:
        print("ℹ️ Нет изменений для коммита.")
        return 0

    # Индексируем всё (git add -A)
    _, _, code = run_cmd(["git", "add", "-A"])
    if code != 0:
        print("❌ Ошибка при выполнении git add -A")
        return 1

    # Формируем стандартизированное сообщение
    commit_body_parts = [message.strip()]
    if desc.strip():
        commit_body_parts.append(f"\n{desc.strip()}")
    commit_body_parts.append(
        "\nGenerated with [Devin](https://devin.ai)\n\n"
        "Co-Authored-By: Devin <158243242+devin-ai-integration[bot]@users.noreply.github.com>"
    )
    full_msg = "\n".join(commit_body_parts)

    res_out, res_err, res_code = run_cmd(["git", "commit", "-F", "-"], input_text=full_msg)
    if res_code != 0:
        print(f"❌ Ошибка коммита:\n{res_err or res_out}")
        return res_code

    hash_out, _, _ = run_cmd(["git", "log", "-1", "--oneline"])
    stat_out, _, _ = run_cmd(["git", "diff", "--shortstat", "HEAD~1", "HEAD"])

    print(f"✅ Закоммичено: {hash_out}")
    if stat_out:
        print(f"📊 {stat_out}")

    if do_push:
        print("🚀 Выполняется git push...")
        branch = get_current_branch()
        # Попробуем обычный push, если не настроен upstream - с флагом -u origin <branch>
        p_out, p_err, p_code = run_cmd(["git", "push"])
        if p_code != 0 and ("no upstream" in p_err.lower() or "set-upstream" in p_err.lower()):
            p_out, p_err, p_code = run_cmd(["git", "push", "-u", "origin", branch])

        if p_code == 0:
            print(f"✅ Изменения успешно отправлены в origin/{branch} (GitHub).")
        else:
            print(f"❌ Ошибка git push:\n{p_err or p_out}")
            return p_code

    return 0

def main():
    parser = argparse.ArgumentParser(description="Компактный Git-клиент для ELECTROMAGNETIC_TOOLS_3.0.")
    parser.add_argument("message", nargs="?", default="", help="Заголовок коммита")
    parser.add_argument("-d", "--desc", default="", help="Подробное описание коммита")
    parser.add_argument("-s", "--status", action="store_true", help="Показать краткий статус (без коммита)")
    parser.add_argument("--no-push", action="store_true", help="Не отправлять на сервер (только локальный коммит)")

    args = parser.parse_args()

    if args.status or not args.message:
        print("📋 Краткий статус репозитория:")
        print(get_compact_status())
        if not args.message:
            print("\nИспользование: python git_commit.py \"Заголовок коммита\" [-d \"Описание\"] [--no-push]")
        return 0

    return do_commit(args.message, desc=args.desc, do_push=not args.no_push)

if __name__ == "__main__":
    sys.exit(main())
