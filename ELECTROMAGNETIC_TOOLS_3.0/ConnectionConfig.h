#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include "DeviceLink.h"

namespace ELECTROMAGNETIC_TOOLS_3_0 {

	/// <summary>
	/// Настройки приложения и связи с прибором.
	/// Формат файла совместим с Conf.txt старой программы AIR_TEMP_1.0
	/// (с дополнительной строкой baud_rate в конце, как в MAGNETOMETR_CAL).
	/// Файл: EM_Tools_Conf.txt рядом с EXE.
	/// </summary>
	struct AppConfig {
		unsigned int surf_interval = 2100;
		unsigned int points = 100;
		unsigned int get_data_interval = 2000;
		bool check_update = false;
		std::string com_name = "COM1";
		unsigned int baud_rate = 125000;
		std::string local_file_to_ftp_upload = "";
		int connection_type = CONN_COM;
		std::string wifi_ip = "192.168.4.1";
		unsigned int wifi_port = 5000;
		int read_timeout = 3000;
		int write_timeout = 3000;
		std::string data_folder = "C:\\EXP";   // папка временного файла телеметрии

		static AppConfig& Instance() {
			static AppConfig instance;
			return instance;
		}

		// Путь к конфигу — всегда рядом с исполняемым файлом
		static std::string GetConfigPath() {
			System::String^ exeDir = System::IO::Path::GetDirectoryName(
				System::Windows::Forms::Application::ExecutablePath);
			msclr::interop::marshal_context ctx;
			return ctx.marshal_as<std::string>(exeDir) + "\\EM_Tools_Conf.txt";
		}

		bool Load() {
			std::string filename = GetConfigPath();
			std::ifstream fin(filename);
			if (!fin.is_open()) {
				std::cout << "[Config] " << filename << " not found, using defaults." << std::endl;
				return false;
			}

			fin >> surf_interval;
			fin >> points;
			fin >> get_data_interval;
			int update_flag = 0;
			fin >> update_flag;
			check_update = (update_flag != 0);
			fin >> com_name;
			fin >> local_file_to_ftp_upload;

			for (int i = 0; i < 11; i++) {
				int dummy;
				if (!(fin >> dummy)) break;
			}

			if (fin >> connection_type) {
				if (fin >> wifi_ip) {
					fin >> wifi_port;
				}
			}

			if (fin >> baud_rate) {
				// optional baud rate loaded
			}
			else {
				baud_rate = 125000;
			}

			// Папка данных — последняя строка (getline: допускает пробелы)
			fin.ignore(1);
			std::string folder;
			if (std::getline(fin, folder) && !folder.empty()) {
				data_folder = folder;
			}

			fin.close();
			std::cout << "[Config] Loaded from " << filename << ": "
				<< (connection_type == CONN_UDP ? "Wi-Fi UDP " + wifi_ip + ":" + std::to_string(wifi_port) : "COM " + com_name + " @" + std::to_string(baud_rate))
				<< std::endl;
			return true;
		}

		bool Save() const {
			std::string filename = GetConfigPath();
			std::ofstream fout(filename);
			if (!fout.is_open()) {
				std::cout << "[Config] Failed to save " << filename << std::endl;
				return false;
			}

			fout << surf_interval << "\n";
			fout << points << "\n";
			fout << get_data_interval << "\n";
			fout << (check_update ? 1 : 0) << "\n";
			fout << com_name << "\n";
			fout << (local_file_to_ftp_upload.empty() ? "-" : local_file_to_ftp_upload) << "\n";

			// 11 dummy values for compatibility
			fout << "0\n0\n1\n0\n1\n1\n10\n11\n5\n0\n1\n";

			fout << connection_type << "\n";
			fout << wifi_ip << "\n";
			fout << wifi_port << "\n";
			fout << baud_rate << "\n";
			fout << data_folder << "\n";

			fout.close();
			std::cout << "[Config] Saved to " << filename << std::endl;
			return true;
		}

		// Применить настройки к объекту связи
		void ApplyTo(DeviceLink^ link) const {
			if (link == nullptr) return;
			link->ConnectionType = connection_type;
			link->PortName = gcnew System::String(com_name.c_str());
			link->BaudRate = baud_rate;
			link->WifiIP = gcnew System::String(wifi_ip.c_str());
			link->WifiPort = wifi_port;
			link->ReadTimeout = read_timeout;
			link->WriteTimeout = write_timeout;
		}
	};

}
