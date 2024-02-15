
#include "StareApps.h"

#include <iostream>

ULONG CreationEventSink::AddRef() {
	return InterlockedIncrement(&m_lRef);
}

ULONG CreationEventSink::Release() {
	LONG lRef = InterlockedDecrement(&m_lRef);
	if (lRef == 0) {
		delete this;
	}

	return lRef;
}

HRESULT CreationEventSink::QueryInterface(REFIID riid, void** ppv) {
	if (riid == IID_IUnknown || riid == IID_IWbemObjectSink) {
		*ppv = (IWbemObjectSink*)this;
		AddRef();
		return WBEM_S_NO_ERROR;
	}
	else {
		return E_NOINTERFACE;
	}
}

HRESULT CreationEventSink::Indicate(long lObjectCount, IWbemClassObject** apObjArray) {
	HRESULT hres = S_OK;

	for (int i = 0; i < lObjectCount; i++) {

		_variant_t vTarget{};
		hres = apObjArray[i]->Get(bstr_t("TargetInstance"), NULL, &vTarget, NULL, NULL);
		if (FAILED(hres))
		{
			MessageBox(NULL, L"Не удалось получить объект", L"Ошибка", MB_OK | MB_ICONERROR);
		}

		IUnknown* unknown = (IUnknown*)vTarget;
		IWbemClassObject* object;

		hres =  unknown->QueryInterface(IID_IWbemClassObject, (void**)&object);
		if (FAILED(hres))
		{
			MessageBox(NULL, L"Не удалось преобразовать объект", L"Ошибка", MB_OK | MB_ICONERROR);
		}

		ProcessCreated(object);
		UpdateProcessCountInfo();

		VariantClear(&vTarget);
		unknown->Release();

		object->Release();
	}

	return WBEM_S_NO_ERROR;
}

HRESULT CreationEventSink::SetStatus(LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR* pObjParam) {
	if (lFlags == WBEM_STATUS_COMPLETE) {

	}
	else if (lFlags == WBEM_STATUS_PROGRESS) {

	}

	return WBEM_S_NO_ERROR;
}