#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <psapi.h>
#include <CommCtrl.h>

#include <string>

#include "StareApps.h"
#include "resource.h"

size_t _bytesWcharLenght = 32;

LRESULT CALLBACK pwWindowEvents(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
void UpdateInfo();
void pwAddControls(HWND hwnd);

HWND hText;
LPCWSTR processName;
int scrollPos;

std::string info;
int second = 0;
int pointer = 0;

void RegisterPFwindow(HINSTANCE hInstance) 
{
	WNDCLASSW windowClass = { 0 };
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	windowClass.lpszClassName = L"PerformanceWindow";
	windowClass.hInstance = hInstance;
	windowClass.lpfnWndProc = pwWindowEvents;
	windowClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	RegisterClassW(&windowClass);
}

LRESULT CALLBACK pwWindowEvents(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) 
{
	switch (msg)
	{
    case WM_WINDOWPOSCHANGED:
        RECT size;
        GetWindowRect(hwnd, &size);
        SetWindowPos(hText, hText, 50, 10, size.right - size.left - 15, size.bottom - size.top - 50, SWP_NOZORDER | SWP_NOMOVE);
        scrollPos = size.bottom - size.top;
        break;
	case WM_CREATE:
		pwAddControls(hwnd);

        SetTimer(hwnd, 1004, 1000, NULL);
		break;
	case WM_USER + 1:
    {
        SetWindowText(hwnd, (LPCWSTR)lp);

        pointer = 0;
        processName = (LPCWSTR)lp;
        UpdateInfo();
    }
		break;
    case WM_TIMER:
        UpdateInfo();
        break;
	default:
		return DefWindowProcW(hwnd, msg, wp, lp);
	}
}

void UpdateInfo() 
{

    HANDLE file = pwUpdate(processName);

    if (file == INVALID_HANDLE_VALUE)
    {
        return;
    }

    SetFilePointer(file, pointer, 0, FILE_BEGIN);

    DWORD byteCount = GetFileSize(file, NULL);

    DWORD readed;

    DWORD byte;
    float CPU;

    char dword[32]{};
    char chars[32]{};


    for (int i = 0; i < (byteCount - pointer) / 8; i++)
    {
        ReadFile(file, &byte, 4, &readed, NULL);

        if (byte == 0)
        {
            info += "\r\nDate : ";

            CPU = 0;
            ReadFile(file, &byte, 4, &readed, NULL);
            _itoa_s(byte, dword, 10);

            second = (byte / 100) % 100;

            info += dword[0];
            info += dword[1];
            info += ":";
            info += dword[2];
            info += dword[3];
            info += ":";
            info += dword[4];
            info += dword[5];
            info += "  day ";
            info += dword[6];
            info += dword[7];

            info += "\r\n\r\n";
        }
        else
        {
            second++;

            _itoa_s(second, dword, 10);
            info += dword;
            info += "s\tRAM : ";

            _itoa_s(byte, dword, 10);
            info += dword;

            info += "\tCPU : ";

            ReadFile(file, &CPU, 4, &readed, NULL);

            _gcvt_s(chars, CPU, 16);

            info += chars;
            info += "\r\n";
        }
    }

    std::wstring stemp = std::wstring(info.begin(), info.end());

    scrollPos = GetScrollPos(hText, SB_VERT);
    SetWindowText(hText, stemp.c_str());
    Edit_Scroll(hText, scrollPos, 0);

    SetFilePointer(file, 0, 0, FILE_END);

    pointer = byteCount;
}

void pwAddControls(HWND hwnd)
{
	hText = CreateWindow(WC_EDIT, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE, 0, 0, 400, 500, hwnd, NULL, NULL, NULL);
}