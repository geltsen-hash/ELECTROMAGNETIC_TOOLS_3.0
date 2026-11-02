#pragma once

#include <msclr/marshal_cppstd.h>

namespace ELECTROMAGNETIC_TOOLS_3_0 {

	using namespace System;
	using namespace System::IO::Ports;
	using namespace System::Net;
	using namespace System::Net::Sockets;
	using namespace System::Collections::Generic;

	// Тип физического канала связи с прибором (как в AIR_TEMP_1.0)
	enum CONNECTION_TYPE {
		CONN_COM = 0,
		CONN_UDP = 1
	};

	/// <summary>
	/// Общий транспорт связи с прибором (COM-порт / Wi-Fi UDP).
	/// Перенесено из AIR_TEMP_1.0/variable.h (через MAGNETOMETR_CAL/ToolConnection.h).
	/// Один экземпляр на приложение: используется телеметрией и стендовой калибровкой.
	/// </summary>
	public ref class DeviceLink {
	private:
		SerialPort^ serialPort;
		Socket^ udpSocket;
		IPEndPoint^ remoteEndPoint;
		Queue<Byte>^ rxQueue;
		int readTimeout;
		int writeTimeout;
		int baudRate;
		int readBufferSize;
		String^ portName;
		int connType;
		String^ wifiIP;
		int wifiPort;
		Object^ syncRoot;

	public:
		DeviceLink() {
			serialPort = gcnew SerialPort();
			rxQueue = gcnew Queue<Byte>();
			syncRoot = gcnew Object();
			readTimeout = 3000;
			writeTimeout = 3000;
			baudRate = 125000;
			readBufferSize = 10000;
			portName = L"COM1";
			connType = CONN_COM;
			wifiIP = L"192.168.4.1";
			wifiPort = 5000;
			udpSocket = nullptr;
			remoteEndPoint = nullptr;
		}

		DeviceLink(System::ComponentModel::IContainer^ container) {
			serialPort = gcnew SerialPort(container);
			rxQueue = gcnew Queue<Byte>();
			syncRoot = gcnew Object();
			readTimeout = 3000;
			writeTimeout = 3000;
			baudRate = 125000;
			readBufferSize = 10000;
			portName = L"COM1";
			connType = CONN_COM;
			wifiIP = L"192.168.4.1";
			wifiPort = 5000;
			udpSocket = nullptr;
			remoteEndPoint = nullptr;
		}

		~DeviceLink() {
			Close();
			if (serialPort != nullptr) {
				delete serialPort;
				serialPort = nullptr;
			}
		}

		property int ConnectionType {
			int get() { return connType; }
			void set(int val) { connType = val; }
		}

		property String^ WifiIP {
			String^ get() { return wifiIP; }
			void set(String^ val) { wifiIP = val; }
		}

		property int WifiPort {
			int get() { return wifiPort; }
			void set(int val) { wifiPort = val; }
		}

		property int ReadTimeout {
			int get() { return readTimeout; }
			void set(int val) {
				readTimeout = val;
				if (serialPort != nullptr) serialPort->ReadTimeout = val;
				if (udpSocket != nullptr) udpSocket->ReceiveTimeout = val;
			}
		}

		property int WriteTimeout {
			int get() { return writeTimeout; }
			void set(int val) {
				writeTimeout = val;
				if (serialPort != nullptr) serialPort->WriteTimeout = val;
				if (udpSocket != nullptr) udpSocket->SendTimeout = val;
			}
		}

		property int BaudRate {
			int get() { return baudRate; }
			void set(int val) {
				baudRate = val;
				if (serialPort != nullptr) serialPort->BaudRate = val;
			}
		}

		property int ReadBufferSize {
			int get() { return readBufferSize; }
			void set(int val) {
				readBufferSize = val;
				if (serialPort != nullptr) serialPort->ReadBufferSize = val;
			}
		}

		property String^ PortName {
			String^ get() { return portName; }
			void set(String^ val) {
				portName = val;
				if (serialPort != nullptr) serialPort->PortName = val;
			}
		}

		// Замок для сериализации обмена по каналу: поток телеметрии и сервисные
		// команды (метрология и пр.) не должны перемешивать пакеты друг с другом.
		property Object^ SyncRoot {
			Object^ get() { return syncRoot; }
		}

		property bool IsOpen {
			bool get() {
				if (connType == CONN_UDP) {
					return (udpSocket != nullptr);
				}
				else {
					return (serialPort != nullptr && serialPort->IsOpen);
				}
			}
		}

		static cli::array<String^>^ GetPortNames() {
			return SerialPort::GetPortNames();
		}

		void Open() {
			if (connType == CONN_UDP) {
				Close();
				rxQueue->Clear();
				udpSocket = gcnew Socket(AddressFamily::InterNetwork, SocketType::Dgram, ProtocolType::Udp);
				udpSocket->SetSocketOption(SocketOptionLevel::Socket, SocketOptionName::ReuseAddress, true);
				udpSocket->Bind(gcnew IPEndPoint(IPAddress::Any, 0));
				udpSocket->ReceiveTimeout = readTimeout;
				udpSocket->SendTimeout = writeTimeout;

				IPAddress^ ip;
				if (!IPAddress::TryParse(wifiIP, ip)) {
					ip = IPAddress::Parse(L"192.168.4.1");
				}
				int port = (wifiPort == 0) ? 5000 : wifiPort;
				remoteEndPoint = gcnew IPEndPoint(ip, port);
			}
			else {
				if (serialPort == nullptr) serialPort = gcnew SerialPort();
				if (serialPort->IsOpen) serialPort->Close();
				serialPort->PortName = portName;
				serialPort->BaudRate = baudRate;
				serialPort->ReadTimeout = readTimeout;
				serialPort->WriteTimeout = writeTimeout;
				serialPort->ReadBufferSize = readBufferSize;
				serialPort->Open();
			}
		}

		void Close() {
			if (udpSocket != nullptr) {
				try { udpSocket->Close(); } catch (...) {}
				delete udpSocket;
				udpSocket = nullptr;
			}
			rxQueue->Clear();
			if (serialPort != nullptr && serialPort->IsOpen) {
				try { serialPort->Close(); } catch (...) {}
			}
		}

		void DiscardInBuffer() {
			if (connType == CONN_UDP) {
				rxQueue->Clear();
				if (udpSocket != nullptr) {
					try {
						while (udpSocket->Available > 0) {
							cli::array<Byte>^ tmp = gcnew cli::array<Byte>(Math::Min(udpSocket->Available, 4096));
							EndPoint^ ep = (EndPoint^)remoteEndPoint;
							udpSocket->ReceiveFrom(tmp, ep);
						}
					} catch (...) {}
				}
			}
			else {
				if (serialPort != nullptr && serialPort->IsOpen) {
					try { serialPort->DiscardInBuffer(); } catch (...) {}
				}
			}
		}

		void DiscardOutBuffer() {
			if (connType == CONN_COM) {
				if (serialPort != nullptr && serialPort->IsOpen) {
					try { serialPort->DiscardOutBuffer(); } catch (...) {}
				}
			}
		}

		void Write(cli::array<Byte>^ buffer, int offset, int count) {
			if (connType == CONN_UDP) {
				if (udpSocket == nullptr) Open();
				cli::array<Byte>^ toSend;
				if (offset == 0 && count == buffer->Length) {
					toSend = buffer;
				}
				else {
					toSend = gcnew cli::array<Byte>(count);
					System::Array::Copy(buffer, offset, toSend, 0, count);
				}
				udpSocket->SendTo(toSend, remoteEndPoint);
			}
			else {
				if (serialPort == nullptr || !serialPort->IsOpen) Open();
				serialPort->Write(buffer, offset, count);
			}
		}

		void Write(String^ text) {
			if (connType == CONN_UDP) {
				cli::array<Byte>^ bytes = System::Text::Encoding::ASCII->GetBytes(text);
				Write(bytes, 0, bytes->Length);
			}
			else {
				if (serialPort == nullptr || !serialPort->IsOpen) Open();
				serialPort->Write(text);
			}
		}

		int Read(cli::array<Byte>^ buffer, int offset, int count) {
			if (connType == CONN_UDP) {
				if (udpSocket == nullptr) throw gcnew InvalidOperationException(L"UDP socket is not open");
				DateTime deadline = DateTime::Now.AddMilliseconds(readTimeout);
				while (rxQueue->Count < count) {
					int msLeft = (int)(deadline - DateTime::Now).TotalMilliseconds;
					if (msLeft <= 0) {
						throw gcnew TimeoutException(L"UDP read timeout");
					}
					udpSocket->ReceiveTimeout = msLeft;
					try {
						cli::array<Byte>^ packet = gcnew cli::array<Byte>(4096);
						EndPoint^ fromEP = gcnew IPEndPoint(IPAddress::Any, 0);
						int received = udpSocket->ReceiveFrom(packet, fromEP);
						for (int i = 0; i < received; i++) {
							rxQueue->Enqueue(packet[i]);
						}
					}
					catch (SocketException^ se) {
						if (se->SocketErrorCode == SocketError::TimedOut) {
							throw gcnew TimeoutException(L"UDP read timeout");
						}
						throw;
					}
				}
				for (int i = 0; i < count; i++) {
					buffer[offset + i] = rxQueue->Dequeue();
				}
				return count;
			}
			else {
				if (serialPort == nullptr || !serialPort->IsOpen) throw gcnew InvalidOperationException(L"SerialPort is not open");
				int totalRead = 0;
				DateTime deadline = DateTime::Now.AddMilliseconds(readTimeout);
				while (totalRead < count) {
					int msLeft = (int)(deadline - DateTime::Now).TotalMilliseconds;
					if (msLeft <= 0) {
						throw gcnew TimeoutException(L"SerialPort read timeout");
					}
					serialPort->ReadTimeout = msLeft;
					try {
						int r = serialPort->Read(buffer, offset + totalRead, count - totalRead);
						if (r > 0) {
							totalRead += r;
						}
					}
					catch (TimeoutException^) {
						throw;
					}
				}
				return totalRead;
			}
		}

		String^ ReadLine() {
			if (connType == CONN_UDP) {
				System::Text::StringBuilder^ sb = gcnew System::Text::StringBuilder();
				cli::array<Byte>^ b = gcnew cli::array<Byte>(1);
				while (true) {
					Read(b, 0, 1);
					char ch = (char)b[0];
					if (ch == '\n') break;
					if (ch != '\r') sb->Append((Char)ch);
				}
				return sb->ToString();
			}
			else {
				if (serialPort == nullptr || !serialPort->IsOpen) throw gcnew InvalidOperationException(L"SerialPort is not open");
				return serialPort->ReadLine();
			}
		}

		String^ GetConnectionDescription() {
			if (connType == CONN_UDP) {
				return String::Format(L"Wi-Fi UDP [{0}:{1}]", wifiIP, wifiPort);
			}
			else {
				return String::Format(L"COM [{0} @ {1}]", portName, baudRate);
			}
		}
	};

}
