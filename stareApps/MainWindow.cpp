#include <windows.h>
#include <stdlib.h>
#include <psapi.h>
#include <CommCtrl.h>

#include "StareApps.h"
#include "resource.h"


typedef struct _LV_COLUMN {
	UINT mask;       // which members of this structure contain valid information
	int fmt;         // alignment of the column heading and the subitem text 
	int cx;	// Specifies the width, in pixels, of the column.
	int cxMin;
	LPWSTR pszText;  // Pointer to a null-terminated string
					 // that contains the column heading 
	int cchTextMax;  // Specifies the size, in characters, of the buffer
	int iSubItem;    // index of subitem
} _LV_COLUMN;

typedef struct _LV_ITEM {
	UINT   mask;        // attributes of this data structure
	int    iItem;       // index of the item to which this structure refers
	int    iSubItem;    // index of the subitem to which this structure refers
	UINT   state;       // Specifies the current state of the item
	UINT   stateMask;   // Specifies the bits of the state member that are valid. 
	LPTSTR  pszText;    // Pointer to a null-terminated string
						// that contains the item text 
	int    cchTextMax;  // Size of the buffer pointed to by the pszText member
	int    iImage;      // index of the list view item's icon 
	LPARAM lParam;      // 32-bit value to associate with item 
} _LV_ITEM;

//Заранее подключаем функции
LRESULT CALLBACK WindowEvents(HWND, UINT, WPARAM, LPARAM);
void AddMenus(HWND);
void AddControls(HWND);
void LoadImages();

void ProcessDeleted(DWORD);
void ProcessCreated(IWbemClassObject*);
void UpdateProcessList();
void UpdateProcessCountInfo();

//Инициализируем переменные, структуры и определения
HWND hProcList;
HWND hProcCount;

HMENU hItemMenu;

HMENU hMenu;
HICON hIconImage;
HBITMAP hLogoImage;

DWORD processCount;

std::vector<float> lastProcKernel;
std::vector<float> lastProcUser;

wchar_t currentProcessName[MAX_PATH];
DWORD currentProcess;

size_t bytesWcharLenght = 32;

#define FileMenu_NEW 1
#define FILEMENU_UPDATEPROCESSLIST 2
#define FileMenu_EXIT 3
#define Menu_ABOUT 4
#define IDC_LISTVIEW                      1111
#define IDC_TIMER                      1003

#define TASK_TERMINATE 10
#define TASK_THREAD_TERMINATE 11
#define TASK_TOGGLE_FREEZE 12
#define TASK_PERFORMANCE 13
#define TASK_PROPERTIES 14

//здесь создается главное окно программы
HWND CreateMainWindow(HINSTANCE hInstance)
{
	hIconImage = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	WNDCLASSW windowClass = { 0 };
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	windowClass.lpszClassName = L"MainWindowClass";
	windowClass.hInstance = hInstance;
	windowClass.lpfnWndProc = WindowEvents;
	windowClass.hIcon = hIconImage;

	if (!RegisterClassW(&windowClass))
	{
		return nullptr;
	}

	return CreateWindowW(L"MainWindowClass", L"MainWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 600, 600, NULL, NULL, NULL, NULL);
}

//здесь обрабатываются события и команды окна
LRESULT CALLBACK WindowEvents(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_TIMER: //обновляем данные о текущих процессах
	{
		_LV_ITEM lvItem{};

		lvItem.mask = LVIF_TEXT; 
		lvItem.cchTextMax = 256; 

		char chars[32];
		wchar_t dword[32];

		std::vector<float> newProcKernel;
		std::vector<float> newProcUser;

		UpdateCPUTime();
		IEnumWbemClassObject* enumProcesses = GetAllProcesses();
		for (int i = 0; i < processCount; i++) 
		{
			lvItem.iItem = i;

			IWbemClassObject* process;
			ULONG returnd;
			enumProcesses->Next(WBEM_INFINITE, 1, &process, &returnd);

			if (returnd == NULL) 
			{
				break;
			}
			else 
			{
				VARIANT data;
				process->Get(L"PrivatePageCount", 0, &data, NULL, NULL);
				DWORD bytes = _wtoi(data.bstrVal);
				double dvalue = bytes;
				VariantClear(&data);

				const size_t cSize = 32;
				if (dvalue > 1024 * 1024 * 1024)
				{
					dvalue /= 1024 * 1024 * 1024;
					_gcvt_s(chars, dvalue, 3);

					mbstowcs_s(&bytesWcharLenght, dword, chars, 32);
					wcscat_s(dword, L" GB");
				}
				else if (dvalue > 1024 * 1024)
				{
					dvalue /= 1024 * 1024;
					_gcvt_s(chars, dvalue, 3);
					mbstowcs_s(&bytesWcharLenght, dword, chars, 32);
					wcscat_s(dword, L" MB");
				}
				else if (dvalue > 1024)
				{
					dvalue /= 1024;
					_gcvt_s(chars, dvalue, 3);
					mbstowcs_s(&bytesWcharLenght, dword, chars, 32);
					wcscat_s(dword, L" KB");
				}

				lvItem.iSubItem = 3;       
				lvItem.pszText = dword;

				ListView_SetItem(hProcList, &lvItem);

				process->Get(L"KernelModeTime", 0, &data, NULL, NULL);
				newProcKernel.push_back(_wtoi64(data.bstrVal));
				VariantClear(&data);

				process->Get(L"UserModeTime", 0, &data, NULL, NULL);
				newProcUser.push_back(_wtoi64(data.bstrVal));
				VariantClear(&data);

				dvalue = (newProcKernel[i] - lastProcKernel[i] + newProcUser[i] - lastProcUser[i]) / deltaTime * 100;

				process->Get(L"Name", 0, &data, NULL, NULL);
				pwAddValue(data.bstrVal, dvalue, bytes);
				VariantClear(&data);

				_gcvt_s(chars, dvalue, 16);
				mbstowcs_s(&bytesWcharLenght, dword, chars, 32);

				lvItem.iSubItem = 2;      
				lvItem.pszText = dword;

				ListView_SetItem(hProcList, &lvItem);
			}

			process->Release();
		}

		lastProcKernel = newProcKernel;
		lastProcUser = newProcUser;

		pwWrite();

		enumProcesses->Release();
	}
		break;
	case WM_NOTIFY: //предоставляем список комманд для выбранного процесса через пкм
	{
		LPNMHDR lpnmhdr = (LPNMHDR)lp;

		if (lpnmhdr->idFrom == IDC_LISTVIEW)
		{
			LPNMLISTVIEW info = (LPNMLISTVIEW)lp;

			if (lpnmhdr->code == NM_RCLICK || lpnmhdr->code == NM_CLICK) 
			{
				wchar_t pid[MAX_PATH];
				ListView_GetItemText(hProcList, info->iItem, 0, pid, MAX_PATH);
				currentProcess = _wtoi(pid);

				ListView_GetItemText(hProcList, info->iItem, 1, currentProcessName, MAX_PATH);

				if (lpnmhdr->code == NM_RCLICK)
				{
					POINT p;
					GetCursorPos(&p);
					TrackPopupMenu(hItemMenu, TPM_TOPALIGN | TPM_LEFTALIGN, p.x, p.y, 0, hwnd, NULL);
				}
			}
		}
	}
	return DefWindowProcW(hwnd, msg, wp, lp); //подстраиваем размеры окна
	case WM_WINDOWPOSCHANGED:
		RECT size;
		GetWindowRect(hwnd, &size);
		SetWindowPos(hProcCount, hProcCount, 100, size.bottom - 180, 200, 20, SWP_NOZORDER);
		SetWindowPos(hProcList, hProcList, 100, 10, size.right - size.left - 15, size.bottom - size.top - 80, SWP_NOZORDER | SWP_NOMOVE);

		break;
	case WM_COMMAND://обрабатывем команды от разных меню
		switch (wp)
		{
		case TASK_TERMINATE:
		{
			wchar_t info[64];
			wchar_t wcount[16];
			wcscpy_s(info, L"вы точно хотите завершить процесс: ");
			wcscat_s(info, currentProcessName);
			wcscat_s(info, L" (PID = ");
			_itow_s(currentProcess, wcount, 10);
			wcscat_s(info, wcount);
			wcscat_s(info, L")");

			if (MessageBox(hwnd, info, L"", MB_YESNO) == IDYES)
			{
				TerminateProcess(currentProcess);
			}
		}
		case TASK_PERFORMANCE:
		{
			HWND pw = CreateWindow(L"PerformanceWindow", L"", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 10, 0, 400, 500, hwnd, NULL, NULL, NULL);
			SendMessage(pw, WM_USER + 1, NULL, (LPARAM)currentProcessName);
		}
			break;
		break;
		case FILEMENU_UPDATEPROCESSLIST:
			UpdateProcessList();
			break;
		case FileMenu_EXIT:
			DestroyWindow(hwnd);
			break;
		case Menu_ABOUT:
			MessageBox(NULL, L"StareApps\nправа принадлежат Марату Акматову", L"О программе", MB_OK);
			break;
		}
		break;
	case WM_CREATE:
		SetWindowText(hwnd, L"StareApps");

		LoadImages();
		AddMenus(hwnd);
		AddControls(hwnd);
		UpdateProcessList();
		
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconImage);
		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconImage);
		SetTimer(hwnd, IDC_TIMER, 1000, NULL);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcW(hwnd, msg, wp, lp);
	}
}

void AddMenus(HWND hWnd)
{
	hMenu = CreateMenu();

	HMENU hFileMenu = CreateMenu();

	hItemMenu = CreatePopupMenu();

	AppendMenu(hItemMenu, MF_STRING, TASK_TERMINATE, L"завершить");
	AppendMenu(hItemMenu, MF_STRING, TASK_THREAD_TERMINATE, L"завершить поток");
	AppendMenu(hItemMenu, MF_STRING, TASK_TOGGLE_FREEZE, L"приостановить/продлить");
	AppendMenu(hItemMenu, MF_STRING, TASK_PERFORMANCE, L"сводка");
	AppendMenu(hItemMenu, MF_STRING, TASK_PROPERTIES, L"свойства");

	AppendMenu(hFileMenu, MF_STRING, FileMenu_NEW, L"Новый");
	AppendMenu(hFileMenu, MF_STRING, FILEMENU_UPDATEPROCESSLIST, L"Обновить список процессов");
	AppendMenu(hFileMenu, MF_SEPARATOR, NULL, L"");
	AppendMenu(hFileMenu, MF_STRING, FileMenu_EXIT, L"Выход");

	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"Файл");
	AppendMenu(hMenu, MF_STRING, Menu_ABOUT, L"О программе");

	SetMenu(hWnd, hMenu);
}

void AddControls(HWND hwnd)
{
	hProcList = CreateWindow(WC_LISTVIEW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER | LVS_EDITLABELS | LVS_SHOWSELALWAYS, 0, 0, 585, 500, hwnd, (HMENU)IDC_LISTVIEW, NULL, NULL);
	hProcCount = CreateWindow(WC_STATIC, L"", WS_CHILD | WS_VISIBLE , 10, 0, 100, 20, hwnd, NULL, NULL, NULL);

	_LV_COLUMN lvColumn{};
	lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_MINWIDTH | LVCF_SUBITEM;
	lvColumn.cx = 200;
	lvColumn.cxMin = 100;

	SendMessage(hProcList, LVM_SETEXTENDEDLISTVIEWSTYLE, NULL, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	lvColumn.pszText = (LPWSTR)L"путь запуска\0";
	lvColumn.fmt = LVCFMT_RIGHT & LVCFMT_JUSTIFYMASK;
	SendMessage(hProcList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
	lvColumn.pszText = (LPWSTR)L"время запуска\0";
	lvColumn.fmt = LVCFMT_RIGHT & LVCFMT_JUSTIFYMASK;
	SendMessage(hProcList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
	lvColumn.pszText = (LPWSTR)L"выделенная память\0";
	SendMessage(hProcList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

	lvColumn.fmt = LVCFMT_LEFT & LVCFMT_JUSTIFYMASK;
	lvColumn.pszText = (LPWSTR)L"CPU\0";
	SendMessage(hProcList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
	lvColumn.pszText = (LPWSTR)L"название\0";
	SendMessage(hProcList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
	lvColumn.pszText = (LPWSTR)L"PID\0";
	SendMessage(hProcList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

	HWND hLogo;
	hLogo = CreateWindowW(L"static", NULL, WS_VISIBLE | WS_CHILD | SS_BITMAP, 0, 0, 50, 50, hwnd, NULL, NULL, NULL);
	SendMessageW(hLogo, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hLogoImage);
}

void ProcessDeleted(DWORD pid)
{
	wchar_t wcount[16];

	_itow_s(pid, wcount, 10);

	LV_FINDINFO lvf;
	lvf.flags = LVFI_STRING;
	lvf.psz = wcount;

	DWORD index = ListView_FindItem(hProcList, -1, &lvf);
	if (index >= 0)
	{
		ListView_DeleteItem(hProcList, index);

		processCount--;
	}
}

//Добавляем новый процесс
void ProcessCreated(IWbemClassObject* process)
{
	VARIANT data;
	process->Get(L"ProcessId", 0, &data, NULL, NULL);

	wchar_t pid[16] ;
	_itow_s(data.intVal, pid, 10);
	VariantClear(&data);
	
	wchar_t name[MAX_PATH];
	process->Get(L"Name", 0, &data, NULL, NULL);
	wcscpy_s(name, data.bstrVal);
	VariantClear(&data);

	_LV_ITEM lvItem{};

	lvItem.mask = LVIF_TEXT; 
	lvItem.cchTextMax = 256; 

	lvItem.iItem = processCount;
	lvItem.iSubItem = 0;
	lvItem.pszText = pid;

	ListView_InsertItem(hProcList, &lvItem);
	
	lvItem.iSubItem = 1;       
	lvItem.pszText = name;

	ListView_SetItem(hProcList, &lvItem);

	wchar_t commandLine[MAX_PATH];

	process->Get(L"ExecutablePath", 0, &data, NULL, NULL);
	if (data.bstrVal != NULL) 
	{
		wcscpy_s(commandLine, data.bstrVal);

		lvItem.iSubItem = 5;    
		lvItem.pszText = commandLine;

		ListView_SetItem(hProcList, &lvItem);
	}
	VariantClear(&data);


	char chars[32];
	wchar_t dword[32];
	process->Get(L"PrivatePageCount", 0, &data, NULL, NULL);
	float dvalue = _wtoi(data.bstrVal);
	VariantClear(&data);
	
	const size_t cSize = 32;
	if (dvalue > 1024 * 1024 * 1024)
	{
		dvalue /= 1024 * 1024 * 1024;
		_gcvt_s(chars, dvalue, 3);

		mbstowcs_s(&bytesWcharLenght, dword, chars, 32);
		wcscat_s(dword, L" GB");
	}
	else if (dvalue > 1024 * 1024)
	{
		dvalue /= 1024 * 1024;
		_gcvt_s(chars, dvalue, 3);
		mbstowcs_s(&bytesWcharLenght, dword, chars, 32);
		wcscat_s(dword, L" MB");
	}
	else if (dvalue > 1024)
	{
		dvalue /= 1024;
		_gcvt_s(chars, dvalue, 3);
		mbstowcs_s(&bytesWcharLenght, dword, chars, 32);
		wcscat_s(dword, L" KB");
	}
	
	lvItem.iSubItem = 3;    
	lvItem.pszText = dword;
	ListView_SetItem(hProcList, &lvItem);

	process->Get(L"KernelModeTime", 0, &data, NULL, NULL);
	lastProcKernel.push_back(_wtoi64(data.bstrVal));
	VariantClear(&data);
	process->Get(L"UserModeTime", 0, &data, NULL, NULL);
	lastProcUser.push_back(_wtoi64(data.bstrVal));
	VariantClear(&data);

	lvItem.iSubItem = 2;     
	lvItem.pszText = bstr_t("N/A");
	ListView_SetItem(hProcList, &lvItem);

	process->Get(L"CreationDate", 0, &data, NULL, NULL);
	{
		wchar_t infos[32]{};
		wcscpy_s(infos, L"");
		
		wcsncat_s(infos, &data.bstrVal[8], 2);
		wcscat_s(infos, L":");
		wcsncat_s(infos, &data.bstrVal[10], 2);
		wcscat_s(infos, L":");
		wcsncat_s(infos, &data.bstrVal[12], 2);

		wcscat_s(infos, L" ");

		wcsncat_s(infos, &data.bstrVal[6], 2);
		wcscat_s(infos, L".");
		wcsncat_s(infos, &data.bstrVal[4], 2);
		wcscat_s(infos, L".");
		wcsncat_s(infos, &data.bstrVal[0], 4);

		lvItem.pszText = infos;
	}
	lvItem.iSubItem = 4;    
	
	ListView_SetItem(hProcList, &lvItem);
	VariantClear(&data);

	pwProcessCreated(name);

	processCount++;
}

void UpdateProcessCountInfo()
{
	wchar_t wcount[16];

	_itow_s(processCount, wcount, 10);
	wchar_t overallInfo[MAX_PATH];
	wcscpy_s(overallInfo, L"процессов: ");
	wcscat_s(overallInfo, wcount);

	SendMessage(hProcCount, WM_SETTEXT, NULL, (LPARAM)&overallInfo);
}

void UpdateProcessList()
{
	processCount = 0;
	ListView_DeleteAllItems(hProcList);

	lastProcKernel.clear();
	lastProcUser.clear();

	pwReset();

	FindAllProcesses();
}

void LoadImages()
{
	hLogoImage = (HBITMAP)LoadImageW(NULL, L"icon.ico", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
}