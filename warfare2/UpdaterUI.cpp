#include "StdInc.h"
#include "resource.h"

#include <CommCtrl.h>
#include <shobjidl.h>

static struct  
{
	HWND rootWindow;
	HWND topStatic;
	HWND bottomStatic;
	HWND progressBar;
	HWND cancelButton;

	UINT taskbarMsg;

	bool canceled;

	ITaskbarList3* tbList;

	wchar_t topText[512];
	wchar_t bottomText[512];
} g_uui;

void UI_CreateWindow()
{
	g_uui.taskbarMsg = RegisterWindowMessage(L"TaskbarButtonCreated");

	HWND rootWindow = CreateWindowEx(0, L"NotSteamAtAll", L"Updating IW4Play", 13238272 /* lol */, 0x80000000, 0, 400, 129, NULL, NULL, GetModuleHandle(NULL), 0);

	INITCOMMONCONTROLSEX controlSex;
	controlSex.dwSize = sizeof(controlSex);
	controlSex.dwICC = 16416; // lazy bum
	InitCommonControlsEx(&controlSex);

	HFONT font = CreateFont(-12, 0, 0, 0, 0, 0, 0, 0, 1, 8, 0, 5, 2, L"Tahoma");

	// TODO: figure out which static is placed where
	HWND static1 = CreateWindowEx(0x20, L"static", L"static1", 0x50000000, 15, 15, 365, 25, rootWindow, 0, GetModuleHandle(NULL) /* what?! */, 0);

	SendMessage(static1, WM_SETFONT, (WPARAM)font, 0);

	HWND cancelButton = CreateWindowEx(0, L"button", L"Cancel", 0x50000000, 305, 64, 75, 25, rootWindow, 0, GetModuleHandle(NULL), 0);
	SendMessage(cancelButton, WM_SETFONT, (WPARAM)font, 0);

	HWND progressBar = CreateWindowEx(0, L"msctls_progress32", 0, 0x50000000, 15, 40, 365, 15, rootWindow, 0, GetModuleHandle(NULL), 0);
	SendMessage(progressBar, PBM_SETRANGE32, 0, 10000);

	HWND static2 = CreateWindowEx(0x20, L"static", L"static2", 0x50000000, 15, 64, 270, 25, rootWindow, 0, GetModuleHandle(NULL) /* what?! */, 0);
	SendMessage(static2, WM_SETFONT, (WPARAM)font, 0);

	g_uui.cancelButton = cancelButton;
	g_uui.progressBar = progressBar;
	g_uui.topStatic = static1;
	g_uui.bottomStatic = static2;
	g_uui.rootWindow = rootWindow;

	RECT wndRect;
	wndRect.left = 0;
	wndRect.top = 0;
	wndRect.right = 400;
	wndRect.bottom = 129;

	HWND desktop = GetDesktopWindow();
	HDC dc = GetDC(desktop);
	int width = GetDeviceCaps(dc, 8);
	int height = GetDeviceCaps(dc, 10);

	ReleaseDC(desktop, dc);

	SetTimer(rootWindow, 0, 20, NULL);

	MoveWindow(rootWindow, (width - 400) / 2, (height - 129) / 2, wndRect.right - wndRect.left + 1, wndRect.bottom - wndRect.top + 1, TRUE);

	ShowWindow(rootWindow, TRUE);
}

LRESULT CALLBACK UI_WndProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
	switch (uMsg)
	{
		case WM_CTLCOLORSTATIC:
			SetBkMode((HDC)wparam, TRANSPARENT);
			return (LRESULT)GetStockObject(0);
		case WM_COMMAND:
			if ((HWND)lparam == g_uui.cancelButton)
			{
				g_uui.canceled = true;
			}

			break;
		case WM_TIMER:
			SetWindowText(g_uui.topStatic, g_uui.topText);
			SetWindowText(g_uui.bottomStatic, g_uui.bottomText);
			break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(hWnd, &ps);
			
				EndPaint(hWnd, &ps);
				break;
			}
		default:
			if (uMsg == g_uui.taskbarMsg)
			{
				if (g_uui.tbList)
				{
					g_uui.tbList->SetProgressState(hWnd, TBPF_NORMAL);
					g_uui.tbList->SetProgressValue(hWnd, 0, 100);
				}
			}
			break;
	}

	return DefWindowProc(hWnd, uMsg, wparam, lparam);
}

void UI_RegisterClass()
{
	WNDCLASSEX wndClass;
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = 3;
	wndClass.lpfnWndProc = UI_WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = GetModuleHandle(NULL);
	wndClass.hIcon = LoadIcon(GetModuleHandle(NULL), (LPCWSTR)IDI_ICON1);
	wndClass.hCursor = LoadCursor(NULL, (LPCWSTR)0x7F02);
	wndClass.hbrBackground = (HBRUSH)6;
	wndClass.lpszClassName = L"NotSteamAtAll";
	wndClass.hIconSm = LoadIcon(GetModuleHandle(NULL), (LPCWSTR)IDI_ICON1);

	RegisterClassEx(&wndClass);
}

void UI_DoCreation()
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	OSVERSIONINFO osVersion;
	GetVersionEx(&osVersion);

	if (osVersion.dwMajorVersion > 6 || (osVersion.dwMajorVersion == 6 && osVersion.dwMinorVersion >= 1))
	{
		CoCreateInstance(CLSID_TaskbarList, 
			NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_uui.tbList));
	}

	UI_RegisterClass();
	UI_CreateWindow();
}

void UI_DoDestruction()
{
	DestroyWindow(g_uui.rootWindow);
}

void UI_UpdateText(int textControl, const wchar_t* text)
{
	if (textControl == 0)
	{
		wcscpy(g_uui.topText, text);
	}
	else
	{
		wcscpy(g_uui.bottomText, text);
	}
}

void UI_UpdateProgress(double percentage)
{
	SendMessage(g_uui.progressBar, PBM_SETPOS, percentage * 100, 0);

	if (g_uui.tbList)
	{
		g_uui.tbList->SetProgressValue(g_uui.rootWindow, percentage, 100);

		if (percentage == 100)
		{
			g_uui.tbList->SetProgressState(g_uui.rootWindow, TBPF_NOPROGRESS);
		}
	}
}

bool UI_IsCanceled()
{
	return g_uui.canceled;
}