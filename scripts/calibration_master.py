# -*- coding: utf-8 -*-
"""
Лаунчер мастера калибровки АМК-120 «Картограф» для ELECTROMAGNETIC_TOOLS_3.0.
Папка скрипта добавляется в sys.path автоматически (embeddable python_runtime
игнорирует PYTHONPATH и рабочий каталог из-за python311._pth).

Организация: НПФ «АМК Горизонт», разработал зам начальника ОП Смирнов С. Г.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from cartograf_autocalib.gui import CalibrationGui

if __name__ == "__main__":
    CalibrationGui().mainloop()
