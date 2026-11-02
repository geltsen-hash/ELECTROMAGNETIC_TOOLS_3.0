#include "pch.h"
#include "MainForm.h"
#include "MetroForm.h"
#include "BinFileReader.h"
#include <msclr/marshal_cppstd.h>
#include <msclr/lock.h>
#include <iostream>
#include <cmath>
#include <cstring>
#include <fstream>
#include <utility>

namespace ELECTROMAGNETIC_TOOLS_3_0 {

	// std::string в UTF-8 → System::String (gcnew String(c_str()) ломает кириллицу)
	static String^ Utf8ToManaged(const char* s)
	{
		if (s == nullptr) return L"";
		int len = (int)strlen(s);
		cli::array<Byte>^ bytes = gcnew cli::array<Byte>(len);
		for (int i = 0; i < len; i++) bytes[i] = (Byte)(uint8_t)s[i];
		return System::Text::Encoding::UTF8->GetString(bytes);
	}

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections::Generic;
	using namespace System::Windows::Forms;
	using namespace System::Drawing;
	using namespace System::Windows::Forms::DataVisualization::Charting;

	static void SetDoubleBufferedControl(Control^ control)
	{
		System::Reflection::PropertyInfo^ prop = control->GetType()->GetProperty(
			L"DoubleBuffered",
			System::Reflection::BindingFlags::Instance | System::Reflection::BindingFlags::NonPublic
		);
		if (prop != nullptr)
		{
			prop->SetValue(control, true, nullptr);
		}
	}

	void MainForm::InitChartAreas(DataVisualization::Charting::Chart^ chart, String^ titleY)
	{
		chart->ChartAreas->Clear();
		chart->Legends->Clear();
		chart->Series->Clear();

		ChartArea^ ca = gcnew ChartArea(L"MainArea");
		ca->BackColor = Color::White;
		ca->AxisX->MajorGrid->LineColor = Color::FromArgb(235, 235, 235);
		ca->AxisY->MajorGrid->LineColor = Color::FromArgb(235, 235, 235);
		ca->AxisX->LabelStyle->Font = gcnew Drawing::Font(L"Segoe UI", 8);
		ca->AxisY->LabelStyle->Font = gcnew Drawing::Font(L"Segoe UI", 8);
		ca->AxisX->Title = L"Кадр (N)";
		ca->AxisX->TitleFont = gcnew Drawing::Font(L"Segoe UI", 8.5F, FontStyle::Bold);
		ca->AxisY->Title = titleY;
		ca->AxisY->TitleFont = gcnew Drawing::Font(L"Segoe UI", 8.5F, FontStyle::Bold);

		// Растягивание графика на 100% площади без пустых зазоров по бокам
		ca->AxisX->IsMarginVisible = false;
		ca->AxisY->IsMarginVisible = false;
		ca->AxisX->IntervalAutoMode = IntervalAutoMode::VariableCount;
		ca->AxisX->LabelStyle->Format = L"F0";
		ca->AxisX->Interval = 0; // 0 = Автоматический шаг (5-8 делений)

		// Фиксируем InnerPlotPosition для строгого выравнивания координатных сеток обоих окон на 100% ширины
		ca->InnerPlotPosition->Auto = false;
		ca->InnerPlotPosition->X = 7.5f;
		ca->InnerPlotPosition->Y = 2.0f;
		ca->InnerPlotPosition->Width = 92.0f;
		ca->InnerPlotPosition->Height = 90.0f;

		// Интерактивный визир и Zoom прямоугольной рамкой
		ca->CursorX->IsUserEnabled = true;
		ca->CursorX->IsUserSelectionEnabled = true;
		ca->CursorX->LineColor = Color::Crimson;
		ca->CursorX->LineWidth = 2;
		ca->CursorX->LineDashStyle = ChartDashStyle::Solid;
		ca->CursorX->Interval = 1.0;

		ca->CursorY->IsUserEnabled = true;
		ca->CursorY->IsUserSelectionEnabled = true;
		ca->CursorY->Interval = 0.0001;

		ca->AxisX->ScaleView->Zoomable = true;
		ca->AxisY->ScaleView->Zoomable = true;
		ca->AxisX->ScrollBar->Enabled = false;
		ca->AxisY->ScrollBar->Enabled = false;

		chart->ChartAreas->Add(ca);

		Legend^ leg = gcnew Legend(L"MainLegend");
		leg->Docking = Docking::Top;
		leg->Alignment = StringAlignment::Far;
		leg->Font = gcnew Drawing::Font(L"Segoe UI", 8);
		leg->BackColor = Color::Transparent;
		chart->Legends->Add(leg);

		// Палитра передатчиков — как на схеме зонда и в старой программе:
		// T1 зелёный, T2 золотисто-коричневый, T3 красный, T4 синий, T5 маджента
		cli::array<Color>^ colors = gcnew cli::array<Color>(5) {
			Color::Green,
			Color::DarkGoldenrod,
			Color::Red,
			Color::RoyalBlue,
			Color::Magenta
		};

		for (int i = 0; i < 5; i++)
		{
			Series^ s = gcnew Series(L"Tx" + (i + 1));
			s->ChartType = SeriesChartType::FastLine;
			s->Color = colors[i];
			s->BorderWidth = 2;
			s->ChartArea = L"MainArea";
			s->Legend = L"MainLegend";
			chart->Series->Add(s);
		}
	}

	void MainForm::InitChartEngine(void)
	{
		SetDoubleBufferedControl(this->chartTime1);
		SetDoubleBufferedControl(this->chartTime2);
		SetDoubleBufferedControl(this->chartSectorRxL);
		SetDoubleBufferedControl(this->chartSectorRxR);

		InitChartAreas(this->chartTime1, L"Фаза Δφ (град) / УЭС (Ом·м)");
		InitChartAreas(this->chartTime2, L"Затухание Att (дБ) / УЭС (Ом·м)");

		for (int index = 1; index <= 2; ++index)
		{
			System::Windows::Forms::ContextMenuStrip^ menu = index == 1 ? this->contextMenuChart1 : this->contextMenuChart2;
			ToolStripMenuItem^ exportRange = gcnew ToolStripMenuItem(L"Экспорт всех данных Min–Max в CSV...");
			exportRange->Tag = index;
			exportRange->Click += gcnew EventHandler(this, &MainForm::menuExportCsv_Click);
			menu->Items->Add(exportRange);
		}
		ToolStripMenuItem^ exportFile = gcnew ToolStripMenuItem(L"Экспорт всего файла в CSV...");
		exportFile->Click += gcnew EventHandler(this, &MainForm::buttonExportCsv_Click);
		this->menuItemFile->DropDownItems->Insert(1, exportFile);

		ToolStripMenuItem^ gpAnalysis = gcnew ToolStripMenuItem(L"Диагностика наводок и дрейфа (GP CSV)...");
		gpAnalysis->Click += gcnew EventHandler(this, &MainForm::menuItemGpAnalysis_Click);
		this->menuItemFile->DropDownItems->Insert(2, gpAnalysis);

		ToolStripMenuItem^ calibMaster = gcnew ToolStripMenuItem(L"Мастер калибровки (Python)...");
		calibMaster->Click += gcnew EventHandler(this, &MainForm::menuItemCalibMaster_Click);
		this->menuItemMetro->DropDownItems->Add(calibMaster);

		// Настройка правого графика секторов Rx_L
		this->chartSectorRxL->ChartAreas->Clear();
		this->chartSectorRxL->Legends->Clear();
		this->chartSectorRxL->Series->Clear();
		ChartArea^ caSecL = gcnew ChartArea(L"SectorAreaL");
		caSecL->BackColor = Color::White;
		caSecL->AxisX->MajorGrid->LineColor = Color::FromArgb(235, 235, 235);
		caSecL->AxisY->MajorGrid->LineColor = Color::FromArgb(235, 235, 235);
		caSecL->AxisX->Interval = 1;
		caSecL->AxisX->Minimum = 0;
		caSecL->AxisX->Maximum = 15;
		caSecL->AxisX->Title = L"Сектор (0..15)";
		caSecL->AxisY->Title = L"Амплитуда V (мВ)";
		this->chartSectorRxL->ChartAreas->Add(caSecL);

		Series^ sColL = gcnew Series(L"SectorsL");
		sColL->ChartType = SeriesChartType::Column;
		sColL->Color = Color::FromArgb(100, 140, 200);
		sColL->IsValueShownAsLabel = false;
		sColL->ChartArea = L"SectorAreaL";
		this->chartSectorRxL->Series->Add(sColL);

		Series^ sFitL = gcnew Series(L"H1_FitL");
		sFitL->ChartType = SeriesChartType::Spline;
		sFitL->Color = Color::Red;
		sFitL->BorderWidth = 2;
		sFitL->ChartArea = L"SectorAreaL";
		this->chartSectorRxL->Series->Add(sFitL);

		// Настройка правого графика секторов Rx_R
		this->chartSectorRxR->ChartAreas->Clear();
		this->chartSectorRxR->Legends->Clear();
		this->chartSectorRxR->Series->Clear();
		ChartArea^ caSecR = gcnew ChartArea(L"SectorAreaR");
		caSecR->BackColor = Color::White;
		caSecR->AxisX->MajorGrid->LineColor = Color::FromArgb(235, 235, 235);
		caSecR->AxisY->MajorGrid->LineColor = Color::FromArgb(235, 235, 235);
		caSecR->AxisX->Interval = 1;
		caSecR->AxisX->Minimum = 0;
		caSecR->AxisX->Maximum = 15;
		caSecR->AxisX->Title = L"Сектор (0..15)";
		caSecR->AxisY->Title = L"Амплитуда V (мВ)";
		this->chartSectorRxR->ChartAreas->Add(caSecR);

		Series^ sColR = gcnew Series(L"SectorsR");
		sColR->ChartType = SeriesChartType::Column;
		sColR->Color = Color::FromArgb(120, 170, 130);
		sColR->IsValueShownAsLabel = false;
		sColR->ChartArea = L"SectorAreaR";
		this->chartSectorRxR->Series->Add(sColR);

		Series^ sFitR = gcnew Series(L"H1_FitR");
		sFitR->ChartType = SeriesChartType::Spline;
		sFitR->Color = Color::DarkBlue;
		sFitR->BorderWidth = 2;
		sFitR->ChartArea = L"SectorAreaR";
		this->chartSectorRxR->Series->Add(sFitR);

		// Подключение событий интерактивного Zoom / Unzoom и синхронного визира
		this->chartTime1->MouseDoubleClick += gcnew MouseEventHandler(this, &MainForm::chartTime_MouseDoubleClick);
		this->chartTime1->MouseWheel += gcnew MouseEventHandler(this, &MainForm::chartTime_MouseWheel);
		this->chartTime1->AxisViewChanged += gcnew EventHandler<ViewEventArgs^>(this, &MainForm::chartTime_AxisViewChanged);
		this->chartTime1->CursorPositionChanged += gcnew EventHandler<CursorEventArgs^>(this, &MainForm::chartTime_CursorPositionChanged);

		this->chartTime2->MouseDoubleClick += gcnew MouseEventHandler(this, &MainForm::chartTime_MouseDoubleClick);
		this->chartTime2->MouseWheel += gcnew MouseEventHandler(this, &MainForm::chartTime_MouseWheel);
		this->chartTime2->AxisViewChanged += gcnew EventHandler<ViewEventArgs^>(this, &MainForm::chartTime_AxisViewChanged);
		this->chartTime2->CursorPositionChanged += gcnew EventHandler<CursorEventArgs^>(this, &MainForm::chartTime_CursorPositionChanged);

		// Инициализация списков закрепленных кнопок
		this->pinnedCurves1 = gcnew List<String^>();
		this->pinnedCurves2 = gcnew List<String^>();
		this->modePinnedCurves = gcnew cli::array<List<String^>^>(4);
		this->modeActiveCurves = gcnew cli::array<String^>(4);
		for (int mode = 0; mode < 4; ++mode) this->modePinnedCurves[mode] = gcnew List<String^>();
		this->isDirectMode1 = false;
		this->isDirectMode2 = false;
		this->isCartographLayout = true;
		this->receiverIndex1 = 0;
		this->receiverIndex2 = 0;
		this->buttonReceiver1 = gcnew Button();
		this->buttonReceiver2 = gcnew Button();
		for (int window = 1; window <= 2; ++window)
		{
			Button^ button = window == 1 ? this->buttonReceiver1 : this->buttonReceiver2;
			button->Name = L"buttonReceiver" + window.ToString();
			button->Size = Drawing::Size(60, 24);
			button->Font = gcnew Drawing::Font(L"Segoe UI", 8, FontStyle::Bold);
			button->Text = L"Rx L";
			button->Tag = window;
			button->Visible = false;
			button->Click += gcnew EventHandler(this, &MainForm::buttonReceiver_Click);
			this->toolTipMain->SetToolTip(button, L"Направленный приёмник: левый (L) / правый (R)");
			(window == 1 ? this->panelTime1Header : this->panelTime2Header)->Controls->Add(button);
		}
		this->isTempXAxis1 = false;
		this->isTempXAxis2 = false;
		this->precisionY1 = 0;
		this->precisionY2 = 0;
		this->minFrame1 = 0;
		this->maxFrame1 = -1;
		this->minFrame2 = 0;
		this->maxFrame2 = -1;
		this->isStreaming = false;
		this->currentStreamPoint = 500;
		this->telemetryRunning = false;
		this->telemetryThread = nullptr;
		this->frameQueue = gcnew System::Collections::Concurrent::ConcurrentQueue<cli::array<Byte>^>();
		this->telemetryRx = 0;
		this->telemetryTimeouts = 0;
		this->telemetryCrcErr = 0;
		this->liveFout = nullptr;
		this->liveFileName = nullptr;
		this->isLiveSession = false;
		this->connectedSondeId = nullptr;
		this->isSyncingSplitter = false;
		this->isSidebarPinned = false;
		this->lastShownTxCount = -1;

		// Схема зондов: привязка к активному графику (аналог panel_Sonde_Picture из AIR_TEMP_1.0)
		this->activeSchemeChart = 0;
		this->sectorsCollapsedByUser = false;
		this->sondeScheme = gcnew SondeSchemeControl();
		this->sondeScheme->Dock = DockStyle::Bottom;
		this->sondeScheme->Height = 104;
		this->sondeScheme->SchemeClicked += gcnew SondeSchemeControl::SchemeClickedHandler(this, &MainForm::sondeScheme_SchemeClicked);
		this->tabPageLogging->Controls->Add(this->sondeScheme);
		// Порядок докинга: тулбар сверху, схема внизу, графики заполняют остаток
		this->tabPageLogging->Controls->SetChildIndex(this->sondeScheme, 1);

		// Стартовый размер окна по разрешению экрана: рабочая область >= Full HD —
		// окно 1920 x <рабочая область> по центру; меньше — на весь экран
		System::Drawing::Rectangle wa = Screen::PrimaryScreen->WorkingArea;
		if (wa.Width >= 1920 && wa.Height >= 1000)
		{
			this->StartPosition = FormStartPosition::CenterScreen;
			this->ClientSize = System::Drawing::Size(1920, wa.Height);
		}
		else
		{
			this->WindowState = FormWindowState::Maximized;
		}

		// Загрузка сохраненной конфигурации станции (быстрые кнопки, режимы)
		LoadStationConfig();
		UpdateSondeScheme();
		EmTools::Updates::UpdateController::Attach(this, this->menuItemHelp);

		// Заполнение тестовыми данными (500 точек) до открытия файла
		for (int i = 0; i < 500; i++)
		{
			double t = i * 0.1;
			double valBase = 15.0 + 5.0 * Math::Sin(t * 0.05);

			this->chartTime1->Series[0]->Points->AddXY(t, valBase + 2.0 * Math::Sin(t * 0.2));
			this->chartTime1->Series[1]->Points->AddXY(t, valBase * 0.9 + 1.8 * Math::Sin(t * 0.2 + 0.3));
			this->chartTime1->Series[2]->Points->AddXY(t, valBase * 0.8 + 1.5 * Math::Sin(t * 0.2 + 0.6));
			this->chartTime1->Series[3]->Points->AddXY(t, valBase * 0.7 + 1.2 * Math::Sin(t * 0.2 + 0.9));
			this->chartTime1->Series[4]->Points->AddXY(t, valBase * 0.6 + 1.0 * Math::Sin(t * 0.2 + 1.2));

			double valBase2 = 8.0 + 3.0 * Math::Cos(t * 0.04);
			this->chartTime2->Series[0]->Points->AddXY(t, valBase2 + 1.2 * Math::Cos(t * 0.18));
			this->chartTime2->Series[1]->Points->AddXY(t, valBase2 * 0.9 + 1.0 * Math::Cos(t * 0.18 + 0.3));
			this->chartTime2->Series[2]->Points->AddXY(t, valBase2 * 0.8 + 0.8 * Math::Cos(t * 0.18 + 0.6));
			this->chartTime2->Series[3]->Points->AddXY(t, valBase2 * 0.7 + 0.6 * Math::Cos(t * 0.18 + 0.9));
			this->chartTime2->Series[4]->Points->AddXY(t, valBase2 * 0.6 + 0.5 * Math::Cos(t * 0.18 + 1.2));
		}

		UpdateTxControlsVisibility();
		UpdateTimeSlices(0.0);
		UpdateCursorReadout(0.0);
		this->panelLeft->BringToFront();
	}

	void MainForm::LoadDataFile(String^ filePath)
	{
		if (String::IsNullOrEmpty(filePath)) return;

		// Открытие файла — в любом случае останавливаем поток отображения
		// и показываем файл целиком (запись из прибора продолжает идти в фоне)
		StopDisplay();

		this->Cursor = Cursors::WaitCursor;
		try
		{
			std::string nativePath = msclr::interop::marshal_as<std::string>(filePath);
			char errBuf[512] = { 0 };

			bool ok = SondeData::LoadFile(nativePath.c_str(), errBuf, sizeof(errBuf));
			this->isLiveSession = false; // сессия теперь — открытый файл, а не онлайн-кадры
			if (!ok)
			{
				this->Cursor = Cursors::Default;
				MessageBox::Show(this, gcnew String(errBuf, 0, static_cast<int>(std::strlen(errBuf)), System::Text::Encoding::UTF8),
					L"Ошибка открытия файла", MessageBoxButtons::OK, MessageBoxIcon::Error);
				return;
			}

			size_t frameCount = SondeData::GetFrameCount();
			String^ sondeName = Utf8ToManaged(SondeData::GetSondeName());
			String^ fileNameOnly = System::IO::Path::GetFileName(filePath);

			this->statusLabelFile->Text = L"Файл: " + fileNameOnly + L" | " + sondeName + L" | Кадров: " + frameCount.ToString();

			UpdateTxControlsVisibility();

			if (frameCount > 0)
			{
				double minT = SondeData::GetTemperature(0);
				double maxT = minT;
				for (size_t i = 1; i < frameCount; ++i)
				{
					double t = SondeData::GetTemperature(i);
					if (t < minT) minT = t;
					if (t > maxT) maxT = t;
				}

				if (this->isTempXAxis1)
				{
					this->textBoxMin1->Text = Math::Floor(minT).ToString();
					this->textBoxMax1->Text = Math::Ceiling(maxT).ToString();
				}
				else
				{
					this->textBoxMin1->Text = L"0";
					this->textBoxMax1->Text = (frameCount - 1).ToString();
				}

				if (this->isTempXAxis2)
				{
					this->textBoxMin2->Text = Math::Floor(minT).ToString();
					this->textBoxMax2->Text = Math::Ceiling(maxT).ToString();
				}
				else
				{
					this->textBoxMin2->Text = L"0";
					this->textBoxMax2->Text = (frameCount - 1).ToString();
				}

				this->minFrame1 = 0;
				this->maxFrame1 = static_cast<int>(frameCount - 1);
				this->minFrame2 = 0;
				this->maxFrame2 = static_cast<int>(frameCount - 1);
			}
			else
			{
				this->textBoxMin1->Text = L"0";
				this->textBoxMax1->Text = L"0";
				this->textBoxMin2->Text = L"0";
				this->textBoxMax2->Text = L"0";
			}

			RenderLoadedCurves();
			UpdateSondeScheme();

			this->chartTime1->ChartAreas[0]->CursorX->Position = 0;
			this->chartTime2->ChartAreas[0]->CursorX->Position = 0;
			UpdateTimeSlices(0.0);
		}
		finally
		{
			this->Cursor = Cursors::Default;
		}
	}

	static int GetCurveTypeByName(String^ name)
	{
		if (String::IsNullOrEmpty(name)) return SondeData::CURVE_PHASE_DIFF;
		bool pc = name->EndsWith(L" PC") || name->Contains(L"from_computer");
		if (name->StartsWith(L"Δφ SMT") || name->StartsWith(L"φ SMT") || name->StartsWith(L"PH_SMT"))
			return pc ? SondeData::CURVE_PHASE_SMT_PC : SondeData::CURVE_PHASE_SMT;
		if (name->StartsWith(L"RO Δφ") || name->StartsWith(L"RO φ") || name->StartsWith(L"УЭС_PH"))
			return pc ? SondeData::CURVE_RHO_PHASE_PC : SondeData::CURVE_RHO_PHASE;
		if (name->StartsWith(L"ATT SMT") || name->StartsWith(L"ATT_SMT"))
			return pc ? SondeData::CURVE_ATT_SMT_PC : SondeData::CURVE_ATT_SMT;
		if (name->StartsWith(L"RO ATT") || name->StartsWith(L"УЭС_ATT"))
			return pc ? SondeData::CURVE_RHO_ATT_PC : SondeData::CURVE_RHO_ATT;

		// Точные короткие имена (новый формат); порядок — от длинных к коротким
		if (name == L"AM silent ZX") return SondeData::CURVE_ZERO_AM_ZX;
		if (name == L"AM silent")    return SondeData::CURVE_ZERO_AM_ZZ;
		if (name == L"AM Z2/Z1")     return SondeData::CURVE_ATTENUATION;
		if (name == L"AMP Z1")       return SondeData::CURVE_AMPLITUDE_RX1;
		if (name == L"AMP Z2")       return SondeData::CURVE_AMPLITUDE_RX2;
		if (name == L"Raw Δφ")        return SondeData::CURVE_PHASE_DIFF;
		if (name == L"TEMP RZX")     return SondeData::CURVE_TEMP_RZX;
		if (name == L"TEMP")         return SondeData::CURVE_TEMPERATURE;
		if (name == L"Δφ SMT")        return SondeData::CURVE_PHASE_SMT;
		if (name == L"RO Δφ")         return SondeData::CURVE_RHO_PHASE;
		if (name == L"ATT SMT")      return SondeData::CURVE_ATT_SMT;
		if (name == L"RO ATT")       return SondeData::CURVE_RHO_ATT;
		if (name == L"ε RO")         return SondeData::CURVE_ERROR_RHO;
		if (name == L"FRAME")        return SondeData::CURVE_FRAME_NUM;
		if (name == L"ACCEL")        return SondeData::CURVE_ACCEL;
		if (name == L"AM GEO SMT")   return SondeData::CURVE_GEO_AM_SMT;
		if (name == L"AM GEO")       return SondeData::CURVE_GEO_AM;
		if (name == L"Δφ GEO SMT")    return SondeData::CURVE_GEO_PH_SMT;
		if (name == L"Δφ GEO")        return SondeData::CURVE_GEO_PH;
		if (name == L"AM VZX")       return SondeData::CURVE_V_ZX;
		if (name == L"AM VZZ")       return SondeData::CURVE_V_ZZ;
		if (name == L"AM VXX")       return SondeData::CURVE_V_XX;
		if (name == L"βz")           return SondeData::CURVE_BETTA_Z;
		if (name == L"Δφ VZX/VZZ")    return SondeData::CURVE_RATIO_ZX_ZZ;
		if (name == L"dV")           return SondeData::CURVE_DV_NOISE;
		if (name == L"SNR")          return SondeData::CURVE_SNR;
		if (name == L"BORDER ∠")     return SondeData::CURVE_BORDER_ANGLE;
		if (name == L"INC")          return SondeData::CURVE_INC_ABS;
		if (name == L"ω")            return SondeData::CURVE_ANGULAR_VELOCITY;
		if (name == L"START SECTOR") return SondeData::CURVE_START_SECTOR;
		if (name == L"DEPTH")        return SondeData::CURVE_DEPTH;

		// Старые/полные имена (совместимость с сохранённой конфигурацией)
		if (name->Contains(L"Разность фаз") || name->Contains(L"PH Z2-Z1") || name->Contains(L"Δφ"))
			return SondeData::CURVE_PHASE_DIFF;
		if (name->Contains(L"Амплитуда Rx1") || name->Contains(L"AM Z1"))
			return SondeData::CURVE_AMPLITUDE_RX1;
		if (name->Contains(L"Амплитуда Rx2") || name->Contains(L"AM Z2"))
			return SondeData::CURVE_AMPLITUDE_RX2;
		if (name->Contains(L"УЭС по фазе") || name->Contains(L"ρ_ph") || name->Contains(L"УЭС_PH"))
			return SondeData::CURVE_RHO_PHASE;
		if (name->Contains(L"Затухание") || name->Contains(L"Att") || name->Contains(L"ATT_SMT"))
			return SondeData::CURVE_ATTENUATION;
		if (name->Contains(L"УЭС по затуханию") || name->Contains(L"ρ_att") || name->Contains(L"УЭС_ATT"))
			return SondeData::CURVE_RHO_ATT;
		if (name->Contains(L"Симметризованная фаза") || name->Contains(L"PH_SMT"))
			return SondeData::CURVE_PHASE_SMT;
		if (name->Contains(L"нуля ZZ") || name->Contains(L"Zero ZZ"))
			return SondeData::CURVE_ZERO_AM_ZZ;
		if (name->Contains(L"нуля ZX") || name->Contains(L"Zero ZX"))
			return SondeData::CURVE_ZERO_AM_ZX;
		if (name->Contains(L"Погрешность"))
			return SondeData::CURVE_ERROR_RHO;
		if (name->Contains(L"Гео-Амплитуда") || name->Contains(L"AM_GEO"))
			return SondeData::CURVE_GEO_AM;
		if (name->Contains(L"Гео-Фаза") || name->Contains(L"PH_GEO"))
			return SondeData::CURVE_GEO_PH;
		if (name->Contains(L"V_ZX / V_ZZ") || name->Contains(L"PH_VZX"))
			return SondeData::CURVE_RATIO_ZX_ZZ;
		if (name->Contains(L"V_ZX"))
			return SondeData::CURVE_V_ZX;
		if (name->Contains(L"V_ZZ"))
			return SondeData::CURVE_V_ZZ;
		if (name->Contains(L"V_XX"))
			return SondeData::CURVE_V_XX;
		if (name->Contains(L"betta_z"))
			return SondeData::CURVE_BETTA_Z;
		if (name->Contains(L"dV"))
			return SondeData::CURVE_DV_NOISE;
		if (name->Contains(L"SNR"))
			return SondeData::CURVE_SNR;
		if (name->Contains(L"BORDER"))
			return SondeData::CURVE_BORDER_ANGLE;
		if (name->Contains(L"INC") || name->Contains(L"Инклинометр"))
			return SondeData::CURVE_INC_ABS;
		if (name->Contains(L"ANGULAR_VELOCITY") || name->Contains(L"Угловая") || name->Contains(L"Wg"))
			return SondeData::CURVE_ANGULAR_VELOCITY;
		if (name->Contains(L"SECTOR") || name->Contains(L"Сектор"))
			return SondeData::CURVE_START_SECTOR;
		if (name->Contains(L"Температура RZX"))
			return SondeData::CURVE_TEMP_RZX;
		if (name->Contains(L"Температура"))
			return SondeData::CURVE_TEMPERATURE;
		if (name->Contains(L"Кадр") || name->Contains(L"FRAME"))
			return SondeData::CURVE_FRAME_NUM;
		if (name->Contains(L"Акселерометр") || name->Contains(L"ACCEL"))
			return SondeData::CURVE_ACCEL;
		if (name->Contains(L"Глубина") || name->Contains(L"DEPTH"))
			return SondeData::CURVE_DEPTH;

		return SondeData::CURVE_PHASE_DIFF;
	}

	// Полное описание кривой — всплывающая подсказка при наведении на «чип»
	static String^ GetCurveTooltip(String^ name)
	{
		int type = GetCurveTypeByName(name);
		if (type >= SondeData::CURVE_PHASE_SMT_PC && type <= SondeData::CURVE_RHO_ATT_PC)
		{
			String^ units = type == SondeData::CURVE_PHASE_SMT_PC ? L"мгр" :
				(type == SondeData::CURVE_ATT_SMT_PC ? L"дБ" : L"Ом·м");
			return name + L" — расчёт на PC по исходным данным и метрологии, " + units;
		}
		if (name->EndsWith(L" Sonde"))
			return L"Sonde — готовое поле кадра прибора. " + GetCurveTooltip(name->Substring(0, name->Length - 6));
		if (name == L"Raw Δφ")        return L"PH Z2-Z1 — разность фаз приёмников (DELTA_PH), мгр";
		if (name == L"AM Z2/Z1")     return L"Затухание 20·log10(AM Z2/AM Z1), знак чередуется по Tx, дБ";
		if (name == L"AMP Z1")       return L"AM Z1 — амплитуда приёмника Z1, мВ";
		if (name == L"AMP Z2")       return L"AM Z2 — амплитуда приёмника Z2, мВ";
		if (name == L"AM silent")    return L"AM Zero ZZ — «тихая» амплитуда без генератора, мкВ";
		if (name == L"TEMP")         return L"Temp RZZ — температура, °C";
		if (name == L"Δφ SMT")        return L"PH_SMT_from_sonde — симметризованная фаза прибора, мгр";
		if (name == L"RO Δφ")         return L"УЭС по фазе (rho_ph_smt), Ом·м";
		if (name == L"ATT SMT")      return L"ATT_SMT_from_sonde — симметризованное затухание прибора, дБ";
		if (name == L"RO ATT")       return L"УЭС по затуханию (rho_att_smt), Ом·м";
		if (name == L"ε RO")         return L"Погрешность УЭС — невязка прибор/компьютер (PC-пересчёт, след. этап)";
		if (name == L"FRAME")        return L"FRAME — номер кадра прибора";
		if (name == L"ACCEL")        return L"ACCEL — акселерометр";
		if (name == L"AM GEO")       return L"AM_GEO — амплитуда геосигнала (|Geo|)";
		if (name == L"Δφ GEO")        return L"PH_GEO — фаза геосигнала (arg Geo), мгр";
		if (name == L"AM GEO SMT")   return L"AM_GEO_SMT — сглаженная гео-амплитуда, дБ";
		if (name == L"Δφ GEO SMT")    return L"PH_GEO_SMT — сглаженная гео-фаза, град";
		if (name == L"AM VZX")       return L"AM_V_ZX — амплитуда направленного приёмника, мВ";
		if (name == L"AM VZZ")       return L"AM_V_ZZ — амплитуда осевого приёмника, мВ";
		if (name == L"AM VXX")       return L"AM_V_XX — амплитуда поперечного приёмника, мВ";
		if (name == L"βz")           return L"betta_z — электрический угол наклона антенны";
		if (name == L"Δφ VZX/VZZ")    return L"PH_VZX / VZZ — разность фаз VZX и VZZ, мгр";
		if (name == L"dV")           return L"dV — разброс по 16 секторам (шум)";
		if (name == L"SNR")          return L"SNR — отношение сигнал/шум";
		if (name == L"AM silent ZX") return L"AM Zero ZX — «тихая» амплитуда направленного приёмника, мкВ";
		if (name == L"BORDER ∠")     return L"BORDER_ANGLE — угол границы пласта, град";
		if (name == L"INC")          return L"INC — инклинометр: Abs / Zen / Azm, град";
		if (name == L"ω")            return L"ANGULAR_VELOCITY — угловая скорость вращения";
		if (name == L"START SECTOR") return L"START_SECTOR — стартовый сектор кадра";
		if (name == L"TEMP RZX")     return L"Temp RZX — температура направленного приёмника, °C";
		if (name == L"DEPTH")        return L"DEPTH — глубина, м";
		return name;
	}

	static void AddCurvePoint(Series^ series, double x, double y)
	{
		bool missing = Double::IsNaN(y) || Double::IsInfinity(y);
		int index = series->Points->AddXY(x, missing ? 0.0 : y);
		series->Points[index]->IsEmpty = missing;
	}

	static String^ GetAxisTitleByCurve(String^ name)
	{
		int type = GetCurveTypeByName(name);
		if (type >= SondeData::CURVE_PHASE_SMT_PC && type <= SondeData::CURVE_RHO_ATT_PC && !SondeData::HasMetrology())
			return L"PC: загрузите метрологию — расчёт недоступен";
		switch (GetCurveTypeByName(name))
		{
		case SondeData::CURVE_PHASE_DIFF:
		case SondeData::CURVE_PHASE_SMT_PC:
		case SondeData::CURVE_PHASE_SMT:
		case SondeData::CURVE_GEO_PH:
		case SondeData::CURVE_RATIO_ZX_ZZ:
			return L"Фаза (мгр)";
		case SondeData::CURVE_GEO_PH_SMT:
		case SondeData::CURVE_BORDER_ANGLE:
		case SondeData::CURVE_INC_ABS:
			return L"Угол (град)";
		case SondeData::CURVE_AMPLITUDE_RX1:
		case SondeData::CURVE_AMPLITUDE_RX2:
		case SondeData::CURVE_GEO_AM:
		case SondeData::CURVE_V_ZX:
		case SondeData::CURVE_V_ZZ:
		case SondeData::CURVE_V_XX:
		case SondeData::CURVE_DV_NOISE:
			return L"Амплитуда (мВ)";
		case SondeData::CURVE_ZERO_AM_ZZ:
		case SondeData::CURVE_ZERO_AM_ZX:
			return L"Амплитуда (мкВ)";
		case SondeData::CURVE_RHO_PHASE_PC:
		case SondeData::CURVE_RHO_ATT_PC:
		case SondeData::CURVE_RHO_PHASE:
		case SondeData::CURVE_RHO_ATT:
		case SondeData::CURVE_ERROR_RHO:
			return L"УЭС (Ом·м)";
		case SondeData::CURVE_ATTENUATION:
		case SondeData::CURVE_ATT_SMT_PC:
		case SondeData::CURVE_ATT_SMT:
		case SondeData::CURVE_GEO_AM_SMT:
			return L"Затухание (дБ)";
		case SondeData::CURVE_TEMPERATURE:
		case SondeData::CURVE_TEMP_RZX:
			return L"Температура (°C)";
		case SondeData::CURVE_ANGULAR_VELOCITY:
			return L"Скорость (об/мин)";
		case SondeData::CURVE_START_SECTOR:
			return L"Сектор";
		case SondeData::CURVE_DEPTH:
			return L"Глубина (м)";
		case SondeData::CURVE_FRAME_NUM:
			return L"Номер кадра";
		case SondeData::CURVE_BETTA_Z:
		case SondeData::CURVE_SNR:
			return L"Отношение";
		}
		return L"Значение";
	}

	static String^ GetPrecisionFormat(int precisionMode, String^ curveName)
	{
		if (precisionMode <= 0) // Авто
		{
			if (String::IsNullOrEmpty(curveName)) return L"F2";
			switch (GetCurveTypeByName(curveName))
			{
			case SondeData::CURVE_TEMPERATURE:
			case SondeData::CURVE_TEMP_RZX:
				return L"F1";
			case SondeData::CURVE_FRAME_NUM:
			case SondeData::CURVE_START_SECTOR:
			case SondeData::CURVE_DEPTH:
				return L"F0";
			case SondeData::CURVE_AMPLITUDE_RX1:
			case SondeData::CURVE_AMPLITUDE_RX2:
			case SondeData::CURVE_ZERO_AM_ZZ:
			case SondeData::CURVE_ZERO_AM_ZX:
			case SondeData::CURVE_V_ZX:
			case SondeData::CURVE_V_ZZ:
			case SondeData::CURVE_V_XX:
			case SondeData::CURVE_DV_NOISE:
				return L"F4";
			}
			return L"F2";
		}
		return L"F" + (precisionMode - 1);
	}

	void MainForm::UpdateTxControlsVisibility()
	{
		int txCount = SondeData::GetTxCount();
		if (txCount <= 0) txCount = 4;

		bool hasTx5 = (txCount >= 5);
		bool hasTx4 = (txCount >= 4);

		// Смена конфигурации прибора — по умолчанию все передатчики видимы;
		// внутри одной сессии ручные снятия галочек не трогаем
		if (txCount != this->lastShownTxCount)
		{
			this->checkTx1_5->Checked = hasTx5;
			this->checkTx2_5->Checked = hasTx5;
			this->checkTx1_4->Checked = hasTx4;
			this->checkTx2_4->Checked = hasTx4;
			this->lastShownTxCount = txCount;
		}

		this->checkTx1_5->Visible = hasTx5;
		this->checkTx2_5->Visible = hasTx5;
		this->checkTx1_4->Visible = hasTx4;
		this->checkTx2_4->Visible = hasTx4;

		if (this->chartTime1->Series->Count >= 5)
		{
			this->chartTime1->Series[4]->IsVisibleInLegend = hasTx5;
			if (!hasTx5)
			{
				this->chartTime1->Series[4]->Enabled = false;
				this->chartTime1->Series[4]->Points->Clear();
			}
		}
		if (this->chartTime2->Series->Count >= 5)
		{
			this->chartTime2->Series[4]->IsVisibleInLegend = hasTx5;
			if (!hasTx5)
			{
				this->chartTime2->Series[4]->Enabled = false;
				this->chartTime2->Series[4]->Points->Clear();
			}
		}
		if (this->chartTime1->Series->Count >= 4 && !hasTx4)
		{
			this->chartTime1->Series[3]->IsVisibleInLegend = false;
			this->chartTime1->Series[3]->Enabled = false;
			this->chartTime1->Series[3]->Points->Clear();
		}
		if (this->chartTime2->Series->Count >= 4 && !hasTx4)
		{
			this->chartTime2->Series[3]->IsVisibleInLegend = false;
			this->chartTime2->Series[3]->Enabled = false;
			this->chartTime2->Series[3]->Points->Clear();
		}
	}

	void MainForm::RenderLoadedCurves()
	{
		RenderChart(1);
		RenderChart(2);
	}

	void MainForm::RenderChart(int windowIndex)
	{
		if (!SondeData::IsLoaded()) return;
		size_t frameCount = SondeData::GetFrameCount();
		if (frameCount == 0) return;
		UpdateTxControlsVisibility();
		UpdateChartPalette(windowIndex);
		Chart^ chart = windowIndex == 1 ? this->chartTime1 : this->chartTime2;
		ChartArea^ area = chart->ChartAreas[0];
		bool temperature = !this->isStreaming && (windowIndex == 1 ? this->isTempXAxis1 : this->isTempXAxis2);
		String^ active = windowIndex == 1 ? this->activeCurve1 : this->activeCurve2;
		int frequency = (windowIndex == 1 ? this->buttonFreq1 : this->buttonFreq2)->Text->Contains(L"2000") ? 1 : 0;
		int receiver = windowIndex == 1 ? this->receiverIndex1 : this->receiverIndex2;
		int precision = windowIndex == 1 ? this->precisionY1 : this->precisionY2;
		int txCount = SondeData::GetTxCount();
		int curveType = GetCurveTypeByName(active);
		area->AxisX->Title = temperature ? L"Температура (°C)" : L"Кадр (N)";
		area->AxisX->LabelStyle->Format = L"F0";
		area->AxisX->Interval = 0;
		area->AxisX->IntervalAutoMode = IntervalAutoMode::VariableCount;
		area->AxisY->Title = GetAxisTitleByCurve(active);
		area->AxisY->LabelStyle->Format = GetPrecisionFormat(precision, active);

		// Запоминаем текущий зум, если включен чекбокс удержания
		bool hold = (windowIndex == 1 ? this->checkHoldZoom1 : this->checkHoldZoom2)->Checked;
		if (!hold)
		{
			area->AxisX->ScaleView->ZoomReset(0);
			area->AxisY->ScaleView->ZoomReset(0);
			area->AxisX->Minimum = Double::NaN;
			area->AxisX->Maximum = Double::NaN;
			area->AxisY->Minimum = Double::NaN;
			area->AxisY->Maximum = Double::NaN;
			area->AxisY->Interval = 0;
		}
		for each (Series^ series in chart->Series) series->Points->Clear();
		if (String::IsNullOrEmpty(active))
		{
			area->RecalculateAxesScale();
			chart->Invalidate();
			return;
		}
		size_t start = 0, end = frameCount - 1;
		int minimum = windowIndex == 1 ? this->minFrame1 : this->minFrame2;
		int maximum = windowIndex == 1 ? this->maxFrame1 : this->maxFrame2;
		if (minimum >= 0 && static_cast<size_t>(minimum) < frameCount) start = minimum;
		if (maximum >= 0 && static_cast<size_t>(maximum) < frameCount) end = maximum;
		if (start > end) std::swap(start, end);
		if (this->isStreaming)
		{
			size_t points = static_cast<size_t>(Math::Max(1, (int)AppConfig::Instance().points));
			start = frameCount > points ? frameCount - points : 0;
			end = frameCount - 1;
			if (windowIndex == 1) { this->minFrame1 = (int)start; this->maxFrame1 = (int)end; }
			else { this->minFrame2 = (int)start; this->maxFrame2 = (int)end; }
			(windowIndex == 1 ? this->textBoxMin1 : this->textBoxMin2)->Text = start.ToString();
			(windowIndex == 1 ? this->textBoxMax1 : this->textBoxMax2)->Text = end.ToString();
			area->AxisX->ScaleView->ZoomReset(0);
			area->AxisX->Minimum = (double)start;
			area->AxisX->Maximum = Math::Max((double)start + 1, (double)end);
		}
		for (size_t i = start; i <= end; ++i)
		{
			double x = temperature ? SondeData::GetTemperature(i) : static_cast<double>(i);
			for (int tx = 0; tx < txCount && tx < chart->Series->Count; ++tx)
				AddCurvePoint(chart->Series[tx], x, SondeData::GetCurveValue(curveType, frequency, tx, i, receiver));
		}

		// Применяем фильтры передатчиков с учетом типа зонда
		cli::array<CheckBox^>^ checks = windowIndex == 1 ?
			gcnew cli::array<CheckBox^> { this->checkTx1_1, this->checkTx1_2, this->checkTx1_3, this->checkTx1_4, this->checkTx1_5 } :
			gcnew cli::array<CheckBox^> { this->checkTx2_1, this->checkTx2_2, this->checkTx2_3, this->checkTx2_4, this->checkTx2_5 };
		for (int tx = 0; tx < chart->Series->Count && tx < checks->Length; ++tx)
			chart->Series[tx]->Enabled = tx < txCount && checks[tx]->Checked;
		area->RecalculateAxesScale();
		chart->Invalidate();
	}

	void MainForm::RefreshChart(int windowIndex)
	{
		UpdateChartHeaderControls();
		SetActiveSchemeChart(windowIndex - 1);
		RenderChart(windowIndex);
		SaveStationConfig();
	}

	void MainForm::UpdateTimeSlices(double timeVal)
	{
		this->chartSectorRxL->Series[L"SectorsL"]->Points->Clear();
		this->chartSectorRxL->Series[L"H1_FitL"]->Points->Clear();
		this->chartSectorRxR->Series[L"SectorsR"]->Points->Clear();
		this->chartSectorRxR->Series[L"H1_FitR"]->Points->Clear();

		if (SondeData::IsLoaded())
		{
			size_t totalFrames = SondeData::GetFrameCount();
			if (totalFrames == 0) return;

			int frameIdx = 0;
			if (this->isTempXAxis1)
			{
				double bestDiff = 1e9;
				for (size_t i = 0; i < totalFrames; ++i)
				{
					double diff = Math::Abs(SondeData::GetTemperature(i) - timeVal);
					if (diff < bestDiff)
					{
						bestDiff = diff;
						frameIdx = static_cast<int>(i);
					}
				}
			}
			else
			{
				frameIdx = static_cast<int>(Math::Round(timeVal));
				if (frameIdx < 0) frameIdx = 0;
				if (static_cast<size_t>(frameIdx) >= totalFrames) frameIdx = static_cast<int>(totalFrames - 1);
			}

			int freqL = (this->buttonFreqRxL->Text->Contains(L"2000")) ? 1 : 0;
			int freqR = (this->buttonFreqRxR->Text->Contains(L"2000")) ? 1 : 0;
			int txL = (this->comboBoxTxRxL->SelectedIndex >= 0) ? this->comboBoxTxRxL->SelectedIndex : 0;
			int txR = (this->comboBoxTxRxR->SelectedIndex >= 0) ? this->comboBoxTxRxR->SelectedIndex : 0;

			// Rx_L
			SondeData::SectorAnalysis saL = {};
			if (SondeData::GetSectorData(0, freqL, txL, frameIdx, saL))
			{
				for (int sec = 0; sec < 16; sec++)
				{
					this->chartSectorRxL->Series[L"SectorsL"]->Points->AddXY(sec, saL.sectors[sec]);
				}
				for (double sec = 0; sec <= 15.0; sec += 0.2)
				{
					double angleRad = sec * (2.0 * Math::PI / 16.0);
					double phRad = saL.phaseDeg * (Math::PI / 180.0);
					double fitVal = saL.h0 + saL.h1 * Math::Cos(angleRad - phRad);
					this->chartSectorRxL->Series[L"H1_FitL"]->Points->AddXY(sec, fitVal);
				}
				this->labelQualityH1H2_L->Text = String::Format(L"H1/H2: {0:F2}", saL.h1_h2_ratio);
				this->labelQualityDV_L->Text = String::Format(L"Шум: {0:F3} мВ", saL.dv);
				this->labelQualitySNR_L->Text = String::Format(L"SNR: {0:F1} дБ", saL.snr);
			}

			// Rx_R
			SondeData::SectorAnalysis saR = {};
			if (SondeData::GetSectorData(1, freqR, txR, frameIdx, saR))
			{
				for (int sec = 0; sec < 16; sec++)
				{
					this->chartSectorRxR->Series[L"SectorsR"]->Points->AddXY(sec, saR.sectors[sec]);
				}
				for (double sec = 0; sec <= 15.0; sec += 0.2)
				{
					double angleRad = sec * (2.0 * Math::PI / 16.0);
					double phRad = saR.phaseDeg * (Math::PI / 180.0);
					double fitVal = saR.h0 + saR.h1 * Math::Cos(angleRad - phRad);
					this->chartSectorRxR->Series[L"H1_FitR"]->Points->AddXY(sec, fitVal);
				}
				this->labelQualityH1H2_R->Text = String::Format(L"H1/H2: {0:F2}", saR.h1_h2_ratio);
				this->labelQualityDV_R->Text = String::Format(L"Шум: {0:F3} мВ", saR.dv);
				this->labelQualitySNR_R->Text = String::Format(L"SNR: {0:F1} дБ", saR.snr);
			}
		}
		else
		{
			double h0 = 12.0 + 3.0 * Math::Sin(timeVal * 0.1);
			double h1 = 4.5 + 1.0 * Math::Cos(timeVal * 0.1);
			double phaseShift = timeVal * 0.2;

			for (int sec = 0; sec < 16; sec++)
			{
				double angleRad = sec * 2.0 * Math::PI / 16.0;
				double valL = h0 + h1 * Math::Cos(angleRad - phaseShift) + 0.2 * Math::Sin(angleRad * 3);
				double valR = (h0 * 0.95) + (h1 * 0.9) * Math::Cos(angleRad - phaseShift + 0.1) + 0.2 * Math::Cos(angleRad * 3);

				this->chartSectorRxL->Series[L"SectorsL"]->Points->AddXY(sec, valL);
				this->chartSectorRxR->Series[L"SectorsR"]->Points->AddXY(sec, valR);
			}

			for (double sec = 0; sec <= 15.0; sec += 0.2)
			{
				double angleRad = sec * 2.0 * Math::PI / 16.0;
				double fitL = h0 + h1 * Math::Cos(angleRad - phaseShift);
				double fitR = (h0 * 0.95) + (h1 * 0.9) * Math::Cos(angleRad - phaseShift + 0.1);

				this->chartSectorRxL->Series[L"H1_FitL"]->Points->AddXY(sec, fitL);
				this->chartSectorRxR->Series[L"H1_FitR"]->Points->AddXY(sec, fitR);
			}
		}

		UpdateCursorReadout(timeVal);
	}

	void MainForm::UpdateCursorReadout(double timeVal)
	{
		int frameNum = 0;
		if (SondeData::IsLoaded())
		{
			size_t totalFrames = SondeData::GetFrameCount();
			if (this->isTempXAxis1)
			{
				double bestDiff = 1e9;
				for (size_t i = 0; i < totalFrames; ++i)
				{
					double diff = Math::Abs(SondeData::GetTemperature(i) - timeVal);
					if (diff < bestDiff)
					{
						bestDiff = diff;
						frameNum = static_cast<int>(i);
					}
				}
			}
			else
			{
				frameNum = static_cast<int>(Math::Round(timeVal));
				if (frameNum < 0) frameNum = 0;
				if (static_cast<size_t>(frameNum) >= totalFrames) frameNum = static_cast<int>(totalFrames - 1);
			}
		}
		else
		{
			frameNum = static_cast<int>(Math::Round(timeVal * 10.0));
			if (frameNum < 0) frameNum = 0;
		}

		double v1 = 0.0, v2 = 0.0, v3 = 0.0, v4 = 0.0, v5 = 0.0;
		if (this->chartTime1->Series->Count >= 5)
		{
			int count = this->chartTime1->Series[0]->Points->Count;
			int ptIdx = (frameNum < count) ? frameNum : (count - 1);
			if (ptIdx >= 0)
			{
				v1 = this->chartTime1->Series[0]->Points[ptIdx]->YValues[0];
				v2 = this->chartTime1->Series[1]->Points[ptIdx]->YValues[0];
				v3 = this->chartTime1->Series[2]->Points[ptIdx]->YValues[0];
				v4 = this->chartTime1->Series[3]->Points[ptIdx]->YValues[0];
				if (this->chartTime1->Series[4]->Points->Count > ptIdx)
					v5 = this->chartTime1->Series[4]->Points[ptIdx]->YValues[0];
			}
		}

		if (SondeData::IsLoaded())
		{
			double tVal = SondeData::GetTemperature(frameNum);
			String^ fmt = GetPrecisionFormat(this->precisionY1, this->activeCurve1);
			int txCount = SondeData::GetTxCount();
			if (txCount >= 5)
			{
				this->labelCursorInfo->Text = String::Format(
					L"Кадр #{0} (T = {1:F0} °C) | Tx1: {2:" + fmt + L"} | Tx2: {3:" + fmt + L"} | Tx3: {4:" + fmt + L"} | Tx4: {5:" + fmt + L"} | Tx5: {6:" + fmt + L"}",
					frameNum, tVal, v1, v2, v3, v4, v5);
			}
			else if (txCount == 4)
			{
				this->labelCursorInfo->Text = String::Format(
					L"Кадр #{0} (T = {1:F0} °C) | Tx1: {2:" + fmt + L"} | Tx2: {3:" + fmt + L"} | Tx3: {4:" + fmt + L"} | Tx4: {5:" + fmt + L"}",
					frameNum, tVal, v1, v2, v3, v4);
			}
			else
			{
				this->labelCursorInfo->Text = String::Format(
					L"Кадр #{0} (T = {1:F0} °C) | Tx1: {2:" + fmt + L"} | Tx2: {3:" + fmt + L"} | Tx3: {4:" + fmt + L"}",
					frameNum, tVal, v1, v2, v3);
			}
		}
		else
		{
			this->labelCursorInfo->Text = String::Format(
				L"Точка #{0} | Tx1: {1:F2} | Tx2: {2:F2} | Tx3: {3:F2} | Tx4: {4:F2} | Tx5: {5:F2}",
				frameNum, v1, v2, v3, v4, v5);
		}
	}

	void MainForm::ApplyRange(int windowIndex)
	{
		TextBox^ tbMin = (windowIndex == 1) ? this->textBoxMin1 : this->textBoxMin2;
		TextBox^ tbMax = (windowIndex == 1) ? this->textBoxMax1 : this->textBoxMax2;
		Chart^ chart = (windowIndex == 1) ? this->chartTime1 : this->chartTime2;
		if (chart == nullptr || chart->ChartAreas->Count == 0) return;

		double minVal = Double::NaN;
		double maxVal = Double::NaN;

		if (!String::IsNullOrWhiteSpace(tbMin->Text))
		{
			double pt = 0.0;
			String^ s = tbMin->Text->Trim()->Replace(L",", L".");
			if (Double::TryParse(s, System::Globalization::NumberStyles::Any, System::Globalization::CultureInfo::InvariantCulture, pt))
			{
				bool isTemp = (windowIndex == 1) ? this->isTempXAxis1 : this->isTempXAxis2;
				minVal = (SondeData::IsLoaded() || isTemp) ? pt : (pt * 0.1);
			}
		}

		if (!String::IsNullOrWhiteSpace(tbMax->Text))
		{
			double pt = 0.0;
			String^ s = tbMax->Text->Trim()->Replace(L",", L".");
			if (Double::TryParse(s, System::Globalization::NumberStyles::Any, System::Globalization::CultureInfo::InvariantCulture, pt))
			{
				bool isTemp = (windowIndex == 1) ? this->isTempXAxis1 : this->isTempXAxis2;
				maxVal = (SondeData::IsLoaded() || isTemp) ? pt : (pt * 0.1);
			}
		}

		if (!Double::IsNaN(minVal) && !Double::IsNaN(maxVal) && minVal > maxVal)
		{
			double tmp = minVal;
			minVal = maxVal;
			maxVal = tmp;
			tbMin->Text = minVal.ToString();
			tbMax->Text = maxVal.ToString();
		}

		if (SondeData::IsLoaded())
		{
			size_t total = SondeData::GetFrameCount();
			if (windowIndex == 1)
			{
				if (!Double::IsNaN(minVal)) this->minFrame1 = static_cast<int>(Math::Round(minVal));
				else this->minFrame1 = 0;

				if (!Double::IsNaN(maxVal)) this->maxFrame1 = static_cast<int>(Math::Round(maxVal));
				else this->maxFrame1 = static_cast<int>(total - 1);
			}
			else
			{
				if (!Double::IsNaN(minVal)) this->minFrame2 = static_cast<int>(Math::Round(minVal));
				else this->minFrame2 = 0;

				if (!Double::IsNaN(maxVal)) this->maxFrame2 = static_cast<int>(Math::Round(maxVal));
				else this->maxFrame2 = static_cast<int>(total - 1);
			}

			chart->ChartAreas[0]->AxisX->Minimum = Double::NaN;
			chart->ChartAreas[0]->AxisX->Maximum = Double::NaN;
			chart->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
			chart->ChartAreas[0]->AxisY->ScaleView->ZoomReset(0);

			RenderLoadedCurves();
		}
		else
		{
			chart->ChartAreas[0]->AxisX->IsMarginVisible = false;
			if (Double::IsNaN(minVal) || Double::IsNaN(maxVal))
			{
				chart->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
				chart->ChartAreas[0]->AxisX->Minimum = Double::NaN;
				chart->ChartAreas[0]->AxisX->Maximum = Double::NaN;
			}
			else
			{
				chart->ChartAreas[0]->AxisX->Minimum = minVal;
				chart->ChartAreas[0]->AxisX->Maximum = maxVal;
				chart->ChartAreas[0]->AxisX->ScaleView->Zoom(minVal, maxVal);
			}
			chart->Invalidate();
		}
		chart->Invalidate();
	}

	static cli::array<String^>^ GetCurvePool(bool direct);

	void MainForm::PopulateCurveMenu(int windowIndex)
	{
		System::Windows::Forms::ContextMenuStrip^ curveMenu = (windowIndex == 1) ? this->contextMenuCurves1 : this->contextMenuCurves2;
		List<String^>^ pinned = (windowIndex == 1) ? this->pinnedCurves1 : this->pinnedCurves2;
		bool isDirect = (windowIndex == 1) ? this->isDirectMode1 : this->isDirectMode2;

		curveMenu->Items->Clear();

		cli::array<String^>^ activePool = GetCurvePool(isDirect);

		for each (String^ curve in activePool)
		{
			if (!pinned->Contains(curve))
			{
				ToolStripMenuItem^ item = gcnew ToolStripMenuItem(curve);
				item->ToolTipText = GetCurveTooltip(curve);
				item->Tag = windowIndex;
				item->Click += gcnew EventHandler(this, &MainForm::CurveMenuItem_Click);
				curveMenu->Items->Add(item);
			}
		}

		if (curveMenu->Items->Count == 0)
		{
			ToolStripMenuItem^ empty = gcnew ToolStripMenuItem(L"(Все кривые вынесены)");
			empty->Enabled = false;
			curveMenu->Items->Add(empty);
		}
	}

	static String^ NormalizeCurveName(String^ curveName)
	{
		switch (GetCurveTypeByName(curveName))
		{
		case SondeData::CURVE_PHASE_SMT: return L"Δφ SMT Sonde";
		case SondeData::CURVE_RHO_PHASE: return L"RO Δφ Sonde";
		case SondeData::CURVE_ATT_SMT: return L"ATT SMT Sonde";
		case SondeData::CURVE_RHO_ATT: return L"RO ATT Sonde";
		case SondeData::CURVE_PHASE_SMT_PC: return L"Δφ SMT PC";
		case SondeData::CURVE_RHO_PHASE_PC: return L"RO Δφ PC";
		case SondeData::CURVE_ATT_SMT_PC: return L"ATT SMT PC";
		case SondeData::CURVE_RHO_ATT_PC: return L"RO ATT PC";
		}
		return curveName;
	}

	static cli::array<String^>^ GetCurvePool(bool direct)
	{
		if (direct) return gcnew cli::array<String^> {
			L"AM GEO",      // AM_GEO — амплитуда геосигнала
			L"Δφ GEO",       // PH_GEO — фаза геосигнала
			L"AM VZX",      // AM_V_ZX
			L"AM VZZ",      // AM_V_ZZ
			L"βz",          // betta_z
			L"Δφ VZX/VZZ",   // PH_VZX / VZZ
			L"AM VXX",      // AM_V_XX
			L"dV",          // шум dV
			L"SNR",         // сигнал/шум
			L"AM silent ZX",// AM Zero ZX
			L"AM GEO SMT",  // сглаженная гео-амплитуда (ATT_dB_geo_signal_smt)
			L"Δφ GEO SMT",   // сглаженная гео-фаза (PH_deg_geo_signal_smt)
			L"BORDER ∠",    // угол границы
			L"INC",         // инклинометр Abs/Zen/Azm
			L"ω",           // ANGULAR_VELOCITY — угловая скорость
			L"START SECTOR",// стартовый сектор
			L"TEMP RZX",    // температура RZX
			L"FRAME",
			L"DEPTH"
		};
		return gcnew cli::array<String^> {
			L"Raw Δφ",       // PH Z2-Z1 — разность фаз Z2-Z1 (DELTA_PH)
			L"AM Z2/Z1",    // затухание, вычисленное из амплитуд
			L"AMP Z1",      // амплитуда приёмника Z1
			L"AMP Z2",      // амплитуда приёмника Z2
			L"AM silent",   // AM Zero ZZ — «тихая» амплитуда
			L"TEMP",        // Temp RZZ
			L"Δφ SMT PC",
			L"RO Δφ PC",
			L"Δφ SMT Sonde",       // PH_SMT_from_sonde — симметризованная фаза прибора
			L"RO Δφ Sonde",        // УЭС по фазе (rho_ph_smt)
			L"ATT SMT PC",
			L"RO ATT PC",
			L"ATT SMT Sonde",     // ATT_SMT_from_sonde — симметризованное затухание прибора
			L"RO ATT Sonde",      // УЭС по затуханию (rho_att_smt)
			L"ε RO",        // Погрешность УЭС (PC-пересчёт — следующий этап)
			L"FRAME",       // номер кадра
			L"ACCEL"        // акселерометр
		};
	}

	static bool IsCurveValidForMode(String^ curveName, bool direct)
	{
		if (String::IsNullOrEmpty(curveName)) return false;
		curveName = NormalizeCurveName(curveName);
		for each (String^ candidate in GetCurvePool(direct))
			if (candidate == curveName) return true;
		return false;
	}

	void MainForm::AddCurveChip(int windowIndex, String^ curveName)
	{
		curveName = NormalizeCurveName(curveName);
		List<String^>^ pinned = (windowIndex == 1) ? this->pinnedCurves1 : this->pinnedCurves2;
		FlowLayoutPanel^ panel = (windowIndex == 1) ? this->flowPanelChips1 : this->flowPanelChips2;

		if (pinned->Contains(curveName)) return;
		pinned->Add(curveName);

		Panel^ chip = gcnew Panel();
		chip->AutoSize = true;
		chip->Height = 26;
		chip->BorderStyle = BorderStyle::FixedSingle;
		chip->Margin = System::Windows::Forms::Padding(2, 1, 2, 1);
		chip->Tag = curveName;
		chip->Cursor = Cursors::Hand;

		Label^ lbl = gcnew Label();
		lbl->Text = curveName;
		lbl->AutoSize = true;
		lbl->Font = gcnew Drawing::Font(L"Segoe UI", 8.25F, FontStyle::Bold);
		lbl->Location = Point(4, 4);
		lbl->Cursor = Cursors::Hand;
		lbl->Tag = chip;
		chip->Controls->Add(lbl);

		Button^ btnX = gcnew Button();
		btnX->Text = L"✕";
		btnX->Size = Drawing::Size(18, 18);
		btnX->Location = Point(lbl->Right + 4, 3);
		btnX->FlatStyle = FlatStyle::Flat;
		btnX->FlatAppearance->BorderSize = 0;
		btnX->Font = gcnew Drawing::Font(L"Segoe UI", 7, FontStyle::Bold);
		btnX->Tag = chip;
		btnX->Cursor = Cursors::Hand;
		btnX->Click += gcnew EventHandler(this, &MainForm::ChipCloseButton_Click);

		chip->Controls->Add(btnX);

		chip->Click += gcnew EventHandler(this, &MainForm::Chip_Click);
		lbl->Click += gcnew EventHandler(this, &MainForm::Chip_Click);

		// Всплывающая подсказка — полное имя кривой (как в старой программе)
		String^ hint = GetCurveTooltip(curveName);
		this->toolTipMain->SetToolTip(chip, hint);
		this->toolTipMain->SetToolTip(lbl, hint);

		panel->Controls->Add(chip);

		if (windowIndex == 1 && String::IsNullOrEmpty(this->activeCurve1))
			this->activeCurve1 = curveName;
		else if (windowIndex == 2 && String::IsNullOrEmpty(this->activeCurve2))
			this->activeCurve2 = curveName;

		UpdateChipStyles(windowIndex);
	}

	System::Void MainForm::Chip_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Control^ ctrl = dynamic_cast<Control^>(sender);
		if (ctrl == nullptr) return;

		Panel^ chip = dynamic_cast<Panel^>(ctrl);
		if (chip == nullptr && ctrl->Tag != nullptr) chip = dynamic_cast<Panel^>(ctrl->Tag);
		if (chip == nullptr && ctrl->Parent != nullptr) chip = dynamic_cast<Panel^>(ctrl->Parent);
		if (chip == nullptr || chip->Tag == nullptr) return;

		String^ curveName = chip->Tag->ToString();
		int wIdx = (chip->Parent == this->flowPanelChips1) ? 1 : 2;

		if (wIdx == 1)
			this->activeCurve1 = curveName;
		else
			this->activeCurve2 = curveName;

		UpdateChipStyles(wIdx);
		RefreshChart(wIdx);
	}

	void MainForm::UpdateChipStyles(int windowIndex)
	{
		FlowLayoutPanel^ panel = (windowIndex == 1) ? this->flowPanelChips1 : this->flowPanelChips2;
		String^ active = (windowIndex == 1) ? this->activeCurve1 : this->activeCurve2;

		for each (Control^ c in panel->Controls)
		{
			Panel^ chip = dynamic_cast<Panel^>(c);
			if (chip == nullptr || chip->Tag == nullptr) continue;

			bool isActive = (chip->Tag->ToString() == active);
			chip->BackColor = isActive ? Color::FromArgb(25, 118, 210) : Color::FromArgb(235, 240, 250);

			for each (Control^ child in chip->Controls)
			{
				Label^ lbl = dynamic_cast<Label^>(child);
				if (lbl != nullptr)
				{
					lbl->ForeColor = isActive ? Color::White : Color::FromArgb(30, 60, 120);
				}
				Button^ btn = dynamic_cast<Button^>(child);
				if (btn != nullptr)
				{
					btn->ForeColor = isActive ? Color::White : Color::DarkRed;
				}
			}
		}
	}

	System::Void MainForm::ChipCloseButton_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Button^ btn = dynamic_cast<Button^>(sender);
		if (btn != nullptr && btn->Tag != nullptr)
		{
			Panel^ chip = dynamic_cast<Panel^>(btn->Tag);
			if (chip != nullptr && chip->Tag != nullptr)
			{
				String^ cName = chip->Tag->ToString();
				int wIdx = (chip->Parent == this->flowPanelChips1) ? 1 : 2;
				RemoveCurveChip(wIdx, cName);
			}
		}
	}

	void MainForm::RemoveCurveChip(int windowIndex, String^ curveName)
	{
		List<String^>^ pinned = (windowIndex == 1) ? this->pinnedCurves1 : this->pinnedCurves2;
		FlowLayoutPanel^ panel = (windowIndex == 1) ? this->flowPanelChips1 : this->flowPanelChips2;

		int removedIndex = pinned->IndexOf(curveName);
		pinned->Remove(curveName);

		Control^ toRemove = nullptr;
		for each (Control^ c in panel->Controls)
		{
			if (c->Tag != nullptr && c->Tag->ToString() == curveName)
			{
				toRemove = c;
				break;
			}
		}
		if (toRemove != nullptr)
		{
			panel->Controls->Remove(toRemove);
			delete toRemove;
		}

		// После закрытия активной кривой активной становится предыдущая в списке,
		// либо первая оставшаяся; если кривых не осталось — график пустой
		if (windowIndex == 1 && this->activeCurve1 == curveName)
		{
			this->activeCurve1 = (pinned->Count > 0) ? pinned[removedIndex > 0 ? removedIndex - 1 : 0] : String::Empty;
		}
		else if (windowIndex == 2 && this->activeCurve2 == curveName)
		{
			this->activeCurve2 = (pinned->Count > 0) ? pinned[removedIndex > 0 ? removedIndex - 1 : 0] : String::Empty;
		}

		UpdateChipStyles(windowIndex);
		RefreshChart(windowIndex);
	}

	System::Void MainForm::CurveMenuItem_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ToolStripMenuItem^ item = dynamic_cast<ToolStripMenuItem^>(sender);
		if (item != nullptr && item->Tag != nullptr)
		{
			int wIdx = safe_cast<int>(item->Tag);
			AddCurveChip(wIdx, item->Text);
			if (wIdx == 1) this->activeCurve1 = item->Text;
			else this->activeCurve2 = item->Text;
			UpdateChipStyles(wIdx);
			RefreshChart(wIdx);
		}
	}

	System::Void MainForm::buttonAddCurve1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		PopulateCurveMenu(1);
		this->contextMenuCurves1->Show(this->buttonAddCurve1, Point(0, this->buttonAddCurve1->Height));
	}

	System::Void MainForm::buttonAddCurve2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		PopulateCurveMenu(2);
		this->contextMenuCurves2->Show(this->buttonAddCurve2, Point(0, this->buttonAddCurve2->Height));
	}

	void MainForm::RememberChartMode(int windowIndex)
	{
		if (this->modePinnedCurves == nullptr) return;
		bool direct = windowIndex == 1 ? this->isDirectMode1 : this->isDirectMode2;
		int index = (windowIndex - 1) * 2 + (direct ? 1 : 0);
		this->modePinnedCurves[index] = gcnew List<String^>(windowIndex == 1 ? this->pinnedCurves1 : this->pinnedCurves2);
		this->modeActiveCurves[index] = windowIndex == 1 ? this->activeCurve1 : this->activeCurve2;
	}

	void MainForm::AppendModeConfig(System::Text::StringBuilder^ builder, int windowIndex)
	{
		for (int mode = 0; mode < 2; ++mode)
		{
			int index = (windowIndex - 1) * 2 + mode;
			if (this->modeActiveCurves[index] == nullptr) continue;
			String^ prefix = mode == 0 ? L"Undirect" : L"Direct";
			builder->AppendLine(String::Format(L"    \"{0}ActiveCurve\": \"{1}\",", prefix, this->modeActiveCurves[index]));
			builder->Append(String::Format(L"    \"{0}PinnedCurves\": [", prefix));
			List<String^>^ pinned = this->modePinnedCurves[index];
			for (int i = 0; i < pinned->Count; ++i)
				builder->Append(String::Format(L"\"{0}\"{1}", pinned[i], i + 1 < pinned->Count ? L", " : L""));
			builder->AppendLine(L"],");
		}
	}

	bool MainForm::LoadModeConfigLine(int windowIndex, String^ line)
	{
		for (int mode = 0; mode < 2; ++mode)
		{
			int index = (windowIndex - 1) * 2 + mode;
			String^ prefix = mode == 0 ? L"Undirect" : L"Direct";
			if (line->StartsWith(L"\"" + prefix + L"ActiveCurve\":"))
			{
				String^ name = NormalizeCurveName(line->Substring(line->IndexOf(':') + 1)->Trim()->Trim(',')->Trim('"'));
				// Кривая чужого режима из повреждённой конфигурации отбрасывается
				this->modeActiveCurves[index] = IsCurveValidForMode(name, mode == 1) ? name : nullptr;
				return true;
			}
			if (line->StartsWith(L"\"" + prefix + L"PinnedCurves\":"))
			{
				int start = line->IndexOf('['), end = line->LastIndexOf(']');
				this->modePinnedCurves[index]->Clear();
				if (start >= 0 && end > start)
					for each (String^ item in line->Substring(start + 1, end - start - 1)->Split(','))
					{
						String^ curve = NormalizeCurveName(item->Trim()->Trim('"'));
						if (IsCurveValidForMode(curve, mode == 1)) this->modePinnedCurves[index]->Add(curve);
					}
				return true;
			}
		}
		return false;
	}

	void MainForm::SetChartMode(int windowIndex, bool direct)
	{
		if ((windowIndex == 1 ? this->isDirectMode1 : this->isDirectMode2) == direct) return;
		RememberChartMode(windowIndex);
		Button^ button = windowIndex == 1 ? this->buttonMode1 : this->buttonMode2;
		FlowLayoutPanel^ chips = windowIndex == 1 ? this->flowPanelChips1 : this->flowPanelChips2;
		List<String^>^ pinned = windowIndex == 1 ? this->pinnedCurves1 : this->pinnedCurves2;
		if (windowIndex == 1) this->isDirectMode1 = direct;
		else this->isDirectMode2 = direct;
		button->Text = direct ? L"DIRECT" : L"UNDIRECT";
		button->BackColor = direct ? Color::FromArgb(255, 240, 210) : Color::FromArgb(220, 245, 225);
		chips->Controls->Clear();
		pinned->Clear();
		int index = (windowIndex - 1) * 2 + (direct ? 1 : 0);
		if (this->modeActiveCurves[index] != nullptr || this->modePinnedCurves[index]->Count > 0)
		{
			for each (String^ curve in this->modePinnedCurves[index])
				if (IsCurveValidForMode(curve, direct)) AddCurveChip(windowIndex, curve);
			String^ restored = this->modeActiveCurves[index];
			if (!IsCurveValidForMode(restored, direct)) restored = nullptr;
			if (restored == nullptr) restored = pinned->Count > 0 ? pinned[0] : String::Empty;
			if (!String::IsNullOrEmpty(restored) && !pinned->Contains(restored)) AddCurveChip(windowIndex, restored);
			if (windowIndex == 1) this->activeCurve1 = restored;
			else this->activeCurve2 = restored;
			UpdateChipStyles(windowIndex);
			return;
		}
		String^ active = windowIndex == 1 ? (direct ? L"Δφ GEO" : L"Raw Δφ") :
			(direct ? L"Δφ VZX/VZZ" : L"AM Z2/Z1");
		if (windowIndex == 1)
		{
			AddCurveChip(windowIndex, direct ? L"AM GEO" : L"Raw Δφ");
			AddCurveChip(windowIndex, direct ? L"Δφ GEO" : L"AMP Z1");
			if (!direct) AddCurveChip(windowIndex, L"RO Δφ Sonde");
			this->activeCurve1 = active;
		}
		else
		{
			AddCurveChip(windowIndex, active);
			AddCurveChip(windowIndex, direct ? L"dV" : L"RO ATT Sonde");
			this->activeCurve2 = active;
		}
		UpdateChipStyles(windowIndex);
	}

	void MainForm::UpdateChartPalette(int windowIndex)
	{
		bool direct = windowIndex == 1 ? this->isDirectMode1 : this->isDirectMode2;
		int receiver = windowIndex == 1 ? this->receiverIndex1 : this->receiverIndex2;
		cli::array<Color>^ colors = gcnew cli::array<Color> {
			Color::Green, Color::DarkGoldenrod, Color::Red, Color::RoyalBlue, Color::Magenta };
		if (direct && receiver == 1)
		{
			colors[0] = Color::DarkGoldenrod;
			colors[1] = Color::Green;
		}
		cli::array<int>^ txLabels = !direct ? gcnew cli::array<int> { 1, 2, 3, 4, 5 } :
			(receiver == 0 ? gcnew cli::array<int> { 1, 2, 4, 5, 0 } : gcnew cli::array<int> { 1, 2, 3, 5, 0 });
		cli::array<CheckBox^>^ checks = windowIndex == 1 ?
			gcnew cli::array<CheckBox^> { this->checkTx1_1, this->checkTx1_2, this->checkTx1_3, this->checkTx1_4, this->checkTx1_5 } :
			gcnew cli::array<CheckBox^> { this->checkTx2_1, this->checkTx2_2, this->checkTx2_3, this->checkTx2_4, this->checkTx2_5 };
		Chart^ chart = windowIndex == 1 ? this->chartTime1 : this->chartTime2;
		for (int tx = 0; tx < checks->Length; ++tx)
		{
			String^ label = txLabels[tx] > 0 ? L"T" + txLabels[tx].ToString() : L"—";
			checks[tx]->ForeColor = colors[tx];
			checks[tx]->Text = label;
			if (tx >= chart->Series->Count) continue;
			chart->Series[tx]->Color = colors[tx];
			chart->Series[tx]->LegendText = label;
		}
	}

	void MainForm::UpdateChartHeaderControls()
	{
		for (int window = 1; window <= 2; ++window)
		{
			bool direct = window == 1 ? this->isDirectMode1 : this->isDirectMode2;
			Panel^ header = window == 1 ? this->panelTime1Header : this->panelTime2Header;
			Button^ receiver = window == 1 ? this->buttonReceiver1 : this->buttonReceiver2;
			if (receiver == nullptr) continue;
			UpdateChartPalette(window);
			receiver->Text = (window == 1 ? this->receiverIndex1 : this->receiverIndex2) == 0 ? L"Rx L" : L"Rx R";
			cli::array<Control^>^ buttons = window == 1 ?
				gcnew cli::array<Control^> { this->buttonMode1, this->buttonFreq1, receiver, this->buttonXMode1, this->buttonAddCurve1 } :
				gcnew cli::array<Control^> { this->buttonMode2, this->buttonFreq2, receiver, this->buttonXMode2, this->buttonAddCurve2 };
			cli::array<bool>^ visible = gcnew cli::array<bool> { this->isCartographLayout, true,
				this->isCartographLayout && direct, !this->isStreaming, true };
			int x = 4;
			for (int i = 0; i < buttons->Length; ++i)
			{
				buttons[i]->Visible = visible[i];
				if (!visible[i]) continue;
				buttons[i]->Location = Point(x, 3);
				x += buttons[i]->Width + 4;
			}
			FlowLayoutPanel^ chips = window == 1 ? this->flowPanelChips1 : this->flowPanelChips2;
			chips->Left = x;
			chips->Width = Math::Max(20, header->ClientSize.Width - x - 8);
		}
	}

	System::Void MainForm::buttonMode1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (!this->isCartographLayout) return;
		SetChartMode(1, !this->isDirectMode1);
		RefreshChart(1);
	}

	System::Void MainForm::buttonMode2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (!this->isCartographLayout) return;
		SetChartMode(2, !this->isDirectMode2);
		RefreshChart(2);
	}

	System::Void MainForm::buttonReceiver_Click(System::Object^ sender, System::EventArgs^ e)
	{
		Button^ button = dynamic_cast<Button^>(sender);
		if (button == nullptr || !this->isCartographLayout) return;
		int window = safe_cast<int>(button->Tag);
		if (window == 1 && this->isDirectMode1) this->receiverIndex1 = 1 - this->receiverIndex1;
		else if (window == 2 && this->isDirectMode2) this->receiverIndex2 = 1 - this->receiverIndex2;
		else return;
		RefreshChart(window);
	}

	System::Void MainForm::buttonFreq1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreq1->Text == L"2000 кГц")
			this->buttonFreq1->Text = L"400 кГц";
		else
			this->buttonFreq1->Text = L"2000 кГц";

		RefreshChart(1);
	}

	System::Void MainForm::buttonFreq2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreq2->Text == L"400 кГц")
			this->buttonFreq2->Text = L"2000 кГц";
		else
			this->buttonFreq2->Text = L"400 кГц";

		RefreshChart(2);
	}

	System::Void MainForm::buttonFreqRxL_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreqRxL->Text == L"2000 кГц")
			this->buttonFreqRxL->Text = L"400 кГц";
		else
			this->buttonFreqRxL->Text = L"2000 кГц";

		UpdateTimeSlices(this->chartTime1->ChartAreas[0]->CursorX->Position);
		SaveStationConfig();
	}

	System::Void MainForm::buttonFreqRxR_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreqRxR->Text == L"2000 кГц")
			this->buttonFreqRxR->Text = L"400 кГц";
		else
			this->buttonFreqRxR->Text = L"2000 кГц";

		UpdateTimeSlices(this->chartTime1->ChartAreas[0]->CursorX->Position);
		SaveStationConfig();
	}

	System::Void MainForm::comboBoxTxRx_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e)
	{
		UpdateTimeSlices(this->chartTime1->ChartAreas[0]->CursorX->Position);
		SaveStationConfig();
	}

	System::Void MainForm::buttonToggleSectors_Click(System::Object^ sender, System::EventArgs^ e)
	{
		bool isCollapsed = !this->splitContainerBand1->Panel2Collapsed;
		this->sectorsCollapsedByUser = isCollapsed;
		this->splitContainerBand1->Panel2Collapsed = isCollapsed;
		this->splitContainerBand2->Panel2Collapsed = isCollapsed;
		this->buttonToggleSectors->Text = isCollapsed ? L"Показать секторы ⮞" : L"⮜ Скрыть секторы";
	}

	void MainForm::ApplySondeTypeLayout(bool isCarto)
	{
		this->isCartographLayout = isCarto;
		bool changed = !isCarto && (this->isDirectMode1 || this->isDirectMode2);
		if (!isCarto && this->isDirectMode1) SetChartMode(1, false);
		if (!isCarto && this->isDirectMode2) SetChartMode(2, false);
		UpdateChartHeaderControls();
		if (changed) RenderLoadedCurves();
		if (isCarto)
		{
			// «Картограф»: секторные окна доступны, учитываем выбор пользователя
			this->buttonToggleSectors->Enabled = true;
			this->splitContainerBand1->Panel2Collapsed = this->sectorsCollapsedByUser;
			this->splitContainerBand2->Panel2Collapsed = this->sectorsCollapsedByUser;
			this->buttonToggleSectors->Text = this->sectorsCollapsedByUser ?
				L"Показать секторы ⮞" : L"⮜ Скрыть секторы";
		}
		else
		{
			// GP-приборы: направленных приёмников нет, секторные окна бессмысленны
			this->splitContainerBand1->Panel2Collapsed = true;
			this->splitContainerBand2->Panel2Collapsed = true;
			this->buttonToggleSectors->Enabled = false;
			this->buttonToggleSectors->Text = L"Секторы: только Картограф";
		}
	}

	System::Void MainForm::radioButtonLayout_CheckedChanged(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->radioButtonLayoutBoth->Checked)
		{
			this->splitContainerRows->Panel1Collapsed = false;
			this->splitContainerRows->Panel2Collapsed = false;
		}
		else if (this->radioButtonLayoutTop->Checked)
		{
			this->splitContainerRows->Panel1Collapsed = false;
			this->splitContainerRows->Panel2Collapsed = true;
		}
		else if (this->radioButtonLayoutBottom->Checked)
		{
			this->splitContainerRows->Panel1Collapsed = true;
			this->splitContainerRows->Panel2Collapsed = false;
		}
	}

	System::Void MainForm::splitContainerBand_SplitterMoved(System::Object^ sender, System::Windows::Forms::SplitterEventArgs^ e)
	{
		if (this->isSyncingSplitter) return;
		this->isSyncingSplitter = true;
		try
		{
			SplitContainer^ source = dynamic_cast<SplitContainer^>(sender);
			if (source == this->splitContainerBand1 && this->splitContainerBand2 != nullptr)
			{
				if (!this->splitContainerBand2->Panel2Collapsed && this->splitContainerBand1->SplitterDistance > 0)
				{
					this->splitContainerBand2->SplitterDistance = this->splitContainerBand1->SplitterDistance;
				}
			}
			else if (source == this->splitContainerBand2 && this->splitContainerBand1 != nullptr)
			{
				if (!this->splitContainerBand1->Panel2Collapsed && this->splitContainerBand2->SplitterDistance > 0)
				{
					this->splitContainerBand1->SplitterDistance = this->splitContainerBand2->SplitterDistance;
				}
			}
		}
		catch (...) {}
		finally
		{
			this->isSyncingSplitter = false;
		}
	}

	System::Void MainForm::checkTx1_CheckedChanged(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->chartTime1->Series->Count >= 5)
		{
			this->chartTime1->Series[0]->Enabled = this->checkTx1_1->Checked;
			this->chartTime1->Series[1]->Enabled = this->checkTx1_2->Checked;
			this->chartTime1->Series[2]->Enabled = this->checkTx1_3->Checked;
			this->chartTime1->Series[3]->Enabled = this->checkTx1_4->Checked;
			this->chartTime1->Series[4]->Enabled = this->checkTx1_5->Checked;
			this->chartTime1->Invalidate();
		}
		SaveStationConfig();
	}

	System::Void MainForm::checkTx2_CheckedChanged(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->chartTime2->Series->Count >= 5)
		{
			this->chartTime2->Series[0]->Enabled = this->checkTx2_1->Checked;
			this->chartTime2->Series[1]->Enabled = this->checkTx2_2->Checked;
			this->chartTime2->Series[2]->Enabled = this->checkTx2_3->Checked;
			this->chartTime2->Series[3]->Enabled = this->checkTx2_4->Checked;
			this->chartTime2->Series[4]->Enabled = this->checkTx2_5->Checked;
			this->chartTime2->Invalidate();
		}
		SaveStationConfig();
	}

	System::Void MainForm::buttonAutoY1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->chartTime1->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime1->ChartAreas[0]->AxisY->Maximum = Double::NaN;
	}

	System::Void MainForm::buttonAutoY2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->chartTime2->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisY->Maximum = Double::NaN;
	}

	System::Void MainForm::chartTime_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
	{
		if (this->panelLeft != nullptr && this->panelLeft->Visible && !this->isSidebarPinned)
		{
			HideSidebar();
		}

		Chart^ chart = dynamic_cast<Chart^>(sender);
		if (chart == nullptr || chart->ChartAreas->Count == 0) return;

		bool isChart1 = (chart == this->chartTime1);
		SetActiveSchemeChart(isChart1 ? 0 : 1);
		if (isChart1) { this->mouseDownPoint1 = e->Location; this->panStartPoint1 = e->Location; }
		else { this->mouseDownPoint2 = e->Location; this->panStartPoint2 = e->Location; }

		if (e->Button == System::Windows::Forms::MouseButtons::Middle)
		{
			if (isChart1) this->isPanning1 = true;
			else this->isPanning2 = true;
			chart->Cursor = Cursors::Hand;
		}
	}

	System::Void MainForm::chartTime_MouseMove(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
	{
		Chart^ chart = dynamic_cast<Chart^>(sender);
		if (chart == nullptr || chart->ChartAreas->Count == 0) return;

		bool isChart1 = (chart == this->chartTime1);
		bool isPanning = isChart1 ? this->isPanning1 : this->isPanning2;
		if (isPanning && e->Button == System::Windows::Forms::MouseButtons::Middle)
		{
			Point lastPt = isChart1 ? this->panStartPoint1 : this->panStartPoint2;
			int pdx = e->X - lastPt.X;
			int pdy = e->Y - lastPt.Y;
			if (pdx != 0 || pdy != 0)
			{
				Axis^ ax = chart->ChartAreas[0]->AxisX;
				Axis^ ay = chart->ChartAreas[0]->AxisY;
				try {
					double vX1 = ax->PixelPositionToValue(lastPt.X);
					double vX2 = ax->PixelPositionToValue(e->X);
					double dvX = vX2 - vX1;
					if (ax->ScaleView->IsZoomed && Math::Abs(dvX) > 0.0001)
					{
						double newPos = ax->ScaleView->Position - dvX;
						ax->ScaleView->Scroll(newPos);
					}
				}
				catch (...) {}
				try {
					double vY1 = ay->PixelPositionToValue(lastPt.Y);
					double vY2 = ay->PixelPositionToValue(e->Y);
					double dvY = vY2 - vY1;
					if (ay->ScaleView->IsZoomed && Math::Abs(dvY) > 0.000001)
					{
						double newPos = ay->ScaleView->Position - dvY;
						ay->ScaleView->Scroll(newPos);
					}
				}
				catch (...) {}

				if (isChart1) this->panStartPoint1 = e->Location;
				else this->panStartPoint2 = e->Location;
			}
			return;
		}

		// Подсказка по кривой под курсором: имя серии и значение ближайшей точки
		try {
			HitTestResult^ hit = chart->HitTest(e->X, e->Y);
			String^ text = nullptr;
			if (hit != nullptr && hit->Series != nullptr)
			{
				Series^ s = hit->Series;
				String^ seriesName = String::IsNullOrEmpty(s->LegendText) ? s->Name : s->LegendText;
				text = seriesName;
				if (hit->PointIndex >= 0 && hit->PointIndex < s->Points->Count)
				{
					DataPoint^ p = s->Points[hit->PointIndex];
					text = seriesName + L"   X=" + p->XValue.ToString("F0") +
						L"   Y=" + p->YValues[0].ToString("F3");
				}
			}
			if (text != this->lastChartTooltip)
			{
				this->lastChartTooltip = text;
				this->toolTipMain->SetToolTip(chart, text);
			}
		}
		catch (...) {}
	}

	System::Void MainForm::chartTime_MouseUp(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
	{
		Chart^ chart = dynamic_cast<Chart^>(sender);
		if (chart == nullptr || chart->ChartAreas->Count == 0) return;

		bool isChart1 = (chart == this->chartTime1);

		if (e->Button == System::Windows::Forms::MouseButtons::Middle)
		{
			if (isChart1) this->isPanning1 = false;
			else this->isPanning2 = false;
			chart->Cursor = Cursors::Default;
			return;
		}

		if (e->Button != System::Windows::Forms::MouseButtons::Left) return;

		Point startPt = isChart1 ? this->mouseDownPoint1 : this->mouseDownPoint2;
		int dx = Math::Abs(e->X - startPt.X);
		int dy = Math::Abs(e->Y - startPt.Y);

		// Порог 35 пикселей: любое перемещение меньше 35 px - это одиночный КЛИК визира (не сбрасывая текущий масштаб!)
		if (dx < 35 && dy < 35)
		{
			try {
				double posX = chart->ChartAreas[0]->AxisX->PixelPositionToValue(e->X);
				if (posX < 0) posX = 0;
				this->chartTime1->ChartAreas[0]->CursorX->Position = posX;
				this->chartTime2->ChartAreas[0]->CursorX->Position = posX;
				UpdateTimeSlices(posX);
				UpdateCursorReadout(posX);
			}
			catch (...) {}
		}
	}

	System::Void MainForm::buttonXMode1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->isStreaming) return;
		this->isTempXAxis1 = !this->isTempXAxis1;
		this->buttonXMode1->Text = this->isTempXAxis1 ? L"Темп." : L"Кадр";
		this->buttonXMode1->BackColor = this->isTempXAxis1 ? Color::FromArgb(255, 235, 205) : Color::FromArgb(240, 240, 245);
		if (SondeData::IsLoaded())
		{
			size_t fc = SondeData::GetFrameCount();
			this->minFrame1 = 0;
			this->maxFrame1 = static_cast<int>(fc - 1);
			if (this->isTempXAxis1)
			{
				double minT = SondeData::GetTemperature(0);
				double maxT = minT;
				for (size_t i = 1; i < fc; ++i)
				{
					double t = SondeData::GetTemperature(i);
					if (t < minT) minT = t;
					if (t > maxT) maxT = t;
				}
				this->textBoxMin1->Text = Math::Floor(minT).ToString();
				this->textBoxMax1->Text = Math::Ceiling(maxT).ToString();
			}
			else
			{
				this->textBoxMin1->Text = L"0";
				this->textBoxMax1->Text = (fc - 1).ToString();
			}
		}
		RefreshChart(1);
	}

	System::Void MainForm::buttonXMode2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->isStreaming) return;
		this->isTempXAxis2 = !this->isTempXAxis2;
		this->buttonXMode2->Text = this->isTempXAxis2 ? L"Темп." : L"Кадр";
		this->buttonXMode2->BackColor = this->isTempXAxis2 ? Color::FromArgb(255, 235, 205) : Color::FromArgb(240, 240, 245);
		if (SondeData::IsLoaded())
		{
			size_t fc = SondeData::GetFrameCount();
			this->minFrame2 = 0;
			this->maxFrame2 = static_cast<int>(fc - 1);
			if (this->isTempXAxis2)
			{
				double minT = SondeData::GetTemperature(0);
				double maxT = minT;
				for (size_t i = 1; i < fc; ++i)
				{
					double t = SondeData::GetTemperature(i);
					if (t < minT) minT = t;
					if (t > maxT) maxT = t;
				}
				this->textBoxMin2->Text = Math::Floor(minT).ToString();
				this->textBoxMax2->Text = Math::Ceiling(maxT).ToString();
			}
			else
			{
				this->textBoxMin2->Text = L"0";
				this->textBoxMax2->Text = (fc - 1).ToString();
			}
		}
		RefreshChart(2);
	}

	System::Void MainForm::comboBoxPrecision1_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e)
	{
		this->precisionY1 = this->comboBoxPrecision1->SelectedIndex;
		if (this->precisionY1 < 0) this->precisionY1 = 0;
		RenderLoadedCurves();
		double pos = this->chartTime1->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(pos) || pos < 0) pos = 0;
		UpdateCursorReadout(pos);
		SaveStationConfig();
	}

	System::Void MainForm::comboBoxPrecision2_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e)
	{
		this->precisionY2 = this->comboBoxPrecision2->SelectedIndex;
		if (this->precisionY2 < 0) this->precisionY2 = 0;
		RenderLoadedCurves();
		double pos = this->chartTime2->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(pos) || pos < 0) pos = 0;
		UpdateCursorReadout(pos);
		SaveStationConfig();
	}

	System::Void MainForm::chartTime_MouseDoubleClick(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
	{
		Chart^ chart = dynamic_cast<Chart^>(sender);
		if (chart != nullptr && chart->ChartAreas->Count > 0)
		{
			int wIdx = (chart == this->chartTime1) ? 1 : 2;
			if (wIdx == 1)
			{
				this->minFrame1 = 0;
				this->maxFrame1 = SondeData::IsLoaded() ? static_cast<int>(SondeData::GetFrameCount() - 1) : -1;
				this->textBoxMin1->Clear();
				this->textBoxMax1->Clear();
				this->chartTime1->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
				this->chartTime1->ChartAreas[0]->AxisY->ScaleView->ZoomReset(0);
				this->chartTime1->ChartAreas[0]->AxisX->Minimum = Double::NaN;
				this->chartTime1->ChartAreas[0]->AxisX->Maximum = Double::NaN;
				this->chartTime1->ChartAreas[0]->AxisY->Minimum = Double::NaN;
				this->chartTime1->ChartAreas[0]->AxisY->Maximum = Double::NaN;
			}
			else
			{
				this->minFrame2 = 0;
				this->maxFrame2 = SondeData::IsLoaded() ? static_cast<int>(SondeData::GetFrameCount() - 1) : -1;
				this->textBoxMin2->Clear();
				this->textBoxMax2->Clear();
				this->chartTime2->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
				this->chartTime2->ChartAreas[0]->AxisY->ScaleView->ZoomReset(0);
				this->chartTime2->ChartAreas[0]->AxisX->Minimum = Double::NaN;
				this->chartTime2->ChartAreas[0]->AxisX->Maximum = Double::NaN;
				this->chartTime2->ChartAreas[0]->AxisY->Minimum = Double::NaN;
				this->chartTime2->ChartAreas[0]->AxisY->Maximum = Double::NaN;
			}
			RenderLoadedCurves();
		}
	}

	System::Void MainForm::chartTime_MouseWheel(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
	{
		Chart^ chart = dynamic_cast<Chart^>(sender);
		if (chart == nullptr || chart->ChartAreas->Count == 0) return;

		Axis^ axisX = chart->ChartAreas[0]->AxisX;
		Axis^ axisY = chart->ChartAreas[0]->AxisY;

		double xMin = axisX->ScaleView->IsZoomed ? axisX->ScaleView->ViewMinimum : axisX->Minimum;
		double xMax = axisX->ScaleView->IsZoomed ? axisX->ScaleView->ViewMaximum : axisX->Maximum;
		if (Double::IsNaN(xMin) || Double::IsNaN(xMax) || xMax <= xMin)
		{
			xMin = 0.0;
			xMax = SondeData::IsLoaded() ? static_cast<double>(SondeData::GetFrameCount() - 1) : 100.0;
		}

		double yMin = axisY->ScaleView->IsZoomed ? axisY->ScaleView->ViewMinimum : axisY->Minimum;
		double yMax = axisY->ScaleView->IsZoomed ? axisY->ScaleView->ViewMaximum : axisY->Maximum;
		if (Double::IsNaN(yMin) || Double::IsNaN(yMax) || yMax <= yMin)
		{
			double sMin = Double::MaxValue, sMax = Double::MinValue;
			for (int s = 0; s < chart->Series->Count; ++s)
			{
				Series^ ser = chart->Series[s];
				if (ser->Enabled && ser->Points->Count > 0)
				{
					for (int p = 0; p < ser->Points->Count; ++p)
					{
						double y = ser->Points[p]->YValues[0];
						if (y < sMin) sMin = y;
						if (y > sMax) sMax = y;
					}
				}
			}
			if (sMin < sMax) { yMin = sMin; yMax = sMax; }
			else { yMin = -1.0; yMax = 1.0; }
		}

		// Точка фокуса под курсором
		double curX = Double::NaN;
		double curY = Double::NaN;
		try {
			curX = axisX->PixelPositionToValue(e->X);
			curY = axisY->PixelPositionToValue(e->Y);
		}
		catch (...) {}

		if (Double::IsNaN(curX) || curX < xMin || curX > xMax)
			curX = chart->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(curX) || curX < xMin || curX > xMax)
			curX = 0.5 * (xMin + xMax);

		if (Double::IsNaN(curY) || curY < yMin || curY > yMax)
			curY = chart->ChartAreas[0]->CursorY->Position;
		if (Double::IsNaN(curY) || curY < yMin || curY > yMax)
			curY = 0.5 * (yMin + yMax);

		if (e->Delta > 0)
		{
			// Приближение (Zoom In) на 25% относительно курсора
			double factor = 0.75;
			double newXMin = curX - (curX - xMin) * factor;
			double newXMax = curX + (xMax - curX) * factor;
			if (newXMax - newXMin >= 5.0)
			{
				axisX->ScaleView->Zoom(newXMin, newXMax);
			}

			double newYMin = curY - (curY - yMin) * factor;
			double newYMax = curY + (yMax - curY) * factor;
			if (Math::Abs(newYMax - newYMin) > 0.00001)
			{
				axisY->ScaleView->Zoom(newYMin, newYMax);
			}
		}
		else if (e->Delta < 0)
		{
			// Отдаление (Zoom Out) относительно курсора
			if (axisX->ScaleView->IsZoomed || axisY->ScaleView->IsZoomed)
			{
				double factor = 1.33;
				double newXMin = curX - (curX - xMin) * factor;
				double newXMax = curX + (xMax - curX) * factor;
				double maxLimX = SondeData::IsLoaded() ? static_cast<double>(SondeData::GetFrameCount() - 1) : 100.0;
				if (newXMin <= 0.0 && newXMax >= maxLimX)
				{
					axisX->ScaleView->ZoomReset(0);
					axisY->ScaleView->ZoomReset(0);
				}
				else
				{
					axisX->ScaleView->Zoom(newXMin, newXMax);
					double newYMin = curY - (curY - yMin) * factor;
					double newYMax = curY + (yMax - curY) * factor;
					axisY->ScaleView->Zoom(newYMin, newYMax);
				}
			}
			else
			{
				axisX->ScaleView->ZoomReset(0);
				axisY->ScaleView->ZoomReset(0);
			}
		}
	}

	System::Void MainForm::chartTime_AxisViewChanged(System::Object^ sender, System::Windows::Forms::DataVisualization::Charting::ViewEventArgs^ e)
	{
		Chart^ chart = dynamic_cast<Chart^>(sender);
		if (chart != nullptr && chart->ChartAreas->Count > 0)
		{
			if (e->Axis == chart->ChartAreas[0]->AxisX)
			{
				double viewMin = chart->ChartAreas[0]->AxisX->ScaleView->ViewMinimum;
				double viewMax = chart->ChartAreas[0]->AxisX->ScaleView->ViewMaximum;

				// Защита от случайного микродрага: отменяем зум, если диапазон меньше 15 кадров
				if (!Double::IsNaN(viewMin) && !Double::IsNaN(viewMax) && Math::Abs(viewMax - viewMin) < 15.0)
				{
					chart->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
					chart->ChartAreas[0]->AxisY->ScaleView->ZoomReset(0);
					return;
				}

				int wIdx = (chart == this->chartTime1) ? 1 : 2;
				TextBox^ tbMin = (wIdx == 1) ? this->textBoxMin1 : this->textBoxMin2;
				TextBox^ tbMax = (wIdx == 1) ? this->textBoxMax1 : this->textBoxMax2;

				if (!Double::IsNaN(viewMin) && !Double::IsNaN(viewMax))
				{
					tbMin->Text = Math::Round(viewMin).ToString();
					tbMax->Text = Math::Round(viewMax).ToString();
				}
			}
		}
	}

	System::Void MainForm::chartTime_CursorPositionChanged(System::Object^ sender, System::Windows::Forms::DataVisualization::Charting::CursorEventArgs^ e)
	{
		try {
			double posX = e->NewPosition;
			if (!Double::IsNaN(posX))
			{
				if (posX < 0) posX = 0;
				this->chartTime1->ChartAreas[0]->CursorX->Position = posX;
				this->chartTime2->ChartAreas[0]->CursorX->Position = posX;
				UpdateTimeSlices(posX);
				UpdateCursorReadout(posX);
			}
		}
		catch (...) {}
	}

	// Инициализация общего канала связи: загрузка EM_Tools_Conf.txt и применение параметров
	void MainForm::InitConnection()
	{
		AppConfig::Instance().Load();
		this->deviceLink = gcnew DeviceLink(this->components);
		AppConfig::Instance().ApplyTo(this->deviceLink);
		// Насос очереди кадров — работает всё время, пока идёт опрос прибора
		this->timerStream->Tick += gcnew EventHandler(this, &MainForm::timerStream_Tick);
		SyncConnectionPanel();
	}

	// Отражение текущих настроек связи в карточке «Связь» боковой панели
	void MainForm::SyncConnectionPanel()
	{
		AppConfig& cfg = AppConfig::Instance();
		if (cfg.connection_type == CONN_UDP)
		{
			this->comboBoxConnType->SelectedIndex = 0;
			this->textBoxConnParam->Text = gcnew String(cfg.wifi_ip.c_str()) + L":" + cfg.wifi_port.ToString();
		}
		else
		{
			this->comboBoxConnType->SelectedIndex = 1;
			this->textBoxConnParam->Text = gcnew String(cfg.com_name.c_str()) + L" @ " + cfg.baud_rate.ToString();
		}
	}

	System::Void MainForm::menuItemConnSettings_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ConnectionForm^ dlg = gcnew ConnectionForm();
		if (dlg->ShowDialog(this) == System::Windows::Forms::DialogResult::OK)
		{
			bool wasOpen = this->deviceLink != nullptr && this->deviceLink->IsOpen;
			AppConfig::Instance().ApplyTo(this->deviceLink);
			SyncConnectionPanel();
			if (wasOpen)
			{
				MessageBox::Show(this,
					L"Настройки сохранены. Для применения новых параметров канала\nвыполните «Отключить» и «Подключить» заново.",
					L"Настройки подключения", MessageBoxButtons::OK, MessageBoxIcon::Information);
			}
		}
		delete dlg;
	}

	System::Void MainForm::buttonConnect_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (!this->isSidebarPinned)
		{
			HideSidebar();
		}

		// Режим «Файл данных» не является живым каналом — открываем через меню Файл
		if (this->comboBoxConnType->SelectedIndex == 2)
		{
			MessageBox::Show(this,
				L"Источник «Файл данных» — это офлайн-режим.\nОткройте файл через меню «Файл → Открыть BIN/LAS...».",
				L"Подключение", MessageBoxButtons::OK, MessageBoxIcon::Information);
			return;
		}

		// Выбор в карточке панели синхронизируем с конфигом связи
		AppConfig& cfg = AppConfig::Instance();
		cfg.connection_type = (this->comboBoxConnType->SelectedIndex == 0) ? CONN_UDP : CONN_COM;
		cfg.ApplyTo(this->deviceLink);

		try
		{
			this->deviceLink->Open();
		}
		catch (Exception^ ex)
		{
			MessageBox::Show(this,
				L"Не удалось открыть канал связи:\n" + ex->Message +
				L"\n\nПроверьте настройки в меню «Связь → Настройки...».",
				L"Ошибка подключения", MessageBoxButtons::OK, MessageBoxIcon::Error);
			return;
		}

		// Верификация прибора: запрос сигнатуры (как toolStripButton_on_off в старой программе).
		// Без ответа прибора канал считается неподключённым (UDP без handshake всегда "открывается").
		uint32_t signature = 0;
		bool gotAnswer = false;
		for (int attempt = 0; attempt < 2 && !gotAnswer; attempt++)
		{
			this->deviceLink->ReadTimeout = 2000;
			gotAnswer = ToolProtocol::RequestSignature(this->deviceLink, signature);
		}
		this->deviceLink->ReadTimeout = AppConfig::Instance().read_timeout;

		if (!gotAnswer)
		{
			this->deviceLink->Close();
			this->labelConnStatus->Text = L"● Отключено";
			this->labelConnStatus->ForeColor = Color::Gray;
			this->statusLabelConnection->Text = L"● Связь: Нет ответа прибора";
			this->statusLabelConnection->ForeColor = Color::DarkRed;
			MessageBox::Show(this,
				L"Канал открыт, но прибор не ответил на запрос сигнатуры.\n"
				L"Проверьте питание прибора и параметры связи\n(«Связь → Настройки...»).",
				L"Прибор не отвечает", MessageBoxButtons::OK, MessageBoxIcon::Warning);
			return;
		}

		if (this->connectedSondeId == nullptr) this->connectedSondeId = new SondeID();
		*this->connectedSondeId = SondeID::FromSignature(signature);
		UpdateSondeScheme(true);
		this->sondeScheme->Refresh();

		this->buttonConnect->Enabled = false;
		this->buttonDisconnect->Enabled = true;
		this->menuItemConnect->Enabled = false;
		this->menuItemDisconnect->Enabled = true;

		String^ connDesc = this->deviceLink->GetConnectionDescription();
		String^ sondeName = Utf8ToManaged(this->connectedSondeId->GetTypeName().c_str());
		this->labelConnStatus->Text = L"● Подключено";
		this->labelConnStatus->ForeColor = Color::Green;
		this->statusLabelConnection->Text = L"● Связь: " + connDesc + L"  |  " + sondeName +
			L" №" + this->connectedSondeId->number.ToString();
		this->statusLabelConnection->ForeColor = Color::DarkGreen;
		this->statusLabelDevice->Text = L"Прибор: " + sondeName;
		this->statusLabelFile->Text = L"Временный файл: " + LiveFileFullPath();

		std::cout << "[COMM] Подключено: " << msclr::interop::marshal_as<std::string>(connDesc)
			<< " signature=" << signature
			<< " type=" << this->connectedSondeId->type
			<< " N_Tx=" << this->connectedSondeId->N_Tx << std::endl;

		// Шаг 2 (как toolStripButton_on_off в старой программе): чтение файла
		// метрологии из прибора и заполнение структуры сессии — до старта опроса
		{
			uint8_t cmd = 0;
			uint16_t metroLen = 0;
			uint8_t metroBuf[2048] = { 0 };
			int metroResult = 1;
			System::Threading::Thread::Sleep(300);
			{
				msclr::lock linkLock(this->deviceLink->SyncRoot);
				int prevTimeout = this->deviceLink->ReadTimeout;
				this->deviceLink->ReadTimeout = 2000;
				uint8_t d0 = 0;
				if (ToolProtocol::SendToTool(this->deviceLink, CMD_GET_METROLOGY, &d0, 0) == 0)
				{
					System::Threading::Thread::Sleep(500);
					metroResult = ToolProtocol::ReceiveFromTool(this->deviceLink, &cmd, metroBuf, &metroLen);
				}
				this->deviceLink->ReadTimeout = prevTimeout;
			}
			if (metroResult == 0 && SondeData::SetMetrology(metroBuf, metroLen))
			{
				UpdateSondeScheme(true);
				this->sondeScheme->Refresh();
				std::cout << "[METRO] Метрология прочитана при подключении: " << metroLen
					<< " байт" << std::endl;
				MessageBox::Show(this, L"ФАЙЛ МЕТРОЛОГИИ СЧИТАН ИЗ ПРИБОРА!",
					L"Метрология", MessageBoxButtons::OK, MessageBoxIcon::Information);
			}
			else
			{
				SondeData::ClearMetrology();
				std::cout << "[METRO] Метрология при подключении не прочитана: result="
					<< metroResult << " len=" << metroLen << std::endl;
				MessageBox::Show(this, L"ФАЙЛ МЕТРОЛОГИИ НЕ БЫЛ СЧИТАН !!!",
					L"Метрология", MessageBoxButtons::OK, MessageBoxIcon::Warning);
			}
		}

		// Опрос прибора и запись во временный файл — всегда, пока есть связь
		StartTelemetry();
	}

	System::Void MainForm::buttonDisconnect_Click(System::Object^ sender, System::EventArgs^ e)
	{
		StopTelemetry();
		if (this->deviceLink != nullptr)
		{
			this->deviceLink->Close();
		}

		this->buttonConnect->Enabled = true;
		this->buttonDisconnect->Enabled = false;
		this->menuItemConnect->Enabled = true;
		this->menuItemDisconnect->Enabled = false;

		this->labelConnStatus->Text = L"● Отключено";
		this->labelConnStatus->ForeColor = Color::Gray;
		this->statusLabelConnection->Text = L"● Связь: Отключено";
		this->statusLabelConnection->ForeColor = Color::DimGray;
		this->statusLabelDevice->Text = L"Прибор: —";

		std::cout << "[COMM] Соединение остановлено пользователем." << std::endl;
	}

	System::Void MainForm::buttonCalibrate_Click(System::Object^ sender, System::EventArgs^ e)
	{
		MessageBox::Show(
			L"Мастер калибровки нулей и гармоник зонда Cartograph 120.\n"
			L"Организация: НПФ «АМК Горизонт»\nРазработал: зам начальника ОП Смирнов С. Г.",
			L"Калибровка нулей",
			MessageBoxButtons::OK,
			MessageBoxIcon::Information
		);
	}

	System::Void MainForm::buttonSelfTest_Click(System::Object^ sender, System::EventArgs^ e)
	{
		MessageBox::Show(
			L"Запуск цикла экспресс-самодиагностики зонда (ТЕСТ).\n"
			L"Проверка генераторов Tx1..Tx5 и АЦП Rx_L/Rx_R...",
			L"Самодиагностика",
			MessageBoxButtons::OK,
			MessageBoxIcon::Information
		);
	}

	void MainForm::ExportCsv(int windowIndex)
	{
		if (!SondeData::IsLoaded())
		{
			MessageBox::Show(this, L"Сначала откройте файл данных.", L"Экспорт CSV", MessageBoxButtons::OK, MessageBoxIcon::Information);
			return;
		}
		SondeData::CsvSelection selection;
		if (windowIndex != 0)
		{
			TextBox^ minimum = windowIndex == 1 ? this->textBoxMin1 : this->textBoxMin2;
			TextBox^ maximum = windowIndex == 1 ? this->textBoxMax1 : this->textBoxMax2;
			double low, high;
			auto culture = System::Globalization::CultureInfo::InvariantCulture;
			auto style = System::Globalization::NumberStyles::Float;
			if (!Double::TryParse(minimum->Text->Trim()->Replace(L",", L"."), style, culture, low)
				|| !Double::TryParse(maximum->Text->Trim()->Replace(L",", L"."), style, culture, high)
				|| Double::IsNaN(low) || Double::IsInfinity(low) || Double::IsNaN(high) || Double::IsInfinity(high) || low > high)
			{
				MessageBox::Show(this, L"Введите корректные границы Min и Max.", L"Экспорт CSV", MessageBoxButtons::OK, MessageBoxIcon::Warning);
				return;
			}
			selection.enabled = true;
			selection.temperature = windowIndex == 1 ? this->isTempXAxis1 : this->isTempXAxis2;
			selection.minimum = low;
			selection.maximum = high;
			if (!selection.temperature && (low < 0 || high >= static_cast<double>(SondeData::GetFrameCount())
				|| Math::Floor(low) != low || Math::Floor(high) != high))
			{
				MessageBox::Show(this, L"Границы кадров должны быть целыми и находиться в пределах файла (0..N−1).", L"Экспорт CSV", MessageBoxButtons::OK, MessageBoxIcon::Warning);
				return;
			}
		}
		SaveFileDialog^ dialog = gcnew SaveFileDialog();
		dialog->Filter = L"CSV (*.csv)|*.csv";
		dialog->DefaultExt = L"csv";
		dialog->AddExtension = true;
		dialog->Title = selection.enabled ? L"Все поля записи в CSV: диапазон Min–Max" : L"Все поля записи в CSV: весь файл";
		dialog->FileName = System::IO::Path::GetFileNameWithoutExtension(gcnew String(SondeData::GetFileName()))
			+ L"_" + gcnew String(SondeData::GetCsvSchema()) + (selection.enabled ? L"_range.csv" : L"_all.csv");
		try
		{
			if (dialog->ShowDialog(this) != System::Windows::Forms::DialogResult::OK) return;
			this->Cursor = Cursors::WaitCursor;
			std::wstring path = msclr::interop::marshal_as<std::wstring>(dialog->FileName);
			size_t count = 0;
			char error[512] = {};
			if (!SondeData::ExportCsv(path.c_str(), selection, count, error, sizeof(error)))
			{
				MessageBox::Show(this, L"Не удалось сохранить CSV:\n" + gcnew String(error)
					+ L"\nСуществующие файлы не перезаписываются: выберите новое имя.", L"Экспорт CSV", MessageBoxButtons::OK, MessageBoxIcon::Warning);
				return;
			}
			String^ schema = gcnew String(SondeData::GetCsvSchema());
			if (schema == L"em_gp_v1")
			{
				auto answer = MessageBox::Show(this, String::Format(L"Экспорт завершен. Кадров: {0}\nСхема: {1}\n{2}\n\nЗапустить диагностику аппаратурных наводок и температурного дрейфа?",
					static_cast<UInt64>(count), schema, dialog->FileName),
					L"Экспорт CSV", MessageBoxButtons::YesNo, MessageBoxIcon::Question);
				if (answer == System::Windows::Forms::DialogResult::Yes) RunGpAnalysis(dialog->FileName);
			}
			else
			{
				MessageBox::Show(this, String::Format(L"Экспорт завершен. Кадров: {0}\nСхема: {1}\n{2}",
					static_cast<UInt64>(count), schema, dialog->FileName),
					L"Экспорт CSV", MessageBoxButtons::OK, MessageBoxIcon::Information);
			}
		}
		catch (System::Exception^ ex)
		{
			MessageBox::Show(this, ex->Message, L"Ошибка экспорта", MessageBoxButtons::OK, MessageBoxIcon::Error);
		}
		finally
		{
			this->Cursor = Cursors::Default;
			delete dialog;
		}
	}

	System::Void MainForm::menuExportCsv_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ToolStripMenuItem^ item = safe_cast<ToolStripMenuItem^>(sender);
		ExportCsv(safe_cast<int>(item->Tag));
	}

	System::Void MainForm::buttonExportCsv_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ExportCsv(0);
	}

	String^ MainForm::FindPythonExe()
	{
		String^ appDir = System::IO::Path::GetDirectoryName(System::Windows::Forms::Application::ExecutablePath);
		String^ localPy = System::IO::Path::Combine(appDir, L"python_runtime\\python.exe");
		if (!System::IO::File::Exists(localPy)) return nullptr;
		try
		{
			System::Diagnostics::ProcessStartInfo^ probe = gcnew System::Diagnostics::ProcessStartInfo();
			probe->FileName = localPy;
			probe->Arguments = L"-c \"import tkinter, sys; sys.exit(0)\"";
			probe->UseShellExecute = false;
			probe->CreateNoWindow = true;
			System::Diagnostics::Process^ check = System::Diagnostics::Process::Start(probe);
			if (check->WaitForExit(5000) && check->ExitCode == 0) return localPy;
		}
		catch (...) {}
		return nullptr;
	}

	void MainForm::RunGpAnalysis(String^ csvPath)
	{
		try
		{
			String^ appDir = System::IO::Path::GetDirectoryName(System::Windows::Forms::Application::ExecutablePath);
			String^ scriptPath = System::IO::Path::Combine(appDir, L"scripts\\gp_temperature_analysis.py");
			if (!System::IO::File::Exists(scriptPath))
			{
				String^ candidate = System::IO::Path::GetFullPath(System::IO::Path::Combine(appDir, L"..\\..\\scripts\\gp_temperature_analysis.py"));
				if (System::IO::File::Exists(candidate)) scriptPath = candidate;
			}
			if (!System::IO::File::Exists(scriptPath))
			{
				MessageBox::Show(this, L"Не найден скрипт диагностики scripts\\gp_temperature_analysis.py.",
					L"Диагностика GP", MessageBoxButtons::OK, MessageBoxIcon::Warning);
				return;
			}
			String^ pythonExe = FindPythonExe();
			if (String::IsNullOrEmpty(pythonExe))
			{
				MessageBox::Show(this, L"Среда Python не установлена.\nУстановите её через «Помощь → Python: установить / обновить среду…».",
					L"Диагностика GP", MessageBoxButtons::OK, MessageBoxIcon::Information);
				return;
			}
			System::Diagnostics::ProcessStartInfo^ psi = gcnew System::Diagnostics::ProcessStartInfo();
			psi->FileName = pythonExe;
			psi->Arguments = String::Format(L"\"{0}\" --file \"{1}\" --open-folder", scriptPath, csvPath);
			psi->UseShellExecute = true;
			psi->CreateNoWindow = false;
			System::Diagnostics::Process::Start(psi);
		}
		catch (System::Exception^ ex)
		{
			MessageBox::Show(this, L"Не удалось запустить скрипт диагностики:\n" + ex->Message,
				L"Диагностика GP", MessageBoxButtons::OK, MessageBoxIcon::Warning);
		}
	}

	System::Void MainForm::menuItemGpAnalysis_Click(System::Object^ sender, System::EventArgs^ e)
	{
		OpenFileDialog^ dialog = gcnew OpenFileDialog();
		dialog->Filter = L"CSV (*.csv)|*.csv";
		dialog->Title = L"Диагностика наводок и дрейфа: выбор GP CSV";
		try
		{
			if (dialog->ShowDialog(this) == System::Windows::Forms::DialogResult::OK) RunGpAnalysis(dialog->FileName);
		}
		finally { delete dialog; }
	}

	System::Void MainForm::menuItemCalibMaster_Click(System::Object^ sender, System::EventArgs^ e)
	{
		try
		{
			String^ appDir = System::IO::Path::GetDirectoryName(System::Windows::Forms::Application::ExecutablePath);
			String^ scriptsDir = System::IO::Path::Combine(appDir, L"scripts");
			if (!System::IO::Directory::Exists(scriptsDir))
			{
				String^ candidate = System::IO::Path::GetFullPath(System::IO::Path::Combine(appDir, L"..\\..\\scripts"));
				if (System::IO::Directory::Exists(candidate)) scriptsDir = candidate;
			}
			String^ launcher = System::IO::Path::Combine(scriptsDir, L"calibration_master.py");
			if (!System::IO::File::Exists(launcher)
				|| !System::IO::File::Exists(System::IO::Path::Combine(scriptsDir, L"cartograf_autocalib\\gui.py")))
			{
				MessageBox::Show(this, L"Не найден пакет scripts\\cartograf_autocalib или лаунчер calibration_master.py.",
					L"Мастер калибровки", MessageBoxButtons::OK, MessageBoxIcon::Warning);
				return;
			}
			String^ pythonExe = FindPythonExe();
			if (String::IsNullOrEmpty(pythonExe))
			{
				MessageBox::Show(this, L"Среда Python не установлена.\nУстановите её через «Помощь → Python: установить / обновить среду…».",
					L"Мастер калибровки", MessageBoxButtons::OK, MessageBoxIcon::Information);
				return;
			}
			String^ pythonW = System::IO::Path::Combine(System::IO::Path::GetDirectoryName(pythonExe), L"pythonw.exe");
			if (System::IO::File::Exists(pythonW)) pythonExe = pythonW;
			System::Diagnostics::ProcessStartInfo^ psi = gcnew System::Diagnostics::ProcessStartInfo();
			psi->FileName = pythonExe;
			psi->Arguments = String::Format(L"\"{0}\"", launcher);
			psi->WorkingDirectory = scriptsDir;
			psi->UseShellExecute = true;
			System::Diagnostics::Process::Start(psi);
		}
		catch (System::Exception^ ex)
		{
			MessageBox::Show(this, L"Не удалось запустить мастер калибровки:\n" + ex->Message,
				L"Мастер калибровки", MessageBoxButtons::OK, MessageBoxIcon::Warning);
		}
	}

	System::Void MainForm::menuItemMetroParams_Click(System::Object^ sender, System::EventArgs^ e)
	{
		MetroForm^ metroForm = gcnew MetroForm(this->deviceLink);
		// Сигнатура из прочитанной метрологии приоритетнее сессионной
		if (!SondeData::HasMetrology())
		{
			uint32_t sig = SondeData::GetSignature();
			if (sig != 0)
				metroForm->ApplySondeSignature(sig);
		}
		metroForm->ShowDialog(this);
		if (SondeData::IsLoaded()) RenderLoadedCurves();
	}

	System::Void MainForm::menuItemOpenBin_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->openFileDialogBin->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			LoadDataFile(this->openFileDialogBin->FileName);
		}
	}

	System::Void MainForm::menuItemOpenTemp_Click(System::Object^ sender, System::EventArgs^ e)
	{
		// Открыть временный файл телеметрии напрямую (аналог открытия Dataout в старой программе)
		String^ path = (this->liveFilePath != nullptr && this->liveFilePath->Length > 0)
			? this->liveFilePath
			: LiveFileFullPath();
		if (!System::IO::File::Exists(path))
		{
			MessageBox::Show(this,
				L"Временный файл ещё не создан.\n" + path,
				L"Нет данных", MessageBoxButtons::OK, MessageBoxIcon::Information);
			return;
		}
		LoadDataFile(path);
	}

	System::Void MainForm::menuItemClearTemp_Click(System::Object^ sender, System::EventArgs^ e)
	{
		// Сброс временного файла телеметрии (аналог «delete» в старой программе).
		// Опрос прибора продолжается — запись возобновляется в новый пустой файл.
		StopDisplay();
		cli::array<Byte>^ skip;
		while (this->frameQueue != nullptr && this->frameQueue->TryDequeue(skip)) {}
		SondeData::BeginLiveSession();
		this->isLiveSession = true;
		for (int s = 0; s < this->chartTime1->Series->Count; s++)
			this->chartTime1->Series[s]->Points->Clear();
		for (int s = 0; s < this->chartTime2->Series->Count; s++)
			this->chartTime2->Series[s]->Points->Clear();
		if (this->liveFout != nullptr && this->liveFout->is_open())
		{
			this->liveFout->close();
		}
		String^ path = (this->liveFilePath != nullptr && this->liveFilePath->Length > 0)
			? this->liveFilePath
			: LiveFileFullPath();
		if (!System::IO::File::Exists(path))
		{
			this->statusLabelFile->Text = L"Временный файл: " + path + L" (пуст)";
			return;
		}
		if (MessageBox::Show(this,
				L"Удалить временный файл записи телеметрии?\n" + path,
				L"Очистка временного файла",
				MessageBoxButtons::YesNo, MessageBoxIcon::Question) == System::Windows::Forms::DialogResult::Yes)
		{
			try { System::IO::File::Delete(path); }
			catch (Exception^ ex)
			{
				MessageBox::Show(this, L"Не удалось удалить файл: " + ex->Message,
					L"Ошибка", MessageBoxButtons::OK, MessageBoxIcon::Warning);
			}
			if (!System::IO::File::Exists(path))
			{
				if (this->liveFout != nullptr && this->liveFout->is_open() == false &&
					this->liveFilePath == path)
				{
					std::string pn = msclr::interop::marshal_as<std::string>(path);
					this->liveFout->open(pn, std::ios::binary | std::ios::app);
				}
				this->statusLabelFile->Text = L"Временный файл: " + path + L" (очищен)";
			}
		}
	}

	System::Void MainForm::menuItemExit_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->Close();
	}

	System::Void MainForm::menuItemAbout_Click(System::Object^ sender, System::EventArgs^ e)
	{
		MessageBox::Show(
			L"Electromagnetic Tools 3.0\n\n"
			L"Флагманская наземная станция оператора для приборов электромагнитного каротажа LWD/Cartograph.\n\n"
			L"Организация: НПФ «АМК Горизонт»\n"
			L"Разработал: зам начальника ОП Смирнов С. Г.\n"
			L"Версия: " + System::Reflection::Assembly::GetExecutingAssembly()->GetName()->Version->ToString()
				+ L" (Visual Studio 2026, MSVC v145, x64, .NET 4.8)",
			L"О программе Electromagnetic Tools 3.0",
			MessageBoxButtons::OK,
			MessageBoxIcon::Information
		);
	}

	System::Void MainForm::buttonStream_Click(System::Object^ sender, System::EventArgs^ e)
	{
		// ПОТОК = приём телеметрии с прибора по общему каналу (как Timer_get_data_from_sonde)
		if (this->deviceLink == nullptr || !this->deviceLink->IsOpen)
		{
			MessageBox::Show(this,
				L"Телеметрия требует активного подключения к прибору.\nСначала выполните «Связь → Подключить».",
				L"Нет связи", MessageBoxButtons::OK, MessageBoxIcon::Information);
			return;
		}

		if (!this->isStreaming)
		{
			StartDisplay();
		}
		else
		{
			StopDisplay();
		}
	}

	System::Void MainForm::buttonRefresh_Click(System::Object^ sender, System::EventArgs^ e)
	{
		// 1. Выключить окно потока — опрос и запись продолжаются (как в старой программе)
		StopDisplay();

		// 2. Весь записанный файл целиком (min=0, max=конец)
		if (SondeData::IsLoaded())
		{
			size_t fc = SondeData::GetFrameCount();
			this->minFrame1 = 0;
			this->maxFrame1 = (int)fc - 1;
			this->minFrame2 = 0;
			this->maxFrame2 = (int)fc - 1;
			this->textBoxMin1->Text = L"0";
			this->textBoxMax1->Text = (fc > 0 ? fc - 1 : 0).ToString();
			this->textBoxMin2->Text = L"0";
			this->textBoxMax2->Text = (fc > 0 ? fc - 1 : 0).ToString();
			RenderLoadedCurves();
			UpdateTimeSlices((double)(fc > 0 ? fc - 1 : 0));
		}
		else
		{
			ApplyRange(1);
			ApplyRange(2);
		}

		// 3. Авто-масштаб оси Y
		this->chartTime1->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime1->ChartAreas[0]->AxisY->Maximum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisY->Maximum = Double::NaN;
	}

	// --- Онлайн-телеметрия ---------------------------------------------------

	String^ MainForm::LiveFileBase()
	{
		String^ fileBase = L"Dataout.bin";
		if (this->connectedSondeId != nullptr)
		{
			switch (this->connectedSondeId->type)
			{
			case CARTOGRAPH_: case CARTOGRAPH_RAW_: fileBase = L"Dataout_cartograf.bin"; break;
			case CARTOGRAPH_LWD_4Tx_:                fileBase = L"Dataout_CARTOGRAPH_LWD_4Tx.bin"; break;
			case LWD_4Tx_NEW_:                      fileBase = L"Dataout_LWD_4Tx_NEW.bin"; break;
			case LWD_3Tx_:                          fileBase = L"Dataout_LWD_3Tx.bin"; break;
			case AUTONOM_5Tx_:                      fileBase = L"Dataout_AUTONOM_5Tx.bin"; break;
			case AUTONOM_5Tx_SDR_:                  fileBase = L"Dataout_AUTONOM_5Tx_SDR.bin"; break;
			}
		}
		return fileBase;
	}

	String^ MainForm::LiveFileFullPath()
	{
		String^ dir = gcnew String(AppConfig::Instance().data_folder.c_str());
		if (!System::IO::Directory::Exists(dir))
		{
			try { System::IO::Directory::CreateDirectory(dir); }
			catch (...) { dir = System::IO::Path::GetDirectoryName(Application::ExecutablePath); }
		}
		return dir + L"\\" + LiveFileBase();
	}

	// Опрос прибора + запись во временный файл — запускается по «Подключить»,
	// идёт всегда, пока есть связь (аналог Timer_get_data_from_sonde в старой программе)
	void MainForm::StartTelemetry()
	{
		if (this->telemetryRunning) return;

		// Сессия не очищается: накопленные кадры живут, файл дописывается.
		// Очистка — только если в памяти не онлайн-данные (открытый файл/пусто).
		if (!this->isLiveSession)
		{
			SondeData::BeginLiveSession();
			this->isLiveSession = true;

			for (int s = 0; s < this->chartTime1->Series->Count; s++)
				this->chartTime1->Series[s]->Points->Clear();
			for (int s = 0; s < this->chartTime2->Series->Count; s++)
				this->chartTime2->Series[s]->Points->Clear();
			this->minFrame1 = 0; this->maxFrame1 = -1;
			this->minFrame2 = 0; this->maxFrame2 = -1;
		}

		// Файл записи телеметрии — фиксированное имя по типу прибора, append
		this->liveFileName = LiveFileBase();
		this->liveFilePath = LiveFileFullPath();
		if (this->liveFout == nullptr)
		{
			msclr::interop::marshal_context mctx;
			std::string livePathNative = mctx.marshal_as<std::string>(this->liveFilePath);
			this->liveFout = new std::ofstream(livePathNative, std::ios::binary | std::ios::app);
			std::cout << "[COMM] Запись телеметрии в " << livePathNative << std::endl;
		}

		this->telemetryRx = 0; this->telemetryTimeouts = 0; this->telemetryCrcErr = 0;
		this->telemetryRunning = true;

		this->telemetryThread = gcnew System::Threading::Thread(
			gcnew System::Threading::ThreadStart(this, &MainForm::TelemetryWorker));
		this->telemetryThread->IsBackground = true;
		this->telemetryThread->Start();

		this->timerStream->Interval = 250;
		this->timerStream->Start();
		this->statusLabelFile->Text = L"ОНЛАЙН → " + this->liveFileName;
	}

	// Остановка опроса+записи — только по «Отключить»/закрытию программы
	void MainForm::StopTelemetry()
	{
		this->telemetryRunning = false;
		if (this->telemetryThread != nullptr && this->telemetryThread->IsAlive)
		{
			this->telemetryThread->Join(4000);
		}
		this->telemetryThread = nullptr;
		this->timerStream->Stop();

		// Добрать остаток очереди
		DrainFrameQueue();

		if (this->liveFout != nullptr)
		{
			this->liveFout->close();
			delete this->liveFout;
			this->liveFout = nullptr;
		}

		StopDisplay();
		std::cout << "[COMM] Опрос прибора остановлен. Принято кадров: " << this->telemetryRx << std::endl;
	}

	// «ПОТОК» вкл — только отображение окна последних points кадров
	void MainForm::StartDisplay()
	{
		this->isStreaming = true;
		this->isTempXAxis1 = false;
		this->isTempXAxis2 = false;
		this->buttonXMode1->Text = L"Кадр";
		this->buttonXMode2->Text = L"Кадр";
		this->buttonXMode1->BackColor = Color::FromArgb(240, 240, 245);
		this->buttonXMode2->BackColor = Color::FromArgb(240, 240, 245);
		for each (Chart^ chart in gcnew cli::array<Chart^> { this->chartTime1, this->chartTime2 })
		{
			Axis^ axis = chart->ChartAreas[0]->AxisX;
			axis->ScaleView->ZoomReset(0);
			axis->Minimum = Double::NaN;
			axis->Maximum = Double::NaN;
			axis->Title = L"Кадр (N)";
		}
		UpdateChartHeaderControls();
		this->buttonStream->BackColor = Color::FromArgb(255, 192, 192);
		this->buttonStream->Text = L"ПОТОК [ВКЛ]";

		// Показать последние points из уже накопленных кадров
		if (SondeData::IsLoaded())
		{
			size_t fc = SondeData::GetFrameCount();
			int win = (int)AppConfig::Instance().points;
			int maxI = (int)fc - 1;
			int minW = Math::Max(0, maxI - win + 1);
			this->minFrame1 = minW; this->maxFrame1 = maxI;
			this->minFrame2 = minW; this->maxFrame2 = maxI;
			this->textBoxMin1->Text = minW.ToString();
			this->textBoxMax1->Text = maxI.ToString();
			this->textBoxMin2->Text = minW.ToString();
			this->textBoxMax2->Text = maxI.ToString();
			RenderLoadedCurves();
			this->chartTime1->ChartAreas[0]->AxisX->Minimum = minW;
			this->chartTime1->ChartAreas[0]->AxisX->Maximum = Math::Max(minW + 1, maxI);
			this->chartTime2->ChartAreas[0]->AxisX->Minimum = minW;
			this->chartTime2->ChartAreas[0]->AxisX->Maximum = Math::Max(minW + 1, maxI);
		}
	}

	// «ПОТОК» выкл — графики замирают, опрос и запись продолжаются
	void MainForm::StopDisplay()
	{
		this->isStreaming = false;
		this->buttonStream->BackColor = SystemColors::Control;
		this->buttonStream->Text = L"ПОТОК";
		UpdateChartHeaderControls();
	}

	// Фоновый поток: циклический опрос прибора CMD_GET_DATA (как timer_get_data_from_sonde)
	void MainForm::TelemetryWorker()
	{
		uint8_t d0 = 0;
		const int bufSize = 4096;
		uint8_t dataIn[bufSize] = { 0 };

		// Опрос идёт всегда, пока открыт канал — даже если прибор молчит/вышел из строя.
		// Период запросов = get_data_interval из настроек (как Timer_get_data_from_sonde).
		while (this->telemetryRunning && this->deviceLink != nullptr && this->deviceLink->IsOpen)
		{
			int result;
			uint16_t incomeLen = 0;
			try
			{
				// Обмен целиком под замком: сервисные команды (метрология и пр.)
				// по тому же каналу ждут конца текущего цикла, а не рвут пакет
				msclr::lock linkLock(this->deviceLink->SyncRoot);
				if (ToolProtocol::SendToTool(this->deviceLink, CMD_GET_DATA, &d0, 0) != 0)
				{
					this->telemetryTimeouts++;
					continue;
				}
				System::Threading::Thread::Sleep(300); // ожидание ответа, как Sleep(300..1000) в старой

				uint8_t cmd = 0;
				result = ToolProtocol::ReceiveFromTool(this->deviceLink, &cmd, dataIn, &incomeLen);
			}
			catch (...)
			{
				// Поток не должен умирать молча: любой сбой канала — таймаут, опрос продолжается
				this->telemetryTimeouts++;
				System::Threading::Thread::Sleep((int)AppConfig::Instance().get_data_interval);
				continue;
			}

			{
				if (result == 0 && incomeLen > 0)
				{
					cli::array<Byte>^ frame = gcnew cli::array<Byte>(incomeLen);
					for (int i = 0; i < incomeLen; i++) frame[i] = dataIn[i];
					this->frameQueue->Enqueue(frame);
					this->telemetryRx++;
				}
				else if (result == 2)
				{
					this->telemetryCrcErr++;
				}
				else
				{
					this->telemetryTimeouts++;
				}
			}
			System::Threading::Thread::Sleep((int)AppConfig::Instance().get_data_interval);
		}
	}

	// UI-насос: разбирает очередь принятых кадров.
	// Разбор и запись в файл — всегда; отрисовка — только в режиме «ПОТОК».
	void MainForm::DrainFrameQueue()
	{
		cli::array<Byte>^ frame;
		while (this->frameQueue != nullptr && this->frameQueue->TryDequeue(frame))
		{
			pin_ptr<Byte> p = &frame[0];
			size_t added = SondeData::AppendLiveRecord(p, frame->Length);

			if (added > 0)
			{
				// Запись полного кадра в BIN-файл — всегда
				const uint8_t* recData = nullptr;
				size_t recLen = 0;
				if (this->liveFout != nullptr && this->liveFout->is_open()
					&& SondeData::GetLastRecordBytes(&recData, &recLen) && recData != nullptr)
				{
					this->liveFout->write(reinterpret_cast<const char*>(recData), recLen);
					this->liveFout->flush();
				}

				size_t lastIdx = SondeData::GetFrameCount() - 1;
				size_t fc = SondeData::GetFrameCount();
				String^ sondeName = Utf8ToManaged(SondeData::GetSondeName());
				this->statusLabelFile->Text = L"ОНЛАЙН → " + this->liveFileName + L" | " + sondeName + L" | Кадров: " + fc.ToString();

				if (this->isStreaming)
				{
					AppendFrameToCharts(lastIdx);
					UpdateTimeSlices(this->isTempXAxis1 ? SondeData::GetTemperature(lastIdx) : (double)lastIdx);
				}
			}
		}

		// Диагностика в статусбаре — пока идёт опрос
		if (this->telemetryRunning)
		{
			this->statusLabelConnection->Text = L"● Связь: " +
				this->deviceLink->GetConnectionDescription() +
				L"  |  Кадров: " + this->telemetryRx.ToString() +
				L"  Таймауты: " + this->telemetryTimeouts.ToString() +
				L"  CRC: " + this->telemetryCrcErr.ToString();
		}
	}

	// Добавить одну точку кадра на оба графика (аналог тела цикла RenderLoadedCurves)
	void MainForm::AppendFrameToCharts(size_t i)
	{
		if (!SondeData::IsLoaded()) return;
		int txCount = SondeData::GetTxCount();
		int freq1 = (this->buttonFreq1->Text->Contains(L"2000")) ? 1 : 0;
		int freq2 = (this->buttonFreq2->Text->Contains(L"2000")) ? 1 : 0;
		int curveType1 = GetCurveTypeByName(this->activeCurve1);
		int curveType2 = GetCurveTypeByName(this->activeCurve2);

		double x1 = this->isTempXAxis1 ? SondeData::GetTemperature(i) : (double)i;
		if (!String::IsNullOrEmpty(this->activeCurve1))
			for (int tx = 0; tx < txCount && tx < this->chartTime1->Series->Count; ++tx)
				AddCurvePoint(this->chartTime1->Series[tx], x1, SondeData::GetCurveValue(curveType1, freq1, tx, i, this->receiverIndex1));

		double x2 = this->isTempXAxis2 ? SondeData::GetTemperature(i) : (double)i;
		if (!String::IsNullOrEmpty(this->activeCurve2))
			for (int tx = 0; tx < txCount && tx < this->chartTime2->Series->Count; ++tx)
				AddCurvePoint(this->chartTime2->Series[tx], x2, SondeData::GetCurveValue(curveType2, freq2, tx, i, this->receiverIndex2));

		// Скользящее окно последних Points кадров (как Points в старой программе)
		int win = (int)AppConfig::Instance().points;
		for (int s = 0; s < this->chartTime1->Series->Count; s++)
			while (this->chartTime1->Series[s]->Points->Count > win)
				this->chartTime1->Series[s]->Points->RemoveAt(0);
		for (int s = 0; s < this->chartTime2->Series->Count; s++)
			while (this->chartTime2->Series[s]->Points->Count > win)
				this->chartTime2->Series[s]->Points->RemoveAt(0);

		int minW = Math::Max(0, (int)i - win + 1);
		this->minFrame1 = minW;
		this->minFrame2 = minW;
		this->maxFrame1 = (int)i;
		this->maxFrame2 = (int)i;
		this->textBoxMin1->Text = minW.ToString();
		this->textBoxMax1->Text = i.ToString();
		this->textBoxMin2->Text = minW.ToString();
		this->textBoxMax2->Text = i.ToString();

		// Ось X следует за окном потока — данные движутся налево (как в старой программе)
		this->chartTime1->ChartAreas[0]->AxisX->Minimum = minW;
		this->chartTime1->ChartAreas[0]->AxisX->Maximum = Math::Max((double)minW + 1, (double)i);
		this->chartTime2->ChartAreas[0]->AxisX->Minimum = minW;
		this->chartTime2->ChartAreas[0]->AxisX->Maximum = Math::Max((double)minW + 1, (double)i);

		this->chartTime1->Invalidate();
		this->chartTime2->Invalidate();
	}

	System::Void MainForm::timerStream_Tick(System::Object^ sender, System::EventArgs^ e)
	{
		DrainFrameQueue();
	}

	System::Void MainForm::buttonApplyRange1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ApplyRange(1);
	}

	System::Void MainForm::buttonResetRange1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (SondeData::IsLoaded())
		{
			size_t fc = SondeData::GetFrameCount();
			this->minFrame1 = 0;
			this->maxFrame1 = static_cast<int>(fc - 1);

			if (this->isTempXAxis1)
			{
				double minT = SondeData::GetTemperature(0);
				double maxT = minT;
				for (size_t i = 1; i < fc; ++i)
				{
					double t = SondeData::GetTemperature(i);
					if (t < minT) minT = t;
					if (t > maxT) maxT = t;
				}
				this->textBoxMin1->Text = Math::Floor(minT).ToString();
				this->textBoxMax1->Text = Math::Ceiling(maxT).ToString();
			}
			else
			{
				this->textBoxMin1->Text = L"0";
				this->textBoxMax1->Text = (fc - 1).ToString();
			}

			this->chartTime1->ChartAreas[0]->AxisX->Minimum = Double::NaN;
			this->chartTime1->ChartAreas[0]->AxisX->Maximum = Double::NaN;
			this->chartTime1->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
			this->chartTime1->ChartAreas[0]->AxisY->ScaleView->ZoomReset(0);
			RenderLoadedCurves();
		}
		else
		{
			this->textBoxMin1->Clear();
			this->textBoxMax1->Clear();
			ApplyRange(1);
		}
	}

	System::Void MainForm::buttonApplyRange2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ApplyRange(2);
	}

	System::Void MainForm::buttonResetRange2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (SondeData::IsLoaded())
		{
			size_t fc = SondeData::GetFrameCount();
			this->minFrame2 = 0;
			this->maxFrame2 = static_cast<int>(fc - 1);

			if (this->isTempXAxis2)
			{
				double minT = SondeData::GetTemperature(0);
				double maxT = minT;
				for (size_t i = 1; i < fc; ++i)
				{
					double t = SondeData::GetTemperature(i);
					if (t < minT) minT = t;
					if (t > maxT) maxT = t;
				}
				this->textBoxMin2->Text = Math::Floor(minT).ToString();
				this->textBoxMax2->Text = Math::Ceiling(maxT).ToString();
			}
			else
			{
				this->textBoxMin2->Text = L"0";
				this->textBoxMax2->Text = (fc - 1).ToString();
			}

			this->chartTime2->ChartAreas[0]->AxisX->Minimum = Double::NaN;
			this->chartTime2->ChartAreas[0]->AxisX->Maximum = Double::NaN;
			this->chartTime2->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
			this->chartTime2->ChartAreas[0]->AxisY->ScaleView->ZoomReset(0);
			RenderLoadedCurves();
		}
		else
		{
			this->textBoxMin2->Clear();
			this->textBoxMax2->Clear();
			ApplyRange(2);
		}
	}

	System::Void MainForm::textBoxRange_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e)
	{
		if (!Char::IsDigit(e->KeyChar) && !Char::IsControl(e->KeyChar) && e->KeyChar != '-' && e->KeyChar != '.' && e->KeyChar != ',')
		{
			e->Handled = true;
		}
	}

	System::Void MainForm::textBoxRange1_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e)
	{
		if (e->KeyCode == System::Windows::Forms::Keys::Enter)
		{
			e->SuppressKeyPress = true;
			ApplyRange(1);
		}
	}

	System::Void MainForm::textBoxRange2_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e)
	{
		if (e->KeyCode == System::Windows::Forms::Keys::Enter)
		{
			e->SuppressKeyPress = true;
			ApplyRange(2);
		}
	}

	System::Void MainForm::menuItemSetMin1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		double curPos = this->chartTime1->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(curPos) || curPos < 0) curPos = 0;
		if (this->isTempXAxis1)
		{
			this->textBoxMin1->Text = Math::Round(curPos, 1).ToString();
		}
		else
		{
			uint32_t frameNum = static_cast<uint32_t>(Math::Round(curPos));
			this->textBoxMin1->Text = frameNum.ToString();
		}
	}

	System::Void MainForm::menuItemSetMax1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		double curPos = this->chartTime1->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(curPos) || curPos < 0) curPos = 0;
		if (this->isTempXAxis1)
		{
			this->textBoxMax1->Text = Math::Round(curPos, 1).ToString();
		}
		else
		{
			uint32_t frameNum = static_cast<uint32_t>(Math::Round(curPos));
			this->textBoxMax1->Text = frameNum.ToString();
		}
	}

	System::Void MainForm::menuItemResetRange1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		buttonResetRange1_Click(sender, e);
	}

	System::Void MainForm::menuItemSetMin2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		double curPos = this->chartTime2->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(curPos) || curPos < 0) curPos = 0;
		if (this->isTempXAxis2)
		{
			this->textBoxMin2->Text = Math::Round(curPos, 1).ToString();
		}
		else
		{
			uint32_t frameNum = static_cast<uint32_t>(Math::Round(curPos));
			this->textBoxMin2->Text = frameNum.ToString();
		}
	}

	System::Void MainForm::menuItemSetMax2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		double curPos = this->chartTime2->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(curPos) || curPos < 0) curPos = 0;
		if (this->isTempXAxis2)
		{
			this->textBoxMax2->Text = Math::Round(curPos, 1).ToString();
		}
		else
		{
			uint32_t frameNum = static_cast<uint32_t>(Math::Round(curPos));
			this->textBoxMax2->Text = frameNum.ToString();
		}
	}

	System::Void MainForm::menuItemResetRange2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		buttonResetRange2_Click(sender, e);
	}

	System::Void MainForm::buttonZoomReset1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->chartTime1->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
		this->chartTime1->ChartAreas[0]->AxisY->ScaleView->ZoomReset(0);
		this->chartTime1->ChartAreas[0]->AxisX->Minimum = Double::NaN;
		this->chartTime1->ChartAreas[0]->AxisX->Maximum = Double::NaN;
		this->chartTime1->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime1->ChartAreas[0]->AxisY->Maximum = Double::NaN;
		if (SondeData::IsLoaded())
		{
			size_t fc = SondeData::GetFrameCount();
			this->minFrame1 = 0;
			this->maxFrame1 = static_cast<int>(fc - 1);
			if (this->isTempXAxis1)
			{
				double minT = SondeData::GetTemperature(0), maxT = minT;
				for (size_t i = 1; i < fc; ++i) {
					double t = SondeData::GetTemperature(i);
					if (t < minT) minT = t; if (t > maxT) maxT = t;
				}
				this->textBoxMin1->Text = Math::Floor(minT).ToString();
				this->textBoxMax1->Text = Math::Ceiling(maxT).ToString();
			}
			else
			{
				this->textBoxMin1->Text = L"0";
				this->textBoxMax1->Text = (fc - 1).ToString();
			}
			RenderLoadedCurves();
		}
	}

	System::Void MainForm::buttonZoomReset2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->chartTime2->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
		this->chartTime2->ChartAreas[0]->AxisY->ScaleView->ZoomReset(0);
		this->chartTime2->ChartAreas[0]->AxisX->Minimum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisX->Maximum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisY->Maximum = Double::NaN;
		if (SondeData::IsLoaded())
		{
			size_t fc = SondeData::GetFrameCount();
			this->minFrame2 = 0;
			this->maxFrame2 = static_cast<int>(fc - 1);
			if (this->isTempXAxis2)
			{
				double minT = SondeData::GetTemperature(0), maxT = minT;
				for (size_t i = 1; i < fc; ++i) {
					double t = SondeData::GetTemperature(i);
					if (t < minT) minT = t; if (t > maxT) maxT = t;
				}
				this->textBoxMin2->Text = Math::Floor(minT).ToString();
				this->textBoxMax2->Text = Math::Ceiling(maxT).ToString();
			}
			else
			{
				this->textBoxMin2->Text = L"0";
				this->textBoxMax2->Text = (fc - 1).ToString();
			}
			RenderLoadedCurves();
		}
	}

	System::Void MainForm::menuItemZoomBackX1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->chartTime1->ChartAreas[0]->AxisX->ScaleView->ZoomReset(1);
	}

	System::Void MainForm::menuItemZoomBackY1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->chartTime1->ChartAreas[0]->AxisY->ScaleView->ZoomReset(1);
	}

	System::Void MainForm::menuItemZoomBackX2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->chartTime2->ChartAreas[0]->AxisX->ScaleView->ZoomReset(1);
	}

	System::Void MainForm::menuItemZoomBackY2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->chartTime2->ChartAreas[0]->AxisY->ScaleView->ZoomReset(1);
	}

	void MainForm::ToggleSidebar()
	{
		if (this->panelLeft != nullptr && this->panelLeft->Visible)
		{
			HideSidebar();
		}
		else
		{
			ShowSidebar();
		}
	}

	void MainForm::ShowSidebar()
	{
		if (this->panelLeft != nullptr && this->tabControlMain != nullptr)
		{
			this->panelLeft->Top = this->tabControlMain->Top;
			this->panelLeft->Height = this->tabControlMain->Height;
			this->panelLeft->Visible = true;
			this->panelLeft->BringToFront();
		}
		this->buttonToggleSidebar->Text = L"✕ Закрыть";
		this->buttonToggleSidebar->BackColor = Color::FromArgb(255, 230, 230);
		this->buttonToggleSidebar->ForeColor = Color::FromArgb(160, 30, 30);
	}

	void MainForm::HideSidebar()
	{
		if (this->panelLeft != nullptr)
		{
			this->panelLeft->Visible = false;
		}
		this->buttonToggleSidebar->Text = L"☰ Параметры";
		this->buttonToggleSidebar->BackColor = Color::FromArgb(225, 233, 245);
		this->buttonToggleSidebar->ForeColor = Color::FromArgb(20, 50, 100);
	}

	System::Void MainForm::buttonToggleSidebar_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ToggleSidebar();
	}

	System::Void MainForm::buttonPinSidebar_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->isSidebarPinned = !this->isSidebarPinned;
		if (this->isSidebarPinned)
		{
			this->buttonPinSidebar->Text = L"📌 Закреп.";
			this->buttonPinSidebar->BackColor = Color::FromArgb(30, 90, 160);
			this->buttonPinSidebar->ForeColor = Color::FromArgb(255, 220, 100);
		}
		else
		{
			this->buttonPinSidebar->Text = L"📌 Авто";
			this->buttonPinSidebar->BackColor = Color::FromArgb(45, 58, 72);
			this->buttonPinSidebar->ForeColor = Color::White;
		}
	}

	System::Void MainForm::buttonCloseSidebar_Click(System::Object^ sender, System::EventArgs^ e)
	{
		HideSidebar();
	}

	System::Void MainForm::menuItemToggleSidebar_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ToggleSidebar();
	}

	System::Void MainForm::MainForm_Resize(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->panelLeft != nullptr && this->tabControlMain != nullptr)
		{
			this->panelLeft->Top = this->tabControlMain->Top;
			this->panelLeft->Height = this->tabControlMain->Height;
		}
	}

	System::Void MainForm::tabControlMain_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
	{
		if (this->panelLeft != nullptr && this->panelLeft->Visible && !this->isSidebarPinned)
		{
			Point clientPt = this->PointToClient(System::Windows::Forms::Cursor::Position);
			if (!this->panelLeft->Bounds.Contains(clientPt))
			{
				HideSidebar();
			}
		}
	}

	System::Void MainForm::chartSector_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
	{
		if (this->panelLeft != nullptr && this->panelLeft->Visible && !this->isSidebarPinned)
		{
			HideSidebar();
		}
	}

	void MainForm::SaveStationConfig()
	{
		try
		{
			RememberChartMode(1);
			RememberChartMode(2);
			String^ configPath = System::IO::Path::Combine(Application::StartupPath, L"station_config.json");
			System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
			sb->AppendLine(L"{");

			// Window1
			sb->AppendLine(L"  \"Window1\": {");
			AppendModeConfig(sb, 1);
			sb->AppendLine(String::Format(L"    \"ActiveCurve\": \"{0}\",", this->activeCurve1 != nullptr ? this->activeCurve1 : L""));
			sb->AppendLine(String::Format(L"    \"Freq\": \"{0}\",", this->buttonFreq1->Text));
			sb->AppendLine(String::Format(L"    \"Mode\": \"{0}\",", this->buttonMode1->Text));
			sb->AppendLine(String::Format(L"    \"Receiver\": {0},", this->receiverIndex1));
			sb->AppendLine(String::Format(L"    \"IsTempX\": {0},", this->isTempXAxis1 ? L"true" : L"false"));
			sb->AppendLine(String::Format(L"    \"HoldZoom\": {0},", this->checkHoldZoom1->Checked ? L"true" : L"false"));
			sb->AppendLine(String::Format(L"    \"Precision\": {0},", this->precisionY1));
			sb->AppendLine(String::Format(L"    \"Tx\": [{0}, {1}, {2}, {3}, {4}],",
				this->checkTx1_1->Checked ? L"true" : L"false",
				this->checkTx1_2->Checked ? L"true" : L"false",
				this->checkTx1_3->Checked ? L"true" : L"false",
				this->checkTx1_4->Checked ? L"true" : L"false",
				this->checkTx1_5->Checked ? L"true" : L"false"));
			sb->Append(L"    \"PinnedCurves\": [");
			for (int i = 0; i < this->pinnedCurves1->Count; i++)
			{
				sb->Append(String::Format(L"\"{0}\"{1}", this->pinnedCurves1[i], (i + 1 < this->pinnedCurves1->Count) ? L", " : L""));
			}
			sb->AppendLine(L"]");
			sb->AppendLine(L"  },");

			// Window2
			sb->AppendLine(L"  \"Window2\": {");
			AppendModeConfig(sb, 2);
			sb->AppendLine(String::Format(L"    \"ActiveCurve\": \"{0}\",", this->activeCurve2 != nullptr ? this->activeCurve2 : L""));
			sb->AppendLine(String::Format(L"    \"Freq\": \"{0}\",", this->buttonFreq2->Text));
			sb->AppendLine(String::Format(L"    \"Mode\": \"{0}\",", this->buttonMode2->Text));
			sb->AppendLine(String::Format(L"    \"Receiver\": {0},", this->receiverIndex2));
			sb->AppendLine(String::Format(L"    \"IsTempX\": {0},", this->isTempXAxis2 ? L"true" : L"false"));
			sb->AppendLine(String::Format(L"    \"HoldZoom\": {0},", this->checkHoldZoom2->Checked ? L"true" : L"false"));
			sb->AppendLine(String::Format(L"    \"Precision\": {0},", this->precisionY2));
			sb->AppendLine(String::Format(L"    \"Tx\": [{0}, {1}, {2}, {3}, {4}],",
				this->checkTx2_1->Checked ? L"true" : L"false",
				this->checkTx2_2->Checked ? L"true" : L"false",
				this->checkTx2_3->Checked ? L"true" : L"false",
				this->checkTx2_4->Checked ? L"true" : L"false",
				this->checkTx2_5->Checked ? L"true" : L"false"));
			sb->Append(L"    \"PinnedCurves\": [");
			for (int i = 0; i < this->pinnedCurves2->Count; i++)
			{
				sb->Append(String::Format(L"\"{0}\"{1}", this->pinnedCurves2[i], (i + 1 < this->pinnedCurves2->Count) ? L", " : L""));
			}
			sb->AppendLine(L"]");
			sb->AppendLine(L"  },");

			// Sectors
			sb->AppendLine(L"  \"Sectors\": {");
			sb->AppendLine(String::Format(L"    \"FreqRxL\": \"{0}\",", this->buttonFreqRxL->Text));
			sb->AppendLine(String::Format(L"    \"TxRxL\": {0},", this->comboBoxTxRxL->SelectedIndex >= 0 ? this->comboBoxTxRxL->SelectedIndex : 0));
			sb->AppendLine(String::Format(L"    \"FreqRxR\": \"{0}\",", this->buttonFreqRxR->Text));
			sb->AppendLine(String::Format(L"    \"TxRxR\": {0}", this->comboBoxTxRxR->SelectedIndex >= 0 ? this->comboBoxTxRxR->SelectedIndex : 0));
			sb->AppendLine(L"  }");

			sb->AppendLine(L"}");

			System::IO::File::WriteAllText(configPath, sb->ToString(), System::Text::Encoding::UTF8);
		}
		catch (...) {}
	}

	void MainForm::LoadStationConfig()
	{
		String^ configPath = System::IO::Path::Combine(Application::StartupPath, L"station_config.json");
		if (!System::IO::File::Exists(configPath))
		{
			// Файла конфигурации еще нет - настраиваем кнопки по умолчанию и сохраняем
			AddCurveChip(1, L"Raw Δφ");
			AddCurveChip(1, L"AMP Z1");
			AddCurveChip(1, L"AMP Z2");
			AddCurveChip(1, L"RO Δφ");

			AddCurveChip(2, L"AM Z2/Z1");
			AddCurveChip(2, L"RO ATT");

			this->activeCurve1 = L"Raw Δφ";
			this->activeCurve2 = L"AM Z2/Z1";

			UpdateChipStyles(1);
			UpdateChipStyles(2);
			SaveStationConfig();
			return;
		}

		try
		{
			cli::array<String^>^ lines = System::IO::File::ReadAllLines(configPath, System::Text::Encoding::UTF8);
			for (int mode = 0; mode < 4; ++mode)
			{
				this->modePinnedCurves[mode]->Clear();
				this->modeActiveCurves[mode] = nullptr;
			}
			this->pinnedCurves1->Clear();
			this->pinnedCurves2->Clear();
			this->flowPanelChips1->Controls->Clear();
			this->flowPanelChips2->Controls->Clear();
			this->activeCurve1 = nullptr;
			this->activeCurve2 = nullptr;
			int currentSection = 0; // 1 = Window1, 2 = Window2, 3 = Sectors
			bool pinnedSeen1 = false, pinnedSeen2 = false;

			for each (String^ rawLine in lines)
			{
				String^ line = rawLine->Trim();
				if (line->Contains(L"\"Window1\"")) currentSection = 1;
				else if (line->Contains(L"\"Window2\"")) currentSection = 2;
				else if (line->Contains(L"\"Sectors\"")) currentSection = 3;

				if ((currentSection == 1 || currentSection == 2) && LoadModeConfigLine(currentSection, line)) continue;
				if (currentSection == 1)
				{
					if (line->StartsWith(L"\"ActiveCurve\":"))
					{
						int q1 = line->IndexOf('"', 14);
						int q2 = line->LastIndexOf('"');
						if (q1 >= 0 && q2 > q1) this->activeCurve1 = line->Substring(q1 + 1, q2 - q1 - 1);
					}
					else if (line->StartsWith(L"\"Freq\":"))
					{
						if (line->Contains(L"2000")) this->buttonFreq1->Text = L"2000 кГц";
						else this->buttonFreq1->Text = L"400 кГц";
					}
					else if (line->StartsWith(L"\"Mode\":"))
					{
						this->isDirectMode1 = line->Contains(L"DIRECT") && !line->Contains(L"UNDIRECT");
						this->buttonMode1->Text = this->isDirectMode1 ? L"DIRECT" : L"UNDIRECT";
						this->buttonMode1->BackColor = this->isDirectMode1 ?
							Color::FromArgb(255, 240, 210) : Color::FromArgb(220, 245, 225);
					}
					else if (line->StartsWith(L"\"Receiver\":"))
					{
						int receiver = 0;
						if (Int32::TryParse(line->Substring(line->IndexOf(':') + 1)->Trim()->Trim(','), receiver) && receiver >= 0 && receiver <= 1)
							this->receiverIndex1 = receiver;
					}
					else if (line->StartsWith(L"\"IsTempX\":"))
					{
						this->isTempXAxis1 = line->Contains(L"true");
						this->buttonXMode1->Text = this->isTempXAxis1 ? L"Темп." : L"Кадр";
						this->buttonXMode1->BackColor = this->isTempXAxis1 ?
							Color::FromArgb(255, 235, 205) : Color::FromArgb(240, 240, 245);
					}
					else if (line->StartsWith(L"\"HoldZoom\":"))
					{
						this->checkHoldZoom1->Checked = line->Contains(L"true");
					}
					else if (line->StartsWith(L"\"Precision\":"))
					{
						int idx = 0;
						int colon = line->IndexOf(':');
						if (colon >= 0)
						{
							String^ val = line->Substring(colon + 1)->Trim()->Trim(',');
							Int32::TryParse(val, idx);
						}
						if (idx >= 0 && idx <= 6)
						{
							this->precisionY1 = idx;
							this->comboBoxPrecision1->SelectedIndex = idx;
						}
					}
					else if (line->StartsWith(L"\"Tx\":"))
					{
						int b1 = line->IndexOf('[');
						int b2 = line->IndexOf(']');
						if (b1 >= 0 && b2 > b1)
						{
							String^ arr = line->Substring(b1 + 1, b2 - b1 - 1);
							cli::array<String^>^ parts = arr->Split(',');
							if (parts->Length >= 5)
							{
								this->checkTx1_1->Checked = parts[0]->Trim()->ToLower() == L"true";
								this->checkTx1_2->Checked = parts[1]->Trim()->ToLower() == L"true";
								this->checkTx1_3->Checked = parts[2]->Trim()->ToLower() == L"true";
								this->checkTx1_4->Checked = parts[3]->Trim()->ToLower() == L"true";
								this->checkTx1_5->Checked = parts[4]->Trim()->ToLower() == L"true";
							}
						}
					}
					else if (line->StartsWith(L"\"PinnedCurves\":"))
					{
						pinnedSeen1 = true;
						int b1 = line->IndexOf('[');
						int b2 = line->IndexOf(']');
						if (b1 >= 0 && b2 > b1)
						{
							String^ arr = line->Substring(b1 + 1, b2 - b1 - 1);
							cli::array<String^>^ parts = arr->Split(',');
							for each (String^ p in parts)
							{
								String^ c = p->Trim()->Trim('"');
								if (IsCurveValidForMode(c, this->isDirectMode1)) AddCurveChip(1, c);
							}
						}
					}
				}
				else if (currentSection == 2)
				{
					if (line->StartsWith(L"\"ActiveCurve\":"))
					{
						int q1 = line->IndexOf('"', 14);
						int q2 = line->LastIndexOf('"');
						if (q1 >= 0 && q2 > q1) this->activeCurve2 = line->Substring(q1 + 1, q2 - q1 - 1);
					}
					else if (line->StartsWith(L"\"Freq\":"))
					{
						if (line->Contains(L"2000")) this->buttonFreq2->Text = L"2000 кГц";
						else this->buttonFreq2->Text = L"400 кГц";
					}
					else if (line->StartsWith(L"\"Mode\":"))
					{
						this->isDirectMode2 = line->Contains(L"DIRECT") && !line->Contains(L"UNDIRECT");
						this->buttonMode2->Text = this->isDirectMode2 ? L"DIRECT" : L"UNDIRECT";
						this->buttonMode2->BackColor = this->isDirectMode2 ?
							Color::FromArgb(255, 240, 210) : Color::FromArgb(220, 245, 225);
					}
					else if (line->StartsWith(L"\"Receiver\":"))
					{
						int receiver = 0;
						if (Int32::TryParse(line->Substring(line->IndexOf(':') + 1)->Trim()->Trim(','), receiver) && receiver >= 0 && receiver <= 1)
							this->receiverIndex2 = receiver;
					}
					else if (line->StartsWith(L"\"IsTempX\":"))
					{
						this->isTempXAxis2 = line->Contains(L"true");
						this->buttonXMode2->Text = this->isTempXAxis2 ? L"Темп." : L"Кадр";
						this->buttonXMode2->BackColor = this->isTempXAxis2 ?
							Color::FromArgb(255, 235, 205) : Color::FromArgb(240, 240, 245);
					}
					else if (line->StartsWith(L"\"HoldZoom\":"))
					{
						this->checkHoldZoom2->Checked = line->Contains(L"true");
					}
					else if (line->StartsWith(L"\"Precision\":"))
					{
						int idx = 0;
						int colon = line->IndexOf(':');
						if (colon >= 0)
						{
							String^ val = line->Substring(colon + 1)->Trim()->Trim(',');
							Int32::TryParse(val, idx);
						}
						if (idx >= 0 && idx <= 6)
						{
							this->precisionY2 = idx;
							this->comboBoxPrecision2->SelectedIndex = idx;
						}
					}
					else if (line->StartsWith(L"\"Tx\":"))
					{
						int b1 = line->IndexOf('[');
						int b2 = line->IndexOf(']');
						if (b1 >= 0 && b2 > b1)
						{
							String^ arr = line->Substring(b1 + 1, b2 - b1 - 1);
							cli::array<String^>^ parts = arr->Split(',');
							if (parts->Length >= 5)
							{
								this->checkTx2_1->Checked = parts[0]->Trim()->ToLower() == L"true";
								this->checkTx2_2->Checked = parts[1]->Trim()->ToLower() == L"true";
								this->checkTx2_3->Checked = parts[2]->Trim()->ToLower() == L"true";
								this->checkTx2_4->Checked = parts[3]->Trim()->ToLower() == L"true";
								this->checkTx2_5->Checked = parts[4]->Trim()->ToLower() == L"true";
							}
						}
					}
					else if (line->StartsWith(L"\"PinnedCurves\":"))
					{
						pinnedSeen2 = true;
						int b1 = line->IndexOf('[');
						int b2 = line->IndexOf(']');
						if (b1 >= 0 && b2 > b1)
						{
							String^ arr = line->Substring(b1 + 1, b2 - b1 - 1);
							cli::array<String^>^ parts = arr->Split(',');
							for each (String^ p in parts)
							{
								String^ c = p->Trim()->Trim('"');
								if (IsCurveValidForMode(c, this->isDirectMode2)) AddCurveChip(2, c);
							}
						}
					}
				}
				else if (currentSection == 3)
				{
					if (line->StartsWith(L"\"FreqRxL\":"))
					{
						if (line->Contains(L"2000")) this->buttonFreqRxL->Text = L"2000 кГц";
						else this->buttonFreqRxL->Text = L"400 кГц";
					}
					else if (line->StartsWith(L"\"TxRxL\":"))
					{
						int val = 0;
						String^ num = line->Substring(8)->Trim()->Trim(',');
						if (Int32::TryParse(num, val) && val >= 0 && val < this->comboBoxTxRxL->Items->Count)
							this->comboBoxTxRxL->SelectedIndex = val;
					}
					else if (line->StartsWith(L"\"FreqRxR\":"))
					{
						if (line->Contains(L"2000")) this->buttonFreqRxR->Text = L"2000 кГц";
						else this->buttonFreqRxR->Text = L"400 кГц";
					}
					else if (line->StartsWith(L"\"TxRxR\":"))
					{
						int val = 0;
						String^ num = line->Substring(8)->Trim()->Trim(',');
						if (Int32::TryParse(num, val) && val >= 0 && val < this->comboBoxTxRxR->Items->Count)
							this->comboBoxTxRxR->SelectedIndex = val;
					}
				}
			}

			// Дефолтные чипы добавляются только если ключ PinnedCurves отсутствовал вовсе;
			// явно сохранённый пустой список означает пустой график
			if (!pinnedSeen1)
			{
				AddCurveChip(1, L"Raw Δφ");
				AddCurveChip(1, L"AMP Z1");
			}
			if (!pinnedSeen2)
			{
				AddCurveChip(2, L"AM Z2/Z1");
				AddCurveChip(2, L"RO ATT");
			}

			this->activeCurve1 = NormalizeCurveName(this->activeCurve1);
			this->activeCurve2 = NormalizeCurveName(this->activeCurve2);
			if (!IsCurveValidForMode(this->activeCurve1, this->isDirectMode1))
				this->activeCurve1 = this->pinnedCurves1->Count > 0 ? this->pinnedCurves1[0] : nullptr;
			if (!IsCurveValidForMode(this->activeCurve2, this->isDirectMode2))
				this->activeCurve2 = this->pinnedCurves2->Count > 0 ? this->pinnedCurves2[0] : nullptr;
			if (String::IsNullOrEmpty(this->activeCurve1) && this->pinnedCurves1->Count > 0)
				this->activeCurve1 = this->pinnedCurves1[0];
			if (String::IsNullOrEmpty(this->activeCurve2) && this->pinnedCurves2->Count > 0)
				this->activeCurve2 = this->pinnedCurves2[0];

			RememberChartMode(1);
			RememberChartMode(2);
			UpdateChipStyles(1);
			UpdateChipStyles(2);
			UpdateSondeScheme();
		}
		catch (...)
		{
			AddCurveChip(1, L"Raw Δφ");
			AddCurveChip(2, L"AM Z2/Z1");
			this->activeCurve1 = L"Raw Δφ";
			this->activeCurve2 = L"AM Z2/Z1";
			UpdateChipStyles(1);
			UpdateChipStyles(2);
			UpdateSondeScheme();
		}
	}

	System::Void MainForm::MainForm_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e)
	{
		if (this->telemetryRunning) StopTelemetry();
		SaveStationConfig();
	}

	void MainForm::SetActiveSchemeChart(int chartIndex)
	{
		if (chartIndex != 0 && chartIndex != 1) return;
		this->activeSchemeChart = chartIndex;
		UpdateSondeScheme();
	}

	void MainForm::UpdateSondeScheme()
	{
		UpdateSondeScheme(false);
	}

	void MainForm::UpdateSondeScheme(bool useConnected)
	{
		if (this->sondeScheme == nullptr) return;
		bool direct = (this->activeSchemeChart == 0) ? this->isDirectMode1 : this->isDirectMode2;
		// Тип прибора: 351/352 — «Картограф» (схема с крайними Rx и DIRECT),
		// остальные (231/242/349/151/152) — GP-приборы: только UNDIRECT, nTx катушек
		bool connected = this->connectedSondeId != nullptr && this->deviceLink != nullptr &&
			this->deviceLink->IsOpen && (useConnected || this->isLiveSession || !SondeData::IsLoaded());
		bool identified = connected || SondeData::GetSignature() != 0;
		SondeID id = connected ? *this->connectedSondeId : SondeID::FromSignature(SondeData::GetSignature());
		int sondeType = identified ? id.type : 352;
		int txCount = identified ? id.N_Tx : 4;
		this->sondeScheme->SetDeviceCaption(identified ?
			Utf8ToManaged(id.GetTypeName().c_str()) + L"  |  Тип " + id.type.ToString() +
			L"  №" + id.number.ToString() : L"Прибор не определён");
		bool carto = (sondeType == 351 || sondeType == 352);
		ApplySondeTypeLayout(carto);
		this->sondeScheme->SetState(this->activeSchemeChart, carto && direct, txCount, carto);
		// Подсветка заголовка активного графика
		if (this->panelTime1Header != nullptr)
			this->panelTime1Header->BorderStyle = (this->activeSchemeChart == 0) ?
				BorderStyle::FixedSingle : BorderStyle::None;
		if (this->panelTime2Header != nullptr)
			this->panelTime2Header->BorderStyle = (this->activeSchemeChart == 1) ?
				BorderStyle::FixedSingle : BorderStyle::None;
	}

	System::Void MainForm::sondeScheme_SchemeClicked(System::Object^ sender, System::EventArgs^ e)
	{
		SetActiveSchemeChart(this->activeSchemeChart == 0 ? 1 : 0);
	}
}
