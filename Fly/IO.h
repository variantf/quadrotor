#ifndef IO_CLASS
#define IO_CLASS
#include <Windows.h>
#include <iostream>
#include <algorithm>
#include "mavlink.h"
#include "c_library-master\ardupilotmega\mavlink_msg_ahrs2.h"
#include "c_library-master\ardupilotmega\mavlink_msg_gimbal_report.h"
using namespace std;

class IO {
public:
	static IO* getIOInstance();
	void operator<<(mavlink_message_t* msg);
	void operator<<(const string&);
private:
	const int nThreads = 1;
	HANDLE reader_thread[1];
	HANDLE g_hIOCompletionPort;
	OVERLAPPED ReadOverlapped;
	OVERLAPPED WriteOverlapped;
	HANDLE h;
	unsigned char recv_buffer[1];
	unsigned char send_buffer[1024];
	mavlink_message_t msg;
	mavlink_status_t status;
	char status_text[128];
	static IO *pIO;

	static DWORD WINAPI WorkerThread(LPVOID _self);

	HANDLE openComm();

	IO();
};
#endif