#include "IO.h"

IO* IO::pIO = NULL;

IO* IO::getIOInstance(){
	if (NULL == pIO)
		pIO = new IO();
	return pIO;
}
void IO::operator<<(mavlink_message_t* msg) {
	int len = mavlink_msg_to_send_buffer(send_buffer, msg);
	memset(&WriteOverlapped, 0, sizeof(OVERLAPPED));
	if (!WriteFile(h, send_buffer, len, NULL, &WriteOverlapped)) {
		if (GetLastError() != ERROR_IO_PENDING){
			throw runtime_error("Write Failed");
		}
	}
}

void IO::operator<<(const string& msg) {
	mavlink_message_t mavlink_msg;
	static char buffer[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN];
	strncpy(buffer, msg.c_str(), MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN);
	mavlink_msg_statustext_pack(1, 1, &mavlink_msg, 0, buffer);
	(*this) << &mavlink_msg;
}
IO::IO(){
	h = openComm();
	if (h == INVALID_HANDLE_VALUE)
		throw runtime_error("Error Open Comm File");
	g_hIOCompletionPort =
		CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	if (INVALID_HANDLE_VALUE == g_hIOCompletionPort)
	{
		throw runtime_error("Error occurred while creating IOCP:");
	}

	DWORD nThreadID;
	for (int i = 0; i < nThreads; i++)
	{
		reader_thread[i] = CreateThread(0, 0,
			WorkerThread, (void *)(this), 0, &nThreadID);
	}

	HANDLE hTemp = CreateIoCompletionPort(h,
		g_hIOCompletionPort, 0, 0);
	if (INVALID_HANDLE_VALUE == hTemp) {
		throw runtime_error("Error while bind handle to iocp");
	}

	memset(&ReadOverlapped, 0, sizeof(ReadOverlapped));
	if (!ReadFile(h, recv_buffer, sizeof(recv_buffer), NULL, &ReadOverlapped)) {
		int err = GetLastError();
		if (ERROR_IO_PENDING != err) {
			throw "Read Failed";
		}
	}
}

DWORD WINAPI IO::WorkerThread(LPVOID _self){
	IO* self = (IO*)_self;
	LPOVERLAPPED       pOverlapped = NULL;
	DWORD            dwBytesTransfered = 0;
	unsigned long long completionKey;
	int nBytesRecv = 0;
	int nBytesSent = 0;
	DWORD             dwBytes = 0, dwFlags = 0;
	LARGE_INTEGER tm, freq;
	QueryPerformanceFrequency(&freq);

	while (true){
		if (!GetQueuedCompletionStatus(
			(HANDLE)self->g_hIOCompletionPort,
			&dwBytesTransfered,
			&completionKey,
			&pOverlapped,
			INFINITE)) {
			int err = GetLastError();
			if (WAIT_TIMEOUT != err) {
				throw "Get Queued Completion Status Failed";
			}
			else {
				continue;
			}
		}
		if (pOverlapped == &self->WriteOverlapped) {
			QueryPerformanceCounter(&tm);
			printf("send %.6f\n", double(tm.QuadPart) / freq.QuadPart);
			continue;
		}
		//printf("bytes read: %d\n", dwBytesTransfered);
		for (int i = 0; i < dwBytesTransfered; i++) {
			if (mavlink_parse_char(0, self->recv_buffer[i], &self->msg, &self->status) == 1) {
				if (self->msg.sysid != 1 || self->msg.compid != 1)
					continue;
				switch (self->msg.msgid) {
				case MAVLINK_MSG_ID_HEARTBEAT:
					break;
				case MAVLINK_MSG_ID_SYSTEM_TIME:
					break;
				case MAVLINK_MSG_ID_RAW_IMU:
					break;
				case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:
					break;
				case MAVLINK_MSG_ID_POWER_STATUS:
					break;
				case MAVLINK_MSG_ID_SYS_STATUS:
					break;
				case MAVLINK_MSG_ID_VFR_HUD:
					break;
				case MAVLINK_MSG_ID_SCALED_PRESSURE:
					//printf("pressure: %.2f\n", mavlink_msg_scaled_pressure_get_press_abs(&self->msg));
					break;
				case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
					break;
				case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
					break;
				case MAVLINK_MSG_ID_VIBRATION:
					break;
				case MAVLINK_MSG_ID_ATTITUDE:
					/*
					printf("roll: %f\n", mavlink_msg_attitude_get_roll(&self->msg));
					printf("pitch: %f\n", mavlink_msg_attitude_get_pitch(&self->msg));
					printf("yaw: %f\n", mavlink_msg_attitude_get_yaw(&self->msg));
					*/
					break;
				case MAVLINK_MSG_ID_STATUSTEXT:
					QueryPerformanceCounter(&tm);
					mavlink_msg_statustext_get_text(&self->msg, self->status_text);
					printf("tm: %.6f text: %s\n", double(tm.QuadPart) / freq.QuadPart, self->status_text);
					break;
				case MAVLINK_MSG_ID_LOG_REQUEST_DATA_CRC:
					break;
				case MAVLINK_MSG_ID_AHRS2_LEN:
					break;
				case MAVLINK_MSG_ID_HIGHRES_IMU_LEN:
					break;
				case MAVLINK_MSG_ID_GIMBAL_REPORT_LEN:
					break;
				case MAVLINK_MSG_ID_PING:
				{
					QueryPerformanceCounter(&tm);
					int seq = mavlink_msg_ping_get_seq(&self->msg);
					printf("tm: %.6f pang: %d\n", double(tm.QuadPart) / freq.QuadPart, seq);
					break;
				}
				default:
					printf("Unknown msg id: %d\n", self->msg.msgid);
					break;
				}
			}
		}
		memset(&self->ReadOverlapped, 0, sizeof(self->ReadOverlapped));
		if (!ReadFile(self->h, self->recv_buffer, sizeof(self->recv_buffer), NULL, &self->ReadOverlapped)) {
			int err = GetLastError();
			if (err != ERROR_IO_PENDING) {
				throw "Read File Error";
			}
		}

	}
}

HANDLE IO::openComm() {
	h = CreateFile(TEXT("COM3"), FILE_SHARE_READ | FILE_SHARE_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if (INVALID_HANDLE_VALUE == h) {
		int err = GetLastError();
		return h;
	}

	COMMTIMEOUTS timeouts;
	GetCommTimeouts(h, &timeouts);
	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(h, &timeouts);

	//设置串口配置信息
	DCB dcb;
	if (!GetCommState(h, &dcb))
	{
		cout << "GetCommState() failed" << endl;
		CloseHandle(h);
		return INVALID_HANDLE_VALUE;
	}
	int nBaud = 57600;
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = nBaud;//波特率为9600
	dcb.Parity = 0;//校验方式为无校验
	dcb.ByteSize = 8;//数据位为8位
	dcb.StopBits = ONESTOPBIT;//停止位为1位
	if (!SetCommState(h, &dcb))
	{
		cout << "SetCommState() failed" << endl;
		CloseHandle(h);
		return INVALID_HANDLE_VALUE;
	}

	//设置读写缓冲区大小
	static const int g_nZhenMax = 16;//32768;
	if (!SetupComm(h, g_nZhenMax, g_nZhenMax))
	{
		cout << "SetupComm() failed" << endl;
		CloseHandle(h);
		return INVALID_HANDLE_VALUE;
	}

	//清空缓冲
	PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

	//清除错误
	DWORD dwError;
	COMSTAT cs;
	if (!ClearCommError(h, &dwError, &cs))
	{
		cout << "ClearCommError() failed" << endl;
		CloseHandle(h);
		return INVALID_HANDLE_VALUE;
	}
	return h;
}