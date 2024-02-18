#include "StareApps.h"

std::vector<HANDLE> performanceFiles;
std::vector<LPCTSTR> names;
std::vector<short> instanceCount;
std::vector<float> CPUs;
std::vector<DWORD> bytes;

int Find(LPCTSTR name) 
{
	for (int i = 0; i < names.size(); i++) 
	{
		if (wcscmp(names[i], name) == 0)
		{
			return i;
		}
	}

	return -1;
}

void pwProcessCreated(LPCTSTR name) 
{
	int index = Find(name);
	if (index > -1) 
	{
		instanceCount[index]++;
		return;
	}

	wchar_t path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);

	wcscat_s(path, L"\\Performances\\");
	CreateDirectory(path, NULL);

	wcscat_s(path, name);
	wcscat_s(path, L".pfd");

	HANDLE performanceFile = CreateFile(path, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	SYSTEMTIME sysTime{};
	GetLocalTime(&sysTime);
	DWORD written = 0;
	wchar_t time[16]{};
	wchar_t dword[8];

	if (sysTime.wHour < 10) {
		wcscat_s(time, L"0");
	}
	_itow_s(sysTime.wHour, dword, 10);
	wcscat_s(time, dword);

	if (sysTime.wMinute < 10) {
		wcscat_s(time, L"0");
	}
	_itow_s(sysTime.wMinute, dword, 10);
	wcscat_s(time, dword);

	if (sysTime.wSecond < 10) {
		wcscat_s(time, L"0");
	}
	_itow_s(sysTime.wSecond, dword, 10);
	wcscat_s(time, dword);

	if (sysTime.wDay < 10) {
		wcscat_s(time, L"0");
	}
	_itow_s(sysTime.wDay, dword, 10);
	wcscat_s(time, dword);

	DWORD date = _wtol(time);
	SetFilePointer(performanceFile, 0, 0, FILE_END);
	WriteFile(performanceFile, &written, sizeof(date), &written, NULL);
	WriteFile(performanceFile, &date, sizeof(date), &written, NULL);

	performanceFiles.push_back(performanceFile);

	wchar_t* wname = new wchar_t[MAX_PATH] {};
	wcscpy_s(wname, MAX_PATH, name);

	names.push_back(wname);
	instanceCount.push_back(1);
	CPUs.push_back(0);
	bytes.push_back(0);
}

void pwProcessDeleted(LPCTSTR name)
{
	int index = Find(name);
	if (index < 0) 
	{
		return;
	}

	instanceCount[index]--;
	if (instanceCount[index] > 0) {
		return;
	}

	CloseHandle(performanceFiles[index]);
	performanceFiles.erase(performanceFiles.begin() + index);
	delete[] names[index];
	instanceCount.erase(instanceCount.begin() + index);
	names.erase(names.begin() + index);
	CPUs.erase(CPUs.begin() + index);
	bytes.erase(bytes.begin() + index);
}

void pwAddValue(LPCTSTR name, float CPU, DWORD RAM)
{
	int index = Find(name);

	if (index < 0) {
		return;
	}

	CPUs[index] += CPU;
	bytes[index] += RAM;
}

void pwWrite() 
{
	for (int i = 0; i < names.size(); i++) 
	{
		DWORD written;
		WriteFile(performanceFiles[i], &bytes[i], sizeof(bytes[i]), &written, NULL);
		WriteFile(performanceFiles[i], &CPUs[i], sizeof(CPUs[i]), &written, NULL);

		CPUs[i] = 0;
		bytes[i] = 0;
	}
}

void pwReset() 
{
	for (int i = 0; i < performanceFiles.size(); i++) 
	{
		CloseHandle(performanceFiles[i]);
	}

	for (int i = 0; i < names.size(); i++) 
	{
		delete[] names[i];
	}

	performanceFiles.clear();
	names.clear();
	instanceCount.clear();
	CPUs.clear();
	bytes.clear();
}

HANDLE pwUpdate(LPCTSTR name)
{
	int index = Find(name);
	if (index < 0) 
	{
		return NULL;
	}

	CloseHandle(performanceFiles[index]);

	wchar_t path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);

	wcscat_s(path, L"\\Performances\\");
	CreateDirectory(path, NULL);

	wcscat_s(path, name);
	wcscat_s(path, L".pfd");

	HANDLE performanceFile = CreateFile(path, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	performanceFiles[index] = performanceFile;

	SetFilePointer(performanceFile, 0, 0, FILE_END);

	return performanceFile;
}