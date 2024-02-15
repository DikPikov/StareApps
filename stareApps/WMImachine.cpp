

#include "StareApps.h"

IWbemLocator* pLoc; 
IWbemServices* pSvc;
IUnsecuredApartment* pUnsecApp;

DeletionEventSink* pDeleteSink;
IUnknown* pDeleteStubUnk;
IWbemObjectSink* pDeleteStubSink;

CreationEventSink* pCreateSink;
IUnknown* pCreateStubUnk;
IWbemObjectSink* pCreateStubSink;

BOOL WMIInitialize() 
{
    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        MessageBox(NULL, L"Failed to initialize COM library. Error code = 0x", L"", MB_OK | MB_ICONERROR);
        return FALSE;           // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    hres = CoInitializeSecurity(
        NULL,
        -1,                          // COM negotiates service
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities
        NULL                         // Reserved
    );


    if (FAILED(hres))
    {
        MessageBox(NULL, L"Failed to initialize security. Error code = 0x", L"", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return FALSE;       // Program has failed.
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        MessageBox(NULL, L"Failed to create IWbemLocator object. Error code = 0x", L"", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return FALSE;       // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

     pSvc = NULL;

     // Connect to the local root\cimv2 namespace
    // and obtain pointer pSvc to make IWbemServices calls.

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc
    );

    if (FAILED(hres))
    {
        MessageBox(NULL, L"Could not connect. Error code = 0x", L"", MB_OK | MB_ICONERROR);
        pLoc->Release();
        CoUninitialize();
        return FALSE;           // Program has failed.
    }

    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities
    );

    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    if (FAILED(hres))
    {
        MessageBox(NULL, L"Could not set proxy blanket. Error code = 0x", L"", MB_OK | MB_ICONERROR);
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;    // Program has failed.
    }

    // Step 6: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    //Запускаем асинхронное прослушивание создания новых процессов
    pUnsecApp = NULL;

    hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL,
        CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment,
        (void**)&pUnsecApp);

     pCreateSink = new CreationEventSink;
     pCreateSink->AddRef();

     pCreateStubUnk = NULL;
    pUnsecApp->CreateObjectStub(pCreateSink, &pCreateStubUnk);

     pCreateStubSink = NULL;
    pCreateStubUnk->QueryInterface(IID_IWbemObjectSink,
        (void**)&pCreateStubSink);
    
    hres = pSvc->ExecNotificationQueryAsync(
        _bstr_t("WQL"),
        _bstr_t("SELECT * "
            "FROM __InstanceCreationEvent WITHIN 1 "
            "WHERE TargetInstance ISA 'Win32_Process'"),
        WBEM_FLAG_SEND_STATUS,
        NULL,
        pCreateStubSink);
   
    if (FAILED(hres))
    {
        MessageBox(NULL, L"Не получилось выполнить асинхронное прослушивание создания процесса", L"Ошибка", MB_OK | MB_ICONERROR); 
        pSvc->CancelAsyncCall(pCreateStubSink);

        pLoc->Release();
        pSvc->Release();
        pUnsecApp->Release();

        pCreateSink->Release();
        pCreateStubUnk->Release();
        pCreateStubSink->Release();

        CoUninitialize();
        return FALSE;
    }

    //Запускаем асинхронное прослушивание удаления новых процессов
    pDeleteSink = new DeletionEventSink;
    pDeleteSink->AddRef();

    pDeleteStubUnk = NULL;
    pUnsecApp->CreateObjectStub(pDeleteSink, &pDeleteStubUnk);

    pDeleteStubSink = NULL;
    pDeleteStubUnk->QueryInterface(IID_IWbemObjectSink,
        (void**)&pDeleteStubSink);

    hres = pSvc->ExecNotificationQueryAsync(
        _bstr_t("WQL"),
        _bstr_t("SELECT * "
            "FROM __InstanceDeletionEvent WITHIN 1 "
            "WHERE TargetInstance ISA 'Win32_Process'"),
        WBEM_FLAG_SEND_STATUS,
        NULL,
        pDeleteStubSink);

    if (FAILED(hres))
    {
        MessageBox(NULL, L"Не получилось выполнить асинхронное прослушивание удаления процесса", L"Ошибка", MB_OK | MB_ICONERROR);
        DeInitialize();
        return FALSE;
    }

    return TRUE;   // Program successfully completed.
}

//Возвращаем все текущие процессы
IEnumWbemClassObject* GetAllProcesses() 
{
    HRESULT hRes;

    IEnumWbemClassObject* objectsEnum;
    hRes = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_Process"), WBEM_FLAG_FORWARD_ONLY, NULL, &objectsEnum);

    if (FAILED(hRes)) {
        MessageBox(NULL, L"Не получилось получить все процессы", L"Ошибка", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    return objectsEnum;
}

//Находим и обрабатываем все текущие процессы
BOOL FindAllProcesses() 
{
    HRESULT hRes;

    IEnumWbemClassObject* objectsEnum;
    hRes = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_Process"), WBEM_FLAG_FORWARD_ONLY, NULL, &objectsEnum);

    if (FAILED(hRes)) {
        MessageBox(NULL, L"Не получилось получить все процессы", L"Ошибка", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    while (TRUE) 
    {
        IWbemClassObject* pObj = 0;
        ULONG uReturned = 0;

        hRes = objectsEnum->Next(
            0,                  // Time out
            1,                  // One object
            &pObj,
            &uReturned
        );

        if (uReturned == NULL) 
        {
            break;
        }
        else 
        {
            ProcessCreated(pObj);

            pObj->Release();
        }
    }

    UpdateProcessCountInfo();

    objectsEnum->Release();

    return TRUE;
}

//Удаляем процесс с PID
void TerminateProcess(DWORD pid) 
{
    wchar_t zaproc[64];
    wcscpy_s(zaproc, L"Win32_Process.Handle=\"");

    wchar_t wpid[32];
    _itow_s(pid, wpid, 10);

    wcscat_s(zaproc, wpid);
    wcscat_s(zaproc, L"\"");
    
    IWbemClassObject* pClass = NULL;
    pSvc->GetObject(bstr_t("Win32_Process"), 0, NULL, &pClass, NULL);

    IWbemClassObject* pInParamsDefinition = NULL;
    IWbemClassObject* pOutMethod = NULL;
    pClass->GetMethod(bstr_t("Terminate"), 0, &pInParamsDefinition, &pOutMethod);

    IWbemClassObject* pClassInstance = NULL;
    pInParamsDefinition->SpawnInstance(0, &pClassInstance);

    // Create the values for the in parameters
    VARIANT pcVal;
    VariantInit(&pcVal);
    V_VT(&pcVal) = VT_I4;

    // Store the value for the in parameters
    pClassInstance->Put(L"Reason", 0, &pcVal, 0);

    // Execute Method
    pSvc->ExecMethod(zaproc, bstr_t("Terminate"), 0, NULL, pClassInstance, NULL, NULL);

    pClass->Release();
    pInParamsDefinition->Release();
    pOutMethod->Release();
    pClassInstance->Release();
    VariantClear(&pcVal);
}

//Возвращаем процесс по PID
IWbemClassObject* GetProcess(DWORD pid)
{
    IEnumWbemClassObject* objectsEnum;
    wchar_t zaproc[64];
    wcscpy_s(zaproc, L"SELECT * FROM Win32_Process WHERE ProcessId = ");

    wchar_t wpid[32];
    _itow_s(pid, wpid, 10);

    wcscat_s(zaproc, wpid);

    HRESULT hRes = pSvc->ExecQuery(bstr_t("WQL"), zaproc, WBEM_FLAG_FORWARD_ONLY, NULL, &objectsEnum);

    if (FAILED(hRes)) {
        return NULL;
    }

    IWbemClassObject* process;
    ULONG returnd;
    objectsEnum->Next(WBEM_INFINITE, 1, &process, &returnd);

    if (returnd == NULL)
    {
        return NULL;
    }

    objectsEnum->Release();



    return process;
}

//Очизаемся
void DeInitialize() 
{
    pSvc->CancelAsyncCall(pCreateStubSink);
    pSvc->CancelAsyncCall(pDeleteStubSink);

     pLoc->Release();
     pSvc->Release();
    pUnsecApp->Release();

     pDeleteSink->Release();
    pDeleteStubUnk->Release();
     pDeleteStubSink->Release();

     pCreateSink->Release();
     pCreateStubUnk->Release();
     pCreateStubSink->Release();

     CoUninitialize();
}