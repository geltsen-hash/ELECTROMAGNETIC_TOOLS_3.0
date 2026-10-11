@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo [GIT SYNC] Local Commit + Remote Push
echo ===================================================

set MSG=%~1
if "%MSG%"=="" (
    set /p MSG="Enter commit message: "
)

if "%MSG%"=="" (
    echo [ERROR] Commit message cannot be empty.
    exit /b 1
)

python git_commit.py "%MSG%"
