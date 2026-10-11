#include "pch.h"
#include <windows.h>
#include <iostream>
#include "MainForm.h"

using namespace System;
using namespace System::Windows::Forms;

[STAThreadAttribute]
int main(array<System::String^>^ args)
{
	// Настройка консоли для корректного вывода кириллицы UTF-8
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	std::cout << "==============================================================" << std::endl;
	std::cout << "  Electromagnetic Tools 3.0 — Наземная станция оператора       " << std::endl;
	std::cout << "  Организация: НПФ «АМК Горизонт»                              " << std::endl;
	std::cout << "  Разработал: зам начальника ОП Смирнов С. Г.                 " << std::endl;
	std::cout << "==============================================================" << std::endl;
	std::cout << "[Init] Запуск графической подсистемы Windows Forms..." << std::endl;

	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	ELECTROMAGNETIC_TOOLS_3_0::MainForm^ mainForm = gcnew ELECTROMAGNETIC_TOOLS_3_0::MainForm();
	Application::Run(mainForm);

	std::cout << "[Shutdown] Станция Electromagnetic Tools 3.0 завершила работу." << std::endl;
	return 0;
}
