// Serial Forwarding.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#define BUFFER_SIZE 128

enum comPort {usb, bt};

HANDLE comPortUSB; //the USB port
HANDLE comPortBT; //the Bluetooth port
OVERLAPPED USBIncoming = { 0 }; //Holds the status of information coming in via USB
OVERLAPPED BTIncoming = { 0 }; //Holds the status of information coming in via Bluetooth
OVERLAPPED USBOutgoing = { 0 }; //Holds the status of information leaving via USB
OVERLAPPED BTOutgoing = { 0 }; //Holds the status of information leaving via Bluetooth
HANDLE threads[2] = { 0 }; //Holds the handles of the two threads processing incoming USB and Bluetooth data (0 = USB, 1 = Bluetooth)
HANDLE processingReady[2] = { 0 }; //Signals to main when a message is ready for processing (0 = from USB, 1 = from Bluetooth)

char bufferUSB[BUFFER_SIZE] = { 0 }; //Holds the global buffer for USB data
char bufferBT[BUFFER_SIZE] = { 0 }; //Holds the global buffer for Bluetooth data

int errorOutput();
DWORD WINAPI ThreadProc(LPVOID parameter);



//Outputs error information - rudimentary
int errorOutput() {
		DWORD errorCode = GetLastError();
		char errorString[BUFFER_SIZE] = { '\0' };
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, errorString, BUFFER_SIZE, NULL);
		printf(errorString);
		system("pause");
		return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	//Initialize stucts and stuff
	DCB dcbUSB = { 0 }; //for USB COM port configuration
	DCB dcbBT = { 0 }; //for Bluetooth COM port configuration
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

	USBOutgoing.hEvent = CreateEvent(0, true, 0, 0);
	if (USBOutgoing.hEvent == NULL) {
		errorOutput();
		return 1;
	}

	BTOutgoing.hEvent = CreateEvent(0, true, 0, 0);
	if (BTOutgoing.hEvent == NULL) {
		errorOutput();
		return 1;
	}

	for (int i = 0; i < 2; i++) {
		processingReady[i] = CreateEvent(0, true, 0, 0);
		if (processingReady[i] == NULL) {
			errorOutput();
			return 1;
		}
	}

	//Initialize COM ports
	comPortUSB = CreateFile("COM4", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if (comPortUSB == INVALID_HANDLE_VALUE) {
		errorOutput();
		return 1;
	}

	comPortBT = CreateFile("COM5", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if (comPortBT == INVALID_HANDLE_VALUE) {
		errorOutput();
		return 1;
	}

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

	//Set timeouts
	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(comPortUSB, &timeouts)) {
		errorOutput();
		return 1;
	}

	if (!SetCommTimeouts(comPortBT, &timeouts)) {
		errorOutput();
		return 1;
	}



	//Set masks
	DWORD mask = 0;
	BOOL retVal = false;
	if (!SetCommMask(comPortUSB, /*EV_TXEMPTY |*/ EV_RXCHAR)) {
		errorOutput();
		return 1;
	}

	retVal = WaitCommEvent(comPortUSB, &mask, &USBIncoming);
	if (retVal == 0 && GetLastError() != ERROR_IO_PENDING){
		errorOutput();
		return 1;
	}

	if (!SetCommMask(comPortBT, /*EV_TXEMPTY |*/ EV_RXCHAR)) {
		errorOutput();
		return 1;
	}

	retVal = WaitCommEvent(comPortBT, &mask, &BTIncoming);
	if (retVal == 0 && GetLastError() != ERROR_IO_PENDING){
		errorOutput();
		return 1;
	}

	printf("Starting threads...\n");

	//create threads
	comPort port[2];
	for (int i = 0; i < 2; i++) {
		port[i] = (comPort)i; //cheap way of creating different threads for different coms
		threads[i] = CreateThread(NULL, 0, ThreadProc, &port[i], 0, NULL);
	}

	//Spawn threads to wait
	while (true) {
		//Wait for a thread to process incoming data, then process it, then spawn a new thread and clear the appropriate buffer

		
		
		/////////////Goals for this section: spawn threads, wait until one terminates, process the data, then respawn the terminated thread.

		if (WaitForSingleObject(processingReady[0], 2) == WAIT_OBJECT_0) {
			ResetEvent(processingReady[0]);
			if (bufferUSB[0] == '|') {
				printf(bufferUSB);
				WriteFile(comPortBT, bufferUSB, strlen(bufferUSB), NULL, &BTOutgoing);
			}
			else if (bufferUSB[0] == '!') {
				printf(bufferUSB);
			}

			memset(bufferUSB, '\0', sizeof(bufferUSB));

			retVal = WaitCommEvent(comPortUSB, &mask, &USBIncoming);
			if (retVal == 0 && GetLastError() != ERROR_IO_PENDING){
				errorOutput();
				return 1;
			}

			comPort port = usb;
			threads[0] = CreateThread(NULL, 0, ThreadProc, &port, 0, NULL);
		}

		if (WaitForSingleObject(processingReady[1], 2) == WAIT_OBJECT_0) {
			ResetEvent(processingReady[1]);
			if (bufferBT[0] == '|') {
				printf(bufferBT);
				WriteFile(comPortUSB, bufferBT, strlen(bufferBT), NULL, &USBOutgoing);
			}
			else if (bufferBT[0] == '!') {
				printf(bufferBT);
			}

			memset(bufferBT, '\0', sizeof(bufferBT));

			retVal = WaitCommEvent(comPortBT, &mask, &BTIncoming);
			if (retVal == 0 && GetLastError() != ERROR_IO_PENDING){
				errorOutput();
				return 1;
			}

			comPort port = bt;
			threads[1] = CreateThread(NULL, 0, ThreadProc, &port, 0, NULL);
		}

		if (WaitForSingleObject(USBOutgoing.hEvent, 2) == WAIT_OBJECT_0) {
			ResetEvent(USBOutgoing.hEvent);
		}

		if (WaitForSingleObject(BTOutgoing.hEvent, 2) == WAIT_OBJECT_0) {
			ResetEvent(BTOutgoing.hEvent);
		}
	}

	

	CloseHandle(USBIncoming.hEvent);
	CloseHandle(BTIncoming.hEvent);
	CloseHandle(USBOutgoing.hEvent);
	CloseHandle(BTOutgoing.hEvent);
	for (int i = 0; i < 2; i++)	CloseHandle(processingReady[i]);

	//Threads will exit on their own

	system("pause");
	return 0;
}

DWORD WINAPI ThreadProc(LPVOID parameter) {
	comPort *typeTemp = (comPort *) parameter; //convert parameter into the right type
	comPort type = *typeTemp;

	DWORD waitResult;
	HANDLE *theEvent;
	HANDLE *port;
	HANDLE *processing;
	OVERLAPPED *overlap;
	char *buffer;

	//Setup variables
	if (type == usb) {
		theEvent = &USBIncoming.hEvent;
		port = &comPortUSB;
		overlap = &USBIncoming;
		buffer = bufferUSB;
		processing = &processingReady[0];
	}
	else {
		theEvent = &BTIncoming.hEvent;
		port = &comPortBT;
		overlap = &BTIncoming;
		buffer = bufferBT;
		processing = &processingReady[1];
	}


	//Stop the thread until a character has entered the buffer
		waitResult = WaitForSingleObject(*theEvent, INFINITE);

	if (waitResult == WAIT_OBJECT_0) {
		DWORD bytesRead = 0;
		char delim = '\0';
		unsigned int counter = 0;
		char internalBuffer[BUFFER_SIZE] = { '\0' };

		do {
			ReadFile(*port, internalBuffer, 1, NULL, overlap);
			GetOverlappedResult(*port, overlap, &bytesRead, true);
			if (internalBuffer[0] == '|') {
				delim = '|';
				break;
			}
			else if (internalBuffer[0] == '!') {
				delim = '\n';
				break;
			}
		} while (true);


		//Reads in the data from the port... I feel like there's going to be a problem here if there's multiple reads, or the ReadFile returns false due to asychronous processing. Also, message grouping... this needs to gather an entire message field
		do {
			counter++;
			ReadFile(*port, internalBuffer + counter, 1, NULL, overlap);
			GetOverlappedResult(*port, overlap, &bytesRead, true);
			if (bytesRead == 0) {
				counter--;
			}
		} while (counter < sizeof(internalBuffer) && internalBuffer[counter] != delim);

		memcpy_s(buffer, BUFFER_SIZE, internalBuffer, BUFFER_SIZE); //copy contents of internal buffer back to global buffer

		SetEvent(*processing);
		ResetEvent(*theEvent);
	}
	else {
		errorOutput();
		return 1;
	}

	return 0;
}