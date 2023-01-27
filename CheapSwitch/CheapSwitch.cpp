/*
	This project serves as a simple demonstration for the article "Combining Raw Input and keyboard Hook to selectively block input from multiple keyboards",
	which you should be able to find in this project folder (HookingRawInput.html), or on the CodeProject website (http://www.codeproject.com/).
	The project is based on the idea shown to me by Petr Medek (http://www.hidmacros.eu/), and is published with his permission, huge thanks to Petr!
	The source code is licensed under The Code Project Open License (CPOL), feel free to adapt it.
	Vít Blecha (sethest@gmail.com), 2014
*/

#include <deque>
#include "CheapSwitch.h"
#include "../CheapSwitchDLL/CheapSwitchDLL.h"


#define DEBUG(x) std::wcout << x << std::endl; 


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// HWND of main executable
HWND mainHwnd;
// Windows message for communication between main executable and DLL module
UINT const WM_HOOK = WM_APP + 1;
// How long should Hook processing wait for the matching Raw Input message (ms)
DWORD maxWaitingTime = 100;
// Device name of my numeric keyboard

// WCHAR* const numericKeyboardDeviceName = L"\\\\?\\HID#VID_145F&PID_02C9&MI_00#7&1501150&0&0000#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}";
// WCHAR* const numericKeyboardDeviceName = L"\\\\?\\HID#VID_046D&PID_C534&MI_00#8&394140f&0&0000#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}";
WCHAR* const numericKeyboardDeviceName = L"\\\\?\\HID#VID_05AC&PID_0274&MI_01&Col01#7&160a4e02&0&0000#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}";
//WCHAR* const numericKeyboardDeviceName = L"\\\\?\\HID#VID_04D9&PID_1203&MI_00#8&13a87ad5&0&0000#{884b96c3-56ef-11d1-bc8c-00a0c91405dd}";
// Buffer for the decisions whether to block the input with Hook
std::deque<DecisionRecord> decisionBuffer;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WindowProc(HWND, UINT, WPARAM, LPARAM);

// APIENTRY 
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	RedirectIOToConsole();
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// AQUI ES ON VINDRIEN ELS CODIS CREC
	MSG msg;
	HACCEL hAccelTable;


	// Initialize global strings
	// LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	// LoadString(hInstance, IDC_HOOKINGRAWINPUTDEMO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	//hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HOOKINGRAWINPUTDEMO));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		/*if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}*/

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{

	const wchar_t* CLASS_NAME = L"CheapSwitch Window Class";

	WNDCLASS wndClass = {};
	wndClass.lpszClassName = CLASS_NAME;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.lpfnWndProc = WindowProc;

	return RegisterClass(&wndClass);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	hInst = hInstance; // Store instance handle in our global variable
	
	const wchar_t* CLASS_NAME = L"CheapSwitch Window Class";
	DWORD style = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;

	int width = 320;
	int height = 200;

	RECT rect;
	rect.left = 250;
	rect.top = 250;
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;

	AdjustWindowRect(&rect, style, false);

	hWnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"CheapSwitch",
		style,
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	
	if (!hWnd)
	{
		return FALSE;
	}

	// Save the HWND
	mainHwnd = hWnd;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Register for receiving Raw Input for keyboards
	RAWINPUTDEVICE rawInputDevice[1];
	rawInputDevice[0].usUsagePage = 1;
	rawInputDevice[0].usUsage = 6;
	rawInputDevice[0].dwFlags = RIDEV_INPUTSINK;
	rawInputDevice[0].hwndTarget = hWnd;
	RegisterRawInputDevices (rawInputDevice, 1, sizeof (rawInputDevice[0]));

	// Set up the keyboard Hook
	InstallHook (hWnd);

	return TRUE;
}

int ProcessRawInput(HWND& hWnd, LPARAM& lParam)
{
	UINT bufferSize;

	// Prepare buffer for the data
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
	LPBYTE dataBuffer = new BYTE[bufferSize];
	// Load data into the buffer
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, dataBuffer, &bufferSize, sizeof(RAWINPUTHEADER));

	RAWINPUT* raw = (RAWINPUT*)dataBuffer;

	// Get the virtual key code of the key and report it
	USHORT virtualKeyCode = raw->data.keyboard.VKey;
	USHORT keyPressed = raw->data.keyboard.Flags & RI_KEY_BREAK ? 0 : 1;
	WCHAR text[128];
	if (keyPressed == 1)
	{
		swprintf_s(text, 128, L"Raw Input: %X (%d)\n", virtualKeyCode, keyPressed);
		// OutputDebugString(text);
		DEBUG(text);
	}
	WCHAR textDN[128];
	swprintf_s(textDN, 128, L"Device name %s \n", numericKeyboardDeviceName);
	// OutputDebugString(textDN);
	// DEBUG(text);

	// Prepare string buffer for the device name
	GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, NULL, &bufferSize);
	WCHAR* stringBuffer = new WCHAR[bufferSize];

	// Load the device name into the buffer
	GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, stringBuffer, &bufferSize);


	// WCHAR textSB[128];
	// swprintf_s(textSB, 128, L"String buffer %s \n", stringBuffer);
	//OutputDebugString(textSB);
	// DEBUG(textSB)

	//printf((const char* const)stringBuffer);
	// Check whether the key struck was a "7" on a numeric keyboard, and remember the decision whether to block the input
	if (virtualKeyCode == 0x57 && wcscmp(stringBuffer, numericKeyboardDeviceName) == 0)
	{
		decisionBuffer.push_back(DecisionRecord(virtualKeyCode, TRUE));
	}
	else
	{
		decisionBuffer.push_back(DecisionRecord(virtualKeyCode, FALSE));
	}

	delete[] stringBuffer;
	delete[] dataBuffer;

	return 0;
}

int ProcessHook(HWND& hWnd, WPARAM& wParam, LPARAM& lParam)
{
	USHORT virtualKeyCode = (USHORT)wParam;
	USHORT keyPressed = lParam & 0x80000000 ? 0 : 1;
	WCHAR text[128];
	if (keyPressed == 1)
	{
		swprintf_s(text, 128, L"Hook: %X (%d)\n", virtualKeyCode, keyPressed);
		// OutputDebugString(text);
		DEBUG(text)
	}
	// Check the buffer if this Hook message is supposed to be blocked; return 1 if it is
	BOOL blockThisHook = FALSE;
	BOOL recordFound = FALSE;
	int index = 1;
	if (!decisionBuffer.empty())
	{
		// Search the buffer for the matching record
		std::deque<DecisionRecord>::iterator iterator = decisionBuffer.begin();
		while (iterator != decisionBuffer.end())
		{
			if (iterator->virtualKeyCode == virtualKeyCode)
			{
				blockThisHook = iterator->decision;
				recordFound = TRUE;
				// Remove this and all preceding messages from the buffer
				for (int i = 0; i < index; ++i)
				{
					decisionBuffer.pop_front();
				}
				// Stop looking
				break;
			}
			++iterator;
			++index;
		}
	}

	// Wait for the matching Raw Input message if the decision buffer was empty or the matching record wasn't there
	DWORD currentTime, startTime;
	startTime = GetTickCount64();
	while (!recordFound)
	{
		MSG rawMessage;
		while (!PeekMessage(&rawMessage, mainHwnd, WM_INPUT, WM_INPUT, PM_REMOVE))
		{
			// Test for the maxWaitingTime
			currentTime = GetTickCount64();
			// If current time is less than start, the time rolled over to 0
			if ((currentTime < startTime ? ULONG_MAX - startTime + currentTime : currentTime - startTime) > maxWaitingTime)
			{
				// Ignore the Hook message, if it exceeded the limit
				WCHAR text[128];
				if (keyPressed == 1)
				{
					swprintf_s(text, 128, L"Hook TIMED OUT: %X (%d)\n", virtualKeyCode, keyPressed);
					// OutputDebugString(text);
					DEBUG(text);
				}
				return 0;
			}
		}

		// The Raw Input message has arrived; decide whether to block the input
		UINT bufferSize;

		// Prepare buffer for the data
		GetRawInputData((HRAWINPUT)rawMessage.lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
		LPBYTE dataBuffer = new BYTE[bufferSize];
		// Load data into the buffer
		GetRawInputData((HRAWINPUT)rawMessage.lParam, RID_INPUT, dataBuffer, &bufferSize, sizeof(RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)dataBuffer;

		// Get the virtual key code of the key and report it
		USHORT rawVirtualKeyCode = raw->data.keyboard.VKey;
		USHORT rawKeyPressed = raw->data.keyboard.Flags & RI_KEY_BREAK ? 0 : 1;
		WCHAR text[128];
		if (keyPressed == 1)
		{
			swprintf_s(text, 128, L"Raw Input WAITING: %X (%d)\n", rawVirtualKeyCode, rawKeyPressed);
			// OutputDebugString(text);
			DEBUG(text);
		}
		// Prepare string buffer for the device name
		GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, NULL, &bufferSize);
		WCHAR* stringBuffer = new WCHAR[bufferSize];

		// Load the device name into the buffer
		GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICENAME, stringBuffer, &bufferSize);

		// If the Raw Input message doesn't match the Hook, push it into the buffer and continue waiting
		if (virtualKeyCode != rawVirtualKeyCode)
		{
			// Check whether the key struck was a "7" on a numeric keyboard, and decide whether to block the input
			if (rawVirtualKeyCode == 0x57 && wcscmp(stringBuffer, numericKeyboardDeviceName) == 0)
			{
				decisionBuffer.push_back(DecisionRecord(rawVirtualKeyCode, TRUE));
			}
			else
			{
				decisionBuffer.push_back(DecisionRecord(rawVirtualKeyCode, FALSE));
			}
		}
		else
		{
			// This is correct Raw Input message
			recordFound = TRUE;

			// Check whether the key struck was a "7" on a numeric keyboard, and decide whether to block the input
			if (rawVirtualKeyCode == 0x57 && wcscmp(stringBuffer, numericKeyboardDeviceName) == 0)
			{

				blockThisHook = TRUE;
			}
			else
			{
				blockThisHook = FALSE;
			}
		}
		delete[] stringBuffer;
		delete[] dataBuffer;
	}
	// Apply the decision
	if (blockThisHook)
	{
		if (keyPressed == 1)
		{
			swprintf_s(text, 128, L"***Keyboard event: %X (%d) is being blocked!\n", virtualKeyCode, keyPressed);
			// OutputDebugString(text);
			DEBUG(text);
		}
		return 1;
	}
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	// Raw Input Message
	case WM_INPUT:
	{
		DEBUG("----------------------------");
		return ProcessRawInput(hWnd, lParam);
	}

	// Message from Hooking DLL
	case WM_HOOK:
	{
		return ProcessHook(hWnd, wParam, lParam);
	}

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		UninstallHook ();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


