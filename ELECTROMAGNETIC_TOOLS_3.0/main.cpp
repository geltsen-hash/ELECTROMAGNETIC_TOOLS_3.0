#include "pch.h"
#include <windows.h>
#include <iostream>
#include "MainForm.h"
#include "BinFileReader.h"
#include <msclr/marshal_cppstd.h>

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

	if (args->Length > 0 && args[0] == L"--export-csv")
	{
		if (args->Length != 3 && args->Length != 6)
		{
			std::cerr << "Usage: --export-csv input.bin output.csv [--frames min max | --temperature min max]" << std::endl;
			return 2;
		}
		SondeData::CsvSelection selection;
		if (args->Length == 6)
		{
			double low, high;
			if ((args[3] != L"--frames" && args[3] != L"--temperature")
				|| !Double::TryParse(args[4], System::Globalization::NumberStyles::Float, System::Globalization::CultureInfo::InvariantCulture, low)
				|| !Double::TryParse(args[5], System::Globalization::NumberStyles::Float, System::Globalization::CultureInfo::InvariantCulture, high))
			{
				std::cerr << "Invalid CSV range arguments." << std::endl;
				return 2;
			}
			selection.enabled = true;
			selection.temperature = args[3] == L"--temperature";
			selection.minimum = low;
			selection.maximum = high;
		}
		String^ inputPath = args[1];
		String^ outputPath = args[2];
		std::string input = msclr::interop::marshal_as<std::string>(inputPath);
		std::wstring output = msclr::interop::marshal_as<std::wstring>(outputPath);
		char error[512] = {};
		size_t count = 0;
		if (!SondeData::LoadFile(input.c_str(), error, sizeof(error))
			|| !SondeData::ExportCsv(output.c_str(), selection, count, error, sizeof(error)))
		{
			std::cerr << "[CSV FAILED] " << error << std::endl;
			return 1;
		}
		std::cout << "[CSV SUCCESS] " << SondeData::GetCsvSchema() << ": " << count << " frames" << std::endl;
		return 0;
	}

	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	ELECTROMAGNETIC_TOOLS_3_0::MainForm^ mainForm = gcnew ELECTROMAGNETIC_TOOLS_3_0::MainForm();
	if (args->Length > 0 && System::IO::File::Exists(args[0]))
	{
		mainForm->LoadDataFile(args[0]);
	}
	Application::Run(mainForm);

	std::cout << "[Shutdown] Станция Electromagnetic Tools 3.0 завершила работу." << std::endl;
	return 0;
}
