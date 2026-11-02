#pragma once

#include "DeviceLink.h"
#include "ConnectionConfig.h"

namespace ELECTROMAGNETIC_TOOLS_3_0 {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Окно настроек подключения к прибору (COM / Wi-Fi UDP).
	/// Перенесено из MAGNETOMETR_CAL/ConnectionForm.h (источник: AIR_TEMP_1.0 Param_Form).
	/// </summary>
	public ref class ConnectionForm : public System::Windows::Forms::Form
	{
	public:
		ConnectionForm(void)
		{
			InitializeComponent();
			LoadCurrentConfig();
		}

	protected:
		~ConnectionForm()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		System::Windows::Forms::GroupBox^  groupBoxType;
		System::Windows::Forms::RadioButton^  radioButtonCOM;
		System::Windows::Forms::RadioButton^  radioButtonUDP;

		System::Windows::Forms::GroupBox^  groupBoxCOM;
		System::Windows::Forms::Label^  labelPort;
		System::Windows::Forms::ComboBox^  comboBoxPort;
		System::Windows::Forms::Button^  buttonRefreshPorts;
		System::Windows::Forms::Label^  labelBaud;
		System::Windows::Forms::ComboBox^  comboBoxBaud;

		System::Windows::Forms::GroupBox^  groupBoxUDP;
		System::Windows::Forms::CheckBox^  checkBoxEmulator;
		System::Windows::Forms::Label^  labelIP;
		System::Windows::Forms::TextBox^  textBoxIP;
		System::Windows::Forms::Label^  labelUDPPort;
		System::Windows::Forms::TextBox^  textBoxUDPPort;

		System::Windows::Forms::GroupBox^  groupBoxTimeouts;
		System::Windows::Forms::Label^  labelReadTimeout;
		System::Windows::Forms::TextBox^  textBoxReadTimeout;
		System::Windows::Forms::Label^  labelWriteTimeout;
		System::Windows::Forms::TextBox^  textBoxWriteTimeout;

		System::Windows::Forms::GroupBox^  groupBoxData;
		System::Windows::Forms::Label^  labelDataFolder;
		System::Windows::Forms::TextBox^  textBoxDataFolder;
		System::Windows::Forms::Button^  buttonBrowseFolder;
		System::Windows::Forms::Label^  labelStreamPoints;
		System::Windows::Forms::NumericUpDown^  numericStreamPoints;

		System::Windows::Forms::Button^  buttonSave;
		System::Windows::Forms::Button^  buttonCancel;

		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		void InitializeComponent(void)
		{
			this->groupBoxType = (gcnew System::Windows::Forms::GroupBox());
			this->radioButtonCOM = (gcnew System::Windows::Forms::RadioButton());
			this->radioButtonUDP = (gcnew System::Windows::Forms::RadioButton());
			this->groupBoxCOM = (gcnew System::Windows::Forms::GroupBox());
			this->labelPort = (gcnew System::Windows::Forms::Label());
			this->comboBoxPort = (gcnew System::Windows::Forms::ComboBox());
			this->buttonRefreshPorts = (gcnew System::Windows::Forms::Button());
			this->labelBaud = (gcnew System::Windows::Forms::Label());
			this->comboBoxBaud = (gcnew System::Windows::Forms::ComboBox());
			this->groupBoxUDP = (gcnew System::Windows::Forms::GroupBox());
			this->checkBoxEmulator = (gcnew System::Windows::Forms::CheckBox());
			this->labelIP = (gcnew System::Windows::Forms::Label());
			this->textBoxIP = (gcnew System::Windows::Forms::TextBox());
			this->labelUDPPort = (gcnew System::Windows::Forms::Label());
			this->textBoxUDPPort = (gcnew System::Windows::Forms::TextBox());
			this->groupBoxTimeouts = (gcnew System::Windows::Forms::GroupBox());
			this->labelReadTimeout = (gcnew System::Windows::Forms::Label());
			this->textBoxReadTimeout = (gcnew System::Windows::Forms::TextBox());
			this->labelWriteTimeout = (gcnew System::Windows::Forms::Label());
			this->textBoxWriteTimeout = (gcnew System::Windows::Forms::TextBox());
			this->groupBoxData = (gcnew System::Windows::Forms::GroupBox());
			this->labelDataFolder = (gcnew System::Windows::Forms::Label());
			this->textBoxDataFolder = (gcnew System::Windows::Forms::TextBox());
			this->buttonBrowseFolder = (gcnew System::Windows::Forms::Button());
			this->labelStreamPoints = (gcnew System::Windows::Forms::Label());
			this->numericStreamPoints = (gcnew System::Windows::Forms::NumericUpDown());
			this->buttonSave = (gcnew System::Windows::Forms::Button());
			this->buttonCancel = (gcnew System::Windows::Forms::Button());
			this->groupBoxType->SuspendLayout();
			this->groupBoxCOM->SuspendLayout();
			this->groupBoxUDP->SuspendLayout();
			this->groupBoxTimeouts->SuspendLayout();
			this->groupBoxData->SuspendLayout();
			this->SuspendLayout();
			// 
			// groupBoxType
			// 
			this->groupBoxType->Controls->Add(this->radioButtonCOM);
			this->groupBoxType->Controls->Add(this->radioButtonUDP);
			this->groupBoxType->Location = System::Drawing::Point(12, 12);
			this->groupBoxType->Name = L"groupBoxType";
			this->groupBoxType->Size = System::Drawing::Size(360, 55);
			this->groupBoxType->TabIndex = 0;
			this->groupBoxType->TabStop = false;
			this->groupBoxType->Text = L"Тип подключения";
			// 
			// radioButtonCOM
			// 
			this->radioButtonCOM->AutoSize = true;
			this->radioButtonCOM->Checked = true;
			this->radioButtonCOM->Location = System::Drawing::Point(20, 22);
			this->radioButtonCOM->Name = L"radioButtonCOM";
			this->radioButtonCOM->Size = System::Drawing::Size(107, 17);
			this->radioButtonCOM->TabIndex = 0;
			this->radioButtonCOM->TabStop = true;
			this->radioButtonCOM->Text = L"COM-порт (UART)";
			this->radioButtonCOM->UseVisualStyleBackColor = true;
			this->radioButtonCOM->CheckedChanged += gcnew System::EventHandler(this, &ConnectionForm::ConnectionType_CheckedChanged);
			// 
			// radioButtonUDP
			// 
			this->radioButtonUDP->AutoSize = true;
			this->radioButtonUDP->Location = System::Drawing::Point(180, 22);
			this->radioButtonUDP->Name = L"radioButtonUDP";
			this->radioButtonUDP->Size = System::Drawing::Size(86, 17);
			this->radioButtonUDP->TabIndex = 1;
			this->radioButtonUDP->Text = L"Wi-Fi (UDP)";
			this->radioButtonUDP->UseVisualStyleBackColor = true;
			this->radioButtonUDP->CheckedChanged += gcnew System::EventHandler(this, &ConnectionForm::ConnectionType_CheckedChanged);
			// 
			// groupBoxCOM
			// 
			this->groupBoxCOM->Controls->Add(this->labelPort);
			this->groupBoxCOM->Controls->Add(this->comboBoxPort);
			this->groupBoxCOM->Controls->Add(this->buttonRefreshPorts);
			this->groupBoxCOM->Controls->Add(this->labelBaud);
			this->groupBoxCOM->Controls->Add(this->comboBoxBaud);
			this->groupBoxCOM->Location = System::Drawing::Point(12, 75);
			this->groupBoxCOM->Name = L"groupBoxCOM";
			this->groupBoxCOM->Size = System::Drawing::Size(360, 95);
			this->groupBoxCOM->TabIndex = 1;
			this->groupBoxCOM->TabStop = false;
			this->groupBoxCOM->Text = L"Параметры COM-порта (UART)";
			// 
			// labelPort
			// 
			this->labelPort->AutoSize = true;
			this->labelPort->Location = System::Drawing::Point(17, 26);
			this->labelPort->Name = L"labelPort";
			this->labelPort->Size = System::Drawing::Size(35, 13);
			this->labelPort->TabIndex = 0;
			this->labelPort->Text = L"Порт:";
			// 
			// comboBoxPort
			// 
			this->comboBoxPort->FormattingEnabled = true;
			this->comboBoxPort->Location = System::Drawing::Point(120, 23);
			this->comboBoxPort->Name = L"comboBoxPort";
			this->comboBoxPort->Size = System::Drawing::Size(130, 21);
			this->comboBoxPort->TabIndex = 1;
			// 
			// buttonRefreshPorts
			// 
			this->buttonRefreshPorts->Location = System::Drawing::Point(260, 21);
			this->buttonRefreshPorts->Name = L"buttonRefreshPorts";
			this->buttonRefreshPorts->Size = System::Drawing::Size(85, 25);
			this->buttonRefreshPorts->TabIndex = 2;
			this->buttonRefreshPorts->Text = L"Обновить";
			this->buttonRefreshPorts->UseVisualStyleBackColor = true;
			this->buttonRefreshPorts->Click += gcnew System::EventHandler(this, &ConnectionForm::ButtonRefreshPorts_Click);
			// 
			// labelBaud
			// 
			this->labelBaud->AutoSize = true;
			this->labelBaud->Location = System::Drawing::Point(17, 60);
			this->labelBaud->Name = L"labelBaud";
			this->labelBaud->Size = System::Drawing::Size(58, 13);
			this->labelBaud->TabIndex = 3;
			this->labelBaud->Text = L"Скорость:";
			// 
			// comboBoxBaud
			// 
			this->comboBoxBaud->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxBaud->FormattingEnabled = true;
			this->comboBoxBaud->Items->AddRange(gcnew cli::array< System::Object^  >(9) {
				L"9600",
				L"19200",
				L"38400",
				L"57600",
				L"115200",
				L"125000",
				L"230400",
				L"460800",
				L"921600"
			});
			this->comboBoxBaud->Location = System::Drawing::Point(120, 57);
			this->comboBoxBaud->Name = L"comboBoxBaud";
			this->comboBoxBaud->Size = System::Drawing::Size(130, 21);
			this->comboBoxBaud->TabIndex = 4;
			// 
			// groupBoxUDP
			// 
			this->groupBoxUDP->Controls->Add(this->checkBoxEmulator);
			this->groupBoxUDP->Controls->Add(this->labelIP);
			this->groupBoxUDP->Controls->Add(this->textBoxIP);
			this->groupBoxUDP->Controls->Add(this->labelUDPPort);
			this->groupBoxUDP->Controls->Add(this->textBoxUDPPort);
			this->groupBoxUDP->Location = System::Drawing::Point(12, 180);
			this->groupBoxUDP->Name = L"groupBoxUDP";
			this->groupBoxUDP->Size = System::Drawing::Size(360, 95);
			this->groupBoxUDP->TabIndex = 2;
			this->groupBoxUDP->TabStop = false;
			this->groupBoxUDP->Text = L"Параметры Wi-Fi (UDP)";
			// 
			// checkBoxEmulator
			// 
			this->checkBoxEmulator->AutoSize = true;
			this->checkBoxEmulator->Location = System::Drawing::Point(260, 26);
			this->checkBoxEmulator->Name = L"checkBoxEmulator";
			this->checkBoxEmulator->Size = System::Drawing::Size(85, 17);
			this->checkBoxEmulator->TabIndex = 4;
			this->checkBoxEmulator->Text = L"Эмуляция";
			this->checkBoxEmulator->UseVisualStyleBackColor = true;
			this->checkBoxEmulator->CheckedChanged += gcnew System::EventHandler(this, &ConnectionForm::checkBoxEmulator_CheckedChanged);
			// 
			// labelIP
			// 
			this->labelIP->AutoSize = true;
			this->labelIP->Location = System::Drawing::Point(17, 26);
			this->labelIP->Name = L"labelIP";
			this->labelIP->Size = System::Drawing::Size(53, 13);
			this->labelIP->TabIndex = 0;
			this->labelIP->Text = L"IP-адрес:";
			// 
			// textBoxIP
			// 
			this->textBoxIP->Location = System::Drawing::Point(120, 23);
			this->textBoxIP->Name = L"textBoxIP";
			this->textBoxIP->Size = System::Drawing::Size(130, 20);
			this->textBoxIP->TabIndex = 1;
			this->textBoxIP->Text = L"192.168.4.1";
			// 
			// labelUDPPort
			// 
			this->labelUDPPort->AutoSize = true;
			this->labelUDPPort->Location = System::Drawing::Point(17, 60);
			this->labelUDPPort->Name = L"labelUDPPort";
			this->labelUDPPort->Size = System::Drawing::Size(35, 13);
			this->labelUDPPort->TabIndex = 2;
			this->labelUDPPort->Text = L"Порт:";
			// 
			// textBoxUDPPort
			// 
			this->textBoxUDPPort->Location = System::Drawing::Point(120, 57);
			this->textBoxUDPPort->Name = L"textBoxUDPPort";
			this->textBoxUDPPort->Size = System::Drawing::Size(130, 20);
			this->textBoxUDPPort->TabIndex = 3;
			this->textBoxUDPPort->Text = L"5000";
			// 
			// groupBoxTimeouts
			// 
			this->groupBoxTimeouts->Controls->Add(this->labelReadTimeout);
			this->groupBoxTimeouts->Controls->Add(this->textBoxReadTimeout);
			this->groupBoxTimeouts->Controls->Add(this->labelWriteTimeout);
			this->groupBoxTimeouts->Controls->Add(this->textBoxWriteTimeout);
			this->groupBoxTimeouts->Location = System::Drawing::Point(12, 285);
			this->groupBoxTimeouts->Name = L"groupBoxTimeouts";
			this->groupBoxTimeouts->Size = System::Drawing::Size(360, 90);
			this->groupBoxTimeouts->TabIndex = 3;
			this->groupBoxTimeouts->TabStop = false;
			this->groupBoxTimeouts->Text = L"Таймауты (мс)";
			// 
			// labelReadTimeout
			// 
			this->labelReadTimeout->AutoSize = true;
			this->labelReadTimeout->Location = System::Drawing::Point(17, 26);
			this->labelReadTimeout->Name = L"labelReadTimeout";
			this->labelReadTimeout->Size = System::Drawing::Size(90, 13);
			this->labelReadTimeout->TabIndex = 0;
			this->labelReadTimeout->Text = L"Таймаут чтения:";
			// 
			// textBoxReadTimeout
			// 
			this->textBoxReadTimeout->Location = System::Drawing::Point(120, 23);
			this->textBoxReadTimeout->Name = L"textBoxReadTimeout";
			this->textBoxReadTimeout->Size = System::Drawing::Size(130, 20);
			this->textBoxReadTimeout->TabIndex = 1;
			this->textBoxReadTimeout->Text = L"3000";
			// 
			// labelWriteTimeout
			// 
			this->labelWriteTimeout->AutoSize = true;
			this->labelWriteTimeout->Location = System::Drawing::Point(17, 57);
			this->labelWriteTimeout->Name = L"labelWriteTimeout";
			this->labelWriteTimeout->Size = System::Drawing::Size(93, 13);
			this->labelWriteTimeout->TabIndex = 2;
			this->labelWriteTimeout->Text = L"Таймаут записи:";
			// 
			// textBoxWriteTimeout
			// 
			this->textBoxWriteTimeout->Location = System::Drawing::Point(120, 54);
			this->textBoxWriteTimeout->Name = L"textBoxWriteTimeout";
			this->textBoxWriteTimeout->Size = System::Drawing::Size(130, 20);
			this->textBoxWriteTimeout->TabIndex = 3;
			this->textBoxWriteTimeout->Text = L"3000";
			// 
			// groupBoxData
			// 
			this->groupBoxData->Controls->Add(this->labelDataFolder);
			this->groupBoxData->Controls->Add(this->textBoxDataFolder);
			this->groupBoxData->Controls->Add(this->buttonBrowseFolder);
			this->groupBoxData->Controls->Add(this->labelStreamPoints);
			this->groupBoxData->Controls->Add(this->numericStreamPoints);
			this->groupBoxData->Location = System::Drawing::Point(12, 385);
			this->groupBoxData->Name = L"groupBoxData";
			this->groupBoxData->Size = System::Drawing::Size(360, 95);
			this->groupBoxData->TabIndex = 5;
			this->groupBoxData->TabStop = false;
			this->groupBoxData->Text = L"Временный файл телеметрии";
			// 
			// labelDataFolder
			// 
			this->labelDataFolder->AutoSize = true;
			this->labelDataFolder->Location = System::Drawing::Point(17, 26);
			this->labelDataFolder->Name = L"labelDataFolder";
			this->labelDataFolder->Size = System::Drawing::Size(40, 13);
			this->labelDataFolder->TabIndex = 0;
			this->labelDataFolder->Text = L"Папка:";
			// 
			// textBoxDataFolder
			// 
			this->textBoxDataFolder->Location = System::Drawing::Point(120, 23);
			this->textBoxDataFolder->Name = L"textBoxDataFolder";
			this->textBoxDataFolder->Size = System::Drawing::Size(185, 20);
			this->textBoxDataFolder->TabIndex = 1;
			this->textBoxDataFolder->Text = L"C:\\EXP";
			// 
			// buttonBrowseFolder
			// 
			this->buttonBrowseFolder->Location = System::Drawing::Point(312, 21);
			this->buttonBrowseFolder->Name = L"buttonBrowseFolder";
			this->buttonBrowseFolder->Size = System::Drawing::Size(33, 25);
			this->buttonBrowseFolder->TabIndex = 2;
			this->buttonBrowseFolder->Text = L"...";
			this->buttonBrowseFolder->UseVisualStyleBackColor = true;
			this->buttonBrowseFolder->Click += gcnew System::EventHandler(this, &ConnectionForm::buttonBrowseFolder_Click);
			// 
			// labelStreamPoints
			// 
			this->labelStreamPoints->AutoSize = true;
			this->labelStreamPoints->Location = System::Drawing::Point(17, 60);
			this->labelStreamPoints->Name = L"labelStreamPoints";
			this->labelStreamPoints->Size = System::Drawing::Size(140, 13);
			this->labelStreamPoints->TabIndex = 3;
			this->labelStreamPoints->Text = L"Кадров в окне потока:";
			// 
			// numericStreamPoints
			// 
			this->numericStreamPoints->Location = System::Drawing::Point(120, 58);
			this->numericStreamPoints->Maximum = System::Decimal(gcnew cli::array<System::Int32>(4) { 100000, 0, 0, 0 });
			this->numericStreamPoints->Minimum = System::Decimal(gcnew cli::array<System::Int32>(4) { 10, 0, 0, 0 });
			this->numericStreamPoints->Name = L"numericStreamPoints";
			this->numericStreamPoints->Size = System::Drawing::Size(80, 20);
			this->numericStreamPoints->TabIndex = 4;
			this->numericStreamPoints->Value = System::Decimal(gcnew cli::array<System::Int32>(4) { 100, 0, 0, 0 });
			// 
			// buttonSave
			// 
			this->buttonSave->Location = System::Drawing::Point(180, 495);
			this->buttonSave->Name = L"buttonSave";
			this->buttonSave->Size = System::Drawing::Size(90, 28);
			this->buttonSave->TabIndex = 4;
			this->buttonSave->Text = L"Сохранить";
			this->buttonSave->UseVisualStyleBackColor = true;
			this->buttonSave->Click += gcnew System::EventHandler(this, &ConnectionForm::ButtonSave_Click);
			// 
			// buttonCancel
			// 
			this->buttonCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->buttonCancel->Location = System::Drawing::Point(282, 495);
			this->buttonCancel->Name = L"buttonCancel";
			this->buttonCancel->Size = System::Drawing::Size(90, 28);
			this->buttonCancel->TabIndex = 5;
			this->buttonCancel->Text = L"Отмена";
			this->buttonCancel->UseVisualStyleBackColor = true;
			this->buttonCancel->Click += gcnew System::EventHandler(this, &ConnectionForm::ButtonCancel_Click);
			// 
			// ConnectionForm
			// 
			this->AcceptButton = this->buttonSave;
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->CancelButton = this->buttonCancel;
			this->ClientSize = System::Drawing::Size(384, 540);
			this->Controls->Add(this->groupBoxData);
			this->Controls->Add(this->buttonCancel);
			this->Controls->Add(this->buttonSave);
			this->Controls->Add(this->groupBoxTimeouts);
			this->Controls->Add(this->groupBoxUDP);
			this->Controls->Add(this->groupBoxCOM);
			this->Controls->Add(this->groupBoxType);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"ConnectionForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
			this->Text = L"Настройки подключения к прибору";
			this->groupBoxType->ResumeLayout(false);
			this->groupBoxType->PerformLayout();
			this->groupBoxCOM->ResumeLayout(false);
			this->groupBoxCOM->PerformLayout();
			this->groupBoxUDP->ResumeLayout(false);
			this->groupBoxUDP->PerformLayout();
			this->groupBoxTimeouts->ResumeLayout(false);
			this->groupBoxTimeouts->PerformLayout();
			this->groupBoxData->ResumeLayout(false);
			this->groupBoxData->PerformLayout();
			this->ResumeLayout(false);

		}
#pragma endregion

	private:
		void RefreshPortList() {
			comboBoxPort->Items->Clear();
			cli::array<String^>^ ports = DeviceLink::GetPortNames();
			if (ports != nullptr && ports->Length > 0) {
				Array::Sort(ports);
				comboBoxPort->Items->AddRange(ports);
			}
			else {
				for (int i = 1; i <= 20; i++) {
					comboBoxPort->Items->Add(L"COM" + i.ToString());
				}
			}
		}

		void LoadCurrentConfig() {
			RefreshPortList();

			AppConfig& cfg = AppConfig::Instance();
			String^ savedPort = gcnew String(cfg.com_name.c_str());
			if (comboBoxPort->Items->Contains(savedPort)) {
				comboBoxPort->SelectedItem = savedPort;
			}
			else {
				comboBoxPort->Text = savedPort;
			}

			String^ savedBaud = cfg.baud_rate.ToString();
			if (comboBoxBaud->Items->Contains(savedBaud)) {
				comboBoxBaud->SelectedItem = savedBaud;
			}
			else {
				comboBoxBaud->Text = savedBaud;
			}

			textBoxIP->Text = gcnew String(cfg.wifi_ip.c_str());
			textBoxUDPPort->Text = cfg.wifi_port.ToString();
			textBoxReadTimeout->Text = cfg.read_timeout.ToString();
			textBoxWriteTimeout->Text = cfg.write_timeout.ToString();
			textBoxDataFolder->Text = gcnew String(cfg.data_folder.c_str());
			numericStreamPoints->Value = Math::Min(numericStreamPoints->Maximum,
				Math::Max(numericStreamPoints->Minimum, (Decimal)cfg.points));

			if (cfg.connection_type == CONN_UDP) {
				radioButtonUDP->Checked = true;
			}
			else {
				radioButtonCOM->Checked = true;
			}
			UpdateControlStates();
		}

		void UpdateControlStates() {
			bool isCOM = radioButtonCOM->Checked;
			groupBoxCOM->Enabled = isCOM;
			groupBoxUDP->Enabled = !isCOM;
		}

		System::Void ConnectionType_CheckedChanged(System::Object^ sender, System::EventArgs^ e) {
			UpdateControlStates();
		}

		System::Void checkBoxEmulator_CheckedChanged(System::Object^ sender, System::EventArgs^ e) {
			// Эмуляция: станция работает с em_probe.py на этом же ПК
			if (checkBoxEmulator->Checked) {
				radioButtonUDP->Checked = true;
				textBoxIP->Text = L"127.0.0.1";
			}
			else {
				textBoxIP->Text = L"192.168.4.1";
			}
		}

		System::Void ButtonRefreshPorts_Click(System::Object^ sender, System::EventArgs^ e) {
			String^ cur = comboBoxPort->Text;
			RefreshPortList();
			if (comboBoxPort->Items->Contains(cur)) {
				comboBoxPort->SelectedItem = cur;
			}
			else if (comboBoxPort->Items->Count > 0) {
				comboBoxPort->SelectedIndex = 0;
			}
		}

		System::Void ButtonSave_Click(System::Object^ sender, System::EventArgs^ e) {
			AppConfig& cfg = AppConfig::Instance();

			cfg.connection_type = radioButtonUDP->Checked ? CONN_UDP : CONN_COM;

			if (!String::IsNullOrEmpty(comboBoxPort->Text)) {
				msclr::interop::marshal_context ctx;
				cfg.com_name = ctx.marshal_as<std::string>(comboBoxPort->Text->Trim());
			}

			unsigned int bRate = 125000;
			if (UInt32::TryParse(comboBoxBaud->Text, bRate)) {
				cfg.baud_rate = bRate;
			}

			if (!String::IsNullOrEmpty(textBoxIP->Text)) {
				msclr::interop::marshal_context ctx;
				cfg.wifi_ip = ctx.marshal_as<std::string>(textBoxIP->Text->Trim());
			}

			unsigned int uPort = 5000;
			if (UInt32::TryParse(textBoxUDPPort->Text, uPort)) {
				cfg.wifi_port = uPort;
			}

			int rTimeout = 3000;
			if (Int32::TryParse(textBoxReadTimeout->Text, rTimeout)) {
				cfg.read_timeout = rTimeout;
			}

			int wTimeout = 3000;
			if (Int32::TryParse(textBoxWriteTimeout->Text, wTimeout)) {
				cfg.write_timeout = wTimeout;
			}

			if (!String::IsNullOrEmpty(textBoxDataFolder->Text)) {
				msclr::interop::marshal_context ctx;
				cfg.data_folder = ctx.marshal_as<std::string>(textBoxDataFolder->Text->Trim());
			}
			cfg.points = (unsigned int)numericStreamPoints->Value;

			cfg.Save();

			this->DialogResult = System::Windows::Forms::DialogResult::OK;
			this->Close();
		}

		System::Void buttonBrowseFolder_Click(System::Object^ sender, System::EventArgs^ e) {
			FolderBrowserDialog^ dlg = gcnew FolderBrowserDialog();
			dlg->Description = L"Папка для временного файла телеметрии";
			dlg->SelectedPath = textBoxDataFolder->Text;
			if (dlg->ShowDialog(this) == System::Windows::Forms::DialogResult::OK) {
				textBoxDataFolder->Text = dlg->SelectedPath;
			}
		}

		System::Void ButtonCancel_Click(System::Object^ sender, System::EventArgs^ e) {
			this->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->Close();
		}
	};

}
