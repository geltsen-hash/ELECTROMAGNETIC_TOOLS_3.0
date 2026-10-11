@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo [GIT PUSH] Pushing changes to remote repository...
echo ===================================================

git push
if %ERRORLEVEL% neq 0 (
    git push -u origin main
)

if %ERRORLEVEL% neq 0 (
    echo.
    echo [WARNING] Push failed (remote error or network issue).
    echo Your commit is safely preserved in local Git repository.
    exit /b %ERRORLEVEL%
)

echo.
echo [SUCCESS] Changes pushed to origin successfully.
