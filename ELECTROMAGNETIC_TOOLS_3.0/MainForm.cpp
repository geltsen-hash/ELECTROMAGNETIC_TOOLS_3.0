#include "pch.h"
#include "MainForm.h"
#include "MetroForm.h"
#include "BinFileReader.h"
#include <msclr/marshal_cppstd.h>
#include <iostream>
#include <cmath>
#include <cstring>

namespace ELECTROMAGNETIC_TOOLS_3_0 {

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

		cli::array<Color>^ colors = gcnew cli::array<Color>(5) {
			Color::RoyalBlue,
			Color::Crimson,
			Color::ForestGreen,
			Color::DarkOrange,
			Color::DarkViolet
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
		this->isDirectMode1 = false;
		this->isDirectMode2 = false;
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
		this->isSyncingSplitter = false;
		this->isSidebarPinned = false;

		// Загрузка сохраненной конфигурации станции (быстрые кнопки, режимы)
		LoadStationConfig();
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

		this->Cursor = Cursors::WaitCursor;
		try
		{
			std::string nativePath = msclr::interop::marshal_as<std::string>(filePath);
			char errBuf[512] = { 0 };

			bool ok = SondeData::LoadFile(nativePath.c_str(), errBuf, sizeof(errBuf));
			if (!ok)
			{
				this->Cursor = Cursors::Default;
				MessageBox::Show(this, gcnew String(errBuf, 0, static_cast<int>(std::strlen(errBuf)), System::Text::Encoding::UTF8),
					L"Ошибка открытия файла", MessageBoxButtons::OK, MessageBoxIcon::Error);
				return;
			}

			size_t frameCount = SondeData::GetFrameCount();
			String^ sondeName = gcnew String(SondeData::GetSondeName());
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

		// Ненаправленные
		if (name->Contains(L"Разность фаз") || name->Contains(L"PH Z2-Z1") || name->Contains(L"Δφ"))
			return SondeData::CURVE_PHASE_DIFF;
		if (name->Contains(L"Амплитуда Rx1") || name->Contains(L"AM Z1") || name->Contains(L"Амплитуда Z1"))
			return SondeData::CURVE_AMPLITUDE_RX1;
		if (name->Contains(L"Амплитуда Rx2") || name->Contains(L"AM Z2") || name->Contains(L"Амплитуда Z2"))
			return SondeData::CURVE_AMPLITUDE_RX2;
		if (name->Contains(L"УЭС по фазе") || name->Contains(L"ρ_ph"))
			return SondeData::CURVE_RHO_PHASE;
		if (name->Contains(L"Затухание") || name->Contains(L"AM Z2/Z1") || name->Contains(L"Att"))
			return SondeData::CURVE_ATTENUATION;
		if (name->Contains(L"УЭС по затуханию") || name->Contains(L"ρ_att"))
			return SondeData::CURVE_RHO_ATT;
		if (name->Contains(L"Симметризованная фаза") || name->Contains(L"PH_SMT"))
			return SondeData::CURVE_PHASE_SMT;
		if (name->Contains(L"Амплитуда нуля ZZ") || name->Contains(L"нуля ZZ"))
			return SondeData::CURVE_ZERO_AM_ZZ;
		if (name->Contains(L"Температура RZZ") || name->Contains(L"Температура"))
			return SondeData::CURVE_TEMPERATURE;
		if (name->Contains(L"Кадр") || name->Contains(L"FRAME"))
			return SondeData::CURVE_FRAME_NUM;
		if (name->Contains(L"Акселерометр") || name->Contains(L"ACCEL"))
			return SondeData::CURVE_ACCEL;

		// Направленные
		if (name->Contains(L"Гео-Амплитуда") || name->Contains(L"AM_GEO"))
			return SondeData::CURVE_GEO_AM;
		if (name->Contains(L"Гео-Фаза") || name->Contains(L"PH_GEO"))
			return SondeData::CURVE_GEO_PH;
		if (name->Contains(L"Напряжение V_ZX") || name->Contains(L"V_ZX"))
			return SondeData::CURVE_V_ZX;
		if (name->Contains(L"Напряжение V_ZZ") || name->Contains(L"V_ZZ"))
			return SondeData::CURVE_V_ZZ;
		if (name->Contains(L"Коэффициент betta_z") || name->Contains(L"betta_z"))
			return SondeData::CURVE_BETTA_Z;
		if (name->Contains(L"Отношение V_ZX / V_ZZ") || name->Contains(L"V_ZX / V_ZZ"))
			return SondeData::CURVE_RATIO_ZX_ZZ;
		if (name->Contains(L"Напряжение V_XX") || name->Contains(L"V_XX"))
			return SondeData::CURVE_V_XX;
		if (name->Contains(L"Шум dV") || name->Contains(L"dV"))
			return SondeData::CURVE_DV_NOISE;
		if (name->Contains(L"Сигнал/шум") || name->Contains(L"SNR"))
			return SondeData::CURVE_SNR;
		if (name->Contains(L"Амплитуда нуля ZX") || name->Contains(L"нуля ZX"))
			return SondeData::CURVE_ZERO_AM_ZX;
		if (name->Contains(L"Угол границы") || name->Contains(L"BORDER_ANGLE"))
			return SondeData::CURVE_BORDER_ANGLE;
		if (name->Contains(L"Инклинометр") || name->Contains(L"INC"))
			return SondeData::CURVE_INC_ABS;
		if (name->Contains(L"Угловая скорость") || name->Contains(L"ANGULAR_VELOCITY") || name->Contains(L"Wg"))
			return SondeData::CURVE_ANGULAR_VELOCITY;
		if (name->Contains(L"Стартовый сектор") || name->Contains(L"START_SECTOR"))
			return SondeData::CURVE_START_SECTOR;
		if (name->Contains(L"Температура RZX"))
			return SondeData::CURVE_TEMP_RZX;
		if (name->Contains(L"Глубина") || name->Contains(L"DEPTH"))
			return SondeData::CURVE_DEPTH;

		return SondeData::CURVE_PHASE_DIFF;
	}

	static String^ GetAxisTitleByCurve(String^ name)
	{
		if (String::IsNullOrEmpty(name)) return L"Фаза Δφ (град)";
		if (name->Contains(L"Разность фаз") || name->Contains(L"PH Z2-Z1") || name->Contains(L"Δφ") || name->Contains(L"PH_SMT") || name->Contains(L"Гео-Фаза"))
			return L"Фаза (град)";
		if (name->Contains(L"Амплитуда нуля") || name->Contains(L"Напряжение") || name->Contains(L"V_ZX") || name->Contains(L"V_ZZ") || name->Contains(L"V_XX") || name->Contains(L"Шум dV"))
			return L"Напряжение (мВ)";
		if (name->Contains(L"Амплитуда") || name->Contains(L"AM Z"))
			return L"Амплитуда (мВ)";
		if (name->Contains(L"УЭС") || name->Contains(L"ρ"))
			return L"УЭС (Ом·м)";
		if (name->Contains(L"Затухание") || name->Contains(L"Att") || name->Contains(L"Гео-Амплитуда"))
			return L"Затухание (дБ)";
		if (name->Contains(L"Температура"))
			return L"Температура (°C)";
		if (name->Contains(L"Угол границы"))
			return L"Угол (град)";
		if (name->Contains(L"Инклинометр"))
			return L"Углы инклинометра (град)";
		if (name->Contains(L"Угловая скорость"))
			return L"Скорость (об/мин)";
		if (name->Contains(L"Глубина"))
			return L"Глубина (м)";
		if (name->Contains(L"Кадр"))
			return L"Номер кадра";
		if (name->Contains(L"betta_z") || name->Contains(L"Отношение") || name->Contains(L"SNR"))
			return L"Отношение";
		return L"Значение";
	}

	static String^ GetPrecisionFormat(int precisionMode, String^ curveName)
	{
		if (precisionMode <= 0) // Авто
		{
			if (String::IsNullOrEmpty(curveName)) return L"F2";
			if (curveName->Contains(L"Температура")) return L"F1";
			if (curveName->Contains(L"Кадр") || curveName->Contains(L"FRAME") || curveName->Contains(L"Глубина") || curveName->Contains(L"Сектор")) return L"F0";
			if (curveName->Contains(L"Напряжение") || curveName->Contains(L"нуля") || curveName->Contains(L"V_") || curveName->Contains(L"dV") || curveName->Contains(L"AM Z")) return L"F4";
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

		this->checkTx1_5->Visible = hasTx5;
		if (!hasTx5) this->checkTx1_5->Checked = false;

		this->checkTx2_5->Visible = hasTx5;
		if (!hasTx5) this->checkTx2_5->Checked = false;

		this->checkTx1_4->Visible = hasTx4;
		if (!hasTx4) this->checkTx1_4->Checked = false;

		this->checkTx2_4->Visible = hasTx4;
		if (!hasTx4) this->checkTx2_4->Checked = false;

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
		if (!SondeData::IsLoaded()) return;

		size_t frameCount = SondeData::GetFrameCount();
		if (frameCount == 0) return;

		UpdateTxControlsVisibility();

		int txCount = SondeData::GetTxCount();
		int freq1 = (this->buttonFreq1->Text->Contains(L"2000")) ? 1 : 0;
		int freq2 = (this->buttonFreq2->Text->Contains(L"2000")) ? 1 : 0;

		this->chartTime1->ChartAreas[0]->AxisX->Title = this->isTempXAxis1 ? L"Температура (°C)" : L"Кадр (N)";
		this->chartTime2->ChartAreas[0]->AxisX->Title = this->isTempXAxis2 ? L"Температура (°C)" : L"Кадр (N)";
		this->chartTime1->ChartAreas[0]->AxisX->LabelStyle->Format = L"F0";
		this->chartTime2->ChartAreas[0]->AxisX->LabelStyle->Format = L"F0";
		this->chartTime1->ChartAreas[0]->AxisX->Interval = 0;
		this->chartTime2->ChartAreas[0]->AxisX->Interval = 0;
		this->chartTime1->ChartAreas[0]->AxisX->IntervalAutoMode = IntervalAutoMode::VariableCount;
		this->chartTime2->ChartAreas[0]->AxisX->IntervalAutoMode = IntervalAutoMode::VariableCount;

		this->chartTime1->ChartAreas[0]->AxisY->Title = GetAxisTitleByCurve(this->activeCurve1);
		this->chartTime2->ChartAreas[0]->AxisY->Title = GetAxisTitleByCurve(this->activeCurve2);

		String^ fmt1 = GetPrecisionFormat(this->precisionY1, this->activeCurve1);
		String^ fmt2 = GetPrecisionFormat(this->precisionY2, this->activeCurve2);
		this->chartTime1->ChartAreas[0]->AxisY->LabelStyle->Format = fmt1;
		this->chartTime2->ChartAreas[0]->AxisY->LabelStyle->Format = fmt2;

		int curveType1 = GetCurveTypeByName(this->activeCurve1);
		int curveType2 = GetCurveTypeByName(this->activeCurve2);

		// Запоминаем текущий зум, если включен чекбокс удержания
		bool hold1 = this->checkHoldZoom1->Checked && this->chartTime1->ChartAreas[0]->AxisX->ScaleView->IsZoomed;
		double vMin1 = this->chartTime1->ChartAreas[0]->AxisX->ScaleView->ViewMinimum;
		double vMax1 = this->chartTime1->ChartAreas[0]->AxisX->ScaleView->ViewMaximum;

		bool hold2 = this->checkHoldZoom2->Checked && this->chartTime2->ChartAreas[0]->AxisX->ScaleView->IsZoomed;
		double vMin2 = this->chartTime2->ChartAreas[0]->AxisX->ScaleView->ViewMinimum;
		double vMax2 = this->chartTime2->ChartAreas[0]->AxisX->ScaleView->ViewMaximum;

		for (int s = 0; s < this->chartTime1->Series->Count; s++)
			this->chartTime1->Series[s]->Points->Clear();
		for (int s = 0; s < this->chartTime2->Series->Count; s++)
			this->chartTime2->Series[s]->Points->Clear();

		size_t start1 = 0;
		size_t end1 = frameCount - 1;
		if (this->minFrame1 >= 0 && static_cast<size_t>(this->minFrame1) < frameCount)
			start1 = static_cast<size_t>(this->minFrame1);
		if (this->maxFrame1 > 0 && static_cast<size_t>(this->maxFrame1) < frameCount)
			end1 = static_cast<size_t>(this->maxFrame1);
		if (start1 > end1) { size_t tmp = start1; start1 = end1; end1 = tmp; }

		size_t start2 = 0;
		size_t end2 = frameCount - 1;
		if (this->minFrame2 >= 0 && static_cast<size_t>(this->minFrame2) < frameCount)
			start2 = static_cast<size_t>(this->minFrame2);
		if (this->maxFrame2 > 0 && static_cast<size_t>(this->maxFrame2) < frameCount)
			end2 = static_cast<size_t>(this->maxFrame2);
		if (start2 > end2) { size_t tmp = start2; start2 = end2; end2 = tmp; }

		for (size_t i = start1; i <= end1 && i < frameCount; ++i)
		{
			double xVal1 = this->isTempXAxis1 ? SondeData::GetTemperature(i) : static_cast<double>(i);
			for (int tx = 0; tx < txCount && tx < this->chartTime1->Series->Count; ++tx)
			{
				double yVal = SondeData::GetCurveValue(curveType1, freq1, tx, i);
				this->chartTime1->Series[tx]->Points->AddXY(xVal1, yVal);
			}
		}

		for (size_t i = start2; i <= end2 && i < frameCount; ++i)
		{
			double xVal2 = this->isTempXAxis2 ? SondeData::GetTemperature(i) : static_cast<double>(i);
			for (int tx = 0; tx < txCount && tx < this->chartTime2->Series->Count; ++tx)
			{
				double yVal = SondeData::GetCurveValue(curveType2, freq2, tx, i);
				this->chartTime2->Series[tx]->Points->AddXY(xVal2, yVal);
			}
		}

		// Применяем фильтры передатчиков с учетом типа зонда
		if (this->chartTime1->Series->Count >= 1) this->chartTime1->Series[0]->Enabled = this->checkTx1_1->Checked;
		if (this->chartTime1->Series->Count >= 2) this->chartTime1->Series[1]->Enabled = this->checkTx1_2->Checked;
		if (this->chartTime1->Series->Count >= 3) this->chartTime1->Series[2]->Enabled = this->checkTx1_3->Checked;
		if (this->chartTime1->Series->Count >= 4) this->chartTime1->Series[3]->Enabled = (txCount >= 4 && this->checkTx1_4->Checked);
		if (this->chartTime1->Series->Count >= 5) this->chartTime1->Series[4]->Enabled = (txCount >= 5 && this->checkTx1_5->Checked);

		if (this->chartTime2->Series->Count >= 1) this->chartTime2->Series[0]->Enabled = this->checkTx2_1->Checked;
		if (this->chartTime2->Series->Count >= 2) this->chartTime2->Series[1]->Enabled = this->checkTx2_2->Checked;
		if (this->chartTime2->Series->Count >= 3) this->chartTime2->Series[2]->Enabled = this->checkTx2_3->Checked;
		if (this->chartTime2->Series->Count >= 4) this->chartTime2->Series[3]->Enabled = (txCount >= 4 && this->checkTx2_4->Checked);
		if (this->chartTime2->Series->Count >= 5) this->chartTime2->Series[4]->Enabled = (txCount >= 5 && this->checkTx2_5->Checked);

		this->chartTime1->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime1->ChartAreas[0]->AxisY->Maximum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisY->Maximum = Double::NaN;

		if (hold1 && !Double::IsNaN(vMin1) && !Double::IsNaN(vMax1) && vMax1 > vMin1)
		{
			this->chartTime1->ChartAreas[0]->AxisX->ScaleView->Zoom(vMin1, vMax1);
		}
		else
		{
			this->chartTime1->ChartAreas[0]->AxisX->Minimum = Double::NaN;
			this->chartTime1->ChartAreas[0]->AxisX->Maximum = Double::NaN;
			this->chartTime1->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
		}

		if (hold2 && !Double::IsNaN(vMin2) && !Double::IsNaN(vMax2) && vMax2 > vMin2)
		{
			this->chartTime2->ChartAreas[0]->AxisX->ScaleView->Zoom(vMin2, vMax2);
		}
		else
		{
			this->chartTime2->ChartAreas[0]->AxisX->Minimum = Double::NaN;
			this->chartTime2->ChartAreas[0]->AxisX->Maximum = Double::NaN;
			this->chartTime2->ChartAreas[0]->AxisX->ScaleView->ZoomReset(0);
		}

		this->chartTime1->Invalidate();
		this->chartTime2->Invalidate();
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

	void MainForm::PopulateCurveMenu(int windowIndex)
	{
		System::Windows::Forms::ContextMenuStrip^ curveMenu = (windowIndex == 1) ? this->contextMenuCurves1 : this->contextMenuCurves2;
		List<String^>^ pinned = (windowIndex == 1) ? this->pinnedCurves1 : this->pinnedCurves2;
		bool isDirect = (windowIndex == 1) ? this->isDirectMode1 : this->isDirectMode2;

		curveMenu->Items->Clear();

		cli::array<String^>^ poolUndirect = gcnew cli::array<String^> {
			L"Фаза Δφ (PH Z2-Z1)",
			L"Затухание Att (AM Z2/Z1)",
			L"УЭС по фазе (ρ_ph)",
			L"УЭС по затуханию (ρ_att)",
			L"Амплитуда Z1 (AM Z1)",
			L"Амплитуда Z2 (AM Z2)",
			L"Амплитуда нуля ZZ",
			L"Температура RZZ",
			L"Погрешность УЭС",
			L"Кадр (FRAME)",
			L"Акселерометр (ACCEL)"
		};

		cli::array<String^>^ poolDirect = gcnew cli::array<String^> {
			L"Гео-Амплитуда (AM_GEO)",
			L"Гео-Фаза (PH_GEO)",
			L"Напряжение V_ZX",
			L"Напряжение V_ZZ",
			L"Коэффициент betta_z",
			L"Отношение V_ZX / V_ZZ",
			L"Напряжение V_XX",
			L"Шум dV (мусор)",
			L"Сигнал/шум (SNR)",
			L"Амплитуда нуля ZX",
			L"Угол границы (BORDER_ANGLE)",
			L"Инклинометр (INC Abs/Zen/Azm)",
			L"Угловая скорость (ANGULAR_VELOCITY)",
			L"Стартовый сектор (START_SECTOR)",
			L"Температура RZX",
			L"Глубина (DEPTH)"
		};

		cli::array<String^>^ activePool = isDirect ? poolDirect : poolUndirect;

		for each (String^ curve in activePool)
		{
			if (!pinned->Contains(curve))
			{
				ToolStripMenuItem^ item = gcnew ToolStripMenuItem(curve);
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

	void MainForm::AddCurveChip(int windowIndex, String^ curveName)
	{
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
		RenderLoadedCurves();
		SaveStationConfig();
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

		if (windowIndex == 1 && this->activeCurve1 == curveName)
		{
			this->activeCurve1 = (this->pinnedCurves1->Count > 0) ? this->pinnedCurves1[0] : "";
		}
		else if (windowIndex == 2 && this->activeCurve2 == curveName)
		{
			this->activeCurve2 = (this->pinnedCurves2->Count > 0) ? this->pinnedCurves2[0] : "";
		}

		UpdateChipStyles(windowIndex);
		RenderLoadedCurves();
		SaveStationConfig();
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
			RenderLoadedCurves();
			SaveStationConfig();
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

	System::Void MainForm::buttonMode1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->isDirectMode1 = !this->isDirectMode1;
		this->buttonMode1->Text = this->isDirectMode1 ? L"DIRECT" : L"UNDIRECT";
		this->buttonMode1->BackColor = this->isDirectMode1 ? Color::FromArgb(255, 240, 210) : Color::FromArgb(220, 245, 225);

		this->flowPanelChips1->Controls->Clear();
		this->pinnedCurves1->Clear();

		if (this->isDirectMode1)
		{
			AddCurveChip(1, L"Гео-Амплитуда (AM_GEO)");
			AddCurveChip(1, L"Гео-Фаза (PH_GEO)");
			this->activeCurve1 = L"Гео-Фаза (PH_GEO)";
		}
		else
		{
			AddCurveChip(1, L"Разность фаз Δφ (Tx1..Tx5)");
			AddCurveChip(1, L"Амплитуда Rx1 (Tx1..Tx5)");
			AddCurveChip(1, L"УЭС по фазе (ρ_ph)");
			this->activeCurve1 = L"Разность фаз Δφ (Tx1..Tx5)";
		}

		UpdateChipStyles(1);
		RenderLoadedCurves();
		SaveStationConfig();
	}

	System::Void MainForm::buttonMode2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->isDirectMode2 = !this->isDirectMode2;
		this->buttonMode2->Text = this->isDirectMode2 ? L"DIRECT" : L"UNDIRECT";
		this->buttonMode2->BackColor = this->isDirectMode2 ? Color::FromArgb(255, 240, 210) : Color::FromArgb(220, 245, 225);

		this->flowPanelChips2->Controls->Clear();
		this->pinnedCurves2->Clear();

		if (this->isDirectMode2)
		{
			AddCurveChip(2, L"Отношение V_ZX / V_ZZ");
			AddCurveChip(2, L"Шум dV (мусор)");
			this->activeCurve2 = L"Отношение V_ZX / V_ZZ";
		}
		else
		{
			AddCurveChip(2, L"Затухание Att (AM Z2/Z1)");
			AddCurveChip(2, L"УЭС по затуханию (ρ_att)");
			this->activeCurve2 = L"Затухание Att (AM Z2/Z1)";
		}

		UpdateChipStyles(2);
		RenderLoadedCurves();
		SaveStationConfig();
	}

	System::Void MainForm::buttonFreq1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreq1->Text == L"2000 кГц")
			this->buttonFreq1->Text = L"400 кГц";
		else
			this->buttonFreq1->Text = L"2000 кГц";

		RenderLoadedCurves();
		SaveStationConfig();
	}

	System::Void MainForm::buttonFreq2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreq2->Text == L"400 кГц")
			this->buttonFreq2->Text = L"2000 кГц";
		else
			this->buttonFreq2->Text = L"400 кГц";

		RenderLoadedCurves();
		SaveStationConfig();
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
		this->splitContainerBand1->Panel2Collapsed = isCollapsed;
		this->splitContainerBand2->Panel2Collapsed = isCollapsed;
		this->buttonToggleSectors->Text = isCollapsed ? L"Показать секторы ⮞" : L"⮜ Скрыть секторы";
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
		}
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
		RenderLoadedCurves();
		SaveStationConfig();
	}

	System::Void MainForm::buttonXMode2_Click(System::Object^ sender, System::EventArgs^ e)
	{
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
		RenderLoadedCurves();
		SaveStationConfig();
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

	System::Void MainForm::buttonConnect_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (!this->isSidebarPinned)
		{
			HideSidebar();
		}

		this->buttonConnect->Enabled = false;
		this->buttonDisconnect->Enabled = true;
		this->menuItemConnect->Enabled = false;
		this->menuItemDisconnect->Enabled = true;

		String^ connInfo = this->comboBoxConnType->Text + L" [" + this->textBoxConnParam->Text + L"]";
		this->labelConnStatus->Text = L"● Подключено";
		this->labelConnStatus->ForeColor = Color::Green;
		this->statusLabelConnection->Text = L"● Связь: Подключено (" + this->comboBoxConnType->Text + L")";
		this->statusLabelConnection->ForeColor = Color::DarkGreen;

		std::cout << "[COMM] Подключение запущено: " << std::endl;
	}

	System::Void MainForm::buttonDisconnect_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->buttonConnect->Enabled = true;
		this->buttonDisconnect->Enabled = false;
		this->menuItemConnect->Enabled = true;
		this->menuItemDisconnect->Enabled = false;

		this->labelConnStatus->Text = L"● Отключено";
		this->labelConnStatus->ForeColor = Color::Gray;
		this->statusLabelConnection->Text = L"● Связь: Отключено";
		this->statusLabelConnection->ForeColor = Color::DimGray;

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
		MetroForm^ metroForm = gcnew MetroForm();
		metroForm->ShowDialog(this);
	}

	System::Void MainForm::menuItemOpenBin_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->openFileDialogBin->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			LoadDataFile(this->openFileDialogBin->FileName);
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
		this->isStreaming = !this->isStreaming;
		if (this->isStreaming)
		{
			this->buttonStream->BackColor = Color::FromArgb(255, 192, 192);
			this->buttonStream->Text = L"ПОТОК [ВКЛ]";
			this->timerStream->Start();
		}
		else
		{
			this->buttonStream->BackColor = SystemColors::Control;
			this->buttonStream->Text = L"ПОТОК";
			this->timerStream->Stop();
		}
	}

	System::Void MainForm::buttonRefresh_Click(System::Object^ sender, System::EventArgs^ e)
	{
		// 1. Остановка режима поток при обновлении
		if (this->isStreaming)
		{
			this->isStreaming = false;
			this->timerStream->Stop();
			this->buttonStream->BackColor = SystemColors::Control;
			this->buttonStream->Text = L"ПОТОК";
		}

		// 2. Применение диапазонов Min..Max (или полного графика при пустых полях)
		ApplyRange(1);
		ApplyRange(2);

		// 3. Авто-масштаб оси Y
		this->chartTime1->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime1->ChartAreas[0]->AxisY->Maximum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisY->Minimum = Double::NaN;
		this->chartTime2->ChartAreas[0]->AxisY->Maximum = Double::NaN;
	}

	System::Void MainForm::timerStream_Tick(System::Object^ sender, System::EventArgs^ e)
	{
		this->currentStreamPoint++;
		int i = this->currentStreamPoint;
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

		// Скользящее бегущее окно 100 точек
		int minPt = Math::Max(0, i - 100);
		int maxPt = i;

		this->textBoxMin1->Text = minPt.ToString();
		this->textBoxMax1->Text = maxPt.ToString();
		this->textBoxMin2->Text = minPt.ToString();
		this->textBoxMax2->Text = maxPt.ToString();

		double minTime = minPt * 0.1;
		double maxTime = maxPt * 0.1;

		this->chartTime1->ChartAreas[0]->AxisX->Minimum = minTime;
		this->chartTime1->ChartAreas[0]->AxisX->Maximum = maxTime;
		this->chartTime2->ChartAreas[0]->AxisX->Minimum = minTime;
		this->chartTime2->ChartAreas[0]->AxisX->Maximum = maxTime;

		this->chartTime1->ChartAreas[0]->CursorX->Position = maxTime;
		this->chartTime2->ChartAreas[0]->CursorX->Position = maxTime;

		UpdateTimeSlices(maxTime);
		UpdateCursorReadout(maxTime);
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
			String^ configPath = System::IO::Path::Combine(Application::StartupPath, L"station_config.json");
			System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
			sb->AppendLine(L"{");

			// Window1
			sb->AppendLine(L"  \"Window1\": {");
			sb->AppendLine(String::Format(L"    \"ActiveCurve\": \"{0}\",", this->activeCurve1 != nullptr ? this->activeCurve1 : L""));
			sb->AppendLine(String::Format(L"    \"Freq\": \"{0}\",", this->buttonFreq1->Text));
			sb->AppendLine(String::Format(L"    \"Mode\": \"{0}\",", this->buttonMode1->Text));
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
			sb->AppendLine(String::Format(L"    \"ActiveCurve\": \"{0}\",", this->activeCurve2 != nullptr ? this->activeCurve2 : L""));
			sb->AppendLine(String::Format(L"    \"Freq\": \"{0}\",", this->buttonFreq2->Text));
			sb->AppendLine(String::Format(L"    \"Mode\": \"{0}\",", this->buttonMode2->Text));
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
			AddCurveChip(1, L"Разность фаз Δφ (Tx1..Tx5)");
			AddCurveChip(1, L"Амплитуда Rx1 (Tx1..Tx5)");
			AddCurveChip(1, L"Амплитуда Rx2 (Tx1..Tx5)");
			AddCurveChip(1, L"УЭС по фазе (ρ_ph)");

			AddCurveChip(2, L"Затухание Att (AM Z2/Z1)");
			AddCurveChip(2, L"УЭС по затуханию (ρ_att)");

			this->activeCurve1 = L"Разность фаз Δφ (Tx1..Tx5)";
			this->activeCurve2 = L"Затухание Att (AM Z2/Z1)";

			UpdateChipStyles(1);
			UpdateChipStyles(2);
			SaveStationConfig();
			return;
		}

		try
		{
			cli::array<String^>^ lines = System::IO::File::ReadAllLines(configPath, System::Text::Encoding::UTF8);
			int currentSection = 0; // 1 = Window1, 2 = Window2, 3 = Sectors

			for each (String^ rawLine in lines)
			{
				String^ line = rawLine->Trim();
				if (line->Contains(L"\"Window1\"")) currentSection = 1;
				else if (line->Contains(L"\"Window2\"")) currentSection = 2;
				else if (line->Contains(L"\"Sectors\"")) currentSection = 3;

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
						int b1 = line->IndexOf('[');
						int b2 = line->IndexOf(']');
						if (b1 >= 0 && b2 > b1)
						{
							String^ arr = line->Substring(b1 + 1, b2 - b1 - 1);
							cli::array<String^>^ parts = arr->Split(',');
							for each (String^ p in parts)
							{
								String^ c = p->Trim()->Trim('"');
								if (!String::IsNullOrEmpty(c)) AddCurveChip(1, c);
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
						int b1 = line->IndexOf('[');
						int b2 = line->IndexOf(']');
						if (b1 >= 0 && b2 > b1)
						{
							String^ arr = line->Substring(b1 + 1, b2 - b1 - 1);
							cli::array<String^>^ parts = arr->Split(',');
							for each (String^ p in parts)
							{
								String^ c = p->Trim()->Trim('"');
								if (!String::IsNullOrEmpty(c)) AddCurveChip(2, c);
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

			// Если после загрузки список пуст - добавляем дефолтные
			if (this->pinnedCurves1->Count == 0)
			{
				AddCurveChip(1, L"Разность фаз Δφ (Tx1..Tx5)");
				AddCurveChip(1, L"Амплитуда Rx1 (Tx1..Tx5)");
			}
			if (this->pinnedCurves2->Count == 0)
			{
				AddCurveChip(2, L"Затухание Att (AM Z2/Z1)");
				AddCurveChip(2, L"УЭС по затуханию (ρ_att)");
			}

			if (String::IsNullOrEmpty(this->activeCurve1) && this->pinnedCurves1->Count > 0)
				this->activeCurve1 = this->pinnedCurves1[0];
			if (String::IsNullOrEmpty(this->activeCurve2) && this->pinnedCurves2->Count > 0)
				this->activeCurve2 = this->pinnedCurves2[0];

			UpdateChipStyles(1);
			UpdateChipStyles(2);
		}
		catch (...)
		{
			AddCurveChip(1, L"Разность фаз Δφ (Tx1..Tx5)");
			AddCurveChip(2, L"Затухание Att (AM Z2/Z1)");
			this->activeCurve1 = L"Разность фаз Δφ (Tx1..Tx5)";
			this->activeCurve2 = L"Затухание Att (AM Z2/Z1)";
			UpdateChipStyles(1);
			UpdateChipStyles(2);
		}
	}

	System::Void MainForm::MainForm_FormClosing(System::Object^ sender, System::Windows::Forms::FormClosingEventArgs^ e)
	{
		SaveStationConfig();
	}
}
