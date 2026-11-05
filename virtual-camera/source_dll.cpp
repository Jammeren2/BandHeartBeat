#define WIN32_LEAN_AND_MEAN
#include "source_contract.h"

// Compileable COM in-process boundary. Returning CLASS_E_CLASSNOTAVAILABLE is
// intentional: an IMFMediaSource implementation is the next isolated spike.
STDAPI DllGetClassObject(REFCLSID clsid, REFIID, LPVOID*) {
    return IsEqualCLSID(clsid, CLSID_BandHeartBeatSource) ? CLASS_E_CLASSNOTAVAILABLE : CLASS_E_CLASSNOTAVAILABLE;
}
STDAPI DllCanUnloadNow() { return S_OK; }
STDAPI DllRegisterServer() { return E_NOTIMPL; }
STDAPI DllUnregisterServer() { return E_NOTIMPL; }

HRESULT RegisterCurrentUserSource() { return E_NOTIMPL; }
HRESULT UnregisterCurrentUserSource() { return E_NOTIMPL; }

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
