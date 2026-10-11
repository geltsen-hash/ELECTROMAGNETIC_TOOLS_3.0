#include "pch.h"
#include "MetroForm.h"
#include <iostream>

namespace ELECTROMAGNETIC_TOOLS_3_0 {

	void MetroForm::InitDefaultValues(void)
	{
		this->comboBoxSondeType->SelectedIndex = 0;
		this->comboBoxRotDir->SelectedIndex = 0;
	}

	System::Void MetroForm::buttonOpenFile_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->openFileDialogMetro->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			this->statusLabelMetroFile->Text = L"Файл метрологии: " + this->openFileDialogMetro->FileName;
			this->labelStatusNotice->Text = L"Файл загружен";
			std::cout << "[METRO] Загружен файл метрологии: " << std::endl;
		}
	}

	System::Void MetroForm::buttonSaveFile_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->saveFileDialogMetro->ShowDialog() == System::Windows::Forms::DialogResult::OK)
		{
			this->statusLabelMetroFile->Text = L"Файл сохранен: " + this->saveFileDialogMetro->FileName;
			this->labelStatusNotice->Text = L"Файл сохранен";
			std::cout << "[METRO] Файл метрологии сохранен: " << std::endl;
		}
	}

	System::Void MetroForm::buttonReadFromSonde_Click(System::Object^ sender, System::EventArgs^ e)
	{
		MessageBox::Show(
			L"Запрос чтения метрологических структур из энергонезависимой памяти зонда (Flash EEPROM).\n"
			L"Организация: НПФ «АМК Горизонт»\nРазработал: зам начальника ОП Смирнов С. Г.",
			L"Чтение из зонда",
			MessageBoxButtons::OK,
			MessageBoxIcon::Information
		);
	}

	System::Void MetroForm::buttonWriteToSonde_Click(System::Object^ sender, System::EventArgs^ e)
	{
		if (this->checkBoxReadOnly->Checked)
		{
			MessageBox::Show(
				L"Запись заблокирована: включен режим «Только чтение»!\nСнимите флажок для разрешения прошивки.",
				L"Защита от записи",
				MessageBoxButtons::OK,
				MessageBoxIcon::Warning
			);
			return;
		}

		if (MessageBox::Show(
			L"Внимание! Будет выполнена перезапись калибровочных констант в памяти зонда.\nПродолжить?",
			L"Подтверждение записи в зонд",
			MessageBoxButtons::YesNo,
			MessageBoxIcon::Question) == System::Windows::Forms::DialogResult::Yes)
		{
			std::cout << "[METRO] Запись параметров в зонд..." << std::endl;
		}
	}

	System::Void MetroForm::buttonApply_Click(System::Object^ sender, System::EventArgs^ e)
	{
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
		this->textBoxDiameter->ReadOnly = readOnly;
		this->textBoxFreq1->ReadOnly = readOnly;
		this->textBoxFreq2->ReadOnly = readOnly;
	}
}
