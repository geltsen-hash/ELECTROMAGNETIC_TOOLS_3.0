@echo off
rem Запуск эмулятора прибора + открытие веб-пульта
rem НПФ «АМК Горизонт», разработал: зам начальника ОП Смирнов С. Г.
cd /d "%~dp0"
start "" python em_probe.py %*
timeout /t 2 /nobreak >nul
start "" http://localhost:8080
