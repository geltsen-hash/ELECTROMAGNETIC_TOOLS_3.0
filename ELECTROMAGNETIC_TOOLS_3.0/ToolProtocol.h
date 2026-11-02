#pragma once

#include "DeviceLink.h"
#include "SondeDataTypes.h"
#include <cstdint>
#include <vector>
#include <iostream>

namespace ELECTROMAGNETIC_TOOLS_3_0 {

	// Команды протокола прибора (перенос из AIR_TEMP_1.0/Constants.h)
	#define TOOL_KEY (0x01 << 4)
	#define CMD_GET_DATA        (0x01 | TOOL_KEY)  // кадр измерений
	#define CMD_SET_METROLOGY   (0x02 | TOOL_KEY)  // запись метрологии в EEPROM ЦК
	#define CMD_GET_METROLOGY   (0x03 | TOOL_KEY)  // чтение метрологии из EEPROM ЦК
	#define CMD_SET_PRESETS     (0x04 | TOOL_KEY)
	#define CMD_GET_PRESETS     (0x05 | TOOL_KEY)
	#define CMD_GET_SIGNATURE   (0x06 | TOOL_KEY)  // чтение сигнатуры прибора
	#define CMD_SET_TELESYSTEM  (0x07 | TOOL_KEY)
	#define CMD_SET_WORK_TYPE   (0x19 | TOOL_KEY)
	#define CMD_SET_AIR         0x1a
	#define CMD_TIME_SYNC       0xF5
	#define CMD_ERAM            0x61
	#define CMD_TURBO           0xFD
	#define CMD_WORK            0x67
	#define CMD_READ_EE         0x65
	#define CMD_WRITE_EE        0x66

	/// <summary>
	/// Транспортный протокол обмена с прибором: [CMD][LEN16][DATA][CRC16].
	/// Прямой перенос send_to_tool / receive_from_tool из AIR_TEMP_1.0/variable.cpp.
	/// CRC16 — табличный Modbus (CRC-16-IBM), идентичен прошивке.
	/// </summary>
	public ref class ToolProtocol {
	public:
		static uint16_t CRC16(const uint8_t* data, uint16_t length) {
			static const uint16_t wCRCTable[] = {
			0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
			0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
			0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
			0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
			0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
			0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
			0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
			0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
			0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
			0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
			0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
			0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
			0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
			0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
			0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
			0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
			0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
			0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
			0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
			0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
			0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
			0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
			0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
			0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
			0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
			0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
			0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
			0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
			0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
			0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
			0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
			0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
			};

			uint8_t nTemp;
			uint16_t wCRCWord = 0xFFFF;
			while (length--) {
				nTemp = *data++ ^ (uint8_t)wCRCWord;
				wCRCWord >>= 8;
				wCRCWord ^= wCRCTable[nTemp];
			}
			return wCRCWord;
		}

		// Отправить команду прибору. Формат: [CMD][LEN=length+5][DATA...][CRC16]
		static int SendToTool(DeviceLink^ conn, uint8_t cmd, const uint8_t* data, uint16_t length) {
			if (conn == nullptr || !conn->IsOpen) return 1;

			std::vector<uint8_t> packet(length + 5);
			packet[0] = cmd;
			uint16_t totalLen = length + 5;
			memcpy(&packet[1], &totalLen, 2);
			for (int i = 3; i < length + 3; i++) {
				packet[i] = data[i - 3];
			}
			uint16_t crc = CRC16(packet.data(), length + 3);
			packet[length + 3] = (uint8_t)(crc & 0xFF);
			packet[length + 4] = (uint8_t)(crc >> 8);

			conn->DiscardInBuffer();
			conn->DiscardOutBuffer();
			try {
				cli::array<Byte>^ packetToSend = gcnew cli::array<Byte>(length + 5);
				for (int i = 0; i < length + 5; i++) packetToSend[i] = packet[i];
				conn->Write(packetToSend, 0, length + 5);
				return 0;
			}
			catch (...) {
				return 1;
			}
		}

		// Принять ответ прибора. Возврат: 0 = ок, 1 = нет ответа/таймаут, 2 = CRC/неполный пакет
		static int ReceiveFromTool(DeviceLink^ conn, uint8_t* outCmd, uint8_t* outData, uint16_t* outDataLen) {
			if (conn == nullptr || !conn->IsOpen) return 1;
			if (outDataLen != nullptr) *outDataLen = 0;

			const int buffLen = 4096;
			std::vector<uint8_t> packet(buffLen, 0xFF);
			cli::array<Byte>^ packetToReceive = gcnew cli::array<Byte>(buffLen);
			for (int i = 0; i < buffLen; i++) packetToReceive[i] = 0xFF;

			// Первые 3 байта: CMD + LEN16
			try {
				conn->Read(packetToReceive, 0, 3);
			}
			catch (...) {
				conn->DiscardInBuffer();
				conn->DiscardOutBuffer();
				return 1;
			}
			for (int i = 0; i < 3; i++) packet[i] = packetToReceive[i];
			int restLengthForRead = (int)(*(uint16_t*)&packet[1]) - 3;
			if (restLengthForRead < 0 || restLengthForRead > buffLen - 3) {
				conn->DiscardInBuffer();
				conn->DiscardOutBuffer();
				return 2;
			}

			int readed = 0;
			try {
				readed = conn->Read(packetToReceive, 3, restLengthForRead);
			}
			catch (...) {
				conn->DiscardInBuffer();
				conn->DiscardOutBuffer();
				return 1;
			}
			for (int i = 3; i < restLengthForRead + 3; i++) packet[i] = packetToReceive[i];

			conn->DiscardInBuffer();
			conn->DiscardOutBuffer();

			// CRC считается по полному пакету без последних 2 байт
			if (CRC16(packet.data(), restLengthForRead + 1) == *(uint16_t*)&packet[restLengthForRead + 1]
				&& restLengthForRead == readed) {
				*outCmd = packet[0];
				for (int i = 0; i < restLengthForRead - 2; i++) {
					outData[i] = packetToReceive[i + 3];
				}
				if (outDataLen != nullptr) *outDataLen = (uint16_t)(readed - 2);
				return 0;
			}

			std::cout << "[Protocol] crc no valid" << std::endl;
			return 2;
		}

		// Запрос сигнатуры прибора (CMD_GET_SIGNATURE) — верификация связи и тип зонда.
		// Возврат: true = ответ получен, outSignature заполнен.
		static bool RequestSignature(DeviceLink^ conn, uint32_t% outSignature) {
			outSignature = 0;
			uint8_t d0 = 0;
			if (SendToTool(conn, CMD_GET_SIGNATURE, &d0, 0) != 0) return false;

			System::Threading::Thread::Sleep(300);
			uint8_t cmd = 0;
			uint16_t dataLen = 0;
			uint8_t dataIn[16] = { 0 };
			if (ReceiveFromTool(conn, &cmd, dataIn, &dataLen) != 0) return false;
			if (dataLen < 4) return false;

			outSignature = *(uint32_t*)dataIn;
			return true;
		}
	};

}
