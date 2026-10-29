#pragma once

namespace ELECTROMAGNETIC_TOOLS_3_0 {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Collections::Generic;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Windows::Forms::DataVisualization::Charting;

	/// <summary>
	/// Главное окно станции оператора Electromagnetic Tools 3.0
	/// Организация: НПФ «АМК Горизонт»
	/// Разработал: зам начальника ОП Смирнов С. Г.
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
		{
			InitializeComponent();
			InitChartEngine();
		}

	protected:
		/// <summary>
		/// Освободить все используемые ресурсы.
		/// </summary>
		~MainForm()
		{
			if (components)
			{
				delete components;
			}
		}

		virtual bool ProcessCmdKey(System::Windows::Forms::Message% msg, System::Windows::Forms::Keys keyData) override
		{
			if (keyData == System::Windows::Forms::Keys::F4)
			{
				ToggleSidebar();
				return true;
			}
			if (keyData == System::Windows::Forms::Keys::Escape && this->panelLeft != nullptr && this->panelLeft->Visible && !this->isSidebarPinned)
			{
				HideSidebar();
				return true;
			}
			return System::Windows::Forms::Form::ProcessCmdKey(msg, keyData);
		}

	private:
		System::ComponentModel::IContainer^ components;

		// Главное меню
		System::Windows::Forms::MenuStrip^ menuStripMain;
		System::Windows::Forms::ToolStripMenuItem^ menuItemFile;
		System::Windows::Forms::ToolStripMenuItem^ menuItemOpenBin;
		System::Windows::Forms::ToolStripSeparator^ menuItemSep1;
		System::Windows::Forms::ToolStripMenuItem^ menuItemExit;
		System::Windows::Forms::ToolStripMenuItem^ menuItemView;
		System::Windows::Forms::ToolStripMenuItem^ menuItemToggleSidebar;
		System::Windows::Forms::ToolStripMenuItem^ menuItemComm;
		System::Windows::Forms::ToolStripMenuItem^ menuItemConnect;
		System::Windows::Forms::ToolStripMenuItem^ menuItemDisconnect;
		System::Windows::Forms::ToolStripMenuItem^ menuItemMetro;
		System::Windows::Forms::ToolStripMenuItem^ menuItemMetroParams;
		System::Windows::Forms::ToolStripMenuItem^ menuItemMetroCalibrate;
		System::Windows::Forms::ToolStripSeparator^ menuItemSepMetro;
		System::Windows::Forms::ToolStripMenuItem^ menuItemMetroRead;
		System::Windows::Forms::ToolStripMenuItem^ menuItemMetroWrite;
		System::Windows::Forms::ToolStripMenuItem^ menuItemHelp;
		System::Windows::Forms::ToolStripMenuItem^ menuItemAbout;

		// Строка статуса
		System::Windows::Forms::StatusStrip^ statusStripMain;
		System::Windows::Forms::ToolStripStatusLabel^ statusLabelDevice;
		System::Windows::Forms::ToolStripStatusLabel^ statusLabelConnection;
		System::Windows::Forms::ToolStripStatusLabel^ statusLabelFPS;
		System::Windows::Forms::ToolStripStatusLabel^ statusLabelPackets;
		System::Windows::Forms::ToolStripStatusLabel^ statusLabelFile;

		// Левая выдвижная панель Drawer с карточками управления
		System::Windows::Forms::Panel^ panelLeft;
		System::Windows::Forms::Panel^ panelDrawerHeader;
		System::Windows::Forms::Label^ labelDrawerTitle;
		System::Windows::Forms::Button^ buttonPinSidebar;
		System::Windows::Forms::Button^ buttonCloseSidebar;

		// Карточка 1: Связь
		System::Windows::Forms::GroupBox^ groupBoxConnection;
		System::Windows::Forms::Label^ labelConnType;
		System::Windows::Forms::ComboBox^ comboBoxConnType;
		System::Windows::Forms::Label^ labelConnParam;
		System::Windows::Forms::TextBox^ textBoxConnParam;
		System::Windows::Forms::Button^ buttonConnect;
		System::Windows::Forms::Button^ buttonDisconnect;
		System::Windows::Forms::Label^ labelConnStatus;

		// Карточка 2: Режим прибора
		System::Windows::Forms::GroupBox^ groupBoxMode;
		System::Windows::Forms::RadioButton^ radioButtonModeStand;
		System::Windows::Forms::RadioButton^ radioButtonModeLog;
		System::Windows::Forms::RadioButton^ radioButtonModeDiag;

		// Карточка 3: Метрология
		System::Windows::Forms::GroupBox^ groupBoxMetro;
		System::Windows::Forms::Button^ buttonMetroParams;
		System::Windows::Forms::Button^ buttonExportCsv;
		System::Windows::Forms::Button^ buttonSelfTest;
		System::Windows::Forms::Button^ buttonCalibrate;

		// Центральная рабочая область с вкладками
		System::Windows::Forms::TabControl^ tabControlMain;
		System::Windows::Forms::TabPage^ tabPageLogging;
		System::Windows::Forms::TabPage^ tabPageInclinometer;
		System::Windows::Forms::TabPage^ tabPageHealth;

		// Верхняя полоса управления раскладкой экрана (Полосы / Секторы)
		System::Windows::Forms::Panel^ panelLoggingToolbar;
		System::Windows::Forms::Button^ buttonToggleSidebar;
		System::Windows::Forms::Button^ buttonToggleSectors;
		System::Windows::Forms::Label^ labelSepLayout1;
		System::Windows::Forms::RadioButton^ radioButtonLayoutBoth;
		System::Windows::Forms::RadioButton^ radioButtonLayoutTop;
		System::Windows::Forms::RadioButton^ radioButtonLayoutBottom;
		System::Windows::Forms::Label^ labelSepLayout2;
		System::Windows::Forms::Button^ buttonStream;
		System::Windows::Forms::Button^ buttonRefresh;
		System::Windows::Forms::Label^ labelSepStream;
		System::Windows::Forms::Label^ labelCursorInfo;
		System::Windows::Forms::Timer^ timerStream;

		// Всплывающие подсказки
		System::Windows::Forms::ToolTip^ toolTipMain;

		// Главные сплиттеры (поэтажная компоновка комплексов)
		System::Windows::Forms::SplitContainer^ splitContainerRows;
		System::Windows::Forms::SplitContainer^ splitContainerBand1;
		System::Windows::Forms::SplitContainer^ splitContainerBand2;

		// Временное окно 1 (Верхнее)
		System::Windows::Forms::Panel^ panelTime1Container;
		System::Windows::Forms::Panel^ panelTime1Header;
		System::Windows::Forms::Button^ buttonMode1;
		System::Windows::Forms::Button^ buttonFreq1;
		System::Windows::Forms::Button^ buttonAddCurve1;
		System::Windows::Forms::ContextMenuStrip^ contextMenuCurves1;
		System::Windows::Forms::FlowLayoutPanel^ flowPanelChips1;
		System::Windows::Forms::CheckBox^ checkTx1_1;
		System::Windows::Forms::CheckBox^ checkTx1_2;
		System::Windows::Forms::CheckBox^ checkTx1_3;
		System::Windows::Forms::CheckBox^ checkTx1_4;
		System::Windows::Forms::CheckBox^ checkTx1_5;
		System::Windows::Forms::Label^ labelMin1;
		System::Windows::Forms::TextBox^ textBoxMin1;
		System::Windows::Forms::Label^ labelMax1;
		System::Windows::Forms::TextBox^ textBoxMax1;
		System::Windows::Forms::Button^ buttonApplyRange1;
		System::Windows::Forms::Button^ buttonResetRange1;
		System::Windows::Forms::CheckBox^ checkHoldZoom1;
		System::Windows::Forms::ComboBox^ comboBoxPrecision1;
		System::Windows::Forms::Button^ buttonAutoY1;
		System::Windows::Forms::Button^ buttonXMode1;
		System::Windows::Forms::Button^ buttonZoomReset1;
		System::Windows::Forms::DataVisualization::Charting::Chart^ chartTime1;
		System::Windows::Forms::ContextMenuStrip^ contextMenuChart1;
		System::Windows::Forms::ToolStripMenuItem^ menuItemSetMin1;
		System::Windows::Forms::ToolStripMenuItem^ menuItemSetMax1;
		System::Windows::Forms::ToolStripSeparator^ menuSepChart1;
		System::Windows::Forms::ToolStripMenuItem^ menuItemZoomBackX1;
		System::Windows::Forms::ToolStripMenuItem^ menuItemZoomBackY1;
		System::Windows::Forms::ToolStripMenuItem^ menuItemResetRange1;

		// Временное окно 2 (Нижнее)
		System::Windows::Forms::Panel^ panelTime2Container;
		System::Windows::Forms::Panel^ panelTime2Header;
		System::Windows::Forms::Button^ buttonMode2;
		System::Windows::Forms::Button^ buttonFreq2;
		System::Windows::Forms::Button^ buttonAddCurve2;
		System::Windows::Forms::ContextMenuStrip^ contextMenuCurves2;
		System::Windows::Forms::FlowLayoutPanel^ flowPanelChips2;
		System::Windows::Forms::CheckBox^ checkTx2_1;
		System::Windows::Forms::CheckBox^ checkTx2_2;
		System::Windows::Forms::CheckBox^ checkTx2_3;
		System::Windows::Forms::CheckBox^ checkTx2_4;
		System::Windows::Forms::CheckBox^ checkTx2_5;
		System::Windows::Forms::Label^ labelMin2;
		System::Windows::Forms::TextBox^ textBoxMin2;
		System::Windows::Forms::Label^ labelMax2;
		System::Windows::Forms::TextBox^ textBoxMax2;
		System::Windows::Forms::Button^ buttonApplyRange2;
		System::Windows::Forms::Button^ buttonResetRange2;
		System::Windows::Forms::CheckBox^ checkHoldZoom2;
		System::Windows::Forms::ComboBox^ comboBoxPrecision2;
		System::Windows::Forms::Button^ buttonAutoY2;
		System::Windows::Forms::Button^ buttonXMode2;
		System::Windows::Forms::Button^ buttonZoomReset2;
		System::Windows::Forms::DataVisualization::Charting::Chart^ chartTime2;
		System::Windows::Forms::ContextMenuStrip^ contextMenuChart2;
		System::Windows::Forms::ToolStripMenuItem^ menuItemSetMin2;
		System::Windows::Forms::ToolStripMenuItem^ menuItemSetMax2;
		System::Windows::Forms::ToolStripSeparator^ menuSepChart2;
		System::Windows::Forms::ToolStripMenuItem^ menuItemZoomBackX2;
		System::Windows::Forms::ToolStripMenuItem^ menuItemZoomBackY2;
		System::Windows::Forms::ToolStripMenuItem^ menuItemResetRange2;

		// Секторное окно Rx_L (Справа верхнее)
		System::Windows::Forms::Panel^ panelSectorRxLContainer;
		System::Windows::Forms::Panel^ panelSectorRxLHeader;
		System::Windows::Forms::Label^ labelSectorRxLTitle;
		System::Windows::Forms::Button^ buttonFreqRxL;
		System::Windows::Forms::ComboBox^ comboBoxTxRxL;
		System::Windows::Forms::DataVisualization::Charting::Chart^ chartSectorRxL;
		System::Windows::Forms::Panel^ panelQualityRxL;
		System::Windows::Forms::Label^ labelQualityH1H2_L;
		System::Windows::Forms::Label^ labelQualityDV_L;
		System::Windows::Forms::Label^ labelQualitySNR_L;

		// Секторное окно Rx_R (Справа нижнее)
		System::Windows::Forms::Panel^ panelSectorRxRContainer;
		System::Windows::Forms::Panel^ panelSectorRxRHeader;
		System::Windows::Forms::Label^ labelSectorRxRTitle;
		System::Windows::Forms::Button^ buttonFreqRxR;
		System::Windows::Forms::ComboBox^ comboBoxTxRxR;
		System::Windows::Forms::DataVisualization::Charting::Chart^ chartSectorRxR;
		System::Windows::Forms::Panel^ panelQualityRxR;
		System::Windows::Forms::Label^ labelQualityH1H2_R;
		System::Windows::Forms::Label^ labelQualityDV_R;
		System::Windows::Forms::Label^ labelQualitySNR_R;

		// Вкладка 2 (Инклинометрия)
		System::Windows::Forms::Panel^ panelInclinometerPlaceholder;
		System::Windows::Forms::Label^ labelInclinometerTitle;

		// Вкладка 3 (Здоровье)
		System::Windows::Forms::Panel^ panelHealthPlaceholder;
		System::Windows::Forms::Label^ labelHealthTitle;

		// Диалог открытия
		System::Windows::Forms::OpenFileDialog^ openFileDialogBin;

		// Динамические списки закрепленных кривых
		List<String^>^ pinnedCurves1;
		List<String^>^ pinnedCurves2;
		String^ activeCurve1;
		String^ activeCurve2;
		bool isDirectMode1;
		bool isDirectMode2;
		bool isTempXAxis1;
		bool isTempXAxis2;
		int precisionY1;
		int precisionY2;
		int minFrame1;
		int maxFrame1;
		int minFrame2;
		int maxFrame2;
		Drawing::Point mouseDownPoint1;
		Drawing::Point mouseDownPoint2;
		Drawing::Point panStartPoint1;
		Drawing::Point panStartPoint2;
		bool isPanning1;
		bool isPanning2;

		void UpdateTxControlsVisibility();
		void ExportCsv(int windowIndex);
		String^ FindPythonExe();
		void RunGpAnalysis(String^ csvPath);
		System::Void menuExportCsv_Click(System::Object^ sender, System::EventArgs^ e);

		// Состояние потока данных и разделителей
		bool isStreaming;
		int currentStreamPoint;
		bool isSyncingSplitter;
		bool isSidebarPinned;

		void ToggleSidebar();
		void ShowSidebar();
		void HideSidebar();

#pragma region Windows Form Designer generated code
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->menuStripMain = (gcnew System::Windows::Forms::MenuStrip());
			this->menuItemFile = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemOpenBin = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemSep1 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->menuItemExit = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemView = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemToggleSidebar = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemComm = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemConnect = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemDisconnect = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemMetro = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemMetroParams = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemMetroCalibrate = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemSepMetro = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->menuItemMetroRead = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemMetroWrite = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemHelp = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemAbout = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->statusStripMain = (gcnew System::Windows::Forms::StatusStrip());
			this->statusLabelDevice = (gcnew System::Windows::Forms::ToolStripStatusLabel());
			this->statusLabelConnection = (gcnew System::Windows::Forms::ToolStripStatusLabel());
			this->statusLabelFPS = (gcnew System::Windows::Forms::ToolStripStatusLabel());
			this->statusLabelPackets = (gcnew System::Windows::Forms::ToolStripStatusLabel());
			this->statusLabelFile = (gcnew System::Windows::Forms::ToolStripStatusLabel());
			this->panelLeft = (gcnew System::Windows::Forms::Panel());
			this->groupBoxMetro = (gcnew System::Windows::Forms::GroupBox());
			this->buttonMetroParams = (gcnew System::Windows::Forms::Button());
			this->buttonExportCsv = (gcnew System::Windows::Forms::Button());
			this->buttonSelfTest = (gcnew System::Windows::Forms::Button());
			this->buttonCalibrate = (gcnew System::Windows::Forms::Button());
			this->groupBoxMode = (gcnew System::Windows::Forms::GroupBox());
			this->radioButtonModeDiag = (gcnew System::Windows::Forms::RadioButton());
			this->radioButtonModeLog = (gcnew System::Windows::Forms::RadioButton());
			this->radioButtonModeStand = (gcnew System::Windows::Forms::RadioButton());
			this->groupBoxConnection = (gcnew System::Windows::Forms::GroupBox());
			this->labelConnStatus = (gcnew System::Windows::Forms::Label());
			this->buttonDisconnect = (gcnew System::Windows::Forms::Button());
			this->buttonConnect = (gcnew System::Windows::Forms::Button());
			this->textBoxConnParam = (gcnew System::Windows::Forms::TextBox());
			this->labelConnParam = (gcnew System::Windows::Forms::Label());
			this->comboBoxConnType = (gcnew System::Windows::Forms::ComboBox());
			this->labelConnType = (gcnew System::Windows::Forms::Label());
			this->panelDrawerHeader = (gcnew System::Windows::Forms::Panel());
			this->buttonCloseSidebar = (gcnew System::Windows::Forms::Button());
			this->buttonPinSidebar = (gcnew System::Windows::Forms::Button());
			this->labelDrawerTitle = (gcnew System::Windows::Forms::Label());
			this->tabControlMain = (gcnew System::Windows::Forms::TabControl());
			this->tabPageLogging = (gcnew System::Windows::Forms::TabPage());
			this->splitContainerRows = (gcnew System::Windows::Forms::SplitContainer());
			this->splitContainerBand1 = (gcnew System::Windows::Forms::SplitContainer());
			this->panelTime1Container = (gcnew System::Windows::Forms::Panel());
			this->chartTime1 = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->contextMenuChart1 = (gcnew System::Windows::Forms::ContextMenuStrip(this->components));
			this->menuItemSetMin1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemSetMax1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuSepChart1 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->menuItemZoomBackX1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemZoomBackY1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemResetRange1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->buttonZoomReset1 = (gcnew System::Windows::Forms::Button());
			this->panelTime1Header = (gcnew System::Windows::Forms::Panel());
			this->buttonAutoY1 = (gcnew System::Windows::Forms::Button());
			this->comboBoxPrecision1 = (gcnew System::Windows::Forms::ComboBox());
			this->checkHoldZoom1 = (gcnew System::Windows::Forms::CheckBox());
			this->buttonResetRange1 = (gcnew System::Windows::Forms::Button());
			this->buttonApplyRange1 = (gcnew System::Windows::Forms::Button());
			this->textBoxMax1 = (gcnew System::Windows::Forms::TextBox());
			this->labelMax1 = (gcnew System::Windows::Forms::Label());
			this->textBoxMin1 = (gcnew System::Windows::Forms::TextBox());
			this->labelMin1 = (gcnew System::Windows::Forms::Label());
			this->checkTx1_5 = (gcnew System::Windows::Forms::CheckBox());
			this->checkTx1_4 = (gcnew System::Windows::Forms::CheckBox());
			this->checkTx1_3 = (gcnew System::Windows::Forms::CheckBox());
			this->checkTx1_2 = (gcnew System::Windows::Forms::CheckBox());
			this->checkTx1_1 = (gcnew System::Windows::Forms::CheckBox());
			this->flowPanelChips1 = (gcnew System::Windows::Forms::FlowLayoutPanel());
			this->buttonAddCurve1 = (gcnew System::Windows::Forms::Button());
			this->buttonXMode1 = (gcnew System::Windows::Forms::Button());
			this->buttonFreq1 = (gcnew System::Windows::Forms::Button());
			this->buttonMode1 = (gcnew System::Windows::Forms::Button());
			this->panelSectorRxLContainer = (gcnew System::Windows::Forms::Panel());
			this->chartSectorRxL = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->panelQualityRxL = (gcnew System::Windows::Forms::Panel());
			this->labelQualitySNR_L = (gcnew System::Windows::Forms::Label());
			this->labelQualityDV_L = (gcnew System::Windows::Forms::Label());
			this->labelQualityH1H2_L = (gcnew System::Windows::Forms::Label());
			this->panelSectorRxLHeader = (gcnew System::Windows::Forms::Panel());
			this->comboBoxTxRxL = (gcnew System::Windows::Forms::ComboBox());
			this->buttonFreqRxL = (gcnew System::Windows::Forms::Button());
			this->labelSectorRxLTitle = (gcnew System::Windows::Forms::Label());
			this->splitContainerBand2 = (gcnew System::Windows::Forms::SplitContainer());
			this->panelTime2Container = (gcnew System::Windows::Forms::Panel());
			this->chartTime2 = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->contextMenuChart2 = (gcnew System::Windows::Forms::ContextMenuStrip(this->components));
			this->menuItemSetMin2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemSetMax2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuSepChart2 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->menuItemZoomBackX2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemZoomBackY2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuItemResetRange2 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->buttonZoomReset2 = (gcnew System::Windows::Forms::Button());
			this->panelTime2Header = (gcnew System::Windows::Forms::Panel());
			this->buttonAutoY2 = (gcnew System::Windows::Forms::Button());
			this->comboBoxPrecision2 = (gcnew System::Windows::Forms::ComboBox());
			this->checkHoldZoom2 = (gcnew System::Windows::Forms::CheckBox());
			this->buttonResetRange2 = (gcnew System::Windows::Forms::Button());
			this->buttonApplyRange2 = (gcnew System::Windows::Forms::Button());
			this->textBoxMax2 = (gcnew System::Windows::Forms::TextBox());
			this->labelMax2 = (gcnew System::Windows::Forms::Label());
			this->textBoxMin2 = (gcnew System::Windows::Forms::TextBox());
			this->labelMin2 = (gcnew System::Windows::Forms::Label());
			this->checkTx2_5 = (gcnew System::Windows::Forms::CheckBox());
			this->checkTx2_4 = (gcnew System::Windows::Forms::CheckBox());
			this->checkTx2_3 = (gcnew System::Windows::Forms::CheckBox());
			this->checkTx2_2 = (gcnew System::Windows::Forms::CheckBox());
			this->checkTx2_1 = (gcnew System::Windows::Forms::CheckBox());
			this->flowPanelChips2 = (gcnew System::Windows::Forms::FlowLayoutPanel());
			this->buttonAddCurve2 = (gcnew System::Windows::Forms::Button());
			this->buttonXMode2 = (gcnew System::Windows::Forms::Button());
			this->buttonFreq2 = (gcnew System::Windows::Forms::Button());
			this->buttonMode2 = (gcnew System::Windows::Forms::Button());
			this->panelSectorRxRContainer = (gcnew System::Windows::Forms::Panel());
			this->chartSectorRxR = (gcnew System::Windows::Forms::DataVisualization::Charting::Chart());
			this->panelQualityRxR = (gcnew System::Windows::Forms::Panel());
			this->labelQualitySNR_R = (gcnew System::Windows::Forms::Label());
			this->labelQualityDV_R = (gcnew System::Windows::Forms::Label());
			this->labelQualityH1H2_R = (gcnew System::Windows::Forms::Label());
			this->panelSectorRxRHeader = (gcnew System::Windows::Forms::Panel());
			this->comboBoxTxRxR = (gcnew System::Windows::Forms::ComboBox());
			this->buttonFreqRxR = (gcnew System::Windows::Forms::Button());
			this->labelSectorRxRTitle = (gcnew System::Windows::Forms::Label());
			this->panelLoggingToolbar = (gcnew System::Windows::Forms::Panel());
			this->labelCursorInfo = (gcnew System::Windows::Forms::Label());
			this->labelSepStream = (gcnew System::Windows::Forms::Label());
			this->buttonRefresh = (gcnew System::Windows::Forms::Button());
			this->buttonStream = (gcnew System::Windows::Forms::Button());
			this->labelSepLayout2 = (gcnew System::Windows::Forms::Label());
			this->radioButtonLayoutBottom = (gcnew System::Windows::Forms::RadioButton());
			this->radioButtonLayoutTop = (gcnew System::Windows::Forms::RadioButton());
			this->radioButtonLayoutBoth = (gcnew System::Windows::Forms::RadioButton());
			this->labelSepLayout1 = (gcnew System::Windows::Forms::Label());
			this->buttonToggleSectors = (gcnew System::Windows::Forms::Button());
			this->buttonToggleSidebar = (gcnew System::Windows::Forms::Button());
			this->tabPageInclinometer = (gcnew System::Windows::Forms::TabPage());
			this->panelInclinometerPlaceholder = (gcnew System::Windows::Forms::Panel());
			this->labelInclinometerTitle = (gcnew System::Windows::Forms::Label());
			this->tabPageHealth = (gcnew System::Windows::Forms::TabPage());
			this->panelHealthPlaceholder = (gcnew System::Windows::Forms::Panel());
			this->labelHealthTitle = (gcnew System::Windows::Forms::Label());
			this->timerStream = (gcnew System::Windows::Forms::Timer(this->components));
			this->toolTipMain = (gcnew System::Windows::Forms::ToolTip(this->components));
			this->openFileDialogBin = (gcnew System::Windows::Forms::OpenFileDialog());
			this->contextMenuCurves1 = (gcnew System::Windows::Forms::ContextMenuStrip(this->components));
			this->contextMenuCurves2 = (gcnew System::Windows::Forms::ContextMenuStrip(this->components));
			this->menuStripMain->SuspendLayout();
			this->statusStripMain->SuspendLayout();
			this->panelLeft->SuspendLayout();
			this->groupBoxMetro->SuspendLayout();
			this->groupBoxMode->SuspendLayout();
			this->groupBoxConnection->SuspendLayout();
			this->panelDrawerHeader->SuspendLayout();
			this->tabControlMain->SuspendLayout();
			this->tabPageLogging->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainerRows))->BeginInit();
			this->splitContainerRows->Panel1->SuspendLayout();
			this->splitContainerRows->Panel2->SuspendLayout();
			this->splitContainerRows->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainerBand1))->BeginInit();
			this->splitContainerBand1->Panel1->SuspendLayout();
			this->splitContainerBand1->Panel2->SuspendLayout();
			this->splitContainerBand1->SuspendLayout();
			this->panelTime1Container->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chartTime1))->BeginInit();
			this->chartTime1->SuspendLayout();
			this->contextMenuChart1->SuspendLayout();
			this->panelTime1Header->SuspendLayout();
			this->panelSectorRxLContainer->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chartSectorRxL))->BeginInit();
			this->panelQualityRxL->SuspendLayout();
			this->panelSectorRxLHeader->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainerBand2))->BeginInit();
			this->splitContainerBand2->Panel1->SuspendLayout();
			this->splitContainerBand2->Panel2->SuspendLayout();
			this->splitContainerBand2->SuspendLayout();
			this->panelTime2Container->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chartTime2))->BeginInit();
			this->chartTime2->SuspendLayout();
			this->contextMenuChart2->SuspendLayout();
			this->panelTime2Header->SuspendLayout();
			this->panelSectorRxRContainer->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chartSectorRxR))->BeginInit();
			this->panelQualityRxR->SuspendLayout();
			this->panelSectorRxRHeader->SuspendLayout();
			this->panelLoggingToolbar->SuspendLayout();
			this->tabPageInclinometer->SuspendLayout();
			this->panelInclinometerPlaceholder->SuspendLayout();
			this->tabPageHealth->SuspendLayout();
			this->panelHealthPlaceholder->SuspendLayout();
			this->SuspendLayout();
			// 
			// menuStripMain
			// 
			this->menuStripMain->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F));
			this->menuStripMain->ImageScalingSize = System::Drawing::Size(20, 20);
			this->menuStripMain->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(5) {
				this->menuItemFile,
					this->menuItemView, this->menuItemComm, this->menuItemMetro, this->menuItemHelp
			});
			this->menuStripMain->Location = System::Drawing::Point(0, 0);
			this->menuStripMain->Name = L"menuStripMain";
			this->menuStripMain->Size = System::Drawing::Size(1384, 29);
			this->menuStripMain->TabIndex = 0;
			this->menuStripMain->Text = L"menuStripMain";
			// 
			// menuItemFile
			// 
			this->menuItemFile->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {
				this->menuItemOpenBin,
					this->menuItemSep1, this->menuItemExit
			});
			this->menuItemFile->Name = L"menuItemFile";
			this->menuItemFile->Size = System::Drawing::Size(61, 25);
			this->menuItemFile->Text = L"&Файл";
			// 
			// menuItemOpenBin
			// 
			this->menuItemOpenBin->Name = L"menuItemOpenBin";
			this->menuItemOpenBin->ShortcutKeys = static_cast<System::Windows::Forms::Keys>((System::Windows::Forms::Keys::Control | System::Windows::Forms::Keys::O));
			this->menuItemOpenBin->Size = System::Drawing::Size(331, 26);
			this->menuItemOpenBin->Text = L"&Открыть файл данных...";
			this->menuItemOpenBin->Click += gcnew System::EventHandler(this, &MainForm::menuItemOpenBin_Click);
			// 
			// menuItemSep1
			// 
			this->menuItemSep1->Name = L"menuItemSep1";
			this->menuItemSep1->Size = System::Drawing::Size(328, 6);
			// 
			// menuItemExit
			// 
			this->menuItemExit->Name = L"menuItemExit";
			this->menuItemExit->Size = System::Drawing::Size(331, 26);
			this->menuItemExit->Text = L"&Выход";
			this->menuItemExit->Click += gcnew System::EventHandler(this, &MainForm::menuItemExit_Click);
			// 
			// menuItemView
			// 
			this->menuItemView->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) { this->menuItemToggleSidebar });
			this->menuItemView->Name = L"menuItemView";
			this->menuItemView->Size = System::Drawing::Size(51, 25);
			this->menuItemView->Text = L"&Вид";
			// 
			// menuItemToggleSidebar
			// 
			this->menuItemToggleSidebar->Name = L"menuItemToggleSidebar";
			this->menuItemToggleSidebar->ShortcutKeys = System::Windows::Forms::Keys::F4;
			this->menuItemToggleSidebar->Size = System::Drawing::Size(266, 26);
			this->menuItemToggleSidebar->Text = L"&Параметры станции";
			this->menuItemToggleSidebar->Click += gcnew System::EventHandler(this, &MainForm::menuItemToggleSidebar_Click);
			// 
			// menuItemComm
			// 
			this->menuItemComm->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {
				this->menuItemConnect,
					this->menuItemDisconnect
			});
			this->menuItemComm->Name = L"menuItemComm";
			this->menuItemComm->Size = System::Drawing::Size(65, 25);
			this->menuItemComm->Text = L"&Связь";
			// 
			// menuItemConnect
			// 
			this->menuItemConnect->Name = L"menuItemConnect";
			this->menuItemConnect->Size = System::Drawing::Size(185, 26);
			this->menuItemConnect->Text = L"&Подключить";
			this->menuItemConnect->Click += gcnew System::EventHandler(this, &MainForm::buttonConnect_Click);
			// 
			// menuItemDisconnect
			// 
			this->menuItemDisconnect->Enabled = false;
			this->menuItemDisconnect->Name = L"menuItemDisconnect";
			this->menuItemDisconnect->Size = System::Drawing::Size(185, 26);
			this->menuItemDisconnect->Text = L"&Отключить";
			this->menuItemDisconnect->Click += gcnew System::EventHandler(this, &MainForm::buttonDisconnect_Click);
			// 
			// menuItemMetro
			// 
			this->menuItemMetro->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(5) {
				this->menuItemMetroParams,
					this->menuItemMetroCalibrate, this->menuItemSepMetro, this->menuItemMetroRead, this->menuItemMetroWrite
			});
			this->menuItemMetro->Name = L"menuItemMetro";
			this->menuItemMetro->Size = System::Drawing::Size(111, 25);
			this->menuItemMetro->Text = L"&Метрология";
			// 
			// menuItemMetroParams
			// 
			this->menuItemMetroParams->Name = L"menuItemMetroParams";
			this->menuItemMetroParams->ShortcutKeys = static_cast<System::Windows::Forms::Keys>((System::Windows::Forms::Keys::Control | System::Windows::Forms::Keys::M));
			this->menuItemMetroParams->Size = System::Drawing::Size(391, 26);
			this->menuItemMetroParams->Text = L"&Параметры метрологии зонда...";
			this->menuItemMetroParams->Click += gcnew System::EventHandler(this, &MainForm::menuItemMetroParams_Click);
			// 
			// menuItemMetroCalibrate
			// 
			this->menuItemMetroCalibrate->Name = L"menuItemMetroCalibrate";
			this->menuItemMetroCalibrate->Size = System::Drawing::Size(391, 26);
			this->menuItemMetroCalibrate->Text = L"&Калибровка нулей...";
			this->menuItemMetroCalibrate->Click += gcnew System::EventHandler(this, &MainForm::buttonCalibrate_Click);
			// 
			// menuItemSepMetro
			// 
			this->menuItemSepMetro->Name = L"menuItemSepMetro";
			this->menuItemSepMetro->Size = System::Drawing::Size(388, 6);
			// 
			// menuItemMetroRead
			// 
			this->menuItemMetroRead->Name = L"menuItemMetroRead";
			this->menuItemMetroRead->Size = System::Drawing::Size(391, 26);
			this->menuItemMetroRead->Text = L"&Прочитать из зонда...";
			this->menuItemMetroRead->Click += gcnew System::EventHandler(this, &MainForm::menuItemMetroParams_Click);
			// 
			// menuItemMetroWrite
			// 
			this->menuItemMetroWrite->Name = L"menuItemMetroWrite";
			this->menuItemMetroWrite->Size = System::Drawing::Size(391, 26);
			this->menuItemMetroWrite->Text = L"&Записать в зонд...";
			this->menuItemMetroWrite->Click += gcnew System::EventHandler(this, &MainForm::menuItemMetroParams_Click);
			// 
			// menuItemHelp
			// 
			this->menuItemHelp->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) { this->menuItemAbout });
			this->menuItemHelp->Name = L"menuItemHelp";
			this->menuItemHelp->Size = System::Drawing::Size(85, 25);
			this->menuItemHelp->Text = L"&Помощь";
			// 
			// menuItemAbout
			// 
			this->menuItemAbout->Name = L"menuItemAbout";
			this->menuItemAbout->Size = System::Drawing::Size(224, 26);
			this->menuItemAbout->Text = L"&О программе...";
			this->menuItemAbout->Click += gcnew System::EventHandler(this, &MainForm::menuItemAbout_Click);
			// 
			// statusStripMain
			// 
			this->statusStripMain->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->statusStripMain->ImageScalingSize = System::Drawing::Size(20, 20);
			this->statusStripMain->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(5) {
				this->statusLabelDevice,
					this->statusLabelConnection, this->statusLabelFPS, this->statusLabelPackets, this->statusLabelFile
			});
			this->statusStripMain->Location = System::Drawing::Point(0, 831);
			this->statusStripMain->Name = L"statusStripMain";
			this->statusStripMain->Size = System::Drawing::Size(1384, 30);
			this->statusStripMain->TabIndex = 1;
			this->statusStripMain->Text = L"statusStripMain";
			// 
			// statusLabelDevice
			// 
			this->statusLabelDevice->BorderSides = System::Windows::Forms::ToolStripStatusLabelBorderSides::Right;
			this->statusLabelDevice->Name = L"statusLabelDevice";
			this->statusLabelDevice->Size = System::Drawing::Size(232, 24);
			this->statusLabelDevice->Text = L"Прибор: АМК-120 «Картограф»";
			// 
			// statusLabelConnection
			// 
			this->statusLabelConnection->BorderSides = System::Windows::Forms::ToolStripStatusLabelBorderSides::Right;
			this->statusLabelConnection->ForeColor = System::Drawing::Color::DimGray;
			this->statusLabelConnection->Name = L"statusLabelConnection";
			this->statusLabelConnection->Size = System::Drawing::Size(151, 24);
			this->statusLabelConnection->Text = L"● Связь: Отключено";
			// 
			// statusLabelFPS
			// 
			this->statusLabelFPS->BorderSides = System::Windows::Forms::ToolStripStatusLabelBorderSides::Right;
			this->statusLabelFPS->Name = L"statusLabelFPS";
			this->statusLabelFPS->Size = System::Drawing::Size(59, 24);
			this->statusLabelFPS->Text = L"FPS: 60";
			// 
			// statusLabelPackets
			// 
			this->statusLabelPackets->BorderSides = System::Windows::Forms::ToolStripStatusLabelBorderSides::Right;
			this->statusLabelPackets->Name = L"statusLabelPackets";
			this->statusLabelPackets->Size = System::Drawing::Size(163, 24);
			this->statusLabelPackets->Text = L"Пакетов: 0 | Потерь: 0";
			// 
			// statusLabelFile
			// 
			this->statusLabelFile->Name = L"statusLabelFile";
			this->statusLabelFile->Size = System::Drawing::Size(764, 24);
			this->statusLabelFile->Spring = true;
			this->statusLabelFile->Text = L"Файл не загружен";
			this->statusLabelFile->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// panelLeft
			// 
			this->panelLeft->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left));
			this->panelLeft->AutoScroll = true;
			this->panelLeft->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(245)), static_cast<System::Int32>(static_cast<System::Byte>(247)),
				static_cast<System::Int32>(static_cast<System::Byte>(250)));
			this->panelLeft->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->panelLeft->Controls->Add(this->groupBoxMetro);
			this->panelLeft->Controls->Add(this->groupBoxMode);
			this->panelLeft->Controls->Add(this->groupBoxConnection);
			this->panelLeft->Controls->Add(this->panelDrawerHeader);
			this->panelLeft->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->panelLeft->Location = System::Drawing::Point(0, 25);
			this->panelLeft->Name = L"panelLeft";
			this->panelLeft->Size = System::Drawing::Size(284, 812);
			this->panelLeft->TabIndex = 2;
			this->panelLeft->Visible = false;
			// 
			// groupBoxMetro
			// 
			this->groupBoxMetro->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->groupBoxMetro->Controls->Add(this->buttonMetroParams);
			this->groupBoxMetro->Controls->Add(this->buttonExportCsv);
			this->groupBoxMetro->Controls->Add(this->buttonSelfTest);
			this->groupBoxMetro->Controls->Add(this->buttonCalibrate);
			this->groupBoxMetro->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F, System::Drawing::FontStyle::Bold));
			this->groupBoxMetro->Location = System::Drawing::Point(8, 394);
			this->groupBoxMetro->Name = L"groupBoxMetro";
			this->groupBoxMetro->Size = System::Drawing::Size(264, 190);
			this->groupBoxMetro->TabIndex = 2;
			this->groupBoxMetro->TabStop = false;
			this->groupBoxMetro->Text = L"3. Метрология и Тест";
			// 
			// buttonMetroParams
			// 
			this->buttonMetroParams->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->buttonMetroParams->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->buttonMetroParams->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(30)),
				static_cast<System::Int32>(static_cast<System::Byte>(70)), static_cast<System::Int32>(static_cast<System::Byte>(140)));
			this->buttonMetroParams->Location = System::Drawing::Point(12, 144);
			this->buttonMetroParams->Name = L"buttonMetroParams";
			this->buttonMetroParams->Size = System::Drawing::Size(240, 32);
			this->buttonMetroParams->TabIndex = 3;
			this->buttonMetroParams->Text = L"Параметры метрологии...";
			this->buttonMetroParams->UseVisualStyleBackColor = true;
			this->buttonMetroParams->Click += gcnew System::EventHandler(this, &MainForm::menuItemMetroParams_Click);
			// 
			// buttonExportCsv
			// 
			this->buttonExportCsv->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->buttonExportCsv->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->buttonExportCsv->Location = System::Drawing::Point(12, 106);
			this->buttonExportCsv->Name = L"buttonExportCsv";
			this->buttonExportCsv->Size = System::Drawing::Size(240, 32);
			this->buttonExportCsv->TabIndex = 2;
			this->buttonExportCsv->Text = L"Экспорт всего файла в CSV";
			this->buttonExportCsv->UseVisualStyleBackColor = true;
			this->buttonExportCsv->Click += gcnew System::EventHandler(this, &MainForm::buttonExportCsv_Click);
			// 
			// buttonSelfTest
			// 
			this->buttonSelfTest->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->buttonSelfTest->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->buttonSelfTest->Location = System::Drawing::Point(12, 68);
			this->buttonSelfTest->Name = L"buttonSelfTest";
			this->buttonSelfTest->Size = System::Drawing::Size(240, 32);
			this->buttonSelfTest->TabIndex = 1;
			this->buttonSelfTest->Text = L"Самодиагностика (ТЕСТ)";
			this->buttonSelfTest->UseVisualStyleBackColor = true;
			this->buttonSelfTest->Click += gcnew System::EventHandler(this, &MainForm::buttonSelfTest_Click);
			// 
			// buttonCalibrate
			// 
			this->buttonCalibrate->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->buttonCalibrate->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->buttonCalibrate->Location = System::Drawing::Point(12, 30);
			this->buttonCalibrate->Name = L"buttonCalibrate";
			this->buttonCalibrate->Size = System::Drawing::Size(240, 32);
			this->buttonCalibrate->TabIndex = 0;
			this->buttonCalibrate->Text = L"Калибровка нулей...";
			this->buttonCalibrate->UseVisualStyleBackColor = true;
			this->buttonCalibrate->Click += gcnew System::EventHandler(this, &MainForm::buttonCalibrate_Click);
			// 
			// groupBoxMode
			// 
			this->groupBoxMode->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->groupBoxMode->Controls->Add(this->radioButtonModeDiag);
			this->groupBoxMode->Controls->Add(this->radioButtonModeLog);
			this->groupBoxMode->Controls->Add(this->radioButtonModeStand);
			this->groupBoxMode->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F, System::Drawing::FontStyle::Bold));
			this->groupBoxMode->Location = System::Drawing::Point(8, 266);
			this->groupBoxMode->Name = L"groupBoxMode";
			this->groupBoxMode->Size = System::Drawing::Size(264, 120);
			this->groupBoxMode->TabIndex = 1;
			this->groupBoxMode->TabStop = false;
			this->groupBoxMode->Text = L"2. Режим прибора";
			// 
			// radioButtonModeDiag
			// 
			this->radioButtonModeDiag->AutoSize = true;
			this->radioButtonModeDiag->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->radioButtonModeDiag->Location = System::Drawing::Point(15, 85);
			this->radioButtonModeDiag->Name = L"radioButtonModeDiag";
			this->radioButtonModeDiag->Size = System::Drawing::Size(118, 24);
			this->radioButtonModeDiag->TabIndex = 2;
			this->radioButtonModeDiag->Text = L"Диагностика";
			this->radioButtonModeDiag->UseVisualStyleBackColor = true;
			// 
			// radioButtonModeLog
			// 
			this->radioButtonModeLog->AutoSize = true;
			this->radioButtonModeLog->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->radioButtonModeLog->Location = System::Drawing::Point(15, 58);
			this->radioButtonModeLog->Name = L"radioButtonModeLog";
			this->radioButtonModeLog->Size = System::Drawing::Size(148, 24);
			this->radioButtonModeLog->TabIndex = 1;
			this->radioButtonModeLog->Text = L"Каротаж (скваж.)";
			this->radioButtonModeLog->UseVisualStyleBackColor = true;
			// 
			// radioButtonModeStand
			// 
			this->radioButtonModeStand->AutoSize = true;
			this->radioButtonModeStand->Checked = true;
			this->radioButtonModeStand->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->radioButtonModeStand->Location = System::Drawing::Point(15, 30);
			this->radioButtonModeStand->Name = L"radioButtonModeStand";
			this->radioButtonModeStand->Size = System::Drawing::Size(155, 24);
			this->radioButtonModeStand->TabIndex = 0;
			this->radioButtonModeStand->TabStop = true;
			this->radioButtonModeStand->Text = L"Стендовая запись";
			this->radioButtonModeStand->UseVisualStyleBackColor = true;
			// 
			// groupBoxConnection
			// 
			this->groupBoxConnection->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->groupBoxConnection->Controls->Add(this->labelConnStatus);
			this->groupBoxConnection->Controls->Add(this->buttonDisconnect);
			this->groupBoxConnection->Controls->Add(this->buttonConnect);
			this->groupBoxConnection->Controls->Add(this->textBoxConnParam);
			this->groupBoxConnection->Controls->Add(this->labelConnParam);
			this->groupBoxConnection->Controls->Add(this->comboBoxConnType);
			this->groupBoxConnection->Controls->Add(this->labelConnType);
			this->groupBoxConnection->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.5F, System::Drawing::FontStyle::Bold));
			this->groupBoxConnection->Location = System::Drawing::Point(8, 46);
			this->groupBoxConnection->Name = L"groupBoxConnection";
			this->groupBoxConnection->Size = System::Drawing::Size(264, 212);
			this->groupBoxConnection->TabIndex = 0;
			this->groupBoxConnection->TabStop = false;
			this->groupBoxConnection->Text = L"1. Связь с прибором";
			// 
			// labelConnStatus
			// 
			this->labelConnStatus->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->labelConnStatus->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelConnStatus->ForeColor = System::Drawing::Color::Gray;
			this->labelConnStatus->Location = System::Drawing::Point(12, 175);
			this->labelConnStatus->Name = L"labelConnStatus";
			this->labelConnStatus->Size = System::Drawing::Size(240, 24);
			this->labelConnStatus->TabIndex = 6;
			this->labelConnStatus->Text = L"● Статус: Отключено";
			this->labelConnStatus->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// buttonDisconnect
			// 
			this->buttonDisconnect->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->buttonDisconnect->Enabled = false;
			this->buttonDisconnect->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->buttonDisconnect->Location = System::Drawing::Point(135, 132);
			this->buttonDisconnect->Name = L"buttonDisconnect";
			this->buttonDisconnect->Size = System::Drawing::Size(117, 34);
			this->buttonDisconnect->TabIndex = 5;
			this->buttonDisconnect->Text = L"Отключить";
			this->buttonDisconnect->UseVisualStyleBackColor = true;
			this->buttonDisconnect->Click += gcnew System::EventHandler(this, &MainForm::buttonDisconnect_Click);
			// 
			// buttonConnect
			// 
			this->buttonConnect->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->buttonConnect->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(220)), static_cast<System::Int32>(static_cast<System::Byte>(245)),
				static_cast<System::Int32>(static_cast<System::Byte>(220)));
			this->buttonConnect->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->buttonConnect->Location = System::Drawing::Point(12, 132);
			this->buttonConnect->Name = L"buttonConnect";
			this->buttonConnect->Size = System::Drawing::Size(117, 34);
			this->buttonConnect->TabIndex = 4;
			this->buttonConnect->Text = L"Старт";
			this->buttonConnect->UseVisualStyleBackColor = false;
			this->buttonConnect->Click += gcnew System::EventHandler(this, &MainForm::buttonConnect_Click);
			// 
			// textBoxConnParam
			// 
			this->textBoxConnParam->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->textBoxConnParam->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->textBoxConnParam->Location = System::Drawing::Point(12, 96);
			this->textBoxConnParam->Name = L"textBoxConnParam";
			this->textBoxConnParam->Size = System::Drawing::Size(240, 27);
			this->textBoxConnParam->TabIndex = 3;
			this->textBoxConnParam->Text = L"192.168.4.1:5000";
			// 
			// labelConnParam
			// 
			this->labelConnParam->AutoSize = true;
			this->labelConnParam->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->labelConnParam->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(70)), static_cast<System::Int32>(static_cast<System::Byte>(70)),
				static_cast<System::Int32>(static_cast<System::Byte>(70)));
			this->labelConnParam->Location = System::Drawing::Point(12, 78);
			this->labelConnParam->Name = L"labelConnParam";
			this->labelConnParam->Size = System::Drawing::Size(103, 20);
			this->labelConnParam->TabIndex = 2;
			this->labelConnParam->Text = L"Адрес / Порт:";
			// 
			// comboBoxConnType
			// 
			this->comboBoxConnType->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->comboBoxConnType->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxConnType->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->comboBoxConnType->FormattingEnabled = true;
			this->comboBoxConnType->Items->AddRange(gcnew cli::array< System::Object^  >(3) {
				L"UDP Сеть (WiFi / Ethernet)", L"Последовательный COM-порт",
					L"Файл данных (.bin / .las)"
			});
			this->comboBoxConnType->Location = System::Drawing::Point(12, 47);
			this->comboBoxConnType->Name = L"comboBoxConnType";
			this->comboBoxConnType->Size = System::Drawing::Size(240, 28);
			this->comboBoxConnType->TabIndex = 1;
			// 
			// labelConnType
			// 
			this->labelConnType->AutoSize = true;
			this->labelConnType->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->labelConnType->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(70)), static_cast<System::Int32>(static_cast<System::Byte>(70)),
				static_cast<System::Int32>(static_cast<System::Byte>(70)));
			this->labelConnType->Location = System::Drawing::Point(12, 28);
			this->labelConnType->Name = L"labelConnType";
			this->labelConnType->Size = System::Drawing::Size(89, 20);
			this->labelConnType->TabIndex = 0;
			this->labelConnType->Text = L"Интерфейс:";
			// 
			// panelDrawerHeader
			// 
			this->panelDrawerHeader->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(33)),
				static_cast<System::Int32>(static_cast<System::Byte>(43)), static_cast<System::Int32>(static_cast<System::Byte>(54)));
			this->panelDrawerHeader->Controls->Add(this->buttonCloseSidebar);
			this->panelDrawerHeader->Controls->Add(this->buttonPinSidebar);
			this->panelDrawerHeader->Controls->Add(this->labelDrawerTitle);
			this->panelDrawerHeader->Dock = System::Windows::Forms::DockStyle::Top;
			this->panelDrawerHeader->Location = System::Drawing::Point(0, 0);
			this->panelDrawerHeader->Name = L"panelDrawerHeader";
			this->panelDrawerHeader->Size = System::Drawing::Size(282, 38);
			this->panelDrawerHeader->TabIndex = 3;
			// 
			// buttonCloseSidebar
			// 
			this->buttonCloseSidebar->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(45)),
				static_cast<System::Int32>(static_cast<System::Byte>(58)), static_cast<System::Int32>(static_cast<System::Byte>(72)));
			this->buttonCloseSidebar->FlatAppearance->BorderSize = 0;
			this->buttonCloseSidebar->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->buttonCloseSidebar->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->buttonCloseSidebar->ForeColor = System::Drawing::Color::White;
			this->buttonCloseSidebar->Location = System::Drawing::Point(246, 6);
			this->buttonCloseSidebar->Name = L"buttonCloseSidebar";
			this->buttonCloseSidebar->Size = System::Drawing::Size(26, 26);
			this->buttonCloseSidebar->TabIndex = 2;
			this->buttonCloseSidebar->Text = L"✕";
			this->toolTipMain->SetToolTip(this->buttonCloseSidebar, L"Скрыть панель параметров (Esc)");
			this->buttonCloseSidebar->UseVisualStyleBackColor = false;
			this->buttonCloseSidebar->Click += gcnew System::EventHandler(this, &MainForm::buttonCloseSidebar_Click);
			// 
			// buttonPinSidebar
			// 
			this->buttonPinSidebar->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(45)), static_cast<System::Int32>(static_cast<System::Byte>(58)),
				static_cast<System::Int32>(static_cast<System::Byte>(72)));
			this->buttonPinSidebar->FlatAppearance->BorderSize = 0;
			this->buttonPinSidebar->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->buttonPinSidebar->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8));
			this->buttonPinSidebar->ForeColor = System::Drawing::Color::White;
			this->buttonPinSidebar->Location = System::Drawing::Point(155, 6);
			this->buttonPinSidebar->Name = L"buttonPinSidebar";
			this->buttonPinSidebar->Size = System::Drawing::Size(85, 26);
			this->buttonPinSidebar->TabIndex = 1;
			this->buttonPinSidebar->Text = L"📌 Авто";
			this->toolTipMain->SetToolTip(this->buttonPinSidebar, L"Переключить режим закрепления панели поверх графиков");
			this->buttonPinSidebar->UseVisualStyleBackColor = false;
			this->buttonPinSidebar->Click += gcnew System::EventHandler(this, &MainForm::buttonPinSidebar_Click);
			// 
			// labelDrawerTitle
			// 
			this->labelDrawerTitle->AutoSize = true;
			this->labelDrawerTitle->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelDrawerTitle->ForeColor = System::Drawing::Color::White;
			this->labelDrawerTitle->Location = System::Drawing::Point(10, 11);
			this->labelDrawerTitle->Name = L"labelDrawerTitle";
			this->labelDrawerTitle->Size = System::Drawing::Size(129, 20);
			this->labelDrawerTitle->TabIndex = 0;
			this->labelDrawerTitle->Text = L"⚙ ПАРАМЕТРЫ";
			// 
			// tabControlMain
			// 
			this->tabControlMain->Controls->Add(this->tabPageLogging);
			this->tabControlMain->Controls->Add(this->tabPageInclinometer);
			this->tabControlMain->Controls->Add(this->tabPageHealth);
			this->tabControlMain->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tabControlMain->Font = (gcnew System::Drawing::Font(L"Segoe UI", 10));
			this->tabControlMain->Location = System::Drawing::Point(0, 29);
			this->tabControlMain->Name = L"tabControlMain";
			this->tabControlMain->SelectedIndex = 0;
			this->tabControlMain->Size = System::Drawing::Size(1384, 802);
			this->tabControlMain->TabIndex = 3;
			this->tabControlMain->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::tabControlMain_MouseDown);
			// 
			// tabPageLogging
			// 
			this->tabPageLogging->Controls->Add(this->splitContainerRows);
			this->tabPageLogging->Controls->Add(this->panelLoggingToolbar);
			this->tabPageLogging->Location = System::Drawing::Point(4, 32);
			this->tabPageLogging->Name = L"tabPageLogging";
			this->tabPageLogging->Padding = System::Windows::Forms::Padding(4);
			this->tabPageLogging->Size = System::Drawing::Size(1376, 766);
			this->tabPageLogging->TabIndex = 0;
			this->tabPageLogging->Text = L"Каротаж и Секторы";
			this->tabPageLogging->UseVisualStyleBackColor = true;
			// 
			// splitContainerRows
			// 
			this->splitContainerRows->Dock = System::Windows::Forms::DockStyle::Fill;
			this->splitContainerRows->Location = System::Drawing::Point(4, 38);
			this->splitContainerRows->Name = L"splitContainerRows";
			this->splitContainerRows->Orientation = System::Windows::Forms::Orientation::Horizontal;
			// 
			// splitContainerRows.Panel1
			// 
			this->splitContainerRows->Panel1->Controls->Add(this->splitContainerBand1);
			this->splitContainerRows->Panel1MinSize = 150;
			// 
			// splitContainerRows.Panel2
			// 
			this->splitContainerRows->Panel2->Controls->Add(this->splitContainerBand2);
			this->splitContainerRows->Panel2MinSize = 150;
			this->splitContainerRows->Size = System::Drawing::Size(1368, 724);
			this->splitContainerRows->SplitterDistance = 357;
			this->splitContainerRows->SplitterWidth = 6;
			this->splitContainerRows->TabIndex = 0;
			// 
			// splitContainerBand1
			// 
			this->splitContainerBand1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->splitContainerBand1->Location = System::Drawing::Point(0, 0);
			this->splitContainerBand1->Name = L"splitContainerBand1";
			// 
			// splitContainerBand1.Panel1
			// 
			this->splitContainerBand1->Panel1->Controls->Add(this->panelTime1Container);
			this->splitContainerBand1->Panel1MinSize = 250;
			// 
			// splitContainerBand1.Panel2
			// 
			this->splitContainerBand1->Panel2->Controls->Add(this->panelSectorRxLContainer);
			this->splitContainerBand1->Panel2MinSize = 200;
			this->splitContainerBand1->Size = System::Drawing::Size(1368, 357);
			this->splitContainerBand1->SplitterDistance = 804;
			this->splitContainerBand1->SplitterWidth = 6;
			this->splitContainerBand1->TabIndex = 0;
			this->splitContainerBand1->SplitterMoved += gcnew System::Windows::Forms::SplitterEventHandler(this, &MainForm::splitContainerBand_SplitterMoved);
			// 
			// panelTime1Container
			// 
			this->panelTime1Container->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->panelTime1Container->Controls->Add(this->chartTime1);
			this->panelTime1Container->Controls->Add(this->panelTime1Header);
			this->panelTime1Container->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panelTime1Container->Location = System::Drawing::Point(0, 0);
			this->panelTime1Container->Name = L"panelTime1Container";
			this->panelTime1Container->Size = System::Drawing::Size(804, 357);
			this->panelTime1Container->TabIndex = 0;
			// 
			// chartTime1
			// 
			this->chartTime1->ContextMenuStrip = this->contextMenuChart1;
			this->chartTime1->Controls->Add(this->buttonZoomReset1);
			this->chartTime1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->chartTime1->Location = System::Drawing::Point(0, 58);
			this->chartTime1->Name = L"chartTime1";
			this->chartTime1->Size = System::Drawing::Size(802, 297);
			this->chartTime1->TabIndex = 1;
			this->chartTime1->Text = L"chartTime1";
			this->chartTime1->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::chartTime_MouseDown);
			this->chartTime1->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::chartTime_MouseMove);
			this->chartTime1->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::chartTime_MouseUp);
			// 
			// contextMenuChart1
			// 
			this->contextMenuChart1->ImageScalingSize = System::Drawing::Size(20, 20);
			this->contextMenuChart1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(6) {
				this->menuItemSetMin1,
					this->menuItemSetMax1, this->menuSepChart1, this->menuItemZoomBackX1, this->menuItemZoomBackY1, this->menuItemResetRange1
			});
			this->contextMenuChart1->Name = L"contextMenuChart1";
			this->contextMenuChart1->Size = System::Drawing::Size(321, 130);
			// 
			// menuItemSetMin1
			// 
			this->menuItemSetMin1->Name = L"menuItemSetMin1";
			this->menuItemSetMin1->Size = System::Drawing::Size(320, 24);
			this->menuItemSetMin1->Text = L"Задать как Min (визир)";
			this->menuItemSetMin1->Click += gcnew System::EventHandler(this, &MainForm::menuItemSetMin1_Click);
			// 
			// menuItemSetMax1
			// 
			this->menuItemSetMax1->Name = L"menuItemSetMax1";
			this->menuItemSetMax1->Size = System::Drawing::Size(320, 24);
			this->menuItemSetMax1->Text = L"Задать как Max (визир)";
			this->menuItemSetMax1->Click += gcnew System::EventHandler(this, &MainForm::menuItemSetMax1_Click);
			// 
			// menuSepChart1
			// 
			this->menuSepChart1->Name = L"menuSepChart1";
			this->menuSepChart1->Size = System::Drawing::Size(317, 6);
			// 
			// menuItemZoomBackX1
			// 
			this->menuItemZoomBackX1->Name = L"menuItemZoomBackX1";
			this->menuItemZoomBackX1->Size = System::Drawing::Size(320, 24);
			this->menuItemZoomBackX1->Text = L"🔍- Шаг назад по X (Zoom -1)";
			this->menuItemZoomBackX1->Click += gcnew System::EventHandler(this, &MainForm::menuItemZoomBackX1_Click);
			// 
			// menuItemZoomBackY1
			// 
			this->menuItemZoomBackY1->Name = L"menuItemZoomBackY1";
			this->menuItemZoomBackY1->Size = System::Drawing::Size(320, 24);
			this->menuItemZoomBackY1->Text = L"🔍- Шаг назад по Y (Zoom -1)";
			this->menuItemZoomBackY1->Click += gcnew System::EventHandler(this, &MainForm::menuItemZoomBackY1_Click);
			// 
			// menuItemResetRange1
			// 
			this->menuItemResetRange1->Name = L"menuItemResetRange1";
			this->menuItemResetRange1->Size = System::Drawing::Size(320, 24);
			this->menuItemResetRange1->Text = L"Сбросить диапазон (Показать всё)";
			this->menuItemResetRange1->Click += gcnew System::EventHandler(this, &MainForm::menuItemResetRange1_Click);
			// 
			// buttonZoomReset1
			// 
			this->buttonZoomReset1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->buttonZoomReset1->Cursor = System::Windows::Forms::Cursors::Hand;
			this->buttonZoomReset1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 7.5F, System::Drawing::FontStyle::Bold));
			this->buttonZoomReset1->Location = System::Drawing::Point(1084, 4);
			this->buttonZoomReset1->Name = L"buttonZoomReset1";
			this->buttonZoomReset1->Size = System::Drawing::Size(50, 22);
			this->buttonZoomReset1->TabIndex = 99;
			this->buttonZoomReset1->Text = L"⛶ 1:1";
			this->buttonZoomReset1->UseVisualStyleBackColor = true;
			this->buttonZoomReset1->Click += gcnew System::EventHandler(this, &MainForm::buttonZoomReset1_Click);
			// 
			// panelTime1Header
			// 
			this->panelTime1Header->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(245)),
				static_cast<System::Int32>(static_cast<System::Byte>(247)), static_cast<System::Int32>(static_cast<System::Byte>(250)));
			this->panelTime1Header->Controls->Add(this->buttonAutoY1);
			this->panelTime1Header->Controls->Add(this->comboBoxPrecision1);
			this->panelTime1Header->Controls->Add(this->checkHoldZoom1);
			this->panelTime1Header->Controls->Add(this->buttonResetRange1);
			this->panelTime1Header->Controls->Add(this->buttonApplyRange1);
			this->panelTime1Header->Controls->Add(this->textBoxMax1);
			this->panelTime1Header->Controls->Add(this->labelMax1);
			this->panelTime1Header->Controls->Add(this->textBoxMin1);
			this->panelTime1Header->Controls->Add(this->labelMin1);
			this->panelTime1Header->Controls->Add(this->checkTx1_5);
			this->panelTime1Header->Controls->Add(this->checkTx1_4);
			this->panelTime1Header->Controls->Add(this->checkTx1_3);
			this->panelTime1Header->Controls->Add(this->checkTx1_2);
			this->panelTime1Header->Controls->Add(this->checkTx1_1);
			this->panelTime1Header->Controls->Add(this->flowPanelChips1);
			this->panelTime1Header->Controls->Add(this->buttonAddCurve1);
			this->panelTime1Header->Controls->Add(this->buttonXMode1);
			this->panelTime1Header->Controls->Add(this->buttonFreq1);
			this->panelTime1Header->Controls->Add(this->buttonMode1);
			this->panelTime1Header->Dock = System::Windows::Forms::DockStyle::Top;
			this->panelTime1Header->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->panelTime1Header->Location = System::Drawing::Point(0, 0);
			this->panelTime1Header->Name = L"panelTime1Header";
			this->panelTime1Header->Padding = System::Windows::Forms::Padding(3);
			this->panelTime1Header->Size = System::Drawing::Size(802, 58);
			this->panelTime1Header->TabIndex = 0;
			// 
			// buttonAutoY1
			// 
			this->buttonAutoY1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->buttonAutoY1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8));
			this->buttonAutoY1->Location = System::Drawing::Point(742, 30);
			this->buttonAutoY1->Name = L"buttonAutoY1";
			this->buttonAutoY1->Size = System::Drawing::Size(52, 24);
			this->buttonAutoY1->TabIndex = 17;
			this->buttonAutoY1->Text = L"Авто Y";
			this->buttonAutoY1->UseVisualStyleBackColor = true;
			this->buttonAutoY1->Click += gcnew System::EventHandler(this, &MainForm::buttonAutoY1_Click);
			// 
			// comboBoxPrecision1
			// 
			this->comboBoxPrecision1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->comboBoxPrecision1->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxPrecision1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 7.8F));
			this->comboBoxPrecision1->FormattingEnabled = true;
			this->comboBoxPrecision1->Items->AddRange(gcnew cli::array< System::Object^  >(7) {
				L"Точн: Авто", L"Точн: 0", L"Точн: 1",
					L"Точн: 2", L"Точн: 3", L"Точн: 4", L"Точн: 5"
			});
			this->comboBoxPrecision1->Location = System::Drawing::Point(659, 32);
			this->comboBoxPrecision1->Name = L"comboBoxPrecision1";
			this->comboBoxPrecision1->Size = System::Drawing::Size(79, 25);
			this->comboBoxPrecision1->TabIndex = 16;
			this->comboBoxPrecision1->SelectedIndexChanged += gcnew System::EventHandler(this, &MainForm::comboBoxPrecision1_SelectedIndexChanged);
			// 
			// checkHoldZoom1
			// 
			this->checkHoldZoom1->AutoSize = true;
			this->checkHoldZoom1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 7.8F));
			this->checkHoldZoom1->Location = System::Drawing::Point(415, 34);
			this->checkHoldZoom1->Name = L"checkHoldZoom1";
			this->checkHoldZoom1->Size = System::Drawing::Size(86, 21);
			this->checkHoldZoom1->TabIndex = 15;
			this->checkHoldZoom1->Text = L"Фикс. зум";
			this->checkHoldZoom1->UseVisualStyleBackColor = true;
			// 
			// buttonResetRange1
			// 
			this->buttonResetRange1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonResetRange1->Location = System::Drawing::Point(375, 30);
			this->buttonResetRange1->Name = L"buttonResetRange1";
			this->buttonResetRange1->Size = System::Drawing::Size(36, 24);
			this->buttonResetRange1->TabIndex = 14;
			this->buttonResetRange1->Text = L"Все";
			this->buttonResetRange1->UseVisualStyleBackColor = true;
			this->buttonResetRange1->Click += gcnew System::EventHandler(this, &MainForm::buttonResetRange1_Click);
			// 
			// buttonApplyRange1
			// 
			this->buttonApplyRange1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonApplyRange1->Location = System::Drawing::Point(344, 30);
			this->buttonApplyRange1->Name = L"buttonApplyRange1";
			this->buttonApplyRange1->Size = System::Drawing::Size(28, 24);
			this->buttonApplyRange1->TabIndex = 13;
			this->buttonApplyRange1->Text = L">>";
			this->buttonApplyRange1->UseVisualStyleBackColor = true;
			this->buttonApplyRange1->Click += gcnew System::EventHandler(this, &MainForm::buttonApplyRange1_Click);
			// 
			// textBoxMax1
			// 
			this->textBoxMax1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->textBoxMax1->Location = System::Drawing::Point(298, 31);
			this->textBoxMax1->Name = L"textBoxMax1";
			this->textBoxMax1->Size = System::Drawing::Size(44, 26);
			this->textBoxMax1->TabIndex = 12;
			this->textBoxMax1->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &MainForm::textBoxRange1_KeyDown);
			this->textBoxMax1->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MainForm::textBoxRange_KeyPress);
			// 
			// labelMax1
			// 
			this->labelMax1->AutoSize = true;
			this->labelMax1->Cursor = System::Windows::Forms::Cursors::Hand;
			this->labelMax1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->labelMax1->Location = System::Drawing::Point(269, 35);
			this->labelMax1->Name = L"labelMax1";
			this->labelMax1->Size = System::Drawing::Size(42, 19);
			this->labelMax1->TabIndex = 11;
			this->labelMax1->Text = L"Max:";
			this->labelMax1->Click += gcnew System::EventHandler(this, &MainForm::menuItemSetMax1_Click);
			// 
			// textBoxMin1
			// 
			this->textBoxMin1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->textBoxMin1->Location = System::Drawing::Point(222, 31);
			this->textBoxMin1->Name = L"textBoxMin1";
			this->textBoxMin1->Size = System::Drawing::Size(44, 26);
			this->textBoxMin1->TabIndex = 10;
			this->textBoxMin1->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &MainForm::textBoxRange1_KeyDown);
			this->textBoxMin1->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MainForm::textBoxRange_KeyPress);
			// 
			// labelMin1
			// 
			this->labelMin1->AutoSize = true;
			this->labelMin1->Cursor = System::Windows::Forms::Cursors::Hand;
			this->labelMin1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->labelMin1->Location = System::Drawing::Point(194, 35);
			this->labelMin1->Name = L"labelMin1";
			this->labelMin1->Size = System::Drawing::Size(38, 19);
			this->labelMin1->TabIndex = 9;
			this->labelMin1->Text = L"Min:";
			this->labelMin1->Click += gcnew System::EventHandler(this, &MainForm::menuItemSetMin1_Click);
			// 
			// checkTx1_5
			// 
			this->checkTx1_5->AutoSize = true;
			this->checkTx1_5->Checked = true;
			this->checkTx1_5->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx1_5->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx1_5->ForeColor = System::Drawing::Color::DarkViolet;
			this->checkTx1_5->Location = System::Drawing::Point(152, 33);
			this->checkTx1_5->Name = L"checkTx1_5";
			this->checkTx1_5->Size = System::Drawing::Size(54, 23);
			this->checkTx1_5->TabIndex = 8;
			this->checkTx1_5->Text = L"Tx5";
			this->checkTx1_5->UseVisualStyleBackColor = true;
			this->checkTx1_5->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx1_CheckedChanged);
			// 
			// checkTx1_4
			// 
			this->checkTx1_4->AutoSize = true;
			this->checkTx1_4->Checked = true;
			this->checkTx1_4->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx1_4->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx1_4->ForeColor = System::Drawing::Color::DarkOrange;
			this->checkTx1_4->Location = System::Drawing::Point(114, 33);
			this->checkTx1_4->Name = L"checkTx1_4";
			this->checkTx1_4->Size = System::Drawing::Size(54, 23);
			this->checkTx1_4->TabIndex = 7;
			this->checkTx1_4->Text = L"Tx4";
			this->checkTx1_4->UseVisualStyleBackColor = true;
			this->checkTx1_4->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx1_CheckedChanged);
			// 
			// checkTx1_3
			// 
			this->checkTx1_3->AutoSize = true;
			this->checkTx1_3->Checked = true;
			this->checkTx1_3->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx1_3->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx1_3->ForeColor = System::Drawing::Color::ForestGreen;
			this->checkTx1_3->Location = System::Drawing::Point(76, 33);
			this->checkTx1_3->Name = L"checkTx1_3";
			this->checkTx1_3->Size = System::Drawing::Size(54, 23);
			this->checkTx1_3->TabIndex = 6;
			this->checkTx1_3->Text = L"Tx3";
			this->checkTx1_3->UseVisualStyleBackColor = true;
			this->checkTx1_3->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx1_CheckedChanged);
			// 
			// checkTx1_2
			// 
			this->checkTx1_2->AutoSize = true;
			this->checkTx1_2->Checked = true;
			this->checkTx1_2->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx1_2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx1_2->ForeColor = System::Drawing::Color::Crimson;
			this->checkTx1_2->Location = System::Drawing::Point(38, 33);
			this->checkTx1_2->Name = L"checkTx1_2";
			this->checkTx1_2->Size = System::Drawing::Size(54, 23);
			this->checkTx1_2->TabIndex = 5;
			this->checkTx1_2->Text = L"Tx2";
			this->checkTx1_2->UseVisualStyleBackColor = true;
			this->checkTx1_2->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx1_CheckedChanged);
			// 
			// checkTx1_1
			// 
			this->checkTx1_1->AutoSize = true;
			this->checkTx1_1->Checked = true;
			this->checkTx1_1->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx1_1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx1_1->ForeColor = System::Drawing::Color::RoyalBlue;
			this->checkTx1_1->Location = System::Drawing::Point(2, 33);
			this->checkTx1_1->Name = L"checkTx1_1";
			this->checkTx1_1->Size = System::Drawing::Size(54, 23);
			this->checkTx1_1->TabIndex = 4;
			this->checkTx1_1->Text = L"Tx1";
			this->checkTx1_1->UseVisualStyleBackColor = true;
			this->checkTx1_1->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx1_CheckedChanged);
			// 
			// flowPanelChips1
			// 
			this->flowPanelChips1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->flowPanelChips1->AutoScroll = true;
			this->flowPanelChips1->Location = System::Drawing::Point(292, 1);
			this->flowPanelChips1->Name = L"flowPanelChips1";
			this->flowPanelChips1->Size = System::Drawing::Size(502, 26);
			this->flowPanelChips1->TabIndex = 3;
			this->flowPanelChips1->WrapContents = false;
			// 
			// buttonAddCurve1
			// 
			this->buttonAddCurve1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonAddCurve1->Location = System::Drawing::Point(208, 3);
			this->buttonAddCurve1->Name = L"buttonAddCurve1";
			this->buttonAddCurve1->Size = System::Drawing::Size(80, 24);
			this->buttonAddCurve1->TabIndex = 2;
			this->buttonAddCurve1->Text = L"+ Кривые ▼";
			this->buttonAddCurve1->UseVisualStyleBackColor = true;
			this->buttonAddCurve1->Click += gcnew System::EventHandler(this, &MainForm::buttonAddCurve1_Click);
			// 
			// buttonXMode1
			// 
			this->buttonXMode1->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(240)), static_cast<System::Int32>(static_cast<System::Byte>(240)),
				static_cast<System::Int32>(static_cast<System::Byte>(245)));
			this->buttonXMode1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonXMode1->Location = System::Drawing::Point(146, 3);
			this->buttonXMode1->Name = L"buttonXMode1";
			this->buttonXMode1->Size = System::Drawing::Size(58, 24);
			this->buttonXMode1->TabIndex = 18;
			this->buttonXMode1->Text = L"Кадр";
			this->buttonXMode1->UseVisualStyleBackColor = false;
			this->buttonXMode1->Click += gcnew System::EventHandler(this, &MainForm::buttonXMode1_Click);
			// 
			// buttonFreq1
			// 
			this->buttonFreq1->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(225)), static_cast<System::Int32>(static_cast<System::Byte>(235)),
				static_cast<System::Int32>(static_cast<System::Byte>(250)));
			this->buttonFreq1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonFreq1->Location = System::Drawing::Point(82, 3);
			this->buttonFreq1->Name = L"buttonFreq1";
			this->buttonFreq1->Size = System::Drawing::Size(62, 24);
			this->buttonFreq1->TabIndex = 1;
			this->buttonFreq1->Text = L"2000 кГц";
			this->buttonFreq1->UseVisualStyleBackColor = false;
			this->buttonFreq1->Click += gcnew System::EventHandler(this, &MainForm::buttonFreq1_Click);
			// 
			// buttonMode1
			// 
			this->buttonMode1->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(220)), static_cast<System::Int32>(static_cast<System::Byte>(245)),
				static_cast<System::Int32>(static_cast<System::Byte>(225)));
			this->buttonMode1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonMode1->Location = System::Drawing::Point(4, 3);
			this->buttonMode1->Name = L"buttonMode1";
			this->buttonMode1->Size = System::Drawing::Size(75, 24);
			this->buttonMode1->TabIndex = 0;
			this->buttonMode1->Text = L"UNDIRECT";
			this->buttonMode1->UseVisualStyleBackColor = false;
			this->buttonMode1->Click += gcnew System::EventHandler(this, &MainForm::buttonMode1_Click);
			// 
			// panelSectorRxLContainer
			// 
			this->panelSectorRxLContainer->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->panelSectorRxLContainer->Controls->Add(this->chartSectorRxL);
			this->panelSectorRxLContainer->Controls->Add(this->panelQualityRxL);
			this->panelSectorRxLContainer->Controls->Add(this->panelSectorRxLHeader);
			this->panelSectorRxLContainer->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panelSectorRxLContainer->Location = System::Drawing::Point(0, 0);
			this->panelSectorRxLContainer->Name = L"panelSectorRxLContainer";
			this->panelSectorRxLContainer->Size = System::Drawing::Size(558, 357);
			this->panelSectorRxLContainer->TabIndex = 0;
			// 
			// chartSectorRxL
			// 
			this->chartSectorRxL->Dock = System::Windows::Forms::DockStyle::Fill;
			this->chartSectorRxL->Location = System::Drawing::Point(0, 32);
			this->chartSectorRxL->Name = L"chartSectorRxL";
			this->chartSectorRxL->Size = System::Drawing::Size(556, 297);
			this->chartSectorRxL->TabIndex = 2;
			this->chartSectorRxL->Text = L"chartSectorRxL";
			this->chartSectorRxL->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::chartSector_MouseDown);
			// 
			// panelQualityRxL
			// 
			this->panelQualityRxL->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(245)), static_cast<System::Int32>(static_cast<System::Byte>(248)),
				static_cast<System::Int32>(static_cast<System::Byte>(252)));
			this->panelQualityRxL->Controls->Add(this->labelQualitySNR_L);
			this->panelQualityRxL->Controls->Add(this->labelQualityDV_L);
			this->panelQualityRxL->Controls->Add(this->labelQualityH1H2_L);
			this->panelQualityRxL->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->panelQualityRxL->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->panelQualityRxL->Location = System::Drawing::Point(0, 329);
			this->panelQualityRxL->Name = L"panelQualityRxL";
			this->panelQualityRxL->Padding = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->panelQualityRxL->Size = System::Drawing::Size(556, 26);
			this->panelQualityRxL->TabIndex = 1;
			// 
			// labelQualitySNR_L
			// 
			this->labelQualitySNR_L->AutoSize = true;
			this->labelQualitySNR_L->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->labelQualitySNR_L->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(30)),
				static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(160)));
			this->labelQualitySNR_L->Location = System::Drawing::Point(280, 5);
			this->labelQualitySNR_L->Name = L"labelQualitySNR_L";
			this->labelQualitySNR_L->Size = System::Drawing::Size(101, 20);
			this->labelQualitySNR_L->TabIndex = 2;
			this->labelQualitySNR_L->Text = L"SNR: 24.5 дБ";
			// 
			// labelQualityDV_L
			// 
			this->labelQualityDV_L->AutoSize = true;
			this->labelQualityDV_L->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->labelQualityDV_L->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(180)),
				static_cast<System::Int32>(static_cast<System::Byte>(70)), static_cast<System::Int32>(static_cast<System::Byte>(20)));
			this->labelQualityDV_L->Location = System::Drawing::Point(140, 5);
			this->labelQualityDV_L->Name = L"labelQualityDV_L";
			this->labelQualityDV_L->Size = System::Drawing::Size(144, 20);
			this->labelQualityDV_L->TabIndex = 1;
			this->labelQualityDV_L->Text = L"Мусор dV: 0.12 мВ";
			// 
			// labelQualityH1H2_L
			// 
			this->labelQualityH1H2_L->AutoSize = true;
			this->labelQualityH1H2_L->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->labelQualityH1H2_L->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(20)),
				static_cast<System::Int32>(static_cast<System::Byte>(120)), static_cast<System::Int32>(static_cast<System::Byte>(60)));
			this->labelQualityH1H2_L->Location = System::Drawing::Point(6, 5);
			this->labelQualityH1H2_L->Name = L"labelQualityH1H2_L";
			this->labelQualityH1H2_L->Size = System::Drawing::Size(123, 20);
			this->labelQualityH1H2_L->TabIndex = 0;
			this->labelQualityH1H2_L->Text = L"H1/H2: 12.4 раз";
			// 
			// panelSectorRxLHeader
			// 
			this->panelSectorRxLHeader->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(240)),
				static_cast<System::Int32>(static_cast<System::Byte>(242)), static_cast<System::Int32>(static_cast<System::Byte>(245)));
			this->panelSectorRxLHeader->Controls->Add(this->comboBoxTxRxL);
			this->panelSectorRxLHeader->Controls->Add(this->buttonFreqRxL);
			this->panelSectorRxLHeader->Controls->Add(this->labelSectorRxLTitle);
			this->panelSectorRxLHeader->Dock = System::Windows::Forms::DockStyle::Top;
			this->panelSectorRxLHeader->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->panelSectorRxLHeader->Location = System::Drawing::Point(0, 0);
			this->panelSectorRxLHeader->Name = L"panelSectorRxLHeader";
			this->panelSectorRxLHeader->Padding = System::Windows::Forms::Padding(3);
			this->panelSectorRxLHeader->Size = System::Drawing::Size(556, 32);
			this->panelSectorRxLHeader->TabIndex = 0;
			// 
			// comboBoxTxRxL
			// 
			this->comboBoxTxRxL->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->comboBoxTxRxL->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxTxRxL->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->comboBoxTxRxL->FormattingEnabled = true;
			this->comboBoxTxRxL->Items->AddRange(gcnew cli::array< System::Object^  >(5) {
				L"Tx1 (365 мм)", L"Tx2 (710 мм)", L"Tx3 (1070 мм)",
					L"Tx4 (1415 мм)", L"Tx5 (1768 мм)"
			});
			this->comboBoxTxRxL->Location = System::Drawing::Point(451, 5);
			this->comboBoxTxRxL->Name = L"comboBoxTxRxL";
			this->comboBoxTxRxL->Size = System::Drawing::Size(100, 27);
			this->comboBoxTxRxL->TabIndex = 2;
			this->comboBoxTxRxL->SelectedIndexChanged += gcnew System::EventHandler(this, &MainForm::comboBoxTxRx_SelectedIndexChanged);
			// 
			// buttonFreqRxL
			// 
			this->buttonFreqRxL->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->buttonFreqRxL->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(225)), static_cast<System::Int32>(static_cast<System::Byte>(235)),
				static_cast<System::Int32>(static_cast<System::Byte>(250)));
			this->buttonFreqRxL->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonFreqRxL->Location = System::Drawing::Point(386, 3);
			this->buttonFreqRxL->Name = L"buttonFreqRxL";
			this->buttonFreqRxL->Size = System::Drawing::Size(60, 26);
			this->buttonFreqRxL->TabIndex = 1;
			this->buttonFreqRxL->Text = L"2000 кГц";
			this->buttonFreqRxL->UseVisualStyleBackColor = false;
			this->buttonFreqRxL->Click += gcnew System::EventHandler(this, &MainForm::buttonFreqRxL_Click);
			// 
			// labelSectorRxLTitle
			// 
			this->labelSectorRxLTitle->AutoSize = true;
			this->labelSectorRxLTitle->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelSectorRxLTitle->Location = System::Drawing::Point(6, 8);
			this->labelSectorRxLTitle->Name = L"labelSectorRxLTitle";
			this->labelSectorRxLTitle->Size = System::Drawing::Size(192, 20);
			this->labelSectorRxLTitle->TabIndex = 0;
			this->labelSectorRxLTitle->Text = L"16 секторов: Приемник L";
			// 
			// splitContainerBand2
			// 
			this->splitContainerBand2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->splitContainerBand2->Location = System::Drawing::Point(0, 0);
			this->splitContainerBand2->Name = L"splitContainerBand2";
			// 
			// splitContainerBand2.Panel1
			// 
			this->splitContainerBand2->Panel1->Controls->Add(this->panelTime2Container);
			this->splitContainerBand2->Panel1MinSize = 250;
			// 
			// splitContainerBand2.Panel2
			// 
			this->splitContainerBand2->Panel2->Controls->Add(this->panelSectorRxRContainer);
			this->splitContainerBand2->Panel2MinSize = 200;
			this->splitContainerBand2->Size = System::Drawing::Size(1368, 361);
			this->splitContainerBand2->SplitterDistance = 804;
			this->splitContainerBand2->SplitterWidth = 6;
			this->splitContainerBand2->TabIndex = 0;
			this->splitContainerBand2->SplitterMoved += gcnew System::Windows::Forms::SplitterEventHandler(this, &MainForm::splitContainerBand_SplitterMoved);
			// 
			// panelTime2Container
			// 
			this->panelTime2Container->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->panelTime2Container->Controls->Add(this->chartTime2);
			this->panelTime2Container->Controls->Add(this->panelTime2Header);
			this->panelTime2Container->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panelTime2Container->Location = System::Drawing::Point(0, 0);
			this->panelTime2Container->Name = L"panelTime2Container";
			this->panelTime2Container->Size = System::Drawing::Size(804, 361);
			this->panelTime2Container->TabIndex = 1;
			// 
			// chartTime2
			// 
			this->chartTime2->ContextMenuStrip = this->contextMenuChart2;
			this->chartTime2->Controls->Add(this->buttonZoomReset2);
			this->chartTime2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->chartTime2->Location = System::Drawing::Point(0, 58);
			this->chartTime2->Name = L"chartTime2";
			this->chartTime2->Size = System::Drawing::Size(802, 301);
			this->chartTime2->TabIndex = 1;
			this->chartTime2->Text = L"chartTime2";
			this->chartTime2->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::chartTime_MouseDown);
			this->chartTime2->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::chartTime_MouseMove);
			this->chartTime2->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::chartTime_MouseUp);
			// 
			// contextMenuChart2
			// 
			this->contextMenuChart2->ImageScalingSize = System::Drawing::Size(20, 20);
			this->contextMenuChart2->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(6) {
				this->menuItemSetMin2,
					this->menuItemSetMax2, this->menuSepChart2, this->menuItemZoomBackX2, this->menuItemZoomBackY2, this->menuItemResetRange2
			});
			this->contextMenuChart2->Name = L"contextMenuChart2";
			this->contextMenuChart2->Size = System::Drawing::Size(321, 130);
			// 
			// menuItemSetMin2
			// 
			this->menuItemSetMin2->Name = L"menuItemSetMin2";
			this->menuItemSetMin2->Size = System::Drawing::Size(320, 24);
			this->menuItemSetMin2->Text = L"Задать как Min (визир)";
			this->menuItemSetMin2->Click += gcnew System::EventHandler(this, &MainForm::menuItemSetMin2_Click);
			// 
			// menuItemSetMax2
			// 
			this->menuItemSetMax2->Name = L"menuItemSetMax2";
			this->menuItemSetMax2->Size = System::Drawing::Size(320, 24);
			this->menuItemSetMax2->Text = L"Задать как Max (визир)";
			this->menuItemSetMax2->Click += gcnew System::EventHandler(this, &MainForm::menuItemSetMax2_Click);
			// 
			// menuSepChart2
			// 
			this->menuSepChart2->Name = L"menuSepChart2";
			this->menuSepChart2->Size = System::Drawing::Size(317, 6);
			// 
			// menuItemZoomBackX2
			// 
			this->menuItemZoomBackX2->Name = L"menuItemZoomBackX2";
			this->menuItemZoomBackX2->Size = System::Drawing::Size(320, 24);
			this->menuItemZoomBackX2->Text = L"🔍- Шаг назад по X (Zoom -1)";
			this->menuItemZoomBackX2->Click += gcnew System::EventHandler(this, &MainForm::menuItemZoomBackX2_Click);
			// 
			// menuItemZoomBackY2
			// 
			this->menuItemZoomBackY2->Name = L"menuItemZoomBackY2";
			this->menuItemZoomBackY2->Size = System::Drawing::Size(320, 24);
			this->menuItemZoomBackY2->Text = L"🔍- Шаг назад по Y (Zoom -1)";
			this->menuItemZoomBackY2->Click += gcnew System::EventHandler(this, &MainForm::menuItemZoomBackY2_Click);
			// 
			// menuItemResetRange2
			// 
			this->menuItemResetRange2->Name = L"menuItemResetRange2";
			this->menuItemResetRange2->Size = System::Drawing::Size(320, 24);
			this->menuItemResetRange2->Text = L"Сбросить диапазон (Показать всё)";
			this->menuItemResetRange2->Click += gcnew System::EventHandler(this, &MainForm::menuItemResetRange2_Click);
			// 
			// buttonZoomReset2
			// 
			this->buttonZoomReset2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->buttonZoomReset2->Cursor = System::Windows::Forms::Cursors::Hand;
			this->buttonZoomReset2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 7.5F, System::Drawing::FontStyle::Bold));
			this->buttonZoomReset2->Location = System::Drawing::Point(1084, 4);
			this->buttonZoomReset2->Name = L"buttonZoomReset2";
			this->buttonZoomReset2->Size = System::Drawing::Size(50, 22);
			this->buttonZoomReset2->TabIndex = 99;
			this->buttonZoomReset2->Text = L"⛶ 1:1";
			this->buttonZoomReset2->UseVisualStyleBackColor = true;
			this->buttonZoomReset2->Click += gcnew System::EventHandler(this, &MainForm::buttonZoomReset2_Click);
			// 
			// panelTime2Header
			// 
			this->panelTime2Header->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(245)),
				static_cast<System::Int32>(static_cast<System::Byte>(247)), static_cast<System::Int32>(static_cast<System::Byte>(250)));
			this->panelTime2Header->Controls->Add(this->buttonAutoY2);
			this->panelTime2Header->Controls->Add(this->comboBoxPrecision2);
			this->panelTime2Header->Controls->Add(this->checkHoldZoom2);
			this->panelTime2Header->Controls->Add(this->buttonResetRange2);
			this->panelTime2Header->Controls->Add(this->buttonApplyRange2);
			this->panelTime2Header->Controls->Add(this->textBoxMax2);
			this->panelTime2Header->Controls->Add(this->labelMax2);
			this->panelTime2Header->Controls->Add(this->textBoxMin2);
			this->panelTime2Header->Controls->Add(this->labelMin2);
			this->panelTime2Header->Controls->Add(this->checkTx2_5);
			this->panelTime2Header->Controls->Add(this->checkTx2_4);
			this->panelTime2Header->Controls->Add(this->checkTx2_3);
			this->panelTime2Header->Controls->Add(this->checkTx2_2);
			this->panelTime2Header->Controls->Add(this->checkTx2_1);
			this->panelTime2Header->Controls->Add(this->flowPanelChips2);
			this->panelTime2Header->Controls->Add(this->buttonAddCurve2);
			this->panelTime2Header->Controls->Add(this->buttonXMode2);
			this->panelTime2Header->Controls->Add(this->buttonFreq2);
			this->panelTime2Header->Controls->Add(this->buttonMode2);
			this->panelTime2Header->Dock = System::Windows::Forms::DockStyle::Top;
			this->panelTime2Header->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->panelTime2Header->Location = System::Drawing::Point(0, 0);
			this->panelTime2Header->Name = L"panelTime2Header";
			this->panelTime2Header->Padding = System::Windows::Forms::Padding(3);
			this->panelTime2Header->Size = System::Drawing::Size(802, 58);
			this->panelTime2Header->TabIndex = 0;
			// 
			// buttonAutoY2
			// 
			this->buttonAutoY2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->buttonAutoY2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8));
			this->buttonAutoY2->Location = System::Drawing::Point(742, 30);
			this->buttonAutoY2->Name = L"buttonAutoY2";
			this->buttonAutoY2->Size = System::Drawing::Size(52, 24);
			this->buttonAutoY2->TabIndex = 17;
			this->buttonAutoY2->Text = L"Авто Y";
			this->buttonAutoY2->UseVisualStyleBackColor = true;
			this->buttonAutoY2->Click += gcnew System::EventHandler(this, &MainForm::buttonAutoY2_Click);
			// 
			// comboBoxPrecision2
			// 
			this->comboBoxPrecision2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->comboBoxPrecision2->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxPrecision2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 7.8F));
			this->comboBoxPrecision2->FormattingEnabled = true;
			this->comboBoxPrecision2->Items->AddRange(gcnew cli::array< System::Object^  >(7) {
				L"Точн: Авто", L"Точн: 0", L"Точн: 1",
					L"Точн: 2", L"Точн: 3", L"Точн: 4", L"Точн: 5"
			});
			this->comboBoxPrecision2->Location = System::Drawing::Point(659, 32);
			this->comboBoxPrecision2->Name = L"comboBoxPrecision2";
			this->comboBoxPrecision2->Size = System::Drawing::Size(79, 25);
			this->comboBoxPrecision2->TabIndex = 16;
			this->comboBoxPrecision2->SelectedIndexChanged += gcnew System::EventHandler(this, &MainForm::comboBoxPrecision2_SelectedIndexChanged);
			// 
			// checkHoldZoom2
			// 
			this->checkHoldZoom2->AutoSize = true;
			this->checkHoldZoom2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 7.8F));
			this->checkHoldZoom2->Location = System::Drawing::Point(415, 34);
			this->checkHoldZoom2->Name = L"checkHoldZoom2";
			this->checkHoldZoom2->Size = System::Drawing::Size(86, 21);
			this->checkHoldZoom2->TabIndex = 15;
			this->checkHoldZoom2->Text = L"Фикс. зум";
			this->checkHoldZoom2->UseVisualStyleBackColor = true;
			// 
			// buttonResetRange2
			// 
			this->buttonResetRange2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonResetRange2->Location = System::Drawing::Point(375, 30);
			this->buttonResetRange2->Name = L"buttonResetRange2";
			this->buttonResetRange2->Size = System::Drawing::Size(36, 24);
			this->buttonResetRange2->TabIndex = 14;
			this->buttonResetRange2->Text = L"Все";
			this->buttonResetRange2->UseVisualStyleBackColor = true;
			this->buttonResetRange2->Click += gcnew System::EventHandler(this, &MainForm::buttonResetRange2_Click);
			// 
			// buttonApplyRange2
			// 
			this->buttonApplyRange2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonApplyRange2->Location = System::Drawing::Point(344, 30);
			this->buttonApplyRange2->Name = L"buttonApplyRange2";
			this->buttonApplyRange2->Size = System::Drawing::Size(28, 24);
			this->buttonApplyRange2->TabIndex = 13;
			this->buttonApplyRange2->Text = L">>";
			this->buttonApplyRange2->UseVisualStyleBackColor = true;
			this->buttonApplyRange2->Click += gcnew System::EventHandler(this, &MainForm::buttonApplyRange2_Click);
			// 
			// textBoxMax2
			// 
			this->textBoxMax2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->textBoxMax2->Location = System::Drawing::Point(298, 31);
			this->textBoxMax2->Name = L"textBoxMax2";
			this->textBoxMax2->Size = System::Drawing::Size(44, 26);
			this->textBoxMax2->TabIndex = 12;
			this->textBoxMax2->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &MainForm::textBoxRange2_KeyDown);
			this->textBoxMax2->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MainForm::textBoxRange_KeyPress);
			// 
			// labelMax2
			// 
			this->labelMax2->AutoSize = true;
			this->labelMax2->Cursor = System::Windows::Forms::Cursors::Hand;
			this->labelMax2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->labelMax2->Location = System::Drawing::Point(269, 35);
			this->labelMax2->Name = L"labelMax2";
			this->labelMax2->Size = System::Drawing::Size(42, 19);
			this->labelMax2->TabIndex = 11;
			this->labelMax2->Text = L"Max:";
			this->labelMax2->Click += gcnew System::EventHandler(this, &MainForm::menuItemSetMax2_Click);
			// 
			// textBoxMin2
			// 
			this->textBoxMin2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->textBoxMin2->Location = System::Drawing::Point(222, 31);
			this->textBoxMin2->Name = L"textBoxMin2";
			this->textBoxMin2->Size = System::Drawing::Size(44, 26);
			this->textBoxMin2->TabIndex = 10;
			this->textBoxMin2->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &MainForm::textBoxRange2_KeyDown);
			this->textBoxMin2->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &MainForm::textBoxRange_KeyPress);
			// 
			// labelMin2
			// 
			this->labelMin2->AutoSize = true;
			this->labelMin2->Cursor = System::Windows::Forms::Cursors::Hand;
			this->labelMin2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->labelMin2->Location = System::Drawing::Point(194, 35);
			this->labelMin2->Name = L"labelMin2";
			this->labelMin2->Size = System::Drawing::Size(38, 19);
			this->labelMin2->TabIndex = 9;
			this->labelMin2->Text = L"Min:";
			this->labelMin2->Click += gcnew System::EventHandler(this, &MainForm::menuItemSetMin2_Click);
			// 
			// checkTx2_5
			// 
			this->checkTx2_5->AutoSize = true;
			this->checkTx2_5->Checked = true;
			this->checkTx2_5->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx2_5->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx2_5->ForeColor = System::Drawing::Color::DarkViolet;
			this->checkTx2_5->Location = System::Drawing::Point(152, 33);
			this->checkTx2_5->Name = L"checkTx2_5";
			this->checkTx2_5->Size = System::Drawing::Size(54, 23);
			this->checkTx2_5->TabIndex = 8;
			this->checkTx2_5->Text = L"Tx5";
			this->checkTx2_5->UseVisualStyleBackColor = true;
			this->checkTx2_5->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx2_CheckedChanged);
			// 
			// checkTx2_4
			// 
			this->checkTx2_4->AutoSize = true;
			this->checkTx2_4->Checked = true;
			this->checkTx2_4->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx2_4->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx2_4->ForeColor = System::Drawing::Color::DarkOrange;
			this->checkTx2_4->Location = System::Drawing::Point(114, 33);
			this->checkTx2_4->Name = L"checkTx2_4";
			this->checkTx2_4->Size = System::Drawing::Size(54, 23);
			this->checkTx2_4->TabIndex = 7;
			this->checkTx2_4->Text = L"Tx4";
			this->checkTx2_4->UseVisualStyleBackColor = true;
			this->checkTx2_4->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx2_CheckedChanged);
			// 
			// checkTx2_3
			// 
			this->checkTx2_3->AutoSize = true;
			this->checkTx2_3->Checked = true;
			this->checkTx2_3->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx2_3->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx2_3->ForeColor = System::Drawing::Color::ForestGreen;
			this->checkTx2_3->Location = System::Drawing::Point(76, 33);
			this->checkTx2_3->Name = L"checkTx2_3";
			this->checkTx2_3->Size = System::Drawing::Size(54, 23);
			this->checkTx2_3->TabIndex = 6;
			this->checkTx2_3->Text = L"Tx3";
			this->checkTx2_3->UseVisualStyleBackColor = true;
			this->checkTx2_3->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx2_CheckedChanged);
			// 
			// checkTx2_2
			// 
			this->checkTx2_2->AutoSize = true;
			this->checkTx2_2->Checked = true;
			this->checkTx2_2->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx2_2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx2_2->ForeColor = System::Drawing::Color::Crimson;
			this->checkTx2_2->Location = System::Drawing::Point(38, 33);
			this->checkTx2_2->Name = L"checkTx2_2";
			this->checkTx2_2->Size = System::Drawing::Size(54, 23);
			this->checkTx2_2->TabIndex = 5;
			this->checkTx2_2->Text = L"Tx2";
			this->checkTx2_2->UseVisualStyleBackColor = true;
			this->checkTx2_2->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx2_CheckedChanged);
			// 
			// checkTx2_1
			// 
			this->checkTx2_1->AutoSize = true;
			this->checkTx2_1->Checked = true;
			this->checkTx2_1->CheckState = System::Windows::Forms::CheckState::Checked;
			this->checkTx2_1->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->checkTx2_1->ForeColor = System::Drawing::Color::RoyalBlue;
			this->checkTx2_1->Location = System::Drawing::Point(2, 33);
			this->checkTx2_1->Name = L"checkTx2_1";
			this->checkTx2_1->Size = System::Drawing::Size(54, 23);
			this->checkTx2_1->TabIndex = 4;
			this->checkTx2_1->Text = L"Tx1";
			this->checkTx2_1->UseVisualStyleBackColor = true;
			this->checkTx2_1->CheckedChanged += gcnew System::EventHandler(this, &MainForm::checkTx2_CheckedChanged);
			// 
			// flowPanelChips2
			// 
			this->flowPanelChips2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->flowPanelChips2->AutoScroll = true;
			this->flowPanelChips2->Location = System::Drawing::Point(292, 1);
			this->flowPanelChips2->Name = L"flowPanelChips2";
			this->flowPanelChips2->Size = System::Drawing::Size(502, 26);
			this->flowPanelChips2->TabIndex = 3;
			this->flowPanelChips2->WrapContents = false;
			// 
			// buttonAddCurve2
			// 
			this->buttonAddCurve2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonAddCurve2->Location = System::Drawing::Point(208, 3);
			this->buttonAddCurve2->Name = L"buttonAddCurve2";
			this->buttonAddCurve2->Size = System::Drawing::Size(80, 24);
			this->buttonAddCurve2->TabIndex = 2;
			this->buttonAddCurve2->Text = L"+ Кривые ▼";
			this->buttonAddCurve2->UseVisualStyleBackColor = true;
			this->buttonAddCurve2->Click += gcnew System::EventHandler(this, &MainForm::buttonAddCurve2_Click);
			// 
			// buttonXMode2
			// 
			this->buttonXMode2->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(240)), static_cast<System::Int32>(static_cast<System::Byte>(240)),
				static_cast<System::Int32>(static_cast<System::Byte>(245)));
			this->buttonXMode2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonXMode2->Location = System::Drawing::Point(146, 3);
			this->buttonXMode2->Name = L"buttonXMode2";
			this->buttonXMode2->Size = System::Drawing::Size(58, 24);
			this->buttonXMode2->TabIndex = 18;
			this->buttonXMode2->Text = L"Кадр";
			this->buttonXMode2->UseVisualStyleBackColor = false;
			this->buttonXMode2->Click += gcnew System::EventHandler(this, &MainForm::buttonXMode2_Click);
			// 
			// buttonFreq2
			// 
			this->buttonFreq2->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(225)), static_cast<System::Int32>(static_cast<System::Byte>(235)),
				static_cast<System::Int32>(static_cast<System::Byte>(250)));
			this->buttonFreq2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonFreq2->Location = System::Drawing::Point(82, 3);
			this->buttonFreq2->Name = L"buttonFreq2";
			this->buttonFreq2->Size = System::Drawing::Size(62, 24);
			this->buttonFreq2->TabIndex = 1;
			this->buttonFreq2->Text = L"400 кГц";
			this->buttonFreq2->UseVisualStyleBackColor = false;
			this->buttonFreq2->Click += gcnew System::EventHandler(this, &MainForm::buttonFreq2_Click);
			// 
			// buttonMode2
			// 
			this->buttonMode2->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(220)), static_cast<System::Int32>(static_cast<System::Byte>(245)),
				static_cast<System::Int32>(static_cast<System::Byte>(225)));
			this->buttonMode2->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonMode2->Location = System::Drawing::Point(4, 3);
			this->buttonMode2->Name = L"buttonMode2";
			this->buttonMode2->Size = System::Drawing::Size(75, 24);
			this->buttonMode2->TabIndex = 0;
			this->buttonMode2->Text = L"UNDIRECT";
			this->buttonMode2->UseVisualStyleBackColor = false;
			this->buttonMode2->Click += gcnew System::EventHandler(this, &MainForm::buttonMode2_Click);
			// 
			// panelSectorRxRContainer
			// 
			this->panelSectorRxRContainer->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->panelSectorRxRContainer->Controls->Add(this->chartSectorRxR);
			this->panelSectorRxRContainer->Controls->Add(this->panelQualityRxR);
			this->panelSectorRxRContainer->Controls->Add(this->panelSectorRxRHeader);
			this->panelSectorRxRContainer->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panelSectorRxRContainer->Location = System::Drawing::Point(0, 0);
			this->panelSectorRxRContainer->Name = L"panelSectorRxRContainer";
			this->panelSectorRxRContainer->Size = System::Drawing::Size(558, 361);
			this->panelSectorRxRContainer->TabIndex = 1;
			// 
			// chartSectorRxR
			// 
			this->chartSectorRxR->Dock = System::Windows::Forms::DockStyle::Fill;
			this->chartSectorRxR->Location = System::Drawing::Point(0, 32);
			this->chartSectorRxR->Name = L"chartSectorRxR";
			this->chartSectorRxR->Size = System::Drawing::Size(556, 301);
			this->chartSectorRxR->TabIndex = 2;
			this->chartSectorRxR->Text = L"chartSectorRxR";
			this->chartSectorRxR->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::chartSector_MouseDown);
			// 
			// panelQualityRxR
			// 
			this->panelQualityRxR->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(245)), static_cast<System::Int32>(static_cast<System::Byte>(248)),
				static_cast<System::Int32>(static_cast<System::Byte>(252)));
			this->panelQualityRxR->Controls->Add(this->labelQualitySNR_R);
			this->panelQualityRxR->Controls->Add(this->labelQualityDV_R);
			this->panelQualityRxR->Controls->Add(this->labelQualityH1H2_R);
			this->panelQualityRxR->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->panelQualityRxR->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->panelQualityRxR->Location = System::Drawing::Point(0, 333);
			this->panelQualityRxR->Name = L"panelQualityRxR";
			this->panelQualityRxR->Padding = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->panelQualityRxR->Size = System::Drawing::Size(556, 26);
			this->panelQualityRxR->TabIndex = 1;
			// 
			// labelQualitySNR_R
			// 
			this->labelQualitySNR_R->AutoSize = true;
			this->labelQualitySNR_R->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->labelQualitySNR_R->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(30)),
				static_cast<System::Int32>(static_cast<System::Byte>(90)), static_cast<System::Int32>(static_cast<System::Byte>(160)));
			this->labelQualitySNR_R->Location = System::Drawing::Point(280, 5);
			this->labelQualitySNR_R->Name = L"labelQualitySNR_R";
			this->labelQualitySNR_R->Size = System::Drawing::Size(101, 20);
			this->labelQualitySNR_R->TabIndex = 2;
			this->labelQualitySNR_R->Text = L"SNR: 23.8 дБ";
			// 
			// labelQualityDV_R
			// 
			this->labelQualityDV_R->AutoSize = true;
			this->labelQualityDV_R->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->labelQualityDV_R->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(180)),
				static_cast<System::Int32>(static_cast<System::Byte>(70)), static_cast<System::Int32>(static_cast<System::Byte>(20)));
			this->labelQualityDV_R->Location = System::Drawing::Point(140, 5);
			this->labelQualityDV_R->Name = L"labelQualityDV_R";
			this->labelQualityDV_R->Size = System::Drawing::Size(144, 20);
			this->labelQualityDV_R->TabIndex = 1;
			this->labelQualityDV_R->Text = L"Мусор dV: 0.15 мВ";
			// 
			// labelQualityH1H2_R
			// 
			this->labelQualityH1H2_R->AutoSize = true;
			this->labelQualityH1H2_R->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->labelQualityH1H2_R->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(20)),
				static_cast<System::Int32>(static_cast<System::Byte>(120)), static_cast<System::Int32>(static_cast<System::Byte>(60)));
			this->labelQualityH1H2_R->Location = System::Drawing::Point(6, 5);
			this->labelQualityH1H2_R->Name = L"labelQualityH1H2_R";
			this->labelQualityH1H2_R->Size = System::Drawing::Size(123, 20);
			this->labelQualityH1H2_R->TabIndex = 0;
			this->labelQualityH1H2_R->Text = L"H1/H2: 11.8 раз";
			// 
			// panelSectorRxRHeader
			// 
			this->panelSectorRxRHeader->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(240)),
				static_cast<System::Int32>(static_cast<System::Byte>(242)), static_cast<System::Int32>(static_cast<System::Byte>(245)));
			this->panelSectorRxRHeader->Controls->Add(this->comboBoxTxRxR);
			this->panelSectorRxRHeader->Controls->Add(this->buttonFreqRxR);
			this->panelSectorRxRHeader->Controls->Add(this->labelSectorRxRTitle);
			this->panelSectorRxRHeader->Dock = System::Windows::Forms::DockStyle::Top;
			this->panelSectorRxRHeader->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F));
			this->panelSectorRxRHeader->Location = System::Drawing::Point(0, 0);
			this->panelSectorRxRHeader->Name = L"panelSectorRxRHeader";
			this->panelSectorRxRHeader->Padding = System::Windows::Forms::Padding(3);
			this->panelSectorRxRHeader->Size = System::Drawing::Size(556, 32);
			this->panelSectorRxRHeader->TabIndex = 0;
			// 
			// comboBoxTxRxR
			// 
			this->comboBoxTxRxR->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->comboBoxTxRxR->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboBoxTxRxR->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->comboBoxTxRxR->FormattingEnabled = true;
			this->comboBoxTxRxR->Items->AddRange(gcnew cli::array< System::Object^  >(5) {
				L"Tx1 (365 мм)", L"Tx2 (710 мм)", L"Tx3 (1070 мм)",
					L"Tx4 (1415 мм)", L"Tx5 (1768 мм)"
			});
			this->comboBoxTxRxR->Location = System::Drawing::Point(451, 5);
			this->comboBoxTxRxR->Name = L"comboBoxTxRxR";
			this->comboBoxTxRxR->Size = System::Drawing::Size(100, 27);
			this->comboBoxTxRxR->TabIndex = 2;
			this->comboBoxTxRxR->SelectedIndexChanged += gcnew System::EventHandler(this, &MainForm::comboBoxTxRx_SelectedIndexChanged);
			// 
			// buttonFreqRxR
			// 
			this->buttonFreqRxR->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->buttonFreqRxR->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(225)), static_cast<System::Int32>(static_cast<System::Byte>(235)),
				static_cast<System::Int32>(static_cast<System::Byte>(250)));
			this->buttonFreqRxR->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8, System::Drawing::FontStyle::Bold));
			this->buttonFreqRxR->Location = System::Drawing::Point(386, 3);
			this->buttonFreqRxR->Name = L"buttonFreqRxR";
			this->buttonFreqRxR->Size = System::Drawing::Size(60, 26);
			this->buttonFreqRxR->TabIndex = 1;
			this->buttonFreqRxR->Text = L"2000 кГц";
			this->buttonFreqRxR->UseVisualStyleBackColor = false;
			this->buttonFreqRxR->Click += gcnew System::EventHandler(this, &MainForm::buttonFreqRxR_Click);
			// 
			// labelSectorRxRTitle
			// 
			this->labelSectorRxRTitle->AutoSize = true;
			this->labelSectorRxRTitle->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9, System::Drawing::FontStyle::Bold));
			this->labelSectorRxRTitle->Location = System::Drawing::Point(6, 8);
			this->labelSectorRxRTitle->Name = L"labelSectorRxRTitle";
			this->labelSectorRxRTitle->Size = System::Drawing::Size(194, 20);
			this->labelSectorRxRTitle->TabIndex = 0;
			this->labelSectorRxRTitle->Text = L"16 секторов: Приемник R";
			// 
			// panelLoggingToolbar
			// 
			this->panelLoggingToolbar->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(240)),
				static_cast<System::Int32>(static_cast<System::Byte>(243)), static_cast<System::Int32>(static_cast<System::Byte>(248)));
			this->panelLoggingToolbar->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->panelLoggingToolbar->Controls->Add(this->labelCursorInfo);
			this->panelLoggingToolbar->Controls->Add(this->labelSepStream);
			this->panelLoggingToolbar->Controls->Add(this->buttonRefresh);
			this->panelLoggingToolbar->Controls->Add(this->buttonStream);
			this->panelLoggingToolbar->Controls->Add(this->labelSepLayout2);
			this->panelLoggingToolbar->Controls->Add(this->radioButtonLayoutBottom);
			this->panelLoggingToolbar->Controls->Add(this->radioButtonLayoutTop);
			this->panelLoggingToolbar->Controls->Add(this->radioButtonLayoutBoth);
			this->panelLoggingToolbar->Controls->Add(this->labelSepLayout1);
			this->panelLoggingToolbar->Controls->Add(this->buttonToggleSectors);
			this->panelLoggingToolbar->Controls->Add(this->buttonToggleSidebar);
			this->panelLoggingToolbar->Dock = System::Windows::Forms::DockStyle::Top;
			this->panelLoggingToolbar->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->panelLoggingToolbar->Location = System::Drawing::Point(4, 4);
			this->panelLoggingToolbar->Name = L"panelLoggingToolbar";
			this->panelLoggingToolbar->Padding = System::Windows::Forms::Padding(4, 2, 4, 2);
			this->panelLoggingToolbar->Size = System::Drawing::Size(1368, 34);
			this->panelLoggingToolbar->TabIndex = 1;
			// 
			// labelCursorInfo
			// 
			this->labelCursorInfo->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			this->labelCursorInfo->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->labelCursorInfo->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(40)), static_cast<System::Int32>(static_cast<System::Byte>(70)),
				static_cast<System::Int32>(static_cast<System::Byte>(130)));
			this->labelCursorInfo->Location = System::Drawing::Point(790, 4);
			this->labelCursorInfo->Name = L"labelCursorInfo";
			this->labelCursorInfo->Size = System::Drawing::Size(570, 24);
			this->labelCursorInfo->TabIndex = 10;
			this->labelCursorInfo->Text = L"Точка #0 (t = 0.0 с) | Визир готов";
			this->labelCursorInfo->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// labelSepStream
			// 
			this->labelSepStream->AutoSize = true;
			this->labelSepStream->ForeColor = System::Drawing::Color::DarkGray;
			this->labelSepStream->Location = System::Drawing::Point(778, 8);
			this->labelSepStream->Name = L"labelSepStream";
			this->labelSepStream->Size = System::Drawing::Size(13, 20);
			this->labelSepStream->TabIndex = 9;
			this->labelSepStream->Text = L"|";
			// 
			// buttonRefresh
			// 
			this->buttonRefresh->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->buttonRefresh->Location = System::Drawing::Point(682, 3);
			this->buttonRefresh->Name = L"buttonRefresh";
			this->buttonRefresh->Size = System::Drawing::Size(92, 26);
			this->buttonRefresh->TabIndex = 8;
			this->buttonRefresh->Text = L"ОБНОВИТЬ";
			this->buttonRefresh->UseVisualStyleBackColor = true;
			this->buttonRefresh->Click += gcnew System::EventHandler(this, &MainForm::buttonRefresh_Click);
			// 
			// buttonStream
			// 
			this->buttonStream->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->buttonStream->Location = System::Drawing::Point(593, 3);
			this->buttonStream->Name = L"buttonStream";
			this->buttonStream->Size = System::Drawing::Size(85, 26);
			this->buttonStream->TabIndex = 7;
			this->buttonStream->Text = L"ПОТОК";
			this->buttonStream->UseVisualStyleBackColor = true;
			this->buttonStream->Click += gcnew System::EventHandler(this, &MainForm::buttonStream_Click);
			// 
			// labelSepLayout2
			// 
			this->labelSepLayout2->AutoSize = true;
			this->labelSepLayout2->ForeColor = System::Drawing::Color::DarkGray;
			this->labelSepLayout2->Location = System::Drawing::Point(579, 8);
			this->labelSepLayout2->Name = L"labelSepLayout2";
			this->labelSepLayout2->Size = System::Drawing::Size(13, 20);
			this->labelSepLayout2->TabIndex = 6;
			this->labelSepLayout2->Text = L"|";
			// 
			// radioButtonLayoutBottom
			// 
			this->radioButtonLayoutBottom->AutoSize = true;
			this->radioButtonLayoutBottom->Location = System::Drawing::Point(477, 6);
			this->radioButtonLayoutBottom->Name = L"radioButtonLayoutBottom";
			this->radioButtonLayoutBottom->Size = System::Drawing::Size(108, 24);
			this->radioButtonLayoutBottom->TabIndex = 5;
			this->radioButtonLayoutBottom->Text = L"Только низ";
			this->radioButtonLayoutBottom->UseVisualStyleBackColor = true;
			this->radioButtonLayoutBottom->CheckedChanged += gcnew System::EventHandler(this, &MainForm::radioButtonLayout_CheckedChanged);
			// 
			// radioButtonLayoutTop
			// 
			this->radioButtonLayoutTop->AutoSize = true;
			this->radioButtonLayoutTop->Location = System::Drawing::Point(373, 6);
			this->radioButtonLayoutTop->Name = L"radioButtonLayoutTop";
			this->radioButtonLayoutTop->Size = System::Drawing::Size(115, 24);
			this->radioButtonLayoutTop->TabIndex = 4;
			this->radioButtonLayoutTop->Text = L"Только верх";
			this->radioButtonLayoutTop->UseVisualStyleBackColor = true;
			this->radioButtonLayoutTop->CheckedChanged += gcnew System::EventHandler(this, &MainForm::radioButtonLayout_CheckedChanged);
			// 
			// radioButtonLayoutBoth
			// 
			this->radioButtonLayoutBoth->AutoSize = true;
			this->radioButtonLayoutBoth->Checked = true;
			this->radioButtonLayoutBoth->Location = System::Drawing::Point(277, 6);
			this->radioButtonLayoutBoth->Name = L"radioButtonLayoutBoth";
			this->radioButtonLayoutBoth->Size = System::Drawing::Size(75, 24);
			this->radioButtonLayoutBoth->TabIndex = 3;
			this->radioButtonLayoutBoth->TabStop = true;
			this->radioButtonLayoutBoth->Text = L"2 окна";
			this->radioButtonLayoutBoth->UseVisualStyleBackColor = true;
			this->radioButtonLayoutBoth->CheckedChanged += gcnew System::EventHandler(this, &MainForm::radioButtonLayout_CheckedChanged);
			// 
			// labelSepLayout1
			// 
			this->labelSepLayout1->AutoSize = true;
			this->labelSepLayout1->ForeColor = System::Drawing::Color::DarkGray;
			this->labelSepLayout1->Location = System::Drawing::Point(265, 8);
			this->labelSepLayout1->Name = L"labelSepLayout1";
			this->labelSepLayout1->Size = System::Drawing::Size(13, 20);
			this->labelSepLayout1->TabIndex = 2;
			this->labelSepLayout1->Text = L"|";
			// 
			// buttonToggleSectors
			// 
			this->buttonToggleSectors->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->buttonToggleSectors->Location = System::Drawing::Point(133, 3);
			this->buttonToggleSectors->Name = L"buttonToggleSectors";
			this->buttonToggleSectors->Size = System::Drawing::Size(130, 26);
			this->buttonToggleSectors->TabIndex = 1;
			this->buttonToggleSectors->Text = L"⮜ Скрыть секторы";
			this->buttonToggleSectors->UseVisualStyleBackColor = true;
			this->buttonToggleSectors->Click += gcnew System::EventHandler(this, &MainForm::buttonToggleSectors_Click);
			// 
			// buttonToggleSidebar
			// 
			this->buttonToggleSidebar->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(225)),
				static_cast<System::Int32>(static_cast<System::Byte>(233)), static_cast<System::Int32>(static_cast<System::Byte>(245)));
			this->buttonToggleSidebar->Font = (gcnew System::Drawing::Font(L"Segoe UI", 8.5F, System::Drawing::FontStyle::Bold));
			this->buttonToggleSidebar->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(20)),
				static_cast<System::Int32>(static_cast<System::Byte>(50)), static_cast<System::Int32>(static_cast<System::Byte>(100)));
			this->buttonToggleSidebar->Location = System::Drawing::Point(4, 3);
			this->buttonToggleSidebar->Name = L"buttonToggleSidebar";
			this->buttonToggleSidebar->Size = System::Drawing::Size(125, 26);
			this->buttonToggleSidebar->TabIndex = 0;
			this->buttonToggleSidebar->Text = L"☰ Параметры";
			this->toolTipMain->SetToolTip(this->buttonToggleSidebar, L"Открыть / скрыть параметры станции и связи (F4)");
			this->buttonToggleSidebar->UseVisualStyleBackColor = false;
			this->buttonToggleSidebar->Click += gcnew System::EventHandler(this, &MainForm::buttonToggleSidebar_Click);
			// 
			// tabPageInclinometer
			// 
			this->tabPageInclinometer->Controls->Add(this->panelInclinometerPlaceholder);
			this->tabPageInclinometer->Location = System::Drawing::Point(4, 32);
			this->tabPageInclinometer->Name = L"tabPageInclinometer";
			this->tabPageInclinometer->Padding = System::Windows::Forms::Padding(4);
			this->tabPageInclinometer->Size = System::Drawing::Size(1376, 766);
			this->tabPageInclinometer->TabIndex = 1;
			this->tabPageInclinometer->Text = L"Инклинометрия / Годографы";
			this->tabPageInclinometer->UseVisualStyleBackColor = true;
			// 
			// panelInclinometerPlaceholder
			// 
			this->panelInclinometerPlaceholder->Controls->Add(this->labelInclinometerTitle);
			this->panelInclinometerPlaceholder->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panelInclinometerPlaceholder->Location = System::Drawing::Point(4, 4);
			this->panelInclinometerPlaceholder->Name = L"panelInclinometerPlaceholder";
			this->panelInclinometerPlaceholder->Size = System::Drawing::Size(1368, 758);
			this->panelInclinometerPlaceholder->TabIndex = 0;
			// 
			// labelInclinometerTitle
			// 
			this->labelInclinometerTitle->Dock = System::Windows::Forms::DockStyle::Top;
			this->labelInclinometerTitle->Font = (gcnew System::Drawing::Font(L"Segoe UI", 12, System::Drawing::FontStyle::Bold));
			this->labelInclinometerTitle->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(40)),
				static_cast<System::Int32>(static_cast<System::Byte>(60)), static_cast<System::Int32>(static_cast<System::Byte>(100)));
			this->labelInclinometerTitle->Location = System::Drawing::Point(0, 0);
			this->labelInclinometerTitle->Name = L"labelInclinometerTitle";
			this->labelInclinometerTitle->Padding = System::Windows::Forms::Padding(8);
			this->labelInclinometerTitle->Size = System::Drawing::Size(1368, 48);
			this->labelInclinometerTitle->TabIndex = 0;
			this->labelInclinometerTitle->Text = L"Модуль инклинометрии: Годографы MMC5983MA и ADXL355 (MAGNETOMETR_CAL)";
			this->labelInclinometerTitle->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// tabPageHealth
			// 
			this->tabPageHealth->Controls->Add(this->panelHealthPlaceholder);
			this->tabPageHealth->Location = System::Drawing::Point(4, 32);
			this->tabPageHealth->Name = L"tabPageHealth";
			this->tabPageHealth->Padding = System::Windows::Forms::Padding(4);
			this->tabPageHealth->Size = System::Drawing::Size(1376, 766);
			this->tabPageHealth->TabIndex = 2;
			this->tabPageHealth->Text = L"Здоровье прибора (Health)";
			this->tabPageHealth->UseVisualStyleBackColor = true;
			// 
			// panelHealthPlaceholder
			// 
			this->panelHealthPlaceholder->Controls->Add(this->labelHealthTitle);
			this->panelHealthPlaceholder->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panelHealthPlaceholder->Location = System::Drawing::Point(4, 4);
			this->panelHealthPlaceholder->Name = L"panelHealthPlaceholder";
			this->panelHealthPlaceholder->Size = System::Drawing::Size(1368, 758);
			this->panelHealthPlaceholder->TabIndex = 0;
			// 
			// labelHealthTitle
			// 
			this->labelHealthTitle->Dock = System::Windows::Forms::DockStyle::Top;
			this->labelHealthTitle->Font = (gcnew System::Drawing::Font(L"Segoe UI", 12, System::Drawing::FontStyle::Bold));
			this->labelHealthTitle->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(40)), static_cast<System::Int32>(static_cast<System::Byte>(60)),
				static_cast<System::Int32>(static_cast<System::Byte>(100)));
			this->labelHealthTitle->Location = System::Drawing::Point(0, 0);
			this->labelHealthTitle->Name = L"labelHealthTitle";
			this->labelHealthTitle->Padding = System::Windows::Forms::Padding(8);
			this->labelHealthTitle->Size = System::Drawing::Size(1368, 48);
			this->labelHealthTitle->TabIndex = 0;
			this->labelHealthTitle->Text = L"Матрица состояния генераторов Tx1..Tx5, приемников Rx_L/Rx_R и телеметрии";
			this->labelHealthTitle->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			// 
			// openFileDialogBin
			// 
			this->openFileDialogBin->DefaultExt = L"bin";
			this->openFileDialogBin->Filter = L"Файлы каротажа (*.bin;*.biv;*.dev)|*.bin;*.biv;*.dev|Бинарные файлы (*.bin;*.biv)"
				L"|*.bin;*.biv|Дампы памяти (*.dev)|*.dev|Все файлы (*.*)|*.*";
			this->openFileDialogBin->Title = L"Открыть файл каротажа";
			// 
			// contextMenuCurves1
			// 
			this->contextMenuCurves1->ImageScalingSize = System::Drawing::Size(20, 20);
			this->contextMenuCurves1->Name = L"contextMenuCurves1";
			this->contextMenuCurves1->Size = System::Drawing::Size(61, 4);
			// 
			// contextMenuCurves2
			// 
			this->contextMenuCurves2->ImageScalingSize = System::Drawing::Size(20, 20);
			this->contextMenuCurves2->Name = L"contextMenuCurves2";
			this->contextMenuCurves2->Size = System::Drawing::Size(61, 4);
			// 
			// MainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 20);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1384, 861);
			this->Controls->Add(this->panelLeft);
			this->Controls->Add(this->tabControlMain);
			this->Controls->Add(this->statusStripMain);
			this->Controls->Add(this->menuStripMain);
			this->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9));
			this->MainMenuStrip = this->menuStripMain;
			this->MinimumSize = System::Drawing::Size(1024, 700);
			this->Name = L"MainForm";
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
			this->Text = L"Electromagnetic Tools 3.0 — НПФ «АМК Горизонт»";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &MainForm::MainForm_FormClosing);
			this->Resize += gcnew System::EventHandler(this, &MainForm::MainForm_Resize);
			this->menuStripMain->ResumeLayout(false);
			this->menuStripMain->PerformLayout();
			this->statusStripMain->ResumeLayout(false);
			this->statusStripMain->PerformLayout();
			this->panelLeft->ResumeLayout(false);
			this->groupBoxMetro->ResumeLayout(false);
			this->groupBoxMode->ResumeLayout(false);
			this->groupBoxMode->PerformLayout();
			this->groupBoxConnection->ResumeLayout(false);
			this->groupBoxConnection->PerformLayout();
			this->panelDrawerHeader->ResumeLayout(false);
			this->panelDrawerHeader->PerformLayout();
			this->tabControlMain->ResumeLayout(false);
			this->tabPageLogging->ResumeLayout(false);
			this->splitContainerRows->Panel1->ResumeLayout(false);
			this->splitContainerRows->Panel2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainerRows))->EndInit();
			this->splitContainerRows->ResumeLayout(false);
			this->splitContainerBand1->Panel1->ResumeLayout(false);
			this->splitContainerBand1->Panel2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainerBand1))->EndInit();
			this->splitContainerBand1->ResumeLayout(false);
			this->panelTime1Container->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chartTime1))->EndInit();
			this->chartTime1->ResumeLayout(false);
			this->contextMenuChart1->ResumeLayout(false);
			this->panelTime1Header->ResumeLayout(false);
			this->panelTime1Header->PerformLayout();
			this->panelSectorRxLContainer->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chartSectorRxL))->EndInit();
			this->panelQualityRxL->ResumeLayout(false);
			this->panelQualityRxL->PerformLayout();
			this->panelSectorRxLHeader->ResumeLayout(false);
			this->panelSectorRxLHeader->PerformLayout();
			this->splitContainerBand2->Panel1->ResumeLayout(false);
			this->splitContainerBand2->Panel2->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->splitContainerBand2))->EndInit();
			this->splitContainerBand2->ResumeLayout(false);
			this->panelTime2Container->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chartTime2))->EndInit();
			this->chartTime2->ResumeLayout(false);
			this->contextMenuChart2->ResumeLayout(false);
			this->panelTime2Header->ResumeLayout(false);
			this->panelTime2Header->PerformLayout();
			this->panelSectorRxRContainer->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->chartSectorRxR))->EndInit();
			this->panelQualityRxR->ResumeLayout(false);
			this->panelQualityRxR->PerformLayout();
			this->panelSectorRxRHeader->ResumeLayout(false);
			this->panelSectorRxRHeader->PerformLayout();
			this->panelLoggingToolbar->ResumeLayout(false);
			this->panelLoggingToolbar->PerformLayout();
			this->tabPageInclinometer->ResumeLayout(false);
			this->panelInclinometerPlaceholder->ResumeLayout(false);
			this->tabPageHealth->ResumeLayout(false);
			this->panelHealthPlaceholder->ResumeLayout(false);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

	public:
		void LoadDataFile(String^ filePath);
		void RenderLoadedCurves();
		void SaveStationConfig();
		void LoadStationConfig();
		void UpdateChipStyles(int windowIndex);

	private:
		void InitChartEngine(void);
		void InitChartAreas(DataVisualization::Charting::Chart^ chart, String^ titleY);
		void PopulateCurveMenu(int windowIndex);
		void AddCurveChip(int windowIndex, String^ curveName);
		void RemoveCurveChip(int windowIndex, String^ curveName);
		void UpdateTimeSlices(double timeVal);
		void ApplyRange(int windowIndex);
		void UpdateCursorReadout(double posX);

		System::Void buttonConnect_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonDisconnect_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonCalibrate_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonSelfTest_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonExportCsv_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemGpAnalysis_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemCalibMaster_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemMetroParams_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemOpenBin_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemExit_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemAbout_Click(System::Object^ sender, System::EventArgs^ e);

		System::Void buttonToggleSectors_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void radioButtonLayout_CheckedChanged(System::Object^ sender, System::EventArgs^ e);
		System::Void splitContainerBand_SplitterMoved(System::Object^ sender, System::Windows::Forms::SplitterEventArgs^ e);
		System::Void buttonStream_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonRefresh_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void timerStream_Tick(System::Object^ sender, System::EventArgs^ e);

		System::Void buttonApplyRange1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonResetRange1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void textBoxRange_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e);
		System::Void textBoxRange1_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e);

		System::Void buttonApplyRange2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonResetRange2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void textBoxRange2_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e);

		System::Void buttonZoomReset1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonZoomReset2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemZoomBackX1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemZoomBackY1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemZoomBackX2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemZoomBackY2_Click(System::Object^ sender, System::EventArgs^ e);

		System::Void menuItemSetMin1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemSetMax1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemResetRange1_Click(System::Object^ sender, System::EventArgs^ e);

		System::Void menuItemSetMin2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemSetMax2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemResetRange2_Click(System::Object^ sender, System::EventArgs^ e);

		System::Void buttonMode1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonMode2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonFreq1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonFreq2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonFreqRxL_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonFreqRxR_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void comboBoxTxRx_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonAddCurve1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonAddCurve2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void CurveMenuItem_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void checkTx1_CheckedChanged(System::Object^ sender, System::EventArgs^ e);
		System::Void checkTx2_CheckedChanged(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonAutoY1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonAutoY2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void chartTime_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
		System::Void chartTime_MouseMove(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
		System::Void chartTime_MouseUp(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
		System::Void buttonXMode1_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonXMode2_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void comboBoxPrecision1_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e);
		System::Void comboBoxPrecision2_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e);
		System::Void chartSector_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
		System::Void tabControlMain_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
		System::Void ChipCloseButton_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonToggleSidebar_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonPinSidebar_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void buttonCloseSidebar_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void menuItemToggleSidebar_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void Chip_Click(System::Object^ sender, System::EventArgs^ e);
		System::Void chartTime_MouseDoubleClick(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
		System::Void chartTime_MouseWheel(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
		System::Void chartTime_AxisViewChanged(System::Object^ sender, System::Windows::Forms::DataVisualization::Charting::ViewEventArgs^ e);
		System::Void chartTime_CursorPositionChanged(System::Object^ sender, System::Windows::Forms::DataVisualization::Charting::CursorEventArgs^ e);
		System::Void MainForm_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e);
		System::Void MainForm_Resize(System::Object^ sender, System::EventArgs^ e);
	};
}
