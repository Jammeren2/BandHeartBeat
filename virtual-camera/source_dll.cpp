#define WIN32_LEAN_AND_MEAN
#include "source_contract.h"
#include <new>

class PlaceholderSource final : public IMFMediaSource {
    volatile long refs_ = 1; bool shutdown_ = false;
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **out) override {
        if (!out) return E_POINTER; *out = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(IMFMediaSource) || riid == __uuidof(IMFMediaEventGenerator)) {
            *out = static_cast<IMFMediaSource*>(this); AddRef(); return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&refs_); }
    STDMETHODIMP_(ULONG) Release() override { ULONG n = InterlockedDecrement(&refs_); if (!n) delete this; return n; }
    STDMETHODIMP GetCharacteristics(DWORD *v) override { if (!v) return E_POINTER; *v = MFMEDIASOURCE_IS_LIVE; return S_OK; }
    STDMETHODIMP CreatePresentationDescriptor(IMFPresentationDescriptor **v) override { if (v) *v = nullptr; return E_NOTIMPL; }
    STDMETHODIMP Start(IMFPresentationDescriptor*, const GUID*, const PROPVARIANT*) override { return shutdown_ ? E_UNEXPECTED : E_NOTIMPL; }
    STDMETHODIMP Pause() override { return shutdown_ ? E_UNEXPECTED : E_NOTIMPL; }
    STDMETHODIMP Stop() override { return shutdown_ ? E_UNEXPECTED : E_NOTIMPL; }
    STDMETHODIMP Shutdown() override { shutdown_ = true; return S_OK; }
    STDMETHODIMP GetEvent(DWORD, IMFMediaEvent **v) override { if (v) *v = nullptr; return shutdown_ ? E_UNEXPECTED : E_NOTIMPL; }
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback*, IUnknown*) override { return shutdown_ ? E_UNEXPECTED : E_NOTIMPL; }
    STDMETHODIMP EndGetEvent(IMFAsyncResult*, IMFMediaEvent **v) override { if (v) *v = nullptr; return E_NOTIMPL; }
    STDMETHODIMP QueueEvent(MediaEventType, REFGUID, HRESULT, const PROPVARIANT*) override { return shutdown_ ? E_UNEXPECTED : E_NOTIMPL; }
};

extern "C" __declspec(dllexport) HRESULT __stdcall CreatePlaceholderSource(IMFMediaSource **out) {
    if (!out) return E_POINTER; *out = new (std::nothrow) PlaceholderSource(); return *out ? S_OK : E_OUTOFMEMORY;
}

// Compileable COM in-process boundary. The source object is lifecycle-valid,
// while media stream/type/sample production is intentionally the next stage.
class Factory final : public IClassFactory {
    volatile long refs_ = 1;
public:
    STDMETHODIMP QueryInterface(REFIID riid, void **out) override { if (!out) return E_POINTER; *out=nullptr; if (riid==IID_IUnknown||riid==IID_IClassFactory){*out=static_cast<IClassFactory*>(this);AddRef();return S_OK;} return E_NOINTERFACE; }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&refs_); }
    STDMETHODIMP_(ULONG) Release() override { ULONG n=InterlockedDecrement(&refs_); if(!n) delete this; return n; }
    STDMETHODIMP CreateInstance(IUnknown *outer, REFIID riid, void **out) override { if (outer) return CLASS_E_NOAGGREGATION; PlaceholderSource *s=new(std::nothrow) PlaceholderSource(); if(!s) return E_OUTOFMEMORY; HRESULT hr=s->QueryInterface(riid,out); s->Release(); return hr; }
    STDMETHODIMP LockServer(BOOL) override { return S_OK; }
};
STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, LPVOID *out) {
    if (!out) return E_POINTER; *out=nullptr; if (!IsEqualCLSID(clsid, CLSID_BandHeartBeatSource)) return CLASS_E_CLASSNOTAVAILABLE; Factory *f=new(std::nothrow) Factory(); if(!f) return E_OUTOFMEMORY; HRESULT hr=f->QueryInterface(riid,out); f->Release(); return hr;
}
STDAPI DllCanUnloadNow() { return S_OK; }
STDAPI DllRegisterServer() { return E_NOTIMPL; }
STDAPI DllUnregisterServer() { return E_NOTIMPL; }

HRESULT RegisterCurrentUserSource() { return E_NOTIMPL; }
HRESULT UnregisterCurrentUserSource() { return E_NOTIMPL; }

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
