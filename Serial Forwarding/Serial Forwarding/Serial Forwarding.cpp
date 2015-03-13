// Serial Forwarding.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>

enum comPort {usb, bt};

const unsigned int threadCount = 2;
HANDLE comPortUSB;
HANDLE comPortBT;
OVERLAPPED USBIncoming = { 0 };
OVERLAPPED BTIncoming = { 0 };
HANDLE threads[threadCount] = { 0 };
char bufferUSB[1024] = { 0 };
char bufferBT[1024] = { 0 };

int errorOutput();
DWORD WINAPI ThreadProc(LPVOID parameter);



int errorOutput() {
		DWORD errorCode = GetLastError();
		char errorString[1024] = { '\0' };
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, errorString, 1024, NULL);
		printf(errorString);
		system("pause");
		return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	//Initialize stucts and stuff
	DCB dcbUSB = { 0 }; //for USB COM port configuration
	DCB dcbBT = { 0 };
	dcbUSB.DCBlength = sizeof(DCB);
	dcbBT.DCBlength = sizeof(DCB);
	


	//Initialize events
	USBIncoming.hEvent = CreateEvent(0, true, 0, 0);
	if (USBIncoming.hEvent == NULL) {
		errorOutput();
		return 1;
	}

	BTIncoming.hEvent = CreateEvent(0, true, 0, 0);
	if (BTIncoming.hEvent == NULL) {
		errorOutput();
		return 1;
	}



	//Initialize COM ports
	comPortUSB = CreateFile("COM4", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

	if (comPortUSB == INVALID_HANDLE_VALUE) {
		errorOutput();
		return 1;
	}

	/*comPortBT = CreateFile("COM5", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

	if (comPortBT == INVALID_HANDLE_VALUE) {
		errorOutput();
		return 1;
	}*/




	//Setup COM ports
	if (!GetCommState(comPortUSB, &dcbUSB)) {
		errorOutput();
		return 1;
	}
	dcbUSB.BaudRate = CBR_9600; //normall 1200
	dcbUSB.fParity = false; //normally false
	dcbUSB.Parity = NOPARITY; //normally no parity
	dcbUSB.StopBits = ONESTOPBIT; //normally no stop bits
	dcbUSB.fRtsControl = RTS_CONTROL_DISABLE; //normally 1
	dcbUSB.fDtrControl = false; //normally 1
	dcbUSB.ByteSize = 8; //normally 7

	if (!SetCommState(comPortUSB, &dcbUSB)) {
		errorOutput();
	}



	//Set masks
	if (!SetCommMask(comPortUSB, EV_TXEMPTY | EV_RXCHAR)) {
		errorOutput();
	}

	DWORD outMask = 0;
	if (!WaitCommEvent(comPortUSB, &outMask, &USBIncoming)){
		//error? Maybe?
	}


	//Spawn threads to wait
	while (true) {
		//clear buffers
		memset(bufferBT, 0, sizeof(bufferBT));
		memset(bufferUSB, 0, sizeof(bufferUSB));

		//Kill any existing threads
		//TODO.

		//create threads
		for (int i = 0; i < threadCount; i++) {
			comPort port = (comPort)i; //cheap way of creating different threads for different coms
			threads[i] = CreateThread(NULL, 0, ThreadProc, &port, 0, NULL);
		}

		//Wait for a thread to process incoming data, then process it
	}

	

	CloseHandle(USBIncoming.hEvent);
	CloseHandle(BTIncoming.hEvent);
	for (int i = 0; i < threadCount; i++) {
		CloseHandle(threads[threadCount]);
	}

	system("pause");
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID parameter) {
	comPort type = *((comPort *)parameter); //convert parameter into the right type

	DWORD waitResult;
	HANDLE *theEvent;
	HANDLE *port;
	OVERLAPPED *overlap;
	char *buffer;

	//Setup variables
	if (type == usb) {
		theEvent = &USBIncoming.hEvent;
		port = &comPortUSB;
		overlap = &USBIncoming;
		buffer = bufferUSB;
	}
	else {
		theEvent = &BTIncoming.hEvent;
		port = &comPortBT;
		overlap = &BTIncoming;
		buffer = bufferBT;
	}


	//Stop the thread until a character has entered the buffer
		waitResult = WaitForSingleObject(*theEvent, INFINITE);

	if (waitResult == WAIT_OBJECT_0) {
		DWORD bytesRead = 0;
		unsigned int counter = 0;

		//Reads in the data from the port... I feel like there's going to be a problem here if there's multiple reads. Also, message grouping...
		do {
			ReadFile(*port, buffer, 30, &bytesRead, overlap);
		} while (bytesRead > 0);
	}
	else {
		errorOutput();
		return 1;
	}

	return 0;
}