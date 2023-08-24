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
#include <shlobj.h>
#include <sstream>

bool exitRequested = false;
std::ofstream logFile;

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{

	if (uMsg == BFFM_INITIALIZED)
	{
		std::string tmp = (const char*)lpData;
		//std::cout << "path: " << tmp << std::endl;
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	}

	return 0;
}

std::string BrowseFolder(std::string saved_path)
{
	// https://stackoverflow.com/questions/12034943/win32-select-directory-dialog-from-c-c
	TCHAR path[MAX_PATH];

	//const char* path_param = saved_path.c_str();
	std::wstring wsaved_path(saved_path.begin(), saved_path.end());
	const wchar_t* path_param = wsaved_path.c_str();

	BROWSEINFO bi = { 0 };
	bi.lpszTitle = (L"Browse for folder...");
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = BrowseCallbackProc;
	bi.lParam = (LPARAM)path_param;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl != 0)
	{
		//get the name of the folder and put it in path
		SHGetPathFromIDList(pidl, path);

		//free memory used
		IMalloc* imalloc = 0;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pidl);
			imalloc->Release();
		}

		// return path;
		std::wstring arr_w(path);
		std::string arr_s(arr_w.begin(), arr_w.end());
		return arr_s; // https://stackoverflow.com/questions/9001241/how-to-convert-tchar-array-to-stdstring-in-vc-help-on-syntax
	}

	return "";
}



UINT64 getCurrentTimestamp() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string timestampToString(UINT64 timestamp, bool withMilliseconds = true) {
	std::chrono::system_clock::time_point time_point =
		std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp));

	// Convert to a time_t (seconds since epoch)
	std::time_t time_t_point = std::chrono::system_clock::to_time_t(time_point);

	// Convert to string datetime of format YYY-mm-dd_hh:mm:ss.ms
	char buffer[80]; // TODO: replace with pure std::string solution somehow?
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", std::localtime(&time_t_point)); // TODO: time separator is usually :, but if this function is used for getting file name, ":" is not a valid character. Make another function for creating file name?
	std::string bufferString = (std::string)buffer;

	if (withMilliseconds) {
		long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count() % 1000;
		bufferString += ".";
		bufferString += std::to_string(milliseconds);
	}

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
			// TODO: add left/right click! i.e. format should be: leftclick, (x, y), timestamp, datetime
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
	// create file name
	std::string fname = "mouseclick_";
	UINT64 timestamp = getCurrentTimestamp();
	fname += timestampToString(timestamp, false);
	fname += ".log";
	// create file path
	std::string fpath = BrowseFolder("C:\\"); // TODO: add saved path if possible, or remove feature.
	fpath += "\\";
	fpath += fname; // TODO: normalize path!
	std::cout << "Writing to log file: " << fpath << std::endl;
	// Open the log file for writing
	logFile.open(fpath);
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