#pragma once

namespace ELECTROMAGNETIC_TOOLS_3_0 {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Форма управления метрологическим файлом и калибровочными константами зонда
	/// Организация: НПФ «АМК Горизонт»
	/// Разработал: зам начальника ОП Смирнов С. Г.
	/// </summary>
	public ref class MetroForm : public System::Windows::Forms::Form
	{
	public:
		MetroForm(void)
		{
			InitializeComponent();
			InitDefaultValues();
		}

	protected:
		/// <summary>
		/// Освободить все используемые ресурсы.
		/// </summary>
		~MetroForm()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		System::ComponentModel::IContainer^ components;

		// Верхняя панель действий (Toolbar)
		System::Windows::Forms::Panel^ panelTopBar;
		System::Windows::Forms::Button^ buttonOpenFile;
		System::Windows::Forms::Button^ buttonSaveFile;
		System::Windows::Forms::Button^ buttonReadFromSonde;
		System::Windows::Forms::Button^ buttonWriteToSonde;
		System::Windows::Forms::CheckBox^ checkBoxReadOnly;
		System::Windows::Forms::Label^ labelStatusNotice;

		// Нижняя панель с кнопками
		System::Windows::Forms::Panel^ panelBottom;
		System::Windows::Forms::Button^ buttonApply;
		System::Windows::Forms::Button^ buttonClose;
		System::Windows::Forms::StatusStrip^ statusStripMetro;
		System::Windows::Forms::ToolStripStatusLabel^ statusLabelMetroFile;
		System::Windows::Forms::ToolStripStatusLabel^ statusLabelSondeType;

		// Вкладки параметров метрологии
		System::Windows::Forms::TabControl^ tabControlMetro;
		System::Windows::Forms::TabPage^ tabPagePassport;
		System::Windows::Forms::TabPage^ tabPageAirOffsets;
		System::Windows::Forms::TabPage^ tabPageThresholds;
		System::Windows::Forms::TabPage^ tabPageInclinometer;

		// Вкладка 1: Паспорт и геометрия
		System::Windows::Forms::GroupBox^ groupBoxPassport;
		System::Windows::Forms::Label^ labelSondeType;
		System::Windows::Forms::ComboBox^ comboBoxSondeType;
		System::Windows::Forms::Label^ labelSerial;
		System::Windows::Forms::TextBox^ textBoxSerial;
		System::Windows::Forms::Label^ labelSignature;
		System::Windows::Forms::TextBox^ textBoxSignature;
		System::Windows::Forms::Label^ labelDiameter;
		System::Windows::Forms::TextBox^ textBoxDiameter;
		System::Windows::Forms::Label^ labelFreq1;
		System::Windows::Forms::TextBox^ textBoxFreq1;
		System::Windows::Forms::Label^ labelFreq2;
		System::Windows::Forms::TextBox^ textBoxFreq2;

		System::Windows::Forms::GroupBox^ groupBoxGeometry;
		System::Windows::Forms::TableLayoutPanel^ tableLayoutGeometry;
		System::Windows::Forms::Label^ labelHdrTx;
		System::Windows::Forms::Label^ labelHdrL1;
		System::Windows::Forms::Label^ labelHdrL2;
		System::Windows::Forms::Label^ labelHdrLgeo;
		System::Windows::Forms::Label^ labelTx1;
		System::Windows::Forms::Label^ labelTx2;
		System::Windows::Forms::Label^ labelTx3;
		System::Windows::Forms::Label^ labelTx4;
		System::Windows::Forms::Label^ labelTx5;
		System::Windows::Forms::TextBox^ textBoxL1_T1;
		System::Windows::Forms::TextBox^ textBoxL1_T2;
		System::Windows::Forms::TextBox^ textBoxL1_T3;
		System::Windows::Forms::TextBox^ textBoxL1_T4;
		System::Windows::Forms::TextBox^ textBoxL1_T5;
		System::Windows::Forms::TextBox^ textBoxL2_T1;
		System::Windows::Forms::TextBox^ textBoxL2_T2;
		System::Windows::Forms::TextBox^ textBoxL2_T3;
		System::Windows::Forms::TextBox^ textBoxL2_T4;
		System::Windows::Forms::TextBox^ textBoxL2_T5;
		System::Windows::Forms::TextBox^ textBoxLgeo_T1;
		System::Windows::Forms::TextBox^ textBoxLgeo_T2;
		System::Windows::Forms::TextBox^ textBoxLgeo_T3;
		System::Windows::Forms::TextBox^ textBoxLgeo_T4;
		System::Windows::Forms::TextBox^ textBoxLgeo_T5;

		// Вкладка 2: Нули в воздухе (Air Offsets)
		System::Windows::Forms::GroupBox^ groupBoxAir400;
		System::Windows::Forms::TableLayoutPanel^ tableLayoutAir400;
		System::Windows::Forms::Label^ labelHdrAirTx1;
		System::Windows::Forms::Label^ labelHdrAirPh1;
		System::Windows::Forms::Label^ labelHdrAirAmp1;
		System::Windows::Forms::TextBox^ textBoxAirPh400_T1;
		System::Windows::Forms::TextBox^ textBoxAirPh400_T2;
		System::Windows::Forms::TextBox^ textBoxAirPh400_T3;
		System::Windows::Forms::TextBox^ textBoxAirPh400_T4;
		System::Windows::Forms::TextBox^ textBoxAirPh400_T5;
		System::Windows::Forms::TextBox^ textBoxAirAmp400_T1;
		System::Windows::Forms::TextBox^ textBoxAirAmp400_T2;
		System::Windows::Forms::TextBox^ textBoxAirAmp400_T3;
		System::Windows::Forms::TextBox^ textBoxAirAmp400_T4;
		System::Windows::Forms::TextBox^ textBoxAirAmp400_T5;

		System::Windows::Forms::GroupBox^ groupBoxAir2000;
		System::Windows::Forms::TableLayoutPanel^ tableLayoutAir2000;
		System::Windows::Forms::Label^ labelHdrAirTx2;
		System::Windows::Forms::Label^ labelHdrAirPh2;
		System::Windows::Forms::Label^ labelHdrAirAmp2;
		System::Windows::Forms::TextBox^ textBoxAirPh2000_T1;
		System::Windows::Forms::TextBox^ textBoxAirPh2000_T2;
		System::Windows::Forms::TextBox^ textBoxAirPh2000_T3;
		System::Windows::Forms::TextBox^ textBoxAirPh2000_T4;
		System::Windows::Forms::TextBox^ textBoxAirPh2000_T5;
		System::Windows::Forms::TextBox^ textBoxAirAmp2000_T1;
		System::Windows::Forms::TextBox^ textBoxAirAmp2000_T2;
		System::Windows::Forms::TextBox^ textBoxAirAmp2000_T3;
		System::Windows::Forms::TextBox^ textBoxAirAmp2000_T4;
		System::Windows::Forms::TextBox^ textBoxAirAmp2000_T5;

		// Вкладка 3: Пороги достоверности сигнала (min_amp)
		System::Windows::Forms::GroupBox^ groupBoxMinAmp;
		System::Windows::Forms::TableLayoutPanel^ tableLayoutMinAmp;
		System::Windows::Forms::Label^ labelHdrMinTx;
		System::Windows::Forms::Label^ labelHdrMin400;
		System::Windows::Forms::Label^ labelHdrMin2000;
		System::Windows::Forms::TextBox^ textBoxMin400_T1;
		System::Windows::Forms::TextBox^ textBoxMin400_T2;
		System::Windows::Forms::TextBox^ textBoxMin400_T3;
		System::Windows::Forms::TextBox^ textBoxMin400_T4;
		System::Windows::Forms::TextBox^ textBoxMin400_T5;
		System::Windows::Forms::TextBox^ textBoxMin2000_T1;
		System::Windows::Forms::TextBox^ textBoxMin2000_T2;
		System::Windows::Forms::TextBox^ textBoxMin2000_T3;
		System::Windows::Forms::TextBox^ textBoxMin2000_T4;
		System::Windows::Forms::TextBox^ textBoxMin2000_T5;

		// Вкладка 4: Инклинометрия и ротация
		System::Windows::Forms::GroupBox^ groupBoxIncliParams;
		System::Windows::Forms::Label^ labelMinRotSpeed;
		System::Windows::Forms::TextBox^ textBoxMinRotSpeed;
		System::Windows::Forms::Label^ labelRotDir;
		System::Windows::Forms::ComboBox^ comboBoxRotDir;
		System::Windows::Forms::Label^ labelAngleHistory;
		System::Windows::Forms::TextBox^ textBoxAngleHistory;
		System::Windows::Forms::Label^ labelFilterK;
		System::Windows::Forms::TextBox^ textBoxFilterK;

		// Диалоги
		System::Windows::Forms::OpenFileDialog^ openFileDialogMetro;
		System::Windows::Forms::SaveFileDialog^ saveFileDialogMetro;

#pragma region Windows Form Designer generated code
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->panelTopBar = (gcnew System::Windows::Forms::Panel());
			this->labelStatusNotice = (gcnew System::Windows::Forms::Label());
			this->checkBoxReadOnly = (gcnew System::Windows::Forms::CheckBox());
			this->buttonWriteToSonde = (gcnew System::Windows::Forms::Button());
			this->buttonReadFromSonde = (gcnew System::Windows::Forms::Button());
			this->buttonSaveFile = (gcnew System::Windows::Forms::Button());
			this->buttonOpenFile = (gcnew System::Windows::Forms::Button());
			this->panelBottom = (gcnew System::Windows::Forms::Panel());
			this->buttonClose = (gcnew System::Windows::Forms::Button());
			this->buttonApply = (gcnew System::Windows::Forms::Button());
			this->statusStripMetro = (gcnew System::Windows::Forms::StatusStrip());
			this->statusLabelMetroFile = (gcnew System::Windows::Forms::ToolStripStatusLabel());
			this->statusLabelSondeType = (gcnew System::Windows::Forms::ToolStripStatusLabel());
			this->tabControlMetro = (gcnew System::Windows::Forms::TabControl());
			this->tabPagePassport = (gcnew System::Windows::Forms::TabPage());
			this->groupBoxGeometry = (gcnew System::Windows::Forms::GroupBox());
			this->tableLayoutGeometry = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->labelHdrTx = (gcnew System::Windows::Forms::Label());
			this->labelHdrL1 = (gcnew System::Windows::Forms::Label());
			this->labelHdrL2 = (gcnew System::Windows::Forms::Label());
			this->labelHdrLgeo = (gcnew System::Windows::Forms::Label());
			this->labelTx1 = (gcnew System::Windows::Forms::Label());
			this->labelTx2 = (gcnew System::Windows::Forms::Label());
			this->labelTx3 = (gcnew System::Windows::Forms::Label());
			this->labelTx4 = (gcnew System::Windows::Forms::Label());
			this->labelTx5 = (gcnew System::Windows::Forms::Label());
			this->textBoxL1_T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL1_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL1_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL1_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL1_T5 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL2_T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL2_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL2_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL2_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxL2_T5 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxLgeo_T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxLgeo_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxLgeo_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxLgeo_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxLgeo_T5 = (gcnew System::Windows::Forms::TextBox());
			this->groupBoxPassport = (gcnew System::Windows::Forms::GroupBox());
			this->textBoxFreq2 = (gcnew System::Windows::Forms::TextBox());
			this->labelFreq2 = (gcnew System::Windows::Forms::Label());
			this->textBoxFreq1 = (gcnew System::Windows::Forms::TextBox());
			this->labelFreq1 = (gcnew System::Windows::Forms::Label());
			this->textBoxDiameter = (gcnew System::Windows::Forms::TextBox());
			this->labelDiameter = (gcnew System::Windows::Forms::Label());
			this->textBoxSignature = (gcnew System::Windows::Forms::TextBox());
			this->labelSignature = (gcnew System::Windows::Forms::Label());
			this->textBoxSerial = (gcnew System::Windows::Forms::TextBox());
			this->labelSerial = (gcnew System::Windows::Forms::Label());
			this->comboBoxSondeType = (gcnew System::Windows::Forms::ComboBox());
			this->labelSondeType = (gcnew System::Windows::Forms::Label());
			this->tabPageAirOffsets = (gcnew System::Windows::Forms::TabPage());
			this->groupBoxAir2000 = (gcnew System::Windows::Forms::GroupBox());
			this->tableLayoutAir2000 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->labelHdrAirTx2 = (gcnew System::Windows::Forms::Label());
			this->labelHdrAirPh2 = (gcnew System::Windows::Forms::Label());
			this->labelHdrAirAmp2 = (gcnew System::Windows::Forms::Label());
			this->textBoxAirPh2000_T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirPh2000_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirPh2000_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirPh2000_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirPh2000_T5 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp2000_T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp2000_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp2000_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp2000_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp2000_T5 = (gcnew System::Windows::Forms::TextBox());
			this->groupBoxAir400 = (gcnew System::Windows::Forms::GroupBox());
			this->tableLayoutAir400 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->labelHdrAirTx1 = (gcnew System::Windows::Forms::Label());
			this->labelHdrAirPh1 = (gcnew System::Windows::Forms::Label());
			this->labelHdrAirAmp1 = (gcnew System::Windows::Forms::Label());
			this->textBoxAirPh400_T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirPh400_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirPh400_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirPh400_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirPh400_T5 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp400_T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp400_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp400_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp400_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxAirAmp400_T5 = (gcnew System::Windows::Forms::TextBox());
			this->tabPageThresholds = (gcnew System::Windows::Forms::TabPage());
			this->groupBoxMinAmp = (gcnew System::Windows::Forms::GroupBox());
			this->tableLayoutMinAmp = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->labelHdrMinTx = (gcnew System::Windows::Forms::Label());
			this->labelHdrMin400 = (gcnew System::Windows::Forms::Label());
			this->labelHdrMin2000 = (gcnew System::Windows::Forms::Label());
			this->textBoxMin400_T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMin400_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMin400_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMin400_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMin400_T5 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMin2000_T1 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMin2000_T2 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMin2000_T3 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMin2000_T4 = (gcnew System::Windows::Forms::TextBox());
			this->textBoxMin2000_T5 = (gcnew System::Windows::Forms::TextBox());
			this->tabPageInclinometer = (gcnew System::Windows::Forms::TabPage());
			this->groupBoxIncliParams = (gcnew System::Windows::Forms::GroupBox());
			this->textBoxFilterK = (gcnew System::Windows::Forms::TextBox());
			this->labelFilterK = (gcnew System::Windows::Forms::Label());
			this->textBoxAngleHistory = (gcnew System::Windows::Forms::TextBox());
			this->labelAngleHistory = (gcnew System::Windows::Forms::Label());
			this->comboBoxRotDir = (gcnew System::Windows::Forms::ComboBox());
			this->labelRotDir = (gcnew System::Windows::Forms::Label());
			this->textBoxMinRotSpeed = (gcnew System::Windows::Forms::TextBox());
			this->labelMinRotSpeed = (gcnew System::Windows::Forms::Label());
			this->openFileDialogMetro = (gcnew System::Windows::Forms::OpenFileDialog());
			this->saveFileDialogMetro = (gcnew System::Windows::Forms::SaveFileDialog());
			this->panelTopBar->SuspendLayout();
			this->panelBottom->SuspendLayout();
			this->statusStripMetro->SuspendLayout();
			this->tabControlMetro->SuspendLayout();
			this->tabPagePassport->SuspendLayout();
			this->groupBoxGeometry->SuspendLayout();
			this->tableLayoutGeometry->SuspendLayout();
			this->groupBoxPassport->SuspendLayout();
			this->tabPageAirOffsets->SuspendLayout();
			this->groupBoxAir2000->SuspendLayout();
			this->tableLayoutAir2000->SuspendLayout();
			this->groupBoxAir400->SuspendLayout();
			this->tableLayoutAir400->SuspendLayout();
			this->tabPageThresholds->SuspendLayout();
			this->groupBoxMinAmp->SuspendLayout();
			this->tableLayoutMinAmp->SuspendLayout();
			this->tabPageInclinometer->SuspendLayout();
			this->groupBoxIncliParams->SuspendLayout();
			this->SuspendLayout();
			// 
			// panelTopBar
			// 
			this->panelTopBar->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(245)), static_cast<System::Int32>(static_cast<System::Byte>(247)),
				static_cast<System::Int32>(static_cast<System::Byte>(250)));
			this->panelTopBar->Controls->Add(this->labelStatusNotice);
			this->panelTopBar->Controls->Add(this->checkBoxReadOnly);
			this->panelTopBar->Controls->Add(this->buttonWriteToSonde);
			this->panelTopBar->Controls->Add(this->buttonReadFromSonde);
			this->panelTopBar->Controls->Add(this->buttonSaveFile);
			this->panelTopBar->Controls->Add(this->buttonOpenFile);
			this->panelTopBar->Dock = System::Windows::Forms::DockStyle::Top;
			this->panelTopBar->Location = System::Drawing::Point(0, 0);
			this->panelTopBar->Name = L"panelTopBar";
			this->panelTopBar->Padding = System::Windows::Forms::Padding(8);
			this->panelTopBar->Size = System::Drawing::Size(884, 52);
			this->panelTopBar->TabIndex = 0;
			// 
			// labelStatusNotice
			// 
			this->labelStatusNotice->AutoSize = true;
			this->labelStatusNotice->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Italic));
			this->labelStatusNotice->ForeColor = System::Drawing::Color::DimGray;
			this->labelStatusNotice->Location = System::Drawing::Point(680, 18);
			this->labelStatusNotice->Name = L"labelStatusNotice";
			this->labelStatusNotice->Size = System::Drawing::Size(180, 15);
			this->labelStatusNotice->TabIndex = 5;
			this->labelStatusNotice->Text = L"Данные загружены из шаблона";
			// 
			// checkBoxReadOnly
			// 
			this->checkBoxReadOnly->AutoSize = true;
			this->checkBoxReadOnly->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->checkBoxReadOnly->Location = System::Drawing::Point(555, 17);
			this->checkBoxReadOnly->Name = L"checkBoxReadOnly";
			this->checkBoxReadOnly->Size = System::Drawing::Size(109, 19);
			this->checkBoxReadOnly->TabIndex = 4;
			this->checkBoxReadOnly->Text = L"Только чтение";
			this->checkBoxReadOnly->UseVisualStyleBackColor = true;
			this->checkBoxReadOnly->CheckedChanged += gcnew System::EventHandler(this, &MetroForm::checkBoxReadOnly_CheckedChanged);
			// 
			// buttonWriteToSonde
			// 
			this->buttonWriteToSonde->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->buttonWriteToSonde->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(160)),
				static_cast<System::Int32>(static_cast<System::Byte>(30)), static_cast<System::Int32>(static_cast<System::Byte>(30)));
			this->buttonWriteToSonde->Location = System::Drawing::Point(408, 10);
			this->buttonWriteToSonde->Name = L"buttonWriteToSonde";
			this->buttonWriteToSonde->Size = System::Drawing::Size(130, 32);
			this->buttonWriteToSonde->TabIndex = 3;
			this->buttonWriteToSonde->Text = L"Записать в зонд";
			this->buttonWriteToSonde->UseVisualStyleBackColor = true;
			this->buttonWriteToSonde->Click += gcnew System::EventHandler(this, &MetroForm::buttonWriteToSonde_Click);
			// 
			// buttonReadFromSonde
			// 
			this->buttonReadFromSonde->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->buttonReadFromSonde->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(20)),
				static_cast<System::Int32>(static_cast<System::Byte>(80)), static_cast<System::Int32>(static_cast<System::Byte>(160)));
			this->buttonReadFromSonde->Location = System::Drawing::Point(268, 10);
			this->buttonReadFromSonde->Name = L"buttonReadFromSonde";
			this->buttonReadFromSonde->Size = System::Drawing::Size(134, 32);
			this->buttonReadFromSonde->TabIndex = 2;
			this->buttonReadFromSonde->Text = L"Прочитать из зонда";
			this->buttonReadFromSonde->UseVisualStyleBackColor = true;
			this->buttonReadFromSonde->Click += gcnew System::EventHandler(this, &MetroForm::buttonReadFromSonde_Click);
			// 
			// buttonSaveFile
			// 
			this->buttonSaveFile->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->buttonSaveFile->Location = System::Drawing::Point(135, 10);
			this->buttonSaveFile->Name = L"buttonSaveFile";
			this->buttonSaveFile->Size = System::Drawing::Size(124, 32);
			this->buttonSaveFile->TabIndex = 1;
			this->buttonSaveFile->Text = L"Сохранить в файл...";
			this->buttonSaveFile->UseVisualStyleBackColor = true;
			this->buttonSaveFile->Click += gcnew System::EventHandler(this, &MetroForm::buttonSaveFile_Click);
			// 
			// buttonOpenFile
			// 
			this->buttonOpenFile->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->buttonOpenFile->Location = System::Drawing::Point(12, 10);
			this->buttonOpenFile->Name = L"buttonOpenFile";
			this->buttonOpenFile->Size = System::Drawing::Size(117, 32);
			this->buttonOpenFile->TabIndex = 0;
			this->buttonOpenFile->Text = L"Открыть файл...";
			this->buttonOpenFile->UseVisualStyleBackColor = true;
			this->buttonOpenFile->Click += gcnew System::EventHandler(this, &MetroForm::buttonOpenFile_Click);
			// 
			// panelBottom
			// 
			this->panelBottom->Controls->Add(this->buttonClose);
			this->panelBottom->Controls->Add(this->buttonApply);
			this->panelBottom->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->panelBottom->Location = System::Drawing::Point(0, 566);
			this->panelBottom->Name = L"panelBottom";
			this->panelBottom->Size = System::Drawing::Size(884, 46);
			this->panelBottom->TabIndex = 1;
			// 
			// buttonClose
			// 
			this->buttonClose->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->buttonClose->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->buttonClose->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->buttonClose->Location = System::Drawing::Point(768, 8);
			this->buttonClose->Name = L"buttonClose";
			this->buttonClose->Size = System::Drawing::Size(104, 30);
			this->buttonClose->TabIndex = 1;
			this->buttonClose->Text = L"Закрыть";
			this->buttonClose->UseVisualStyleBackColor = true;
			this->buttonClose->Click += gcnew System::EventHandler(this, &MetroForm::buttonClose_Click);
			// 
			// buttonApply
			// 
			this->buttonApply->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->buttonApply->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->buttonApply->Location = System::Drawing::Point(642, 8);
			this->buttonApply->Name = L"buttonApply";
			this->buttonApply->Size = System::Drawing::Size(120, 30);
			this->buttonApply->TabIndex = 0;
			this->buttonApply->Text = L"Применить";
			this->buttonApply->UseVisualStyleBackColor = true;
			this->buttonApply->Click += gcnew System::EventHandler(this, &MetroForm::buttonApply_Click);
			// 
			// statusStripMetro
			// 
			this->statusStripMetro->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->statusStripMetro->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {
				this->statusLabelMetroFile,
				this->statusLabelSondeType
			});
			this->statusStripMetro->Location = System::Drawing::Point(0, 612);
			this->statusStripMetro->Name = L"statusStripMetro";
			this->statusStripMetro->Size = System::Drawing::Size(884, 22);
			this->statusStripMetro->TabIndex = 2;
			this->statusStripMetro->Text = L"statusStripMetro";
			// 
			// statusLabelMetroFile
			// 
			this->statusLabelMetroFile->Name = L"statusLabelMetroFile";
			this->statusLabelMetroFile->Size = System::Drawing::Size(635, 17);
			this->statusLabelMetroFile->Spring = true;
			this->statusLabelMetroFile->Text = L"Файл метрологии: По умолчанию (Cartograph 120)";
			this->statusLabelMetroFile->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// statusLabelSondeType
			// 
			this->statusLabelSondeType->Name = L"statusLabelSondeType";
			this->statusLabelSondeType->Size = System::Drawing::Size(234, 17);
			this->statusLabelSondeType->Text = L"Тип зонда: 351 (Cartograph 120 5TX)";
			// 
			// tabControlMetro
			// 
			this->tabControlMetro->Controls->Add(this->tabPagePassport);
			this->tabControlMetro->Controls->Add(this->tabPageAirOffsets);
			this->tabControlMetro->Controls->Add(this->tabPageThresholds);
			this->tabControlMetro->Controls->Add(this->tabPageInclinometer);
			this->tabControlMetro->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tabControlMetro->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F));
			this->tabControlMetro->Location = System::Drawing::Point(0, 52);
			this->tabControlMetro->Name = L"tabControlMetro";
			this->tabControlMetro->SelectedIndex = 0;
			this->tabControlMetro->Size = System::Drawing::Size(884, 514);
			this->tabControlMetro->TabIndex = 3;
			// 
			// tabPagePassport
			// 
			this->tabPagePassport->Controls->Add(this->groupBoxGeometry);
			this->tabPagePassport->Controls->Add(this->groupBoxPassport);
			this->tabPagePassport->Location = System::Drawing::Point(4, 26);
			this->tabPagePassport->Name = L"tabPagePassport";
			this->tabPagePassport->Padding = System::Windows::Forms::Padding(8);
			this->tabPagePassport->Size = System::Drawing::Size(876, 484);
			this->tabPagePassport->TabIndex = 0;
			this->tabPagePassport->Text = L"Паспорт и Геометрия";
			this->tabPagePassport->UseVisualStyleBackColor = true;
			// 
			// groupBoxGeometry
			// 
			this->groupBoxGeometry->Controls->Add(this->tableLayoutGeometry);
			this->groupBoxGeometry->Dock = System::Windows::Forms::DockStyle::Fill;
			this->groupBoxGeometry->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F, System::Drawing::FontStyle::Bold));
			this->groupBoxGeometry->Location = System::Drawing::Point(8, 140);
			this->groupBoxGeometry->Name = L"groupBoxGeometry";
			this->groupBoxGeometry->Padding = System::Windows::Forms::Padding(8);
			this->groupBoxGeometry->Size = System::Drawing::Size(860, 336);
			this->groupBoxGeometry->TabIndex = 1;
			this->groupBoxGeometry->TabStop = false;
			this->groupBoxGeometry->Text = L"Геометрия зондовых баз (мм)";
			// 
			// tableLayoutGeometry
			// 
			this->tableLayoutGeometry->ColumnCount = 4;
			this->tableLayoutGeometry->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				120)));
			this->tableLayoutGeometry->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				33.33333F)));
			this->tableLayoutGeometry->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				33.33333F)));
			this->tableLayoutGeometry->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				33.33333F)));
			this->tableLayoutGeometry->Controls->Add(this->labelHdrTx, 0, 0);
			this->tableLayoutGeometry->Controls->Add(this->labelHdrL1, 1, 0);
			this->tableLayoutGeometry->Controls->Add(this->labelHdrL2, 2, 0);
			this->tableLayoutGeometry->Controls->Add(this->labelHdrLgeo, 3, 0);
			this->tableLayoutGeometry->Controls->Add(this->labelTx1, 0, 1);
			this->tableLayoutGeometry->Controls->Add(this->labelTx2, 0, 2);
			this->tableLayoutGeometry->Controls->Add(this->labelTx3, 0, 3);
			this->tableLayoutGeometry->Controls->Add(this->labelTx4, 0, 4);
			this->tableLayoutGeometry->Controls->Add(this->labelTx5, 0, 5);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL1_T1, 1, 1);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL1_T2, 1, 2);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL1_T3, 1, 3);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL1_T4, 1, 4);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL1_T5, 1, 5);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL2_T1, 2, 1);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL2_T2, 2, 2);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL2_T3, 2, 3);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL2_T4, 2, 4);
			this->tableLayoutGeometry->Controls->Add(this->textBoxL2_T5, 2, 5);
			this->tableLayoutGeometry->Controls->Add(this->textBoxLgeo_T1, 3, 1);
			this->tableLayoutGeometry->Controls->Add(this->textBoxLgeo_T2, 3, 2);
			this->tableLayoutGeometry->Controls->Add(this->textBoxLgeo_T3, 3, 3);
			this->tableLayoutGeometry->Controls->Add(this->textBoxLgeo_T4, 3, 4);
			this->tableLayoutGeometry->Controls->Add(this->textBoxLgeo_T5, 3, 5);
			this->tableLayoutGeometry->Dock = System::Windows::Forms::DockStyle::Top;
			this->tableLayoutGeometry->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->tableLayoutGeometry->Location = System::Drawing::Point(8, 25);
			this->tableLayoutGeometry->Name = L"tableLayoutGeometry";
			this->tableLayoutGeometry->RowCount = 6;
			this->tableLayoutGeometry->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutGeometry->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutGeometry->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutGeometry->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutGeometry->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutGeometry->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutGeometry->Size = System::Drawing::Size(844, 200);
			this->tableLayoutGeometry->TabIndex = 0;
			// 
			// labelHdrTx
			// 
			this->labelHdrTx->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrTx->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelHdrTx->Location = System::Drawing::Point(3, 0);
			this->labelHdrTx->Name = L"labelHdrTx";
			this->labelHdrTx->Size = System::Drawing::Size(114, 32);
			this->labelHdrTx->TabIndex = 0;
			this->labelHdrTx->Text = L"Генератор";
			this->labelHdrTx->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// labelHdrL1
			// 
			this->labelHdrL1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrL1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelHdrL1->Location = System::Drawing::Point(123, 0);
			this->labelHdrL1->Name = L"labelHdrL1";
			this->labelHdrL1->Size = System::Drawing::Size(235, 32);
			this->labelHdrL1->TabIndex = 1;
			this->labelHdrL1->Text = L"L1 (Tx - Rx1, мм)";
			this->labelHdrL1->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// labelHdrL2
			// 
			this->labelHdrL2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrL2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelHdrL2->Location = System::Drawing::Point(364, 0);
			this->labelHdrL2->Name = L"labelHdrL2";
			this->labelHdrL2->Size = System::Drawing::Size(235, 32);
			this->labelHdrL2->TabIndex = 2;
			this->labelHdrL2->Text = L"L2 (Tx - Rx2, мм)";
			this->labelHdrL2->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// labelHdrLgeo
			// 
			this->labelHdrLgeo->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrLgeo->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelHdrLgeo->Location = System::Drawing::Point(605, 0);
			this->labelHdrLgeo->Name = L"labelHdrLgeo";
			this->labelHdrLgeo->Size = System::Drawing::Size(236, 32);
			this->labelHdrLgeo->TabIndex = 3;
			this->labelHdrLgeo->Text = L"Lgeo (Геом. база, мм)";
			this->labelHdrLgeo->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// labelTx1
			// 
			this->labelTx1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelTx1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelTx1->Location = System::Drawing::Point(3, 32);
			this->labelTx1->Name = L"labelTx1";
			this->labelTx1->Size = System::Drawing::Size(114, 32);
			this->labelTx1->TabIndex = 4;
			this->labelTx1->Text = L"Tx1";
			this->labelTx1->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// labelTx2
			// 
			this->labelTx2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelTx2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelTx2->Location = System::Drawing::Point(3, 64);
			this->labelTx2->Name = L"labelTx2";
			this->labelTx2->Size = System::Drawing::Size(114, 32);
			this->labelTx2->TabIndex = 5;
			this->labelTx2->Text = L"Tx2";
			this->labelTx2->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// labelTx3
			// 
			this->labelTx3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelTx3->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelTx3->Location = System::Drawing::Point(3, 96);
			this->labelTx3->Name = L"labelTx3";
			this->labelTx3->Size = System::Drawing::Size(114, 32);
			this->labelTx3->TabIndex = 6;
			this->labelTx3->Text = L"Tx3";
			this->labelTx3->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// labelTx4
			// 
			this->labelTx4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelTx4->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelTx4->Location = System::Drawing::Point(3, 128);
			this->labelTx4->Name = L"labelTx4";
			this->labelTx4->Size = System::Drawing::Size(114, 32);
			this->labelTx4->TabIndex = 7;
			this->labelTx4->Text = L"Tx4";
			this->labelTx4->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// labelTx5
			// 
			this->labelTx5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelTx5->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelTx5->Location = System::Drawing::Point(3, 160);
			this->labelTx5->Name = L"labelTx5";
			this->labelTx5->Size = System::Drawing::Size(114, 40);
			this->labelTx5->TabIndex = 8;
			this->labelTx5->Text = L"Tx5";
			this->labelTx5->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// textBoxL1_T1
			// 
			this->textBoxL1_T1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL1_T1->Location = System::Drawing::Point(123, 35);
			this->textBoxL1_T1->Name = L"textBoxL1_T1";
			this->textBoxL1_T1->Size = System::Drawing::Size(235, 23);
			this->textBoxL1_T1->TabIndex = 9;
			this->textBoxL1_T1->Text = L"365";
			this->textBoxL1_T1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxL1_T2
			// 
			this->textBoxL1_T2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL1_T2->Location = System::Drawing::Point(123, 67);
			this->textBoxL1_T2->Name = L"textBoxL1_T2";
			this->textBoxL1_T2->Size = System::Drawing::Size(235, 23);
			this->textBoxL1_T2->TabIndex = 10;
			this->textBoxL1_T2->Text = L"710";
			this->textBoxL1_T2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxL1_T3
			// 
			this->textBoxL1_T3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL1_T3->Location = System::Drawing::Point(123, 99);
			this->textBoxL1_T3->Name = L"textBoxL1_T3";
			this->textBoxL1_T3->Size = System::Drawing::Size(235, 23);
			this->textBoxL1_T3->TabIndex = 11;
			this->textBoxL1_T3->Text = L"1070";
			this->textBoxL1_T3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxL1_T4
			// 
			this->textBoxL1_T4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL1_T4->Location = System::Drawing::Point(123, 131);
			this->textBoxL1_T4->Name = L"textBoxL1_T4";
			this->textBoxL1_T4->Size = System::Drawing::Size(235, 23);
			this->textBoxL1_T4->TabIndex = 12;
			this->textBoxL1_T4->Text = L"1415";
			this->textBoxL1_T4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxL1_T5
			// 
			this->textBoxL1_T5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL1_T5->Location = System::Drawing::Point(123, 163);
			this->textBoxL1_T5->Name = L"textBoxL1_T5";
			this->textBoxL1_T5->Size = System::Drawing::Size(235, 23);
			this->textBoxL1_T5->TabIndex = 13;
			this->textBoxL1_T5->Text = L"1768";
			this->textBoxL1_T5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxL2_T1
			// 
			this->textBoxL2_T1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL2_T1->Location = System::Drawing::Point(364, 35);
			this->textBoxL2_T1->Name = L"textBoxL2_T1";
			this->textBoxL2_T1->Size = System::Drawing::Size(235, 23);
			this->textBoxL2_T1->TabIndex = 14;
			this->textBoxL2_T1->Text = L"635";
			this->textBoxL2_T1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxL2_T2
			// 
			this->textBoxL2_T2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL2_T2->Location = System::Drawing::Point(364, 67);
			this->textBoxL2_T2->Name = L"textBoxL2_T2";
			this->textBoxL2_T2->Size = System::Drawing::Size(235, 23);
			this->textBoxL2_T2->TabIndex = 15;
			this->textBoxL2_T2->Text = L"980";
			this->textBoxL2_T2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxL2_T3
			// 
			this->textBoxL2_T3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL2_T3->Location = System::Drawing::Point(364, 99);
			this->textBoxL2_T3->Name = L"textBoxL2_T3";
			this->textBoxL2_T3->Size = System::Drawing::Size(235, 23);
			this->textBoxL2_T3->TabIndex = 16;
			this->textBoxL2_T3->Text = L"1340";
			this->textBoxL2_T3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxL2_T4
			// 
			this->textBoxL2_T4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL2_T4->Location = System::Drawing::Point(364, 131);
			this->textBoxL2_T4->Name = L"textBoxL2_T4";
			this->textBoxL2_T4->Size = System::Drawing::Size(235, 23);
			this->textBoxL2_T4->TabIndex = 17;
			this->textBoxL2_T4->Text = L"1685";
			this->textBoxL2_T4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxL2_T5
			// 
			this->textBoxL2_T5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxL2_T5->Location = System::Drawing::Point(364, 163);
			this->textBoxL2_T5->Name = L"textBoxL2_T5";
			this->textBoxL2_T5->Size = System::Drawing::Size(235, 23);
			this->textBoxL2_T5->TabIndex = 18;
			this->textBoxL2_T5->Text = L"2038";
			this->textBoxL2_T5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxLgeo_T1
			// 
			this->textBoxLgeo_T1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxLgeo_T1->Location = System::Drawing::Point(605, 35);
			this->textBoxLgeo_T1->Name = L"textBoxLgeo_T1";
			this->textBoxLgeo_T1->Size = System::Drawing::Size(236, 23);
			this->textBoxLgeo_T1->TabIndex = 19;
			this->textBoxLgeo_T1->Text = L"500";
			this->textBoxLgeo_T1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxLgeo_T2
			// 
			this->textBoxLgeo_T2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxLgeo_T2->Location = System::Drawing::Point(605, 67);
			this->textBoxLgeo_T2->Name = L"textBoxLgeo_T2";
			this->textBoxLgeo_T2->Size = System::Drawing::Size(236, 23);
			this->textBoxLgeo_T2->TabIndex = 20;
			this->textBoxLgeo_T2->Text = L"845";
			this->textBoxLgeo_T2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxLgeo_T3
			// 
			this->textBoxLgeo_T3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxLgeo_T3->Location = System::Drawing::Point(605, 99);
			this->textBoxLgeo_T3->Name = L"textBoxLgeo_T3";
			this->textBoxLgeo_T3->Size = System::Drawing::Size(236, 23);
			this->textBoxLgeo_T3->TabIndex = 21;
			this->textBoxLgeo_T3->Text = L"1205";
			this->textBoxLgeo_T3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxLgeo_T4
			// 
			this->textBoxLgeo_T4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxLgeo_T4->Location = System::Drawing::Point(605, 131);
			this->textBoxLgeo_T4->Name = L"textBoxLgeo_T4";
			this->textBoxLgeo_T4->Size = System::Drawing::Size(236, 23);
			this->textBoxLgeo_T4->TabIndex = 22;
			this->textBoxLgeo_T4->Text = L"1550";
			this->textBoxLgeo_T4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxLgeo_T5
			// 
			this->textBoxLgeo_T5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxLgeo_T5->Location = System::Drawing::Point(605, 163);
			this->textBoxLgeo_T5->Name = L"textBoxLgeo_T5";
			this->textBoxLgeo_T5->Size = System::Drawing::Size(236, 23);
			this->textBoxLgeo_T5->TabIndex = 23;
			this->textBoxLgeo_T5->Text = L"1903";
			this->textBoxLgeo_T5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// groupBoxPassport
			// 
			this->groupBoxPassport->Controls->Add(this->textBoxFreq2);
			this->groupBoxPassport->Controls->Add(this->labelFreq2);
			this->groupBoxPassport->Controls->Add(this->textBoxFreq1);
			this->groupBoxPassport->Controls->Add(this->labelFreq1);
			this->groupBoxPassport->Controls->Add(this->textBoxDiameter);
			this->groupBoxPassport->Controls->Add(this->labelDiameter);
			this->groupBoxPassport->Controls->Add(this->textBoxSignature);
			this->groupBoxPassport->Controls->Add(this->labelSignature);
			this->groupBoxPassport->Controls->Add(this->textBoxSerial);
			this->groupBoxPassport->Controls->Add(this->labelSerial);
			this->groupBoxPassport->Controls->Add(this->comboBoxSondeType);
			this->groupBoxPassport->Controls->Add(this->labelSondeType);
			this->groupBoxPassport->Dock = System::Windows::Forms::DockStyle::Top;
			this->groupBoxPassport->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F, System::Drawing::FontStyle::Bold));
			this->groupBoxPassport->Location = System::Drawing::Point(8, 8);
			this->groupBoxPassport->Name = L"groupBoxPassport";
			this->groupBoxPassport->Size = System::Drawing::Size(860, 132);
			this->groupBoxPassport->TabIndex = 0;
			this->groupBoxPassport->TabStop = false;
			this->groupBoxPassport->Text = L"Паспортные данные зонда";
			// 
			// textBoxFreq2
			// 
			this->textBoxFreq2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->textBoxFreq2->Location = System::Drawing::Point(670, 88);
			this->textBoxFreq2->Name = L"textBoxFreq2";
			this->textBoxFreq2->Size = System::Drawing::Size(120, 23);
			this->textBoxFreq2->TabIndex = 11;
			this->textBoxFreq2->Text = L"2000";
			// 
			// labelFreq2
			// 
			this->labelFreq2->AutoSize = true;
			this->labelFreq2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelFreq2->Location = System::Drawing::Point(580, 91);
			this->labelFreq2->Name = L"labelFreq2";
			this->labelFreq2->Size = System::Drawing::Size(76, 15);
			this->labelFreq2->TabIndex = 10;
			this->labelFreq2->Text = L"Частота 2 (кГц):";
			// 
			// textBoxFreq1
			// 
			this->textBoxFreq1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->textBoxFreq1->Location = System::Drawing::Point(430, 88);
			this->textBoxFreq1->Name = L"textBoxFreq1";
			this->textBoxFreq1->Size = System::Drawing::Size(120, 23);
			this->textBoxFreq1->TabIndex = 9;
			this->textBoxFreq1->Text = L"400";
			// 
			// labelFreq1
			// 
			this->labelFreq1->AutoSize = true;
			this->labelFreq1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelFreq1->Location = System::Drawing::Point(340, 91);
			this->labelFreq1->Name = L"labelFreq1";
			this->labelFreq1->Size = System::Drawing::Size(76, 15);
			this->labelFreq1->TabIndex = 8;
			this->labelFreq1->Text = L"Частота 1 (кГц):";
			// 
			// textBoxDiameter
			// 
			this->textBoxDiameter->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->textBoxDiameter->Location = System::Drawing::Point(140, 88);
			this->textBoxDiameter->Name = L"textBoxDiameter";
			this->textBoxDiameter->Size = System::Drawing::Size(150, 23);
			this->textBoxDiameter->TabIndex = 7;
			this->textBoxDiameter->Text = L"120";
			// 
			// labelDiameter
			// 
			this->labelDiameter->AutoSize = true;
			this->labelDiameter->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelDiameter->Location = System::Drawing::Point(20, 91);
			this->labelDiameter->Name = L"labelDiameter";
			this->labelDiameter->Size = System::Drawing::Size(100, 15);
			this->labelDiameter->TabIndex = 6;
			this->labelDiameter->Text = L"Диаметр корп. (мм):";
			// 
			// textBoxSignature
			// 
			this->textBoxSignature->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->textBoxSignature->Location = System::Drawing::Point(670, 42);
			this->textBoxSignature->Name = L"textBoxSignature";
			this->textBoxSignature->Size = System::Drawing::Size(120, 23);
			this->textBoxSignature->TabIndex = 5;
			this->textBoxSignature->Text = L"0x24DD9";
			// 
			// labelSignature
			// 
			this->labelSignature->AutoSize = true;
			this->labelSignature->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelSignature->Location = System::Drawing::Point(580, 45);
			this->labelSignature->Name = L"labelSignature";
			this->labelSignature->Size = System::Drawing::Size(75, 15);
			this->labelSignature->TabIndex = 4;
			this->labelSignature->Text = L"Сигнатура:";
			// 
			// textBoxSerial
			// 
			this->textBoxSerial->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->textBoxSerial->Location = System::Drawing::Point(430, 42);
			this->textBoxSerial->Name = L"textBoxSerial";
			this->textBoxSerial->Size = System::Drawing::Size(120, 23);
			this->textBoxSerial->TabIndex = 3;
			this->textBoxSerial->Text = L"12026";
			// 
			// labelSerial
			// 
			this->labelSerial->AutoSize = true;
			this->labelSerial->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelSerial->Location = System::Drawing::Point(340, 45);
			this->labelSerial->Name = L"labelSerial";
			this->labelSerial->Size = System::Drawing::Size(76, 15);
			this->labelSerial->TabIndex = 2;
			this->labelSerial->Text = L"Заводской №:";
			// 
			// comboBoxSondeType
			// 
			this->comboBoxSondeType->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxSondeType->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->comboBoxSondeType->FormattingEnabled = true;
			this->comboBoxSondeType->Items->AddRange(gcnew cli::array< System::Object^  >(3) {
				L"351: Cartograph 120 (5TX)",
				L"350: Cartograph 120 (4TX)",
				L"352: Cartograph Custom"
			});
			this->comboBoxSondeType->Location = System::Drawing::Point(140, 42);
			this->comboBoxSondeType->Name = L"comboBoxSondeType";
			this->comboBoxSondeType->Size = System::Drawing::Size(180, 23);
			this->comboBoxSondeType->TabIndex = 1;
			this->comboBoxSondeType->SelectedIndex = 0;
			// 
			// labelSondeType
			// 
			this->labelSondeType->AutoSize = true;
			this->labelSondeType->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelSondeType->Location = System::Drawing::Point(20, 45);
			this->labelSondeType->Name = L"labelSondeType";
			this->labelSondeType->Size = System::Drawing::Size(67, 15);
			this->labelSondeType->TabIndex = 0;
			this->labelSondeType->Text = L"Тип зонда:";
			// 
			// tabPageAirOffsets
			// 
			this->tabPageAirOffsets->Controls->Add(this->groupBoxAir2000);
			this->tabPageAirOffsets->Controls->Add(this->groupBoxAir400);
			this->tabPageAirOffsets->Location = System::Drawing::Point(4, 26);
			this->tabPageAirOffsets->Name = L"tabPageAirOffsets";
			this->tabPageAirOffsets->Padding = System::Windows::Forms::Padding(8);
			this->tabPageAirOffsets->Size = System::Drawing::Size(876, 484);
			this->tabPageAirOffsets->TabIndex = 1;
			this->tabPageAirOffsets->Text = L"Нули в воздухе (Air Offsets)";
			this->tabPageAirOffsets->UseVisualStyleBackColor = true;
			// 
			// groupBoxAir2000
			// 
			this->groupBoxAir2000->Controls->Add(this->tableLayoutAir2000);
			this->groupBoxAir2000->Dock = System::Windows::Forms::DockStyle::Top;
			this->groupBoxAir2000->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F, System::Drawing::FontStyle::Bold));
			this->groupBoxAir2000->Location = System::Drawing::Point(8, 240);
			this->groupBoxAir2000->Name = L"groupBoxAir2000";
			this->groupBoxAir2000->Padding = System::Windows::Forms::Padding(8);
			this->groupBoxAir2000->Size = System::Drawing::Size(860, 230);
			this->groupBoxAir2000->TabIndex = 1;
			this->groupBoxAir2000->TabStop = false;
			this->groupBoxAir2000->Text = L"Частота 2000 кГц (F2)";
			// 
			// tableLayoutAir2000
			// 
			this->tableLayoutAir2000->ColumnCount = 6;
			this->tableLayoutAir2000->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				140)));
			this->tableLayoutAir2000->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir2000->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir2000->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir2000->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir2000->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir2000->Controls->Add(this->labelHdrAirTx2, 0, 0);
			this->tableLayoutAir2000->Controls->Add(this->labelHdrAirPh2, 0, 1);
			this->tableLayoutAir2000->Controls->Add(this->labelHdrAirAmp2, 0, 2);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirPh2000_T1, 1, 1);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirPh2000_T2, 2, 1);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirPh2000_T3, 3, 1);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirPh2000_T4, 4, 1);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirPh2000_T5, 5, 1);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirAmp2000_T1, 1, 2);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirAmp2000_T2, 2, 2);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirAmp2000_T3, 3, 2);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirAmp2000_T4, 4, 2);
			this->tableLayoutAir2000->Controls->Add(this->textBoxAirAmp2000_T5, 5, 2);
			this->tableLayoutAir2000->Dock = System::Windows::Forms::DockStyle::Top;
			this->tableLayoutAir2000->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->tableLayoutAir2000->Location = System::Drawing::Point(8, 25);
			this->tableLayoutAir2000->Name = L"tableLayoutAir2000";
			this->tableLayoutAir2000->RowCount = 3;
			this->tableLayoutAir2000->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutAir2000->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutAir2000->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutAir2000->Size = System::Drawing::Size(844, 100);
			this->tableLayoutAir2000->TabIndex = 0;
			// 
			// labelHdrAirTx2
			// 
			this->labelHdrAirTx2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrAirTx2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelHdrAirTx2->Location = System::Drawing::Point(3, 0);
			this->labelHdrAirTx2->Name = L"labelHdrAirTx2";
			this->labelHdrAirTx2->Size = System::Drawing::Size(134, 32);
			this->labelHdrAirTx2->TabIndex = 0;
			this->labelHdrAirTx2->Text = L"Параметр";
			this->labelHdrAirTx2->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// labelHdrAirPh2
			// 
			this->labelHdrAirPh2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrAirPh2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelHdrAirPh2->Location = System::Drawing::Point(3, 32);
			this->labelHdrAirPh2->Name = L"labelHdrAirPh2";
			this->labelHdrAirPh2->Size = System::Drawing::Size(134, 32);
			this->labelHdrAirPh2->TabIndex = 1;
			this->labelHdrAirPh2->Text = L"Сдвиг фазы (град):";
			this->labelHdrAirPh2->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// labelHdrAirAmp2
			// 
			this->labelHdrAirAmp2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrAirAmp2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelHdrAirAmp2->Location = System::Drawing::Point(3, 64);
			this->labelHdrAirAmp2->Name = L"labelHdrAirAmp2";
			this->labelHdrAirAmp2->Size = System::Drawing::Size(134, 36);
			this->labelHdrAirAmp2->TabIndex = 2;
			this->labelHdrAirAmp2->Text = L"Аттеньюация (дБ):";
			this->labelHdrAirAmp2->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// textBoxAirPh2000_T1
			// 
			this->textBoxAirPh2000_T1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh2000_T1->Location = System::Drawing::Point(143, 35);
			this->textBoxAirPh2000_T1->Name = L"textBoxAirPh2000_T1";
			this->textBoxAirPh2000_T1->Size = System::Drawing::Size(134, 23);
			this->textBoxAirPh2000_T1->TabIndex = 3;
			this->textBoxAirPh2000_T1->Text = L"0.0";
			this->textBoxAirPh2000_T1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirPh2000_T2
			// 
			this->textBoxAirPh2000_T2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh2000_T2->Location = System::Drawing::Point(283, 35);
			this->textBoxAirPh2000_T2->Name = L"textBoxAirPh2000_T2";
			this->textBoxAirPh2000_T2->Size = System::Drawing::Size(134, 23);
			this->textBoxAirPh2000_T2->TabIndex = 4;
			this->textBoxAirPh2000_T2->Text = L"0.0";
			this->textBoxAirPh2000_T2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirPh2000_T3
			// 
			this->textBoxAirPh2000_T3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh2000_T3->Location = System::Drawing::Point(423, 35);
			this->textBoxAirPh2000_T3->Name = L"textBoxAirPh2000_T3";
			this->textBoxAirPh2000_T3->Size = System::Drawing::Size(134, 23);
			this->textBoxAirPh2000_T3->TabIndex = 5;
			this->textBoxAirPh2000_T3->Text = L"0.0";
			this->textBoxAirPh2000_T3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirPh2000_T4
			// 
			this->textBoxAirPh2000_T4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh2000_T4->Location = System::Drawing::Point(563, 35);
			this->textBoxAirPh2000_T4->Name = L"textBoxAirPh2000_T4";
			this->textBoxAirPh2000_T4->Size = System::Drawing::Size(134, 23);
			this->textBoxAirPh2000_T4->TabIndex = 6;
			this->textBoxAirPh2000_T4->Text = L"0.0";
			this->textBoxAirPh2000_T4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirPh2000_T5
			// 
			this->textBoxAirPh2000_T5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh2000_T5->Location = System::Drawing::Point(703, 35);
			this->textBoxAirPh2000_T5->Name = L"textBoxAirPh2000_T5";
			this->textBoxAirPh2000_T5->Size = System::Drawing::Size(138, 23);
			this->textBoxAirPh2000_T5->TabIndex = 7;
			this->textBoxAirPh2000_T5->Text = L"0.0";
			this->textBoxAirPh2000_T5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp2000_T1
			// 
			this->textBoxAirAmp2000_T1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp2000_T1->Location = System::Drawing::Point(143, 67);
			this->textBoxAirAmp2000_T1->Name = L"textBoxAirAmp2000_T1";
			this->textBoxAirAmp2000_T1->Size = System::Drawing::Size(134, 23);
			this->textBoxAirAmp2000_T1->TabIndex = 8;
			this->textBoxAirAmp2000_T1->Text = L"0.0";
			this->textBoxAirAmp2000_T1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp2000_T2
			// 
			this->textBoxAirAmp2000_T2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp2000_T2->Location = System::Drawing::Point(283, 67);
			this->textBoxAirAmp2000_T2->Name = L"textBoxAirAmp2000_T2";
			this->textBoxAirAmp2000_T2->Size = System::Drawing::Size(134, 23);
			this->textBoxAirAmp2000_T2->TabIndex = 9;
			this->textBoxAirAmp2000_T2->Text = L"0.0";
			this->textBoxAirAmp2000_T2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp2000_T3
			// 
			this->textBoxAirAmp2000_T3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp2000_T3->Location = System::Drawing::Point(423, 67);
			this->textBoxAirAmp2000_T3->Name = L"textBoxAirAmp2000_T3";
			this->textBoxAirAmp2000_T3->Size = System::Drawing::Size(134, 23);
			this->textBoxAirAmp2000_T3->TabIndex = 10;
			this->textBoxAirAmp2000_T3->Text = L"0.0";
			this->textBoxAirAmp2000_T3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp2000_T4
			// 
			this->textBoxAirAmp2000_T4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp2000_T4->Location = System::Drawing::Point(563, 67);
			this->textBoxAirAmp2000_T4->Name = L"textBoxAirAmp2000_T4";
			this->textBoxAirAmp2000_T4->Size = System::Drawing::Size(134, 23);
			this->textBoxAirAmp2000_T4->TabIndex = 11;
			this->textBoxAirAmp2000_T4->Text = L"0.0";
			this->textBoxAirAmp2000_T4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp2000_T5
			// 
			this->textBoxAirAmp2000_T5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp2000_T5->Location = System::Drawing::Point(703, 67);
			this->textBoxAirAmp2000_T5->Name = L"textBoxAirAmp2000_T5";
			this->textBoxAirAmp2000_T5->Size = System::Drawing::Size(138, 23);
			this->textBoxAirAmp2000_T5->TabIndex = 12;
			this->textBoxAirAmp2000_T5->Text = L"0.0";
			this->textBoxAirAmp2000_T5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// groupBoxAir400
			// 
			this->groupBoxAir400->Controls->Add(this->tableLayoutAir400);
			this->groupBoxAir400->Dock = System::Windows::Forms::DockStyle::Top;
			this->groupBoxAir400->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F, System::Drawing::FontStyle::Bold));
			this->groupBoxAir400->Location = System::Drawing::Point(8, 8);
			this->groupBoxAir400->Name = L"groupBoxAir400";
			this->groupBoxAir400->Padding = System::Windows::Forms::Padding(8);
			this->groupBoxAir400->Size = System::Drawing::Size(860, 232);
			this->groupBoxAir400->TabIndex = 0;
			this->groupBoxAir400->TabStop = false;
			this->groupBoxAir400->Text = L"Частота 400 кГц (F1)";
			// 
			// tableLayoutAir400
			// 
			this->tableLayoutAir400->ColumnCount = 6;
			this->tableLayoutAir400->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				140)));
			this->tableLayoutAir400->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir400->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir400->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir400->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir400->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutAir400->Controls->Add(this->labelHdrAirTx1, 0, 0);
			this->tableLayoutAir400->Controls->Add(this->labelHdrAirPh1, 0, 1);
			this->tableLayoutAir400->Controls->Add(this->labelHdrAirAmp1, 0, 2);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirPh400_T1, 1, 1);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirPh400_T2, 2, 1);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirPh400_T3, 3, 1);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirPh400_T4, 4, 1);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirPh400_T5, 5, 1);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirAmp400_T1, 1, 2);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirAmp400_T2, 2, 2);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirAmp400_T3, 3, 2);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirAmp400_T4, 4, 2);
			this->tableLayoutAir400->Controls->Add(this->textBoxAirAmp400_T5, 5, 2);
			this->tableLayoutAir400->Dock = System::Windows::Forms::DockStyle::Top;
			this->tableLayoutAir400->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->tableLayoutAir400->Location = System::Drawing::Point(8, 25);
			this->tableLayoutAir400->Name = L"tableLayoutAir400";
			this->tableLayoutAir400->RowCount = 3;
			this->tableLayoutAir400->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutAir400->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutAir400->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutAir400->Size = System::Drawing::Size(844, 100);
			this->tableLayoutAir400->TabIndex = 0;
			// 
			// labelHdrAirTx1
			// 
			this->labelHdrAirTx1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrAirTx1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelHdrAirTx1->Location = System::Drawing::Point(3, 0);
			this->labelHdrAirTx1->Name = L"labelHdrAirTx1";
			this->labelHdrAirTx1->Size = System::Drawing::Size(134, 32);
			this->labelHdrAirTx1->TabIndex = 0;
			this->labelHdrAirTx1->Text = L"Параметр";
			this->labelHdrAirTx1->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// labelHdrAirPh1
			// 
			this->labelHdrAirPh1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrAirPh1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelHdrAirPh1->Location = System::Drawing::Point(3, 32);
			this->labelHdrAirPh1->Name = L"labelHdrAirPh1";
			this->labelHdrAirPh1->Size = System::Drawing::Size(134, 32);
			this->labelHdrAirPh1->TabIndex = 1;
			this->labelHdrAirPh1->Text = L"Сдвиг фазы (град):";
			this->labelHdrAirPh1->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// labelHdrAirAmp1
			// 
			this->labelHdrAirAmp1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrAirAmp1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelHdrAirAmp1->Location = System::Drawing::Point(3, 64);
			this->labelHdrAirAmp1->Name = L"labelHdrAirAmp1";
			this->labelHdrAirAmp1->Size = System::Drawing::Size(134, 36);
			this->labelHdrAirAmp1->TabIndex = 2;
			this->labelHdrAirAmp1->Text = L"Аттеньюация (дБ):";
			this->labelHdrAirAmp1->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// textBoxAirPh400_T1
			// 
			this->textBoxAirPh400_T1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh400_T1->Location = System::Drawing::Point(143, 35);
			this->textBoxAirPh400_T1->Name = L"textBoxAirPh400_T1";
			this->textBoxAirPh400_T1->Size = System::Drawing::Size(134, 23);
			this->textBoxAirPh400_T1->TabIndex = 3;
			this->textBoxAirPh400_T1->Text = L"0.0";
			this->textBoxAirPh400_T1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirPh400_T2
			// 
			this->textBoxAirPh400_T2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh400_T2->Location = System::Drawing::Point(283, 35);
			this->textBoxAirPh400_T2->Name = L"textBoxAirPh400_T2";
			this->textBoxAirPh400_T2->Size = System::Drawing::Size(134, 23);
			this->textBoxAirPh400_T2->TabIndex = 4;
			this->textBoxAirPh400_T2->Text = L"0.0";
			this->textBoxAirPh400_T2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirPh400_T3
			// 
			this->textBoxAirPh400_T3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh400_T3->Location = System::Drawing::Point(423, 35);
			this->textBoxAirPh400_T3->Name = L"textBoxAirPh400_T3";
			this->textBoxAirPh400_T3->Size = System::Drawing::Size(134, 23);
			this->textBoxAirPh400_T3->TabIndex = 5;
			this->textBoxAirPh400_T3->Text = L"0.0";
			this->textBoxAirPh400_T3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirPh400_T4
			// 
			this->textBoxAirPh400_T4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh400_T4->Location = System::Drawing::Point(563, 35);
			this->textBoxAirPh400_T4->Name = L"textBoxAirPh400_T4";
			this->textBoxAirPh400_T4->Size = System::Drawing::Size(134, 23);
			this->textBoxAirPh400_T4->TabIndex = 6;
			this->textBoxAirPh400_T4->Text = L"0.0";
			this->textBoxAirPh400_T4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirPh400_T5
			// 
			this->textBoxAirPh400_T5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirPh400_T5->Location = System::Drawing::Point(703, 35);
			this->textBoxAirPh400_T5->Name = L"textBoxAirPh400_T5";
			this->textBoxAirPh400_T5->Size = System::Drawing::Size(138, 23);
			this->textBoxAirPh400_T5->TabIndex = 7;
			this->textBoxAirPh400_T5->Text = L"0.0";
			this->textBoxAirPh400_T5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp400_T1
			// 
			this->textBoxAirAmp400_T1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp400_T1->Location = System::Drawing::Point(143, 67);
			this->textBoxAirAmp400_T1->Name = L"textBoxAirAmp400_T1";
			this->textBoxAirAmp400_T1->Size = System::Drawing::Size(134, 23);
			this->textBoxAirAmp400_T1->TabIndex = 8;
			this->textBoxAirAmp400_T1->Text = L"0.0";
			this->textBoxAirAmp400_T1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp400_T2
			// 
			this->textBoxAirAmp400_T2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp400_T2->Location = System::Drawing::Point(283, 67);
			this->textBoxAirAmp400_T2->Name = L"textBoxAirAmp400_T2";
			this->textBoxAirAmp400_T2->Size = System::Drawing::Size(134, 23);
			this->textBoxAirAmp400_T2->TabIndex = 9;
			this->textBoxAirAmp400_T2->Text = L"0.0";
			this->textBoxAirAmp400_T2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp400_T3
			// 
			this->textBoxAirAmp400_T3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp400_T3->Location = System::Drawing::Point(423, 67);
			this->textBoxAirAmp400_T3->Name = L"textBoxAirAmp400_T3";
			this->textBoxAirAmp400_T3->Size = System::Drawing::Size(134, 23);
			this->textBoxAirAmp400_T3->TabIndex = 10;
			this->textBoxAirAmp400_T3->Text = L"0.0";
			this->textBoxAirAmp400_T3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp400_T4
			// 
			this->textBoxAirAmp400_T4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp400_T4->Location = System::Drawing::Point(563, 67);
			this->textBoxAirAmp400_T4->Name = L"textBoxAirAmp400_T4";
			this->textBoxAirAmp400_T4->Size = System::Drawing::Size(134, 23);
			this->textBoxAirAmp400_T4->TabIndex = 11;
			this->textBoxAirAmp400_T4->Text = L"0.0";
			this->textBoxAirAmp400_T4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxAirAmp400_T5
			// 
			this->textBoxAirAmp400_T5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxAirAmp400_T5->Location = System::Drawing::Point(703, 67);
			this->textBoxAirAmp400_T5->Name = L"textBoxAirAmp400_T5";
			this->textBoxAirAmp400_T5->Size = System::Drawing::Size(138, 23);
			this->textBoxAirAmp400_T5->TabIndex = 12;
			this->textBoxAirAmp400_T5->Text = L"0.0";
			this->textBoxAirAmp400_T5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// tabPageThresholds
			// 
			this->tabPageThresholds->Controls->Add(this->groupBoxMinAmp);
			this->tabPageThresholds->Location = System::Drawing::Point(4, 26);
			this->tabPageThresholds->Name = L"tabPageThresholds";
			this->tabPageThresholds->Padding = System::Windows::Forms::Padding(8);
			this->tabPageThresholds->Size = System::Drawing::Size(876, 484);
			this->tabPageThresholds->TabIndex = 2;
			this->tabPageThresholds->Text = L"Пороги достоверности (min_amp)";
			this->tabPageThresholds->UseVisualStyleBackColor = true;
			// 
			// groupBoxMinAmp
			// 
			this->groupBoxMinAmp->Controls->Add(this->tableLayoutMinAmp);
			this->groupBoxMinAmp->Dock = System::Windows::Forms::DockStyle::Top;
			this->groupBoxMinAmp->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F, System::Drawing::FontStyle::Bold));
			this->groupBoxMinAmp->Location = System::Drawing::Point(8, 8);
			this->groupBoxMinAmp->Name = L"groupBoxMinAmp";
			this->groupBoxMinAmp->Padding = System::Windows::Forms::Padding(8);
			this->groupBoxMinAmp->Size = System::Drawing::Size(860, 240);
			this->groupBoxMinAmp->TabIndex = 0;
			this->groupBoxMinAmp->TabStop = false;
			this->groupBoxMinAmp->Text = L"Минимальные пороговые амплитуды сигнала (мВ) для детекции отказов генераторов";
			// 
			// tableLayoutMinAmp
			// 
			this->tableLayoutMinAmp->ColumnCount = 6;
			this->tableLayoutMinAmp->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute,
				140)));
			this->tableLayoutMinAmp->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutMinAmp->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutMinAmp->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutMinAmp->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutMinAmp->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				20)));
			this->tableLayoutMinAmp->Controls->Add(this->labelHdrMinTx, 0, 0);
			this->tableLayoutMinAmp->Controls->Add(this->labelHdrMin400, 0, 1);
			this->tableLayoutMinAmp->Controls->Add(this->labelHdrMin2000, 0, 2);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin400_T1, 1, 1);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin400_T2, 2, 1);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin400_T3, 3, 1);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin400_T4, 4, 1);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin400_T5, 5, 1);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin2000_T1, 1, 2);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin2000_T2, 2, 2);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin2000_T3, 3, 2);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin2000_T4, 4, 2);
			this->tableLayoutMinAmp->Controls->Add(this->textBoxMin2000_T5, 5, 2);
			this->tableLayoutMinAmp->Dock = System::Windows::Forms::DockStyle::Top;
			this->tableLayoutMinAmp->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->tableLayoutMinAmp->Location = System::Drawing::Point(8, 25);
			this->tableLayoutMinAmp->Name = L"tableLayoutMinAmp";
			this->tableLayoutMinAmp->RowCount = 3;
			this->tableLayoutMinAmp->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutMinAmp->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutMinAmp->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute,
				32)));
			this->tableLayoutMinAmp->Size = System::Drawing::Size(844, 100);
			this->tableLayoutMinAmp->TabIndex = 0;
			// 
			// labelHdrMinTx
			// 
			this->labelHdrMinTx->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrMinTx->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelHdrMinTx->Location = System::Drawing::Point(3, 0);
			this->labelHdrMinTx->Name = L"labelHdrMinTx";
			this->labelHdrMinTx->Size = System::Drawing::Size(134, 32);
			this->labelHdrMinTx->TabIndex = 0;
			this->labelHdrMinTx->Text = L"Частота / Tx";
			this->labelHdrMinTx->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// labelHdrMin400
			// 
			this->labelHdrMin400->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrMin400->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelHdrMin400->Location = System::Drawing::Point(3, 32);
			this->labelHdrMin400->Name = L"labelHdrMin400";
			this->labelHdrMin400->Size = System::Drawing::Size(134, 32);
			this->labelHdrMin400->TabIndex = 1;
			this->labelHdrMin400->Text = L"min_amp 400 кГц:";
			this->labelHdrMin400->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// labelHdrMin2000
			// 
			this->labelHdrMin2000->Dock = System::Windows::Forms::DockStyle::Fill;
			this->labelHdrMin2000->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelHdrMin2000->Location = System::Drawing::Point(3, 64);
			this->labelHdrMin2000->Name = L"labelHdrMin2000";
			this->labelHdrMin2000->Size = System::Drawing::Size(134, 36);
			this->labelHdrMin2000->TabIndex = 2;
			this->labelHdrMin2000->Text = L"min_amp 2000 кГц:";
			this->labelHdrMin2000->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// textBoxMin400_T1
			// 
			this->textBoxMin400_T1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin400_T1->Location = System::Drawing::Point(143, 35);
			this->textBoxMin400_T1->Name = L"textBoxMin400_T1";
			this->textBoxMin400_T1->Size = System::Drawing::Size(134, 23);
			this->textBoxMin400_T1->TabIndex = 3;
			this->textBoxMin400_T1->Text = L"5.0";
			this->textBoxMin400_T1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxMin400_T2
			// 
			this->textBoxMin400_T2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin400_T2->Location = System::Drawing::Point(283, 35);
			this->textBoxMin400_T2->Name = L"textBoxMin400_T2";
			this->textBoxMin400_T2->Size = System::Drawing::Size(134, 23);
			this->textBoxMin400_T2->TabIndex = 4;
			this->textBoxMin400_T2->Text = L"5.0";
			this->textBoxMin400_T2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxMin400_T3
			// 
			this->textBoxMin400_T3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin400_T3->Location = System::Drawing::Point(423, 35);
			this->textBoxMin400_T3->Name = L"textBoxMin400_T3";
			this->textBoxMin400_T3->Size = System::Drawing::Size(134, 23);
			this->textBoxMin400_T3->TabIndex = 5;
			this->textBoxMin400_T3->Text = L"5.0";
			this->textBoxMin400_T3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxMin400_T4
			// 
			this->textBoxMin400_T4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin400_T4->Location = System::Drawing::Point(563, 35);
			this->textBoxMin400_T4->Name = L"textBoxMin400_T4";
			this->textBoxMin400_T4->Size = System::Drawing::Size(134, 23);
			this->textBoxMin400_T4->TabIndex = 6;
			this->textBoxMin400_T4->Text = L"5.0";
			this->textBoxMin400_T4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxMin400_T5
			// 
			this->textBoxMin400_T5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin400_T5->Location = System::Drawing::Point(703, 35);
			this->textBoxMin400_T5->Name = L"textBoxMin400_T5";
			this->textBoxMin400_T5->Size = System::Drawing::Size(138, 23);
			this->textBoxMin400_T5->TabIndex = 7;
			this->textBoxMin400_T5->Text = L"5.0";
			this->textBoxMin400_T5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxMin2000_T1
			// 
			this->textBoxMin2000_T1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin2000_T1->Location = System::Drawing::Point(143, 67);
			this->textBoxMin2000_T1->Name = L"textBoxMin2000_T1";
			this->textBoxMin2000_T1->Size = System::Drawing::Size(134, 23);
			this->textBoxMin2000_T1->TabIndex = 8;
			this->textBoxMin2000_T1->Text = L"5.0";
			this->textBoxMin2000_T1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxMin2000_T2
			// 
			this->textBoxMin2000_T2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin2000_T2->Location = System::Drawing::Point(283, 67);
			this->textBoxMin2000_T2->Name = L"textBoxMin2000_T2";
			this->textBoxMin2000_T2->Size = System::Drawing::Size(134, 23);
			this->textBoxMin2000_T2->TabIndex = 9;
			this->textBoxMin2000_T2->Text = L"5.0";
			this->textBoxMin2000_T2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxMin2000_T3
			// 
			this->textBoxMin2000_T3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin2000_T3->Location = System::Drawing::Point(423, 67);
			this->textBoxMin2000_T3->Name = L"textBoxMin2000_T3";
			this->textBoxMin2000_T3->Size = System::Drawing::Size(134, 23);
			this->textBoxMin2000_T3->TabIndex = 10;
			this->textBoxMin2000_T3->Text = L"5.0";
			this->textBoxMin2000_T3->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxMin2000_T4
			// 
			this->textBoxMin2000_T4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin2000_T4->Location = System::Drawing::Point(563, 67);
			this->textBoxMin2000_T4->Name = L"textBoxMin2000_T4";
			this->textBoxMin2000_T4->Size = System::Drawing::Size(134, 23);
			this->textBoxMin2000_T4->TabIndex = 11;
			this->textBoxMin2000_T4->Text = L"5.0";
			this->textBoxMin2000_T4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// textBoxMin2000_T5
			// 
			this->textBoxMin2000_T5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->textBoxMin2000_T5->Location = System::Drawing::Point(703, 67);
			this->textBoxMin2000_T5->Name = L"textBoxMin2000_T5";
			this->textBoxMin2000_T5->Size = System::Drawing::Size(138, 23);
			this->textBoxMin2000_T5->TabIndex = 12;
			this->textBoxMin2000_T5->Text = L"5.0";
			this->textBoxMin2000_T5->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// tabPageInclinometer
			// 
			this->tabPageInclinometer->Controls->Add(this->groupBoxIncliParams);
			this->tabPageInclinometer->Location = System::Drawing::Point(4, 26);
			this->tabPageInclinometer->Name = L"tabPageInclinometer";
			this->tabPageInclinometer->Padding = System::Windows::Forms::Padding(8);
			this->tabPageInclinometer->Size = System::Drawing::Size(876, 484);
			this->tabPageInclinometer->TabIndex = 3;
			this->tabPageInclinometer->Text = L"Инклинометрия и Ротация";
			this->tabPageInclinometer->UseVisualStyleBackColor = true;
			// 
			// groupBoxIncliParams
			// 
			this->groupBoxIncliParams->Controls->Add(this->textBoxFilterK);
			this->groupBoxIncliParams->Controls->Add(this->labelFilterK);
			this->groupBoxIncliParams->Controls->Add(this->textBoxAngleHistory);
			this->groupBoxIncliParams->Controls->Add(this->labelAngleHistory);
			this->groupBoxIncliParams->Controls->Add(this->comboBoxRotDir);
			this->groupBoxIncliParams->Controls->Add(this->labelRotDir);
			this->groupBoxIncliParams->Controls->Add(this->textBoxMinRotSpeed);
			this->groupBoxIncliParams->Controls->Add(this->labelMinRotSpeed);
			this->groupBoxIncliParams->Dock = System::Windows::Forms::DockStyle::Top;
			this->groupBoxIncliParams->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F, System::Drawing::FontStyle::Bold));
			this->groupBoxIncliParams->Location = System::Drawing::Point(8, 8);
			this->groupBoxIncliParams->Name = L"groupBoxIncliParams";
			this->groupBoxIncliParams->Size = System::Drawing::Size(860, 160);
			this->groupBoxIncliParams->TabIndex = 0;
			this->groupBoxIncliParams->TabStop = false;
			this->groupBoxIncliParams->Text = L"Параметры инклинометра и разбиения секторов";
			// 
			// textBoxFilterK
			// 
			this->textBoxFilterK->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->textBoxFilterK->Location = System::Drawing::Point(630, 42);
			this->textBoxFilterK->Name = L"textBoxFilterK";
			this->textBoxFilterK->Size = System::Drawing::Size(120, 23);
			this->textBoxFilterK->TabIndex = 7;
			this->textBoxFilterK->Text = L"0.85";
			// 
			// labelFilterK
			// 
			this->labelFilterK->AutoSize = true;
			this->labelFilterK->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelFilterK->Location = System::Drawing::Point(460, 45);
			this->labelFilterK->Name = L"labelFilterK";
			this->labelFilterK->Size = System::Drawing::Size(155, 15);
			this->labelFilterK->TabIndex = 6;
			this->labelFilterK->Text = L"Коэф. фильтрации (K_fil):";
			// 
			// textBoxAngleHistory
			// 
			this->textBoxAngleHistory->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->textBoxAngleHistory->Location = System::Drawing::Point(630, 88);
			this->textBoxAngleHistory->Name = L"textBoxAngleHistory";
			this->textBoxAngleHistory->Size = System::Drawing::Size(120, 23);
			this->textBoxAngleHistory->TabIndex = 5;
			this->textBoxAngleHistory->Text = L"16";
			// 
			// labelAngleHistory
			// 
			this->labelAngleHistory->AutoSize = true;
			this->labelAngleHistory->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelAngleHistory->Location = System::Drawing::Point(460, 91);
			this->labelAngleHistory->Name = L"labelAngleHistory";
			this->labelAngleHistory->Size = System::Drawing::Size(150, 15);
			this->labelAngleHistory->TabIndex = 4;
			this->labelAngleHistory->Text = L"Глубина истории углов:";
			// 
			// comboBoxRotDir
			// 
			this->comboBoxRotDir->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxRotDir->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->comboBoxRotDir->FormattingEnabled = true;
			this->comboBoxRotDir->Items->AddRange(gcnew cli::array< System::Object^  >(2) {
				L"Прямое (по часовой)",
				L"Обратное (против часовой)"
			});
			this->comboBoxRotDir->Location = System::Drawing::Point(230, 88);
			this->comboBoxRotDir->Name = L"comboBoxRotDir";
			this->comboBoxRotDir->Size = System::Drawing::Size(180, 23);
			this->comboBoxRotDir->TabIndex = 3;
			this->comboBoxRotDir->SelectedIndex = 0;
			// 
			// labelRotDir
			// 
			this->labelRotDir->AutoSize = true;
			this->labelRotDir->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelRotDir->Location = System::Drawing::Point(20, 91);
			this->labelRotDir->Name = L"labelRotDir";
			this->labelRotDir->Size = System::Drawing::Size(139, 15);
			this->labelRotDir->TabIndex = 2;
			this->labelRotDir->Text = L"Направление ротации:";
			// 
			// textBoxMinRotSpeed
			// 
			this->textBoxMinRotSpeed->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->textBoxMinRotSpeed->Location = System::Drawing::Point(230, 42);
			this->textBoxMinRotSpeed->Name = L"textBoxMinRotSpeed";
			this->textBoxMinRotSpeed->Size = System::Drawing::Size(120, 23);
			this->textBoxMinRotSpeed->TabIndex = 1;
			this->textBoxMinRotSpeed->Text = L"10.0";
			// 
			// labelMinRotSpeed
			// 
			this->labelMinRotSpeed->AutoSize = true;
			this->labelMinRotSpeed->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->labelMinRotSpeed->Location = System::Drawing::Point(20, 45);
			this->labelMinRotSpeed->Name = L"labelMinRotSpeed";
			this->labelMinRotSpeed->Size = System::Drawing::Size(183, 15);
			this->labelMinRotSpeed->TabIndex = 0;
			this->labelMinRotSpeed->Text = L"Мин. скорость ротации (об/мин):";
			// 
			// openFileDialogMetro
			// 
			this->openFileDialogMetro->DefaultExt = L"calib";
			this->openFileDialogMetro->Filter = L"Файлы калибровки и метрологии (*.calib;*.bin;*.csv)|*.calib;*.bin;*.csv|Все файлы"
				L" (*.*)|*.*";
			this->openFileDialogMetro->Title = L"Открыть файл параметров метрологии";
			// 
			// saveFileDialogMetro
			// 
			this->saveFileDialogMetro->DefaultExt = L"calib";
			this->saveFileDialogMetro->Filter = L"Файлы калибровки (*.calib)|*.calib|Бинарный образ Flash (*.bin)|*.bin|Калибровоч"
				L"ный CSV (*.csv)|*.csv";
			this->saveFileDialogMetro->Title = L"Сохранить файл параметров метрологии";
			// 
			// MetroForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(7, 15);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(884, 634);
			this->Controls->Add(this->tabControlMetro);
			this->Controls->Add(this->panelBottom);
			this->Controls->Add(this->statusStripMetro);
			this->Controls->Add(this->panelTopBar);
			this->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->MinimumSize = System::Drawing::Size(780, 500);
			this->Name = L"MetroForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
			this->Text = L"Параметры метрологии зонда — НПФ «АМК Горизонт»";
			this->panelTopBar->ResumeLayout(false);
			this->panelTopBar->PerformLayout();
			this->panelBottom->ResumeLayout(false);
			this->statusStripMetro->ResumeLayout(false);
			this->statusStripMetro->PerformLayout();
			this->tabControlMetro->ResumeLayout(false);
			this->tabPagePassport->ResumeLayout(false);
			this->groupBoxGeometry->ResumeLayout(false);
			this->tableLayoutGeometry->ResumeLayout(false);
			this->tableLayoutGeometry->PerformLayout();
			this->groupBoxPassport->ResumeLayout(false);
			this->groupBoxPassport->PerformLayout();
			this->tabPageAirOffsets->ResumeLayout(false);
			this->groupBoxAir2000->ResumeLayout(false);
			this->tableLayoutAir2000->ResumeLayout(false);
			this->tableLayoutAir2000->PerformLayout();
			this->groupBoxAir400->ResumeLayout(false);
			this->tableLayoutAir400->ResumeLayout(false);
			this->tableLayoutAir400->PerformLayout();
			this->tabPageThresholds->ResumeLayout(false);
			this->groupBoxMinAmp->ResumeLayout(false);
			this->tableLayoutMinAmp->ResumeLayout(false);
			this->tableLayoutMinAmp->PerformLayout();
			this->tabPageInclinometer->ResumeLayout(false);
			this->groupBoxIncliParams->ResumeLayout(false);
			this->groupBoxIncliParams->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

	private:
		void InitDefaultValues(void);
		System::Void buttonOpenFile_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonSaveFile_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonReadFromSonde_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonWriteToSonde_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonApply_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonClose_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void checkBoxReadOnly_CheckedChanged(System::Object^ sender, System::EventArgs^ e);
	};
}
