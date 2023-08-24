// MouseClickLogger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// Ideas taken from
// https://github.com/PervasiveWellbeingTech/windows-logger-app/blob/master/MouseLogger/Main.cpp

#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#include <iostream>
#include <chrono>

bool exitRequested = false;
using namespace std;


UINT64 getCurrentTimestamp() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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
			// Print "clicked" to the console
			std::cout << "clicked at (" << mouseEvent->pt.x << ", " << mouseEvent->pt.y << "), time " << timestamp << std::endl;
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