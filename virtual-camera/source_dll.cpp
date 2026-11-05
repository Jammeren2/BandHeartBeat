#define WIN32_LEAN_AND_MEAN
#include "source_contract.h"
#include <new>
#include <strsafe.h>

class PlaceholderStream final : public IMFMediaStream {
    volatile long refs_ = 1; LONGLONG next_time_ = 0; bool stopped_ = false; IMFMediaEventQueue *queue_ = nullptr;
public:
    PlaceholderStream() { MFCreateEventQueue(&queue_); }
    ~PlaceholderStream(){ if(queue_) queue_->Shutdown(); }
    STDMETHODIMP QueryInterface(REFIID r, void **o) override { if(!o)return E_POINTER;*o=nullptr; if(r==IID_IUnknown||r==__uuidof(IMFMediaStream)||r==__uuidof(IMFMediaEventGenerator)){*o=static_cast<IMFMediaStream*>(this);AddRef();return S_OK;} return E_NOINTERFACE; }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&refs_); }
    STDMETHODIMP_(ULONG) Release() override { ULONG n=InterlockedDecrement(&refs_);if(!n)delete this;return n; }
    STDMETHODIMP GetEvent(DWORD f,IMFMediaEvent **o) override{return queue_?queue_->GetEvent(f,o):E_UNEXPECTED;}
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback*c,IUnknown*s) override{return queue_?queue_->BeginGetEvent(c,s):E_UNEXPECTED;}
    STDMETHODIMP EndGetEvent(IMFAsyncResult*r,IMFMediaEvent **o) override{return queue_?queue_->EndGetEvent(r,o):E_UNEXPECTED;}
    STDMETHODIMP QueueEvent(MediaEventType t,REFGUID g,HRESULT h,const PROPVARIANT*v) override{return queue_?queue_->QueueEventParamVar(t,g,h,v):E_UNEXPECTED;}
    STDMETHODIMP GetMediaSource(IMFMediaSource **o) override{if(o)*o=nullptr;return E_NOTIMPL;}
    STDMETHODIMP GetStreamDescriptor(IMFStreamDescriptor **o) override{if(o)*o=nullptr;return E_NOTIMPL;}
    STDMETHODIMP RequestSample(IUnknown*) override {
        if(stopped_)return E_UNEXPECTED; IMFSample *sample=nullptr; IMFMediaBuffer *buffer=nullptr;
        HRESULT hr=MFCreateMemoryBuffer(320*240*4,&buffer); if(FAILED(hr))return hr; BYTE *p=nullptr;DWORD max=0,cur=0;
        hr=buffer->Lock(&p,&max,&cur); if(SUCCEEDED(hr)){for(DWORD i=0;i<max;i+=4){p[i]=0x73;p[i+1]=0x8b;p[i+2]=0xff;p[i+3]=0xff;} buffer->Unlock();hr=MFCreateSample(&sample);}
        if(SUCCEEDED(hr))hr=sample->AddBuffer(buffer); if(SUCCEEDED(hr))hr=sample->SetSampleTime(next_time_); if(SUCCEEDED(hr))hr=sample->SetSampleDuration(200000);
        if(SUCCEEDED(hr)&&queue_)hr=queue_->QueueEventParamUnk(MEMediaSample,GUID_NULL,S_OK,sample);
        if(sample)sample->Release();buffer->Release();next_time_+=200000;return hr;
    }
    void Stop(){stopped_=true;if(queue_)queue_->QueueEventParamVar(MEStreamStopped,GUID_NULL,S_OK,nullptr);}
    void Start(){stopped_=false;if(queue_)queue_->QueueEventParamVar(MEStreamStarted,GUID_NULL,S_OK,nullptr);}
};
extern "C" __declspec(dllexport) HRESULT __stdcall CreatePlaceholderStream(IMFMediaStream **o){if(!o)return E_POINTER;*o=new(std::nothrow)PlaceholderStream();return *o?S_OK:E_OUTOFMEMORY;}

class PlaceholderSource final : public IMFMediaSource {
    volatile long refs_ = 1; bool shutdown_ = false; PlaceholderStream *stream_ = nullptr; IMFMediaEventQueue *queue_ = nullptr;
public:
    PlaceholderSource() : stream_(new(std::nothrow) PlaceholderStream()) { MFCreateEventQueue(&queue_); }
    ~PlaceholderSource() { if (stream_) stream_->Release(); if(queue_) queue_->Shutdown(); }
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
    STDMETHODIMP CreatePresentationDescriptor(IMFPresentationDescriptor **v) override {
        if (!v) return E_POINTER; *v = nullptr;
        IMFMediaType *type = nullptr; IMFStreamDescriptor *sd = nullptr;
        HRESULT hr = MFCreateMediaType(&type); if (FAILED(hr)) return hr;
        hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (SUCCEEDED(hr)) hr = type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        if (SUCCEEDED(hr)) hr = type->SetUINT32(MF_MT_AVG_BITRATE, 320u * 240u * 32u * 5u);
        if (SUCCEEDED(hr)) hr = MFCreateStreamDescriptor(1, 1, &type, &sd);
        type->Release();
        if (FAILED(hr)) return hr;
        IMFPresentationDescriptor *pd = nullptr;
        hr = MFCreatePresentationDescriptor(1, &sd, &pd); sd->Release();
        if (FAILED(hr)) return hr;
        hr = pd->SelectStream(0);
        if (SUCCEEDED(hr)) *v = pd; else pd->Release();
        return hr;
    }
    STDMETHODIMP Start(IMFPresentationDescriptor*, const GUID*, const PROPVARIANT*) override { if (shutdown_) return E_UNEXPECTED; if(stream_)stream_->Start(); if(queue_)queue_->QueueEventParamVar(MESourceStarted,GUID_NULL,S_OK,nullptr); return stream_ ? stream_->RequestSample(nullptr) : E_OUTOFMEMORY; }
    STDMETHODIMP Pause() override { return shutdown_ ? E_UNEXPECTED : S_OK; }
    STDMETHODIMP Stop() override { if (stream_) stream_->Stop(); if(queue_)queue_->QueueEventParamVar(MESourceStopped,GUID_NULL,S_OK,nullptr); return shutdown_ ? E_UNEXPECTED : S_OK; }
    STDMETHODIMP Shutdown() override { if (!shutdown_ && stream_) stream_->Stop(); shutdown_ = true; if(queue_)queue_->Shutdown(); return S_OK; }
    STDMETHODIMP GetEvent(DWORD f, IMFMediaEvent **v) override { return queue_?queue_->GetEvent(f,v):E_UNEXPECTED; }
    STDMETHODIMP BeginGetEvent(IMFAsyncCallback*c, IUnknown*s) override { return queue_?queue_->BeginGetEvent(c,s):E_UNEXPECTED; }
    STDMETHODIMP EndGetEvent(IMFAsyncResult*r, IMFMediaEvent **v) override { return queue_?queue_->EndGetEvent(r,v):E_UNEXPECTED; }
    STDMETHODIMP QueueEvent(MediaEventType t, REFGUID g, HRESULT h, const PROPVARIANT*v) override { return queue_?queue_->QueueEventParamVar(t,g,h,v):E_UNEXPECTED; }
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
static HMODULE self_module = nullptr;
static HRESULT RegisterCurrentUserSource() {
    wchar_t guid[64]; StringFromGUID2(CLSID_BandHeartBeatSource, guid, 64);
    wchar_t key[256]; StringCchPrintfW(key, 256, L"Software\\Classes\\CLSID\\%s\\InprocServer32", guid);
    HKEY h = nullptr; LONG e = RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, nullptr, 0, KEY_WRITE, nullptr, &h, nullptr);
    if (e != ERROR_SUCCESS) return HRESULT_FROM_WIN32(e);
    wchar_t path[MAX_PATH]; DWORD n = GetModuleFileNameW(self_module, path, MAX_PATH);
    if (!n || RegSetValueExW(h, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE*>(path), (n+1)*sizeof(wchar_t)) != ERROR_SUCCESS) { RegCloseKey(h); return E_FAIL; }
    const wchar_t *model=L"Both"; e=RegSetValueExW(h,L"ThreadingModel",0,REG_SZ,reinterpret_cast<const BYTE*>(model),6*sizeof(wchar_t)); RegCloseKey(h); return HRESULT_FROM_WIN32(e);
}
static HRESULT UnregisterCurrentUserSource() { wchar_t guid[64], key[256]; StringFromGUID2(CLSID_BandHeartBeatSource,guid,64); StringCchPrintfW(key,256,L"Software\\Classes\\CLSID\\%s",guid); LONG e=RegDeleteTreeW(HKEY_CURRENT_USER,key); return e==ERROR_FILE_NOT_FOUND?S_OK:HRESULT_FROM_WIN32(e); }
STDAPI DllRegisterServer() { return RegisterCurrentUserSource(); }
STDAPI DllUnregisterServer() { return UnregisterCurrentUserSource(); }

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) { if(reason==DLL_PROCESS_ATTACH)self_module=module; return TRUE; }
