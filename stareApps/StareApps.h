#pragma once
#include <Wbemidl.h>
#include <comdef.h>

#pragma comment(lib, "wbemuuid.lib")

//Соединяем все элементы проекта
extern UINT64 deltaTime;
void UpdateCPUTime();

HWND CreateMainWindow(HINSTANCE);
void ProcessDeleted(DWORD);
void ProcessCreated(IWbemClassObject*);
void UpdateProcessList();
void UpdateProcessCountInfo();

IWbemClassObject* GetProcess(DWORD pid);
void TerminateProcess(DWORD pid);
BOOL WMIInitialize();
BOOL FindAllProcesses();
IEnumWbemClassObject* GetAllProcesses();
void DeInitialize();

class ProcessObject : public IWbemClassObject {};
	
class CreationEventSink : public IWbemObjectSink
{
	LONG m_lRef;
	bool bDone;

public:
	CreationEventSink() { m_lRef = 0; }
	~CreationEventSink() { bDone = true; }

	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riidm, void** ppv);
	virtual HRESULT STDMETHODCALLTYPE Indicate(LONG iObjectCount, IWbemClassObject __RPC_FAR* __RPC_FAR* apObjArray);
	virtual HRESULT STDMETHODCALLTYPE SetStatus(LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR* pObjParam);
};

class DeletionEventSink : public IWbemObjectSink
{
	LONG m_lRef;
	bool bDone;

public:
	DeletionEventSink() { m_lRef = 0; }
	~DeletionEventSink() { bDone = true; }

	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riidm, void** ppv);
	virtual HRESULT STDMETHODCALLTYPE Indicate(LONG iObjectCount, IWbemClassObject __RPC_FAR* __RPC_FAR* apObjArray);
	virtual HRESULT STDMETHODCALLTYPE SetStatus(LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR* pObjParam);
};
