
#include "StareApps.h"

#include <iostream>

ULONG DeletionEventSink::AddRef() {
	return InterlockedIncrement(&m_lRef);
}

ULONG DeletionEventSink::Release() {
	LONG lRef = InterlockedDecrement(&m_lRef);
	if (lRef == 0) {
		delete this;
	}

	return lRef;
}

HRESULT DeletionEventSink::QueryInterface(REFIID riid, void** ppv) {
	if (riid == IID_IUnknown || riid == IID_IWbemObjectSink) {
		*ppv = (IWbemObjectSink*)this;
		AddRef();
		return WBEM_S_NO_ERROR;
	}
	else {
		return E_NOINTERFACE;
	}
}

HRESULT DeletionEventSink::Indicate(long lObjectCount, IWbemClassObject** apObjArray) {
	HRESULT hres = S_OK;

	for (int i = 0; i < lObjectCount; i++) {

		_variant_t vTarget{};
		hres = apObjArray[i]->Get(bstr_t("TargetInstance"), NULL, &vTarget, NULL, NULL);
		if (FAILED(hres))
		{
			MessageBox(NULL, L"Не удалось получить объект", L"Ошибка", MB_OK | MB_ICONERROR);
		}

		IUnknown* pIUnknown = (IUnknown*)vTarget;
		IWbemClassObject* object;
		hres = pIUnknown->QueryInterface(IID_IWbemClassObject, (void**)&object);
		if (FAILED(hres))
		{
			MessageBox(NULL, L"Не удалось преобразовать объект", L"Ошибка", MB_OK | MB_ICONERROR);
		}

		VARIANT data;
		hres = object->Get(L"ProcessId", 0, &data, NULL, NULL);
		if (FAILED(hres))
		{
			MessageBox(NULL, L"Не удалось получить PID процесса", L"Ошибка", MB_OK | MB_ICONERROR);
		}

		ProcessDeleted(data.intVal);
		UpdateProcessCountInfo();

		VariantClear(&vTarget);
		VariantClear(&data);

		object->Get(L"Name", 0, &data, NULL, NULL);
		pwProcessDeleted(data.bstrVal);
		VariantClear(&data);

		object->Release();
		pIUnknown->Release();
	}

	return WBEM_S_NO_ERROR;
}

HRESULT DeletionEventSink::SetStatus(LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR* pObjParam) {
	if (lFlags == WBEM_STATUS_COMPLETE) {

	}
	else if (lFlags == WBEM_STATUS_PROGRESS) {

	}

	return WBEM_S_NO_ERROR;
}