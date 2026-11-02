#pragma once

namespace ELECTROMAGNETIC_TOOLS_3_0 {

	using namespace System;
	using namespace System::Drawing;
	using namespace System::Drawing::Drawing2D;
	using namespace System::Windows::Forms;

	/// <summary>
	/// Схема зондов «Картограф» — owner-draw аналог panel_Sonde_Picture из AIR_TEMP_1.0.
	/// Стержень прибора с катушками Tx (чёрные) / Rx (синие); пары показаны
	/// сплющенными дугами-скобками: направленные Rx_L над стержнем, Rx_R под ним,
	/// ненаправленные Z1/Z2 — под стержнем. Подписи — базы пар в метрах
	/// (направленные: номиналы 0.96/1.68/2.28/3.00; ненаправленные: полусуммы
	/// (L1+L2)/2 из метрологии — T1=0.48, T2=0.84, T3=1.20, T4=1.56).
	/// Для GP-приборов (LWD_3Tx / LWD_4Tx_NEW / AUTONOM_5Tx(+SDR), Картограф LWD 349)
	/// рисуется упрощенная схема: нечётные Tx слева, чётные справа, приёмники
	/// Z1/Z2 по центру, дуги только ненаправленные, позиции пропорциональны базе.
	/// Клик по схеме переключает привязку к другому графику.
	/// Координаты элементов и пар — схематичные, перенесены из старой программы.
	/// Организация: НПФ «АМК Горизонт»
	/// Разработал: зам начальника ОП Смирнов С. Г.
	/// </summary>
	public ref class SondeSchemeControl : public System::Windows::Forms::Panel
	{
	public:
		delegate void SchemeClickedHandler(System::Object^ sender, System::EventArgs^ e);

		event SchemeClickedHandler^ SchemeClicked
		{
			void add(SchemeClickedHandler^ h) { _schemeClicked += h; }
			void remove(SchemeClickedHandler^ h) { _schemeClicked -= h; }
			void raise(System::Object^ s, System::EventArgs^ e)
			{
				if (_schemeClicked != nullptr) _schemeClicked(s, e);
			}
		}

		SondeSchemeControl()
		{
			this->DoubleBuffered = true;
			this->chartIndex = 0;
			this->isDirect = false;
			this->nTx = 4;
			this->carto = true;
			this->BackColor = Color::FromArgb(250, 250, 252);
			this->Cursor = Cursors::Hand;
		}

		void SetDeviceCaption(String^ caption)
		{
			if (this->deviceCaption == caption) return;
			this->deviceCaption = caption;
			this->Invalidate();
		}

		void SetState(int chartIdx, bool direct, int txCount, bool isCarto)
		{
			if (txCount < 0) txCount = 0;
			if (txCount > 5) txCount = 5;
			if (this->chartIndex != chartIdx || this->isDirect != direct ||
				this->nTx != txCount || this->carto != isCarto)
			{
				this->chartIndex = chartIdx;
				this->isDirect = direct;
				this->nTx = txCount;
				this->carto = isCarto;
				this->Cursor = isCarto ? Cursors::Hand : Cursors::Default;
				this->Invalidate();
			}
		}

	protected:
		virtual void OnMouseClick(System::Windows::Forms::MouseEventArgs^ e) override
		{
			System::Windows::Forms::Panel::OnMouseClick(e);
			if (e->Button == System::Windows::Forms::MouseButtons::Left && this->carto)
				SchemeClicked(this, System::EventArgs::Empty);
		}

		virtual void OnPaint(System::Windows::Forms::PaintEventArgs^ e) override
		{
			System::Windows::Forms::Panel::OnPaint(e);
			Graphics^ g = e->Graphics;
			g->SmoothingMode = System::Drawing::Drawing2D::SmoothingMode::AntiAlias;

			// Опорная ширина схемы из старой программы (макс. X элемента ~1071)
			float scale = (float)(this->Width - BADGE_W - 24) / 1080.0f;
			if (scale <= 0.0f) scale = 0.1f;
			this->_scale = scale;

			float cy = (float)this->Height * 0.52f;   // ось стержня

			Drawing::Font^ fontEl = gcnew Drawing::Font(L"Segoe UI", 7.5f, FontStyle::Bold);
			Drawing::Font^ fontBadge = gcnew Drawing::Font(L"Segoe UI", 8.25f, FontStyle::Bold);
			Drawing::Font^ fontMode = gcnew Drawing::Font(L"Segoe UI", 7.5f, FontStyle::Regular);
			Drawing::Font^ fontBase = gcnew Drawing::Font(L"Segoe UI", 7.0f, FontStyle::Bold);
			Drawing::Font^ fontRow = gcnew Drawing::Font(L"Segoe UI", 6.5f);
			StringFormat^ center = gcnew StringFormat();
			center->Alignment = StringAlignment::Center;
			center->LineAlignment = StringAlignment::Center;
			if (!String::IsNullOrEmpty(this->deviceCaption))
				g->DrawString(this->deviceCaption, fontBadge, Brushes::DimGray,
					RectangleF((float)BADGE_W + 10.0f, 0.0f,
						Math::Max(1.0f, (float)this->Width - BADGE_W - 20.0f), 20.0f));

			// ---- Бейдж привязки к графику (только для «Картографа») ----
			Color badgeColor = (chartIndex == 0) ? Color::FromArgb(70, 110, 190) : Color::FromArgb(190, 120, 50);
			if (carto)
			{
			GraphicsPath^ badgePath = RoundRect(5.0f, cy - 30.0f, (float)BADGE_W - 14.0f, 24.0f, 6.0f);
			g->FillPath(gcnew SolidBrush(badgeColor), badgePath);
			g->DrawString(String::Format(L"График {0}", chartIndex + 1), fontBadge,
				Brushes::White, RectangleF(5.0f, cy - 30.0f, (float)BADGE_W - 14.0f, 24.0f), center);
			g->DrawString((carto && isDirect) ? L"DIRECT" : L"UNDIRECT", fontMode,
				Brushes::DimGray, RectangleF(5.0f, cy - 4.0f, (float)BADGE_W - 14.0f, 15.0f), center);
			g->DrawString(L"клик — перекл.", gcnew Drawing::Font(L"Segoe UI", 6.5f),
				Brushes::Silver, RectangleF(5.0f, cy + 11.0f, (float)BADGE_W - 14.0f, 13.0f), center);
			}

			if (!carto)
			{
				DrawGP(g, cy, fontEl, fontRow);
				delete fontEl; delete fontBadge; delete fontMode; delete fontBase; delete fontRow;
				return;
			}

			// ---- Стержень прибора ----
			GraphicsPath^ rod = RoundRect(SX(0), cy - 9.0f, SX(1080) - SX(0), 18.0f, 9.0f);
			g->FillPath(gcnew SolidBrush(Color::FromArgb(232, 234, 238)), rod);
			g->DrawPath(gcnew Pen(Color::FromArgb(138, 143, 152), 1.2f), rod);

			// ---- Элементы: катушки Tx (чёрные) / Rx (синие) ----
			// R3  T3   T1   R1   R2   T5   T2    T4    R4
			array<float>^ elemX = { 15, 75, 339, 444, 504, 545, 742, 1009, 1065 };
			array<String^>^ elemN = { L"RL", L"T3", L"T1", L"R1", L"R2", L"T5", L"T2", L"T4", L"RR" };
			array<bool>^ elemRx = { true, false, false, true, true, false, false, false, true };
			// В UNDIRECT-режиме старая программа скрывала R3, R4, T5 — рисуем их бледными
			array<bool>^ elemUndir = { false, true, true, true, true, false, true, true, false };

			for (int i = 0; i < elemX->Length; i++)
			{
				bool active = isDirect || elemUndir[i];
				int alpha = active ? 255 : 60;
				Color c = elemRx[i] ? Color::FromArgb(alpha, 30, 90, 160)
									: Color::FromArgb(alpha, 40, 40, 40);
				DrawCoil(g, SX(elemX[i]), cy + 3.0f, c);
				g->DrawString(elemN[i], fontEl, gcnew SolidBrush(c),
					PointF(SX(elemX[i]) - 11.0f, cy - 27.0f));
			}

			// ---- Дуги пар Tx–Rx ----
			array<Color>^ probeColors = {
				Color::Red, Color::DarkGoldenrod, Color::Green, Color::RoyalBlue };

			if (isDirect)
			{
				// DIRECT: зеркальные пары (концы полос D1..D8 старой схемы).
				// Базы по концам: red=3.00 (R3–T4/T3–R4), gold=2.28,
				// blue=1.68 (R3–T5/T5–R4), green=0.96 (R3–T1/T2–R4)
				// Rx_L — над стержнем: красные дуги ниже, синие выше (без пересечения)
				DrawArc_(g, SX(20), SX(1011), cy - 11.0f, true, probeColors[0], L"3.00", 0, +0.55f);
				DrawArc_(g, SX(79), SX(1070), cy - 11.0f, true, probeColors[0], L"3.00", 0, -0.55f);
				DrawArc_(g, SX(16), SX(546), cy - 11.0f, true, probeColors[3], L"1.68", 2, -0.55f);
				DrawArc_(g, SX(550), SX(1071), cy - 11.0f, true, probeColors[3], L"1.68", 2, +0.55f);
				// Rx_R — под стержнем: зелёные дальше от стержня, золотые ближе
				DrawArc_(g, SX(15), SX(345), cy + 11.0f, false, probeColors[2], L"0.96", 2, -0.55f);
				DrawArc_(g, SX(742), SX(1076), cy + 11.0f, false, probeColors[2], L"0.96", 2, +0.55f);
				DrawArc_(g, SX(15), SX(749), cy + 11.0f, false, probeColors[1], L"2.28", 0, +0.55f);
				DrawArc_(g, SX(336), SX(1070), cy + 11.0f, false, probeColors[1], L"2.28", 0, -0.55f);
				g->DrawString(L"Rx_L", fontRow, Brushes::Gray, PointF(SX(1095), cy - 40.0f));
				g->DrawString(L"Rx_R", fontRow, Brushes::Gray, PointF(SX(1095), cy + 30.0f));
			}
			else
			{
				// UNDIRECT: пары Tx–центр (старые D9..D12), база = (L1+L2)/2
				DrawArc_(g, SX(340), SX(479), cy + 11.0f, false, probeColors[2], L"0.48", 0, +0.75f);
				DrawArc_(g, SX(479), SX(748), cy + 11.0f, false, probeColors[1], L"0.84", 1, -0.75f);
				DrawArc_(g, SX(79), SX(478), cy + 11.0f, false, probeColors[0], L"1.20", 0, +0.75f);
				DrawArc_(g, SX(479), SX(1008), cy + 11.0f, false, probeColors[3], L"1.56", 1, -0.75f);
				g->DrawString(L"Z1/Z2", fontRow, Brushes::Gray, PointF(SX(1095), cy + 30.0f));
			}

			delete fontEl; delete fontBadge; delete fontMode; delete fontBase; delete fontRow;
		}

	private:
		String^ deviceCaption;
		literal int BADGE_W = 92;
		int chartIndex;
		bool isDirect;
		int nTx;      // 0..5 для GP-приборов
		bool carto;   // true — схема «Картограф», false — GP
		float _scale;
		SchemeClickedHandler^ _schemeClicked;

		// Схема GP-прибора: нечётные Tx слева, чётные справа, Z1/Z2 в центре.
		// Позиции пропорциональны базе: T1 на сдвиге 0.48, шаг +0.36.
		void DrawGP(Graphics^ g, float cy, Drawing::Font^ fontEl, Drawing::Font^ fontRow)
		{
			// Базы (L1+L2)/2, м: T1..T5
			array<float>^ baseM = { 0.48f, 0.84f, 1.20f, 1.56f, 1.92f };
			array<String^>^ txN = { L"T1", L"T2", L"T3", L"T4", L"T5" };
			array<Color>^ txC = {
				Color::Green, Color::DarkGoldenrod, Color::Red,
				Color::RoyalBlue, Color::Magenta };

			int tx = (nTx <= 0) ? 4 : nTx;

			// Бейдж для GP не рисуется — схема по центру всей ширины
			float avail = (float)this->Width - 24.0f;
			float cx = 12.0f + avail / 2.0f;
			float mpx = (avail / 2.0f - 80.0f) / 2.0f;      // px на метр
			if (mpx < 30.0f) mpx = 30.0f;

			// Крайние Tx задают длину стержня
			float maxL = 0.48f, maxR = 0.84f;
			for (int i = 0; i < tx; i++)
			{
				bool oddTx = (i % 2 == 0);                  // T1,T3,T5 — слева
				float b = baseM[i];
				if (oddTx && b > maxL) maxL = b;
				if (!oddTx && b > maxR) maxR = b;
			}
			float rodX0 = cx - maxL * mpx - 55.0f;
			float rodX1 = cx + maxR * mpx + 55.0f;

			GraphicsPath^ rod = RoundRect(rodX0, cy - 9.0f, rodX1 - rodX0, 18.0f, 9.0f);
			g->FillPath(gcnew SolidBrush(Color::FromArgb(232, 234, 238)), rod);
			g->DrawPath(gcnew Pen(Color::FromArgb(138, 143, 152), 1.2f), rod);
			delete rod;

			// Приёмники Z1/Z2 — две синие катушки у центра
			Color rxC = Color::FromArgb(30, 90, 160);
			DrawCoil(g, cx - 14.0f, cy + 3.0f, rxC);
			DrawCoil(g, cx + 14.0f, cy + 3.0f, rxC);
			g->DrawString(L"Z1", fontEl, gcnew SolidBrush(rxC), PointF(cx - 25.0f, cy - 27.0f));
			g->DrawString(L"Z2", fontEl, gcnew SolidBrush(rxC), PointF(cx + 3.0f, cy - 27.0f));

			// Передатчики и дуги Tx -> центр
			Color txCol = Color::FromArgb(40, 40, 40);
			int li = 0, ri = 0;
			for (int i = 0; i < tx; i++)
			{
				bool oddTx = (i % 2 == 0);
				float x = cx + (oddTx ? -1.0f : 1.0f) * baseM[i] * mpx;
				DrawCoil(g, x, cy + 3.0f, txCol);
				g->DrawString(txN[i], fontEl, gcnew SolidBrush(txCol),
					PointF(x - 11.0f, cy - 27.0f));

				float x0 = Math::Min(x, cx), x1 = Math::Max(x, cx);
				int k = oddTx ? li++ : ri++;                 // разнос по высоте
				float shoulder = oddTx ? -0.6f : 0.6f;       // подпись к внешнему плечу
				String^ lb = baseM[i].ToString(L"0.00",
					System::Globalization::CultureInfo::InvariantCulture);
				DrawArc_(g, x0, x1, cy + 11.0f, false, txC[i], lb, k, shoulder);
			}

			g->DrawString(L"Z1/Z2", fontRow, Brushes::Gray, PointF(rodX1 + 8.0f, cy + 22.0f));
		}

		float SX(float x) { return (float)BADGE_W + 10.0f + x * _scale; }

		GraphicsPath^ RoundRect(float x, float y, float w, float h, float r)
		{
			GraphicsPath^ p = gcnew GraphicsPath();
			float d = r * 2.0f;
			p->AddArc(x, y, d, d, 180.0f, 90.0f);
			p->AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
			p->AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
			p->AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
			p->CloseFigure();
			return p;
		}

		// Катушка: три полуокружности + линия основания
		void DrawCoil(Graphics^ g, float x, float y, Color c)
		{
			Pen^ pen = gcnew Pen(c, 1.5f);
			for (int k = -1; k <= 1; k++)
				g->DrawArc(pen, x + k * 4.5f - 3.0f, y - 10.0f, 6.0f, 10.0f, 180.0f, 180.0f);
			g->DrawLine(pen, x - 7.5f, y, x + 7.5f, y);
			delete pen;
		}

		// Сплющенная дуга-скобка пары; подпись на плече дуги
		void DrawArc_(Graphics^ g, float x0, float x1, float yb, bool up,
			Color c, String^ baseLabel, int k, float shoulder)
		{
			float w = x1 - x0;
			if (w < 8.0f) return;
			float cx = (x0 + x1) / 2.0f;
			float h = Math::Min(5.0f + w / 55.0f, 22.0f) + k * 2.5f;

			Pen^ pen = gcnew Pen(c, 2.4f);
			pen->StartCap = LineCap::Round;
			pen->EndCap = LineCap::Round;
			RectangleF rect(x0, yb - h, w, h * 2.0f);
			g->DrawArc(pen, rect, up ? 180.0f : 0.0f, 180.0f);
			delete pen;

			// Подпись базы на плече дуги (t: pi/2 — вершина, ±pi/3 — плечо)
			double t = Math::PI / 2.0 + (double)shoulder * Math::PI / 3.0;
			float tx = cx + (w / 2.0f) * (float)Math::Cos(t);
			float ty = yb + (up ? -1.0f : 1.0f) * h * (float)Math::Sin(t);
			Drawing::Font^ f = gcnew Drawing::Font(L"Segoe UI", 7.0f, FontStyle::Bold);
			g->DrawString(baseLabel, f, gcnew SolidBrush(c),
				PointF(tx - 12.0f, ty + (up ? -12.0f : 5.0f)));
			delete f;
		}
	};
}
