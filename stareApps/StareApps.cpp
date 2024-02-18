#include <windows.h>
#include <locale.h>
#include <psapi.h>
#include <stdio.h>

#include "StareApps.h"

//Заранее подключаем функции
LONG MainLoop();
BOOL EnableDebugPrivelege();
void UpdateCPUTime();

//Инициализируем переменные
UINT64 lastKernelCPU, lastUserCPU, lastIdleCPU; 
UINT64 deltaTime;
int numProcessors;

HWND mainWnd;

//Начало работы программы
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	LONG result;
	
    EnableDebugPrivelege();
    
    if (!WMIInitialize()) {  //Подключаемся к инструментарию системного мониторинга Windows
        return -1;
    }

    UpdateCPUTime();

    RegisterPFwindow(hInstance);

    mainWnd = CreateMainWindow(hInstance);
    if (mainWnd == nullptr)
    {
        return -1;
    }

	result = MainLoop();
    DeInitialize();
    pwReset();

	return 0;
}

LONG MainLoop() 
{
	MSG msg = { 0 };

	while (GetMessage(&msg, NULL, NULL, NULL)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

BOOL EnableDebugPrivelege()
{
	HANDLE hToken;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

    CHAR privilegesBuffer[FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges) + sizeof(LUID_AND_ATTRIBUTES) * 9];
    PTOKEN_PRIVILEGES privileges;
    ULONG i;

    privileges = (PTOKEN_PRIVILEGES)privilegesBuffer;
    privileges->PrivilegeCount = 9;

    for (i = 0; i < privileges->PrivilegeCount; i++)
    {
        privileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
        privileges->Privileges[i].Luid.HighPart = 0;
    }

    privileges->Privileges[0].Luid.LowPart = 20L;
    privileges->Privileges[1].Luid.LowPart = 14L;
    privileges->Privileges[2].Luid.LowPart = 33L;
    privileges->Privileges[3].Luid.LowPart = 10L;
    privileges->Privileges[4].Luid.LowPart = 13L;
    privileges->Privileges[5].Luid.LowPart = 17L;
    privileges->Privileges[6].Luid.LowPart = 18L;
    privileges->Privileges[7].Luid.LowPart = 19L;
    privileges->Privileges[8].Luid.LowPart = 9L;

    if (!AdjustTokenPrivileges(hToken, FALSE, privileges, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
    {
        CloseHandle(hToken);
        return FALSE;
    }
   
    if(GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        CloseHandle(hToken);
        return FALSE;
    }
    
    CloseHandle(hToken);
    return TRUE;
	
}

//обновляем процессорное время
void UpdateCPUTime() 
{
    FILETIME fuserTime, fkernelTime, fidleTime;
    UINT64 user, kernel;

    GetSystemTimes(&fidleTime, &fkernelTime, &fuserTime);
    memcpy(&user, &fuserTime, sizeof(UINT64));
    memcpy(&kernel, &fkernelTime, sizeof(UINT64));
    memcpy(&lastIdleCPU, &fidleTime, sizeof(UINT64));

    deltaTime = user + kernel - lastKernelCPU - lastUserCPU;

    lastKernelCPU = kernel;
    lastUserCPU = user;
}