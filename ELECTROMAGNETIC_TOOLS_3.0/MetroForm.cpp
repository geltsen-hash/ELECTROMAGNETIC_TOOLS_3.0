#include "pch.h"
#include "MetroForm.h"
#include "SondeDataTypes.h"
#include "ToolProtocol.h"
#include "BinFileReader.h"
#include <msclr/lock.h>
#include <msclr/marshal_cppstd.h>
#include <fstream>
#include <iostream>

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

	void MetroForm::InitDefaultValues(void)
	{
		// Полный список типов приборов (как таблица SondeID, не только Картографы)
		this->comboBoxSondeType->Items->Clear();
		const uint32_t types[] = { CARTOGRAPH_, CARTOGRAPH_RAW_, CARTOGRAPH_LWD_4Tx_,
		                           LWD_4Tx_NEW_, LWD_3Tx_, AUTONOM_5Tx_, AUTONOM_5Tx_SDR_ };
		for (uint32_t t : types)
		{
			SondeID id; id.type = t;
			this->comboBoxSondeType->Items->Add(
				t.ToString() + L": " + Utf8ToManaged(id.GetTypeName().c_str()));
		}
		this->comboBoxSondeType->SelectedIndex = 0;
		this->comboBoxRotDir->SelectedIndex = 0;
	}

	void MetroForm::ApplySondeSignature(uint32_t signature)
	{
		SondeID id = SondeID::FromSignature(signature);

		// Паспорт — сигнатура десятичная, серийник и размер из сигнатуры
		this->textBoxSignature->Text = signature.ToString();
		this->textBoxSerial->Text = id.number.ToString();
		this->textBoxStructSize->Text = id.struct_size.ToString();

		String^ typeName = Utf8ToManaged(id.GetTypeName().c_str());
		this->statusLabelSondeType->Text =
			L"Тип зонда: " + id.type.ToString() + L" (" + typeName + L")";

		for (int i = 0; i < this->comboBoxSondeType->Items->Count; i++)
		{
			if (this->comboBoxSondeType->Items[i]->ToString()->StartsWith(
				id.type.ToString() + L":"))
			{
				this->comboBoxSondeType->SelectedIndex = i;
				break;
			}
		}

		// Как sells_load в старой программе: клинометрия и ротация — только Картограф
		bool cart = is_cartograph(id.type);
		if (!cart)
			this->tabControlMetro->TabPages->Remove(this->tabPageInclinometer);

		// Lgeo есть только в METROLOGY_CARTOGRAPH — у ненаправленных колонка скрыта
		this->labelHdrLgeo->Visible = cart;
		array<TextBox^>^ tbLgeo = { textBoxLgeo_T1, textBoxLgeo_T2, textBoxLgeo_T3, textBoxLgeo_T4, textBoxLgeo_T5 };
		for each (TextBox^ tb in tbLgeo) tb->Visible = cart;
	}

	// Все редактируемые поля метрологии — пустые, если данных нет (как у старой:
	// без прочитанной структуры шаблонные значения не показываются)
	void MetroForm::ClearAllFields(void)
	{
		array<TextBox^>^ all = {
			textBoxDiameter, textBoxFreq1, textBoxFreq2, textBoxStructSize,
			textBoxL1_T1, textBoxL1_T2, textBoxL1_T3, textBoxL1_T4, textBoxL1_T5,
			textBoxL2_T1, textBoxL2_T2, textBoxL2_T3, textBoxL2_T4, textBoxL2_T5,
			textBoxLgeo_T1, textBoxLgeo_T2, textBoxLgeo_T3, textBoxLgeo_T4, textBoxLgeo_T5,
			textBoxAirPh400_T1, textBoxAirPh400_T2, textBoxAirPh400_T3, textBoxAirPh400_T4, textBoxAirPh400_T5,
			textBoxAirAmp400_T1, textBoxAirAmp400_T2, textBoxAirAmp400_T3, textBoxAirAmp400_T4, textBoxAirAmp400_T5,
			textBoxAirPh2000_T1, textBoxAirPh2000_T2, textBoxAirPh2000_T3, textBoxAirPh2000_T4, textBoxAirPh2000_T5,
			textBoxAirAmp2000_T1, textBoxAirAmp2000_T2, textBoxAirAmp2000_T3, textBoxAirAmp2000_T4, textBoxAirAmp2000_T5,
			textBoxMin400_T1, textBoxMin400_T2, textBoxMin400_T3, textBoxMin400_T4, textBoxMin400_T5,
			textBoxMin2000_T1, textBoxMin2000_T2, textBoxMin2000_T3, textBoxMin2000_T4, textBoxMin2000_T5,
			textBoxMinRotSpeed, textBoxAngleHistory, textBoxFilterK };
		for each (TextBox^ tb in all) tb->Text = L"";
	}

	void MetroForm::LoadSessionMetrology(void)
	{
		ClearAllFields();
		if (!SondeData::HasMetrology())
		{
			this->statusLabelMetroFile->Text = L"Метрология: не загружена";
			this->labelStatusNotice->Text = L"Метрология не загружена — поля пустые";
			return;
		}
		if (SondeData::IsMetrologyCartograph())
		{
			const METROLOGY_CARTOGRAPH* m = SondeData::GetMetrologyCartograph();
			FillFromMetrology((const METROLOGY_GP*)m, true, m->L_geo_mm);
		}
		else
		{
			FillFromMetrology(SondeData::GetMetrologyGP(), false, nullptr);
		}
		ApplySondeSignature(SondeData::GetMetrologySignature());
		this->statusLabelMetroFile->Text = L"Метрология: загружена (зонд/файл)";
		this->labelStatusNotice->Text = L"Данные метрологии загружены";
	}

	bool MetroForm::SelectedIsCartograph(void)
	{
		Int64 sigVal = 0;
		if (Int64::TryParse(this->textBoxSignature->Text, sigVal) && sigVal > 0)
			return is_cartograph(SondeID::FromSignature((uint32_t)sigVal).type);
		String^ item = this->comboBoxSondeType->SelectedItem != nullptr
			? this->comboBoxSondeType->SelectedItem->ToString() : L"";
		return item->StartsWith(L"352") || item->StartsWith(L"351");
	}

	// Общий разбор полей в структуру (как validate_and_fill в старой программе).
	// Начинаем с текущей структуры сессии — служебные поля/timestamp сохраняются.
	bool MetroForm::CollectMetrologyGP(METROLOGY_GP* out,
		System::Collections::Generic::List<String^>^ errors)
	{
		const METROLOGY_GP* base = SondeData::GetMetrologyGP();
		if (base != nullptr) *out = *base; else memset(out, 0, sizeof(*out));

		Int64 v = 0; double d = 0;
		if (Int64::TryParse(this->textBoxSignature->Text, v)) out->signature = (uint32_t)v;
		// Размер структуры — часть сигнатуры (struct_size·10⁶), как get_signature у старой
		if (Int64::TryParse(this->textBoxStructSize->Text, v) && v > 0 && v < 1000)
			out->signature = (uint32_t)v * 1000000 + out->signature % 1000000;
		if (Int64::TryParse(this->textBoxSerial->Text, v)) out->serial = (uint32_t)v;
		if (Int64::TryParse(this->textBoxDiameter->Text, v)) out->D_sonde_mm = (uint32_t)v;
		if (!Int64::TryParse(this->textBoxFreq1->Text, v)) errors->Add(L"Частота 1: не число");
		else out->F[0] = (uint16_t)v;
		if (!Int64::TryParse(this->textBoxFreq2->Text, v)) errors->Add(L"Частота 2: не число");
		else out->F[1] = (uint16_t)v;

		array<TextBox^>^ tbL1 = { textBoxL1_T1, textBoxL1_T2, textBoxL1_T3, textBoxL1_T4, textBoxL1_T5 };
		array<TextBox^>^ tbL2 = { textBoxL2_T1, textBoxL2_T2, textBoxL2_T3, textBoxL2_T4, textBoxL2_T5 };
		array<array<TextBox^>^>^ tbAirPh = {
			gcnew array<TextBox^>{ textBoxAirPh400_T1, textBoxAirPh400_T2, textBoxAirPh400_T3, textBoxAirPh400_T4, textBoxAirPh400_T5 },
			gcnew array<TextBox^>{ textBoxAirPh2000_T1, textBoxAirPh2000_T2, textBoxAirPh2000_T3, textBoxAirPh2000_T4, textBoxAirPh2000_T5 } };
		array<array<TextBox^>^>^ tbAirAmp = {
			gcnew array<TextBox^>{ textBoxAirAmp400_T1, textBoxAirAmp400_T2, textBoxAirAmp400_T3, textBoxAirAmp400_T4, textBoxAirAmp400_T5 },
			gcnew array<TextBox^>{ textBoxAirAmp2000_T1, textBoxAirAmp2000_T2, textBoxAirAmp2000_T3, textBoxAirAmp2000_T4, textBoxAirAmp2000_T5 } };
		array<array<TextBox^>^>^ tbMin = {
			gcnew array<TextBox^>{ textBoxMin400_T1, textBoxMin400_T2, textBoxMin400_T3, textBoxMin400_T4, textBoxMin400_T5 },
			gcnew array<TextBox^>{ textBoxMin2000_T1, textBoxMin2000_T2, textBoxMin2000_T3, textBoxMin2000_T4, textBoxMin2000_T5 } };

		String^ inv = System::Globalization::CultureInfo::InvariantCulture->NumberFormat->NumberDecimalSeparator;
		for (int tx = 0; tx < 5; tx++)
		{
			if (String::IsNullOrWhiteSpace(tbL1[tx]->Text)) continue;
			if (Int64::TryParse(tbL1[tx]->Text, v)) out->L1[tx] = (uint16_t)v;
			else errors->Add(L"L1 Tx" + (tx + 1) + L": не число");
			if (Int64::TryParse(tbL2[tx]->Text, v)) out->L2[tx] = (uint16_t)v;
			else errors->Add(L"L2 Tx" + (tx + 1) + L": не число");
			for (int f = 0; f < 2; f++)
			{
				String^ fr = (f == 0) ? L"400" : L"2000";
				if (Int64::TryParse(tbAirPh[f][tx]->Text, v)) out->air_ph[f][tx] = (int16_t)v;
				else errors->Add(L"air_ph " + fr + L" Tx" + (tx + 1) + L": не число");
				String^ t = tbAirAmp[f][tx]->Text->Replace(L",", inv);
				if (Double::TryParse(t, System::Globalization::NumberStyles::Float,
					System::Globalization::CultureInfo::InvariantCulture, d))
					out->air_att_dB[f][tx] = (float)d;
				else errors->Add(L"air_att_dB " + fr + L" Tx" + (tx + 1) + L": не число");
				if (Int64::TryParse(tbMin[f][tx]->Text, v)) out->min_amp[f][tx] = (int16_t)v;
				else errors->Add(L"min_amp " + fr + L" Tx" + (tx + 1) + L": не число");
			}
		}
		out->timestamp = time(nullptr);
		return errors->Count == 0;
	}

	bool MetroForm::CollectMetrologyCartograph(METROLOGY_CARTOGRAPH* out,
		System::Collections::Generic::List<String^>^ errors)
	{
		const METROLOGY_CARTOGRAPH* base = SondeData::GetMetrologyCartograph();
		if (base != nullptr) *out = *base; else memset(out, 0, sizeof(*out));

		// Общая часть — через сборку GP-префикса
		METROLOGY_GP gp;
		if (!CollectMetrologyGP(&gp, errors)) return false;
		memcpy(out, &gp, sizeof(METROLOGY_GP));

		array<TextBox^>^ tbLgeo = { textBoxLgeo_T1, textBoxLgeo_T2, textBoxLgeo_T3, textBoxLgeo_T4, textBoxLgeo_T5 };
		Int64 v = 0;
		for (int tx = 0; tx < 5; tx++)
		{
			if (String::IsNullOrWhiteSpace(tbLgeo[tx]->Text)) continue;
			if (Int64::TryParse(tbLgeo[tx]->Text, v)) out->L_geo_mm[tx] = (uint32_t)v;
			else { errors->Add(L"Lgeo Tx" + (tx + 1) + L": не число"); return false; }
		}
		return true;
	}

	// «Открыть файл» — как buttonOpen_file_Click: сигнатура файла решает структуру
	System::Void MetroForm::buttonOpenFile_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->openFileDialogMetro->Filter = L"metrology files (*.bin)|*.bin";
		if (this->openFileDialogMetro->ShowDialog() != System::Windows::Forms::DialogResult::OK)
			return;

		std::string path = msclr::interop::marshal_as<std::string>(this->openFileDialogMetro->FileName);
		std::ifstream in(path, std::ios::binary);
		if (!in.is_open())
		{
			MessageBox::Show(this, L"Не удалось открыть файл метрологии",
				L"Открыть файл", MessageBoxButtons::OK, MessageBoxIcon::Error);
			return;
		}
		uint32_t signature = 0;
		in.read((char*)&signature, sizeof(signature));
		SondeID id = SondeID::FromSignature(signature);

		bool ok = false;
		if (is_cartograph(id.type))
		{
			METROLOGY_CARTOGRAPH m;
			in.seekg(0); in.read((char*)&m, sizeof(m));
			ok = in.good() && SondeData::SetMetrology((const uint8_t*)&m, sizeof(m));
		}
		else
		{
			METROLOGY_GP m;
			in.seekg(0); in.read((char*)&m, sizeof(m));
			ok = in.good() && SondeData::SetMetrology((const uint8_t*)&m, sizeof(m));
		}
		in.close();

		if (!ok)
		{
			this->labelStatusNotice->Text = L"Файл прочитан не полностью";
			MessageBox::Show(this, L"Файл короче структуры метрологии этого типа прибора.",
				L"Открыть файл", MessageBoxButtons::OK, MessageBoxIcon::Warning);
			return;
		}
		LoadSessionMetrology();
		this->statusLabelMetroFile->Text = L"Файл метрологии: " + this->openFileDialogMetro->FileName;
		std::cout << "[METRO] Файл метрологии загружен: " << path << std::endl;
	}

	// «Сохранить в файл» — как buttonWrite_tofile_Click: сборка структуры → .bin
	System::Void MetroForm::buttonSaveFile_Click(System::Object^ sender, System::EventArgs^ e)
	{
		auto errors = gcnew System::Collections::Generic::List<String^>();
		bool cart = SelectedIsCartograph();
		METROLOGY_CARTOGRAPH cart_m;
		METROLOGY_GP gp_m;
		if (cart)
		{
			if (!CollectMetrologyCartograph(&cart_m, errors)) { ShowErrors(errors, L"запись в файл"); return; }
		}
		else
		{
			if (!CollectMetrologyGP(&gp_m, errors)) { ShowErrors(errors, L"запись в файл"); return; }
		}

		this->saveFileDialogMetro->Filter = L"metrology files (*.bin)|*.bin";
		if (this->saveFileDialogMetro->ShowDialog() != System::Windows::Forms::DialogResult::OK)
			return;

		std::string path = msclr::interop::marshal_as<std::string>(this->saveFileDialogMetro->FileName);
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!out.is_open())
		{
			MessageBox::Show(this, L"Не удалось создать файл метрологии",
				L"Сохранить в файл", MessageBoxButtons::OK, MessageBoxIcon::Error);
			return;
		}
		if (cart)
		{
			out.write((const char*)&cart_m, sizeof(cart_m));
			SondeData::SetMetrologyCartograph(&cart_m);
		}
		else
		{
			out.write((const char*)&gp_m, sizeof(gp_m));
			SondeData::SetMetrologyGP(&gp_m);
		}
		out.close();
		this->statusLabelMetroFile->Text = L"Файл сохранен: " + this->saveFileDialogMetro->FileName;
		this->labelStatusNotice->Text = L"Файл метрологии сохранен";
		std::cout << "[METRO] Файл метрологии сохранен: " << path << std::endl;
	}

	void MetroForm::FillFromMetrology(const METROLOGY_GP* metro, bool hasLgeo, const uint32_t* Lgeo)
	{
		cli::array<TextBox^>^ tbL1 = { textBoxL1_T1, textBoxL1_T2, textBoxL1_T3, textBoxL1_T4, textBoxL1_T5 };
		cli::array<TextBox^>^ tbL2 = { textBoxL2_T1, textBoxL2_T2, textBoxL2_T3, textBoxL2_T4, textBoxL2_T5 };
		cli::array<TextBox^>^ tbLgeo = { textBoxLgeo_T1, textBoxLgeo_T2, textBoxLgeo_T3, textBoxLgeo_T4, textBoxLgeo_T5 };
		cli::array<cli::array<TextBox^>^>^ tbAirPh = {
			gcnew cli::array<TextBox^>{ textBoxAirPh400_T1, textBoxAirPh400_T2, textBoxAirPh400_T3, textBoxAirPh400_T4, textBoxAirPh400_T5 },
			gcnew cli::array<TextBox^>{ textBoxAirPh2000_T1, textBoxAirPh2000_T2, textBoxAirPh2000_T3, textBoxAirPh2000_T4, textBoxAirPh2000_T5 } };
		cli::array<cli::array<TextBox^>^>^ tbAirAmp = {
			gcnew cli::array<TextBox^>{ textBoxAirAmp400_T1, textBoxAirAmp400_T2, textBoxAirAmp400_T3, textBoxAirAmp400_T4, textBoxAirAmp400_T5 },
			gcnew cli::array<TextBox^>{ textBoxAirAmp2000_T1, textBoxAirAmp2000_T2, textBoxAirAmp2000_T3, textBoxAirAmp2000_T4, textBoxAirAmp2000_T5 } };
		cli::array<cli::array<TextBox^>^>^ tbMin = {
			gcnew cli::array<TextBox^>{ textBoxMin400_T1, textBoxMin400_T2, textBoxMin400_T3, textBoxMin400_T4, textBoxMin400_T5 },
			gcnew cli::array<TextBox^>{ textBoxMin2000_T1, textBoxMin2000_T2, textBoxMin2000_T3, textBoxMin2000_T4, textBoxMin2000_T5 } };

		this->textBoxSerial->Text = metro->serial.ToString();
		this->textBoxDiameter->Text = metro->D_sonde_mm.ToString();
		this->textBoxFreq1->Text = metro->F[0].ToString();
		this->textBoxFreq2->Text = metro->F[1].ToString();

		for (int tx = 0; tx < 5; tx++)
		{
			tbL1[tx]->Text = metro->L1[tx].ToString();
			tbL2[tx]->Text = metro->L2[tx].ToString();
			tbLgeo[tx]->Text = hasLgeo ? Lgeo[tx].ToString() : L"";
			for (int f = 0; f < 2; f++)
			{
				tbAirPh[f][tx]->Text = metro->air_ph[f][tx].ToString();
				tbAirAmp[f][tx]->Text = metro->air_att_dB[f][tx].ToString();
				tbMin[f][tx]->Text = metro->min_amp[f][tx].ToString();
			}
		}
	}

	void MetroForm::ShowErrors(System::Collections::Generic::List<String^>^ errors, String^ action)
	{
		String^ msg = L"Обнаружены некорректные значения параметров (" + action + L" заблокирована):\n\n";
		for (int i = 0; i < Math::Min(errors->Count, 15); i++)
			msg += L"• " + errors[i] + L"\n";
		if (errors->Count > 15)
			msg += String::Format(L"... и еще {0} ошибок.\n", errors->Count - 15);
		MessageBox::Show(this, msg, L"Ошибка валидации параметров",
			MessageBoxButtons::OK, MessageBoxIcon::Error);
	}

	// Чтение метрологии из зонда — как read_metro_from_tool в старой программе:
	// CMD_GET_METROLOGY, ответ разбирается по сигнатуре (GP 240 Б / Картограф)
	System::Void MetroForm::buttonReadFromSonde_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->sondeLink == nullptr || !this->sondeLink->IsOpen)
		{
			this->labelStatusNotice->Text = L"Нет связи с прибором";
			MessageBox::Show(this,
				L"Чтение метрологии требует активного подключения к прибору.\n"
				L"Сначала выполните «Связь → Подключить».",
				L"Нет связи", MessageBoxButtons::OK, MessageBoxIcon::Information);
			return;
		}

		uint8_t cmd = 0;
		uint16_t dataLen = 0;
		uint8_t dataIn[2048] = { 0 };
		int result = 1;
		this->labelStatusNotice->Text = L"Чтение метрологии из зонда...";
		this->Refresh();

		// Обмен под общим замком канала — не пересекаемся с фоновым опросом телеметрии
		{
			msclr::lock linkLock(this->sondeLink->SyncRoot);
			int prevTimeout = this->sondeLink->ReadTimeout;
			this->sondeLink->ReadTimeout = 2000;
			uint8_t d0 = 0;
			if (ToolProtocol::SendToTool(this->sondeLink, CMD_GET_METROLOGY, &d0, 0) == 0)
			{
				System::Threading::Thread::Sleep(500); // как Sleep(500) в read_metro_from_tool
				result = ToolProtocol::ReceiveFromTool(this->sondeLink, &cmd, dataIn, &dataLen);
			}
			this->sondeLink->ReadTimeout = prevTimeout;
		}

		if (result != 0 || dataLen < (uint16_t)sizeof(METROLOGY_GP))
		{
			this->labelStatusNotice->Text = L"Зонд не ответил (таймаут/CRC)";
			MessageBox::Show(this,
				L"Не удалось прочитать метрологию из зонда.\nКод ошибки транспорта: " + result.ToString() +
				L", принято байт: " + dataLen.ToString(),
				L"Чтение из зонда", MessageBoxButtons::OK, MessageBoxIcon::Warning);
			return;
		}

		// Структура в сессию (расчёты на компьютере) и на форму — как fill_the_form
		if (SondeData::SetMetrology(dataIn, dataLen))
		{
			LoadSessionMetrology();
			this->statusLabelMetroFile->Text = L"Метрология: прочитана из зонда";
			this->labelStatusNotice->Text = L"Метрология прочитана из зонда";
			MessageBox::Show(this, L"ФАЙЛ МЕТРОЛОГИИ СЧИТАН ИЗ ПРИБОРА!",
				L"Чтение из зонда", MessageBoxButtons::OK, MessageBoxIcon::Information);
			std::cout << "[METRO] Прочитано из зонда: " << dataLen
				<< " байт, signature=" << *(uint32_t*)dataIn << std::endl;
		}
	}

	// «Записать в зонд» — как buttonWrite_to_sonde_Click: валидация → «УВЕРЕНЫ?» →
	// CMD_SET_METROLOGY (структура по типу прибора), ответ — ACK прибора
	System::Void MetroForm::buttonWriteToSonde_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->checkBoxReadOnly->Checked)
		{
			MessageBox::Show(this,
				L"Запись заблокирована: включен режим «Только чтение»!\nСнимите флажок для разрешения прошивки.",
				L"Защита от записи",
				MessageBoxButtons::OK,
				MessageBoxIcon::Warning
			);
			return;
		}
		if (this->sondeLink == nullptr || !this->sondeLink->IsOpen)
		{
			MessageBox::Show(this,
				L"Запись метрологии требует активного подключения к прибору.\n"
				L"Сначала выполните «Связь → Подключить».",
				L"Нет связи", MessageBoxButtons::OK, MessageBoxIcon::Information);
			return;
		}

		auto errors = gcnew System::Collections::Generic::List<String^>();
		bool cart = SelectedIsCartograph();
		METROLOGY_CARTOGRAPH cart_m;
		METROLOGY_GP gp_m;
		const uint8_t* blob = nullptr;
		uint16_t blobLen = 0;
		if (cart)
		{
			if (!CollectMetrologyCartograph(&cart_m, errors)) { ShowErrors(errors, L"запись в прибор"); return; }
			blob = (const uint8_t*)&cart_m; blobLen = (uint16_t)sizeof(cart_m);
		}
		else
		{
			if (!CollectMetrologyGP(&gp_m, errors)) { ShowErrors(errors, L"запись в прибор"); return; }
			blob = (const uint8_t*)&gp_m; blobLen = (uint16_t)sizeof(gp_m);
		}

		if (MessageBox::Show(this, L"УВЕРЕНЫ?", L"Подтверждение записи в зонд",
			MessageBoxButtons::OKCancel, MessageBoxIcon::Warning,
			MessageBoxDefaultButton::Button2) != System::Windows::Forms::DialogResult::OK)
			return;

		this->labelStatusNotice->Text = L"Запись метрологии в зонд...";
		this->Refresh();

		uint8_t cmd = 0;
		uint16_t dataLen = 0;
		uint8_t dataIn[64] = { 0 };
		int result = 1;
		{
			msclr::lock linkLock(this->sondeLink->SyncRoot);
			int prevTimeout = this->sondeLink->ReadTimeout;
			this->sondeLink->ReadTimeout = 2000;
			if (ToolProtocol::SendToTool(this->sondeLink, CMD_SET_METROLOGY, blob, blobLen) == 0)
			{
				System::Threading::Thread::Sleep(500);
				result = ToolProtocol::ReceiveFromTool(this->sondeLink, &cmd, dataIn, &dataLen);
			}
			this->sondeLink->ReadTimeout = prevTimeout;
		}

		if (result == 0)
		{
			if (cart) SondeData::SetMetrologyCartograph(&cart_m);
			else SondeData::SetMetrologyGP(&gp_m);
			this->labelStatusNotice->Text = L"Метрология записана в зонд";
			MessageBox::Show(this, L"ФАЙЛ МЕТРОЛОГИИ ЗАПИСАН В ПРИБОР !",
				L"Запись в зонд", MessageBoxButtons::OK, MessageBoxIcon::Information);
		}
		else
		{
			this->labelStatusNotice->Text = L"Запись в зонд не удалась";
			MessageBox::Show(this, L"ФАЙЛ МЕТРОЛОГИИ НЕ БЫЛ ЗАПИСАН В ПРИБОР !",
				L"Запись в зонд", MessageBoxButtons::OK, MessageBoxIcon::Warning);
		}
		std::cout << "[METRO] Запись в зонд: result=" << result << std::endl;
	}

	// «Применить» — собрать структуру и сохранить в сессии (как fill_the_struct)
	System::Void MetroForm::buttonApply_Click(System::Object^ sender, System::EventArgs^ e)
	{
		auto errors = gcnew System::Collections::Generic::List<String^>();
		if (SelectedIsCartograph())
		{
			METROLOGY_CARTOGRAPH cart_m;
			if (!CollectMetrologyCartograph(&cart_m, errors)) { ShowErrors(errors, L"применение"); return; }
			SondeData::SetMetrologyCartograph(&cart_m);
		}
		else
		{
			METROLOGY_GP gp_m;
			if (!CollectMetrologyGP(&gp_m, errors)) { ShowErrors(errors, L"применение"); return; }
			SondeData::SetMetrologyGP(&gp_m);
		}
		this->labelStatusNotice->Text = L"Параметры применены в сессии";
		this->DialogResult = System::Windows::Forms::DialogResult::OK;
	}

	System::Void MetroForm::buttonClose_Click(System::Object^ sender, System::EventArgs^ e)
	{
		this->Close();
	}

	System::Void MetroForm::checkBoxReadOnly_CheckedChanged(System::Object^ sender, System::EventArgs^ e)
	{
		bool readOnly = this->checkBoxReadOnly->Checked;
		this->buttonWriteToSonde->Enabled = !readOnly;
		this->textBoxSerial->ReadOnly = readOnly;
		this->textBoxSignature->ReadOnly = readOnly;
		this->textBoxStructSize->ReadOnly = readOnly;
		this->textBoxDiameter->ReadOnly = readOnly;
		this->textBoxFreq1->ReadOnly = readOnly;
		this->textBoxFreq2->ReadOnly = readOnly;
	}
}
