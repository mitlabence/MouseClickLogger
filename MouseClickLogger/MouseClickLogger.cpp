// MouseClickLogger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// Ideas taken from
// https://github.com/PervasiveWellbeingTech/windows-logger-app/blob/master/MouseLogger/Main.cpp

#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <string>

bool exitRequested = false;
std::ofstream logFile;


UINT64 getCurrentTimestamp() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string timestampToString(UINT64 timestamp) {
	std::chrono::system_clock::time_point time_point =
		std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp));

	// Convert to a time_t (seconds since epoch)
	std::time_t time_t_point = std::chrono::system_clock::to_time_t(time_point);

	// Convert to string datetime of format YYY-mm-dd_hh:mm:ss.ms
	char buffer[80]; // TODO: replace with pure std::string solution somehow?
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H:%M:%S", std::localtime(&time_t_point));
	long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count() % 1000;
	//std::cout << buffer << milliseconds;
	// add milliseconds
	std::string bufferString = (std::string)buffer;
	bufferString += ".";
	bufferString += std::to_string(milliseconds);
	return bufferString;
}

// The code above seems to be unsuited for the purpose of the program: to register timestamps only when there is a click.
HHOOK mouseHook;
// Hook procedure to process mouse events
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		// Extract the mouse event data
		MSLLHOOKSTRUCT* mouseEvent = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

		if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
			UINT64 timestamp = getCurrentTimestamp();
			
			std::string datetime = timestampToString(timestamp);
			std::cout << "clicked at (" << mouseEvent->pt.x << ", " << mouseEvent->pt.y << "), time " << timestamp << ", " << datetime << std::endl;

			logFile << "click (" << mouseEvent->pt.x << ", " << mouseEvent->pt.y << "), " << timestamp << ", " << datetime << std::endl;

		}
	}

	// Call the next hook in the chain
	return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

BOOL CtrlHandler(DWORD fdwCtrlType) {
	if (fdwCtrlType == CTRL_C_EVENT) {
		exitRequested = true;  // Set the exit flag when Ctrl + C is pressed
		return TRUE;           
	}
	return FALSE;
}

int main()
{
	// Set Ctrl + C handler
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
	// Set the log file path
	const std::string logFilePath = "D:\\Downloads\\mouseclick.log";

	// Open the log file for writing
	logFile.open(logFilePath);
	if (!logFile.is_open()) {
		std::cerr << "Failed to open log file." << std::endl;
		return 1;
	}

	// Install the mouse hook
	mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
	if (!mouseHook) {
		std::cerr << "Failed to install hook." << std::endl;
		return 1;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(mouseHook);
}