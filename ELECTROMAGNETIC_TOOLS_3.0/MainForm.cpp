#include "pch.h"
#include "MainForm.h"
#include "MetroForm.h"
#include <iostream>
#include <cmath>

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
		ca->AxisX->Title = L"Время (с)";
		ca->AxisX->TitleFont = gcnew Drawing::Font(L"Segoe UI", 8.5F, FontStyle::Bold);
		ca->AxisY->Title = titleY;
		ca->AxisY->TitleFont = gcnew Drawing::Font(L"Segoe UI", 8.5F, FontStyle::Bold);

		ca->CursorX->IsUserEnabled = true;
		ca->CursorX->IsUserSelectionEnabled = false;
		ca->CursorX->LineColor = Color::Crimson;
		ca->CursorX->LineWidth = 2;
		ca->CursorX->LineDashStyle = ChartDashStyle::Solid;

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

		// Инициализация списков закрепленных кнопок
		this->pinnedCurves1 = gcnew List<String^>();
		this->pinnedCurves2 = gcnew List<String^>();
		this->isDirectMode1 = false;
		this->isDirectMode2 = false;
		this->isStreaming = false;
		this->currentStreamPoint = 500;
		this->isSyncingSplitter = false;

		// Начальные чипы по умолчанию для UNDIRECT
		AddCurveChip(1, L"Фаза Δφ (PH Z2-Z1)");
		AddCurveChip(1, L"Затухание Att (AM Z2/Z1)");
		AddCurveChip(1, L"УЭС по фазе (ρ_ph)");

		AddCurveChip(2, L"Затухание Att (AM Z2/Z1)");
		AddCurveChip(2, L"УЭС по затуханию (ρ_att)");

		// Заполнение тестовыми данными (500 точек)
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

		UpdateTimeSlices(0.0);
		UpdateCursorReadout(0.0);
	}

	void MainForm::UpdateTimeSlices(double timeVal)
	{
		this->chartSectorRxL->Series[L"SectorsL"]->Points->Clear();
		this->chartSectorRxL->Series[L"H1_FitL"]->Points->Clear();
		this->chartSectorRxR->Series[L"SectorsR"]->Points->Clear();
		this->chartSectorRxR->Series[L"H1_FitR"]->Points->Clear();

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

		UpdateCursorReadout(timeVal);
	}

	void MainForm::UpdateCursorReadout(double timeVal)
	{
		int frameNum = static_cast<int>(Math::Round(timeVal * 10.0));
		if (frameNum < 0) frameNum = 0;

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
				v5 = this->chartTime1->Series[4]->Points[ptIdx]->YValues[0];
			}
		}

		this->labelCursorInfo->Text = String::Format(
			L"Точка #{0} (t = {1:F1} с) | Tx1: {2:F1} | Tx2: {3:F1} | Tx3: {4:F1} | Tx4: {5:F1} | Tx5: {6:F1}",
			frameNum, timeVal, v1, v2, v3, v4, v5);
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
			if (Double::TryParse(tbMin->Text->Trim(), pt))
			{
				minVal = pt * 0.1;
			}
		}

		if (!String::IsNullOrWhiteSpace(tbMax->Text))
		{
			double pt = 0.0;
			if (Double::TryParse(tbMax->Text->Trim(), pt))
			{
				maxVal = pt * 0.1;
			}
		}

		if (!Double::IsNaN(minVal) && !Double::IsNaN(maxVal) && minVal >= maxVal)
		{
			double tmp = minVal;
			minVal = maxVal;
			maxVal = tmp;
		}

		chart->ChartAreas[0]->AxisX->Minimum = minVal;
		chart->ChartAreas[0]->AxisX->Maximum = maxVal;
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
		chip->BackColor = Color::FromArgb(235, 240, 250);
		chip->BorderStyle = BorderStyle::FixedSingle;
		chip->Margin = System::Windows::Forms::Padding(2, 1, 2, 1);
		chip->Tag = curveName;

		Label^ lbl = gcnew Label();
		lbl->Text = curveName;
		lbl->AutoSize = true;
		lbl->Font = gcnew Drawing::Font(L"Segoe UI", 8.25F, FontStyle::Bold);
		lbl->ForeColor = Color::FromArgb(30, 60, 120);
		lbl->Location = Point(4, 4);
		lbl->Cursor = Cursors::Hand;
		chip->Controls->Add(lbl);

		Button^ btnX = gcnew Button();
		btnX->Text = L"✕";
		btnX->Size = Drawing::Size(18, 18);
		btnX->Location = Point(lbl->Right + 4, 3);
		btnX->FlatStyle = FlatStyle::Flat;
		btnX->FlatAppearance->BorderSize = 0;
		btnX->Font = gcnew Drawing::Font(L"Segoe UI", 7, FontStyle::Bold);
		btnX->ForeColor = Color::DarkRed;
		btnX->Tag = chip;
		btnX->Click += gcnew EventHandler(this, &MainForm::ChipCloseButton_Click);

		chip->Controls->Add(btnX);
		panel->Controls->Add(chip);
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
	}

	System::Void MainForm::CurveMenuItem_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ToolStripMenuItem^ item = dynamic_cast<ToolStripMenuItem^>(sender);
		if (item != nullptr && item->Tag != nullptr)
		{
			int wIdx = safe_cast<int>(item->Tag);
			AddCurveChip(wIdx, item->Text);
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
		}
		else
		{
			AddCurveChip(1, L"Фаза Δφ (PH Z2-Z1)");
			AddCurveChip(1, L"Затухание Att (AM Z2/Z1)");
			AddCurveChip(1, L"УЭС по фазе (ρ_ph)");
		}
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
		}
		else
		{
			AddCurveChip(2, L"Затухание Att (AM Z2/Z1)");
			AddCurveChip(2, L"УЭС по затуханию (ρ_att)");
		}
	}

	System::Void MainForm::buttonFreq1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreq1->Text == L"2000 кГц")
			this->buttonFreq1->Text = L"400 кГц";
		else
			this->buttonFreq1->Text = L"2000 кГц";
	}

	System::Void MainForm::buttonFreq2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreq2->Text == L"400 кГц")
			this->buttonFreq2->Text = L"2000 кГц";
		else
			this->buttonFreq2->Text = L"400 кГц";
	}

	System::Void MainForm::buttonFreqRxL_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreqRxL->Text == L"2000 кГц")
			this->buttonFreqRxL->Text = L"400 кГц";
		else
			this->buttonFreqRxL->Text = L"2000 кГц";
	}

	System::Void MainForm::buttonFreqRxR_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->buttonFreqRxR->Text == L"2000 кГц")
			this->buttonFreqRxR->Text = L"400 кГц";
		else
			this->buttonFreqRxR->Text = L"2000 кГц";
	}

	System::Void MainForm::comboBoxTxRx_SelectedIndexChanged(System::Object^ sender, System::EventArgs^ e)
	{
		UpdateTimeSlices(this->chartTime1->ChartAreas[0]->CursorX->Position);
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
		}
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
		}
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
		Chart^ chart = dynamic_cast<Chart^>(sender);
		if (chart == nullptr || chart->ChartAreas->Count == 0) return;

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

	System::Void MainForm::chartTime_MouseMove(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
	{
		if (e->Button == System::Windows::Forms::MouseButtons::Left)
		{
			chartTime_MouseDown(sender, e);
		}
	}

	System::Void MainForm::buttonConnect_Click(System::Object^ sender, System::EventArgs^ e)
	{
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

	System::Void MainForm::buttonExportCsv_Click(System::Object^ sender, System::EventArgs^ e)
	{
		MessageBox::Show(
			L"Экспорт калибровочных данных в формат RAW CSV для внешнего мастера калибровки Python.",
			L"Экспорт CSV",
			MessageBoxButtons::OK,
			MessageBoxIcon::Information
		);
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
			this->statusLabelFile->Text = L"Файл: " + this->openFileDialogBin->FileName;
			std::cout << "[FILE] Выбран файл данных: " << std::endl;
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
			L"Версия: 3.0.0 (Visual Studio 2026, MSVC v145, x64, .NET 4.8)",
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
		this->textBoxMin1->Clear();
		this->textBoxMax1->Clear();
		ApplyRange(1);
	}

	System::Void MainForm::buttonApplyRange2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		ApplyRange(2);
	}

	System::Void MainForm::buttonResetRange2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->textBoxMin2->Clear();
		this->textBoxMax2->Clear();
		ApplyRange(2);
	}

	System::Void MainForm::textBoxRange_KeyPress(System::Object^ sender, System::Windows::Forms::KeyPressEventArgs^ e)
	{
		if (!Char::IsDigit(e->KeyChar) && !Char::IsControl(e->KeyChar))
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
		int pt = static_cast<int>(Math::Round(curPos * 10.0));
		this->textBoxMin1->Text = pt.ToString();
		ApplyRange(1);
	}

	System::Void MainForm::menuItemSetMax1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		double curPos = this->chartTime1->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(curPos) || curPos < 0) curPos = 0;
		int pt = static_cast<int>(Math::Round(curPos * 10.0));
		this->textBoxMax1->Text = pt.ToString();
		ApplyRange(1);
	}

	System::Void MainForm::menuItemResetRange1_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->textBoxMin1->Clear();
		this->textBoxMax1->Clear();
		ApplyRange(1);
	}

	System::Void MainForm::menuItemSetMin2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		double curPos = this->chartTime2->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(curPos) || curPos < 0) curPos = 0;
		int pt = static_cast<int>(Math::Round(curPos * 10.0));
		this->textBoxMin2->Text = pt.ToString();
		ApplyRange(2);
	}

	System::Void MainForm::menuItemSetMax2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		double curPos = this->chartTime2->ChartAreas[0]->CursorX->Position;
		if (Double::IsNaN(curPos) || curPos < 0) curPos = 0;
		int pt = static_cast<int>(Math::Round(curPos * 10.0));
		this->textBoxMax2->Text = pt.ToString();
		ApplyRange(2);
	}

	System::Void MainForm::menuItemResetRange2_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->textBoxMin2->Clear();
		this->textBoxMax2->Clear();
		ApplyRange(2);
	}
}
