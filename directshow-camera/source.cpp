#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strmif.h>
#include <new>
#include "shared_frame.h"
#include "filter.h"
const CLSID CLSID_BandHeartBeatDirectShow={0x5b4131af,0x3298,0x47aa,{0x9c,0x12,0x33,0x40,0x7a,0x81,0x91,0x4b}};
class Factory: public IClassFactory { volatile long refs_=1; public:
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID r,void**o) override{if(!o)return E_POINTER;*o=nullptr;if(r==IID_IUnknown||r==IID_IClassFactory){*o=(IClassFactory*)this;AddRef();return S_OK;}return E_NOINTERFACE;} ULONG STDMETHODCALLTYPE AddRef() override{return InterlockedIncrement(&refs_);} ULONG STDMETHODCALLTYPE Release() override{ULONG n=InterlockedDecrement(&refs_);if(!n)delete this;return n;} HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown*outer,REFIID r,void**o) override{if(!o)return E_POINTER;*o=nullptr;if(outer)return CLASS_E_NOAGGREGATION;auto f=new(std::nothrow)Filter();if(!f)return E_OUTOFMEMORY;auto hr=f->QueryInterface(r,o);f->Release();return hr;} HRESULT STDMETHODCALLTYPE LockServer(BOOL) override{return S_OK;}
};
STDAPI DllGetClassObject(REFCLSID c,REFIID r,void**o){if(!IsEqualCLSID(c,CLSID_BandHeartBeatDirectShow))return CLASS_E_CLASSNOTAVAILABLE;Factory*f=new(std::nothrow)Factory();if(!f)return E_OUTOFMEMORY;HRESULT h=f->QueryInterface(r,o);f->Release();return h;} STDAPI DllCanUnloadNow(){return S_OK;} STDAPI DllRegisterServer(){return E_NOTIMPL;} STDAPI DllUnregisterServer(){return E_NOTIMPL;} BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID){return TRUE;}



