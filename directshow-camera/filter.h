#pragma once
#include <dshow.h>
#include <new>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <ks.h>
#include <ksmedia.h>
#include "shared_frame.h"
inline std::atomic<long> live_objects{0};
inline std::atomic<long> server_locks{0};
extern const CLSID CLSID_BandHeartBeatDirectShow;
inline HRESULT VideoType(AM_MEDIA_TYPE* t,bool rgb24=false,REFERENCE_TIME interval=333333) {
 if(!t)return E_POINTER; ZeroMemory(t,sizeof(*t)); t->majortype=MEDIATYPE_Video;t->subtype=rgb24?MEDIASUBTYPE_RGB24:MEDIASUBTYPE_RGB32;t->formattype=FORMAT_VideoInfo;t->bFixedSizeSamples=TRUE;t->lSampleSize=640*480*(rgb24?3:4);t->cbFormat=sizeof(VIDEOINFOHEADER);
 t->pbFormat=static_cast<BYTE*>(CoTaskMemAlloc(t->cbFormat));if(!t->pbFormat)return E_OUTOFMEMORY;ZeroMemory(t->pbFormat,t->cbFormat);auto v=reinterpret_cast<VIDEOINFOHEADER*>(t->pbFormat);v->AvgTimePerFrame=interval;v->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);v->bmiHeader.biWidth=640;v->bmiHeader.biHeight=480;v->bmiHeader.biPlanes=1;v->bmiHeader.biBitCount=rgb24?24:32;v->bmiHeader.biCompression=BI_RGB;v->bmiHeader.biSizeImage=t->lSampleSize;return S_OK;
}
inline bool ValidType(const AM_MEDIA_TYPE*t){if(!t||t->majortype!=MEDIATYPE_Video||(t->subtype!=MEDIASUBTYPE_RGB32&&t->subtype!=MEDIASUBTYPE_RGB24)||t->formattype!=FORMAT_VideoInfo||!t->pbFormat||t->cbFormat<sizeof(VIDEOINFOHEADER))return false;auto v=reinterpret_cast<const VIDEOINFOHEADER*>(t->pbFormat);return v->bmiHeader.biWidth==640&&v->bmiHeader.biHeight==480&&v->bmiHeader.biBitCount==(t->subtype==MEDIASUBTYPE_RGB24?24:32)&&v->bmiHeader.biCompression==BI_RGB&&v->AvgTimePerFrame>=333333&&v->AvgTimePerFrame<=2000000;}
class Types final:public IEnumMediaTypes { LONG refs=1;ULONG index=0;bool preferred24=false;REFERENCE_TIME interval=333333;public:Types(bool rgb24=false,REFERENCE_TIME frame_interval=333333):preferred24(rgb24),interval(frame_interval){++live_objects;}~Types(){--live_objects;}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID r,void**o)override{if(!o)return E_POINTER;*o=nullptr;if(r!=IID_IUnknown&&r!=IID_IEnumMediaTypes)return E_NOINTERFACE;*o=static_cast<IEnumMediaTypes*>(this);AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs);}ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs);if(!n)delete this;return n;}
 HRESULT STDMETHODCALLTYPE Next(ULONG n,AM_MEDIA_TYPE**t,ULONG*f)override{if(!t||(!f&&n!=1))return E_POINTER;ULONG count=0;while(count<n&&index<2){auto entry=static_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));if(!entry)return E_OUTOFMEMORY;auto hr=VideoType(entry,index==0?preferred24:!preferred24,interval);if(FAILED(hr)){CoTaskMemFree(entry);return hr;}t[count++]=entry;++index;}if(f)*f=count;return count==n?S_OK:S_FALSE;}
 HRESULT STDMETHODCALLTYPE Skip(ULONG n)override{ULONG remaining=2-index;index+=n<remaining?n:remaining;return n<=remaining?S_OK:S_FALSE;}HRESULT STDMETHODCALLTYPE Reset()override{index=0;return S_OK;}HRESULT STDMETHODCALLTYPE Clone(IEnumMediaTypes**o)override{if(!o)return E_POINTER;auto p=new(std::nothrow)Types(preferred24,interval);if(!p)return E_OUTOFMEMORY;p->index=index;*o=p;return S_OK;}
};
class Filter;
class Pin final:public IPin, public IAMStreamConfig, public IKsPropertySet,public IAMDroppedFrames { Filter* owner; IPin* peer=nullptr; IMemInputPin* input=nullptr; IMemAllocator* allocator=nullptr; std::thread worker; std::recursive_mutex control; std::mutex mutex; std::condition_variable wake; bool quitting=false,running=false,rgb24=false;REFERENCE_TIME interval=333333; public:std::atomic<long> delivered{0};
 HRESULT STDMETHODCALLTYPE GetNumDropped(long*n)override{if(!n)return E_POINTER;*n=0;return S_OK;}
 HRESULT STDMETHODCALLTYPE GetNumNotDropped(long*n)override{if(!n)return E_POINTER;*n=delivered;return S_OK;}
 HRESULT STDMETHODCALLTYPE GetDroppedInfo(long,long*,long*n)override{if(!n)return E_POINTER;*n=0;return S_OK;}
 HRESULT STDMETHODCALLTYPE GetAverageFrameSize(long*n)override{if(!n)return E_POINTER;*n=640*480*(rgb24?3:4);return S_OK;}
 explicit Pin(Filter*f):owner(f){}HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void**)override;ULONG STDMETHODCALLTYPE AddRef()override;ULONG STDMETHODCALLTYPE Release()override;
 HRESULT STDMETHODCALLTYPE Connect(IPin*,const AM_MEDIA_TYPE*)override; HRESULT StartDelivery(); void StopDelivery(); void PauseDelivery(); ~Pin(){StopDelivery(); Disconnect();}HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin*,const AM_MEDIA_TYPE*)override{return VFW_E_INVALID_DIRECTION;}HRESULT STDMETHODCALLTYPE Disconnect()override;HRESULT STDMETHODCALLTYPE ConnectedTo(IPin**o)override{if(!o)return E_POINTER;std::lock_guard<std::mutex>l(mutex);*o=peer;if(peer)peer->AddRef();return peer?S_OK:VFW_E_NOT_CONNECTED;}HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE*t)override{std::lock_guard<std::mutex>l(mutex);return peer?VideoType(t,rgb24,interval):VFW_E_NOT_CONNECTED;}

 HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE*t)override;
 HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE**t)override{if(!t)return E_POINTER;*t=static_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));return *t?VideoType(*t,rgb24,interval):E_OUTOFMEMORY;}
 HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int*n,int*z)override{if(!n||!z)return E_POINTER;*n=2;*z=sizeof(VIDEO_STREAM_CONFIG_CAPS);return S_OK;}
 HRESULT STDMETHODCALLTYPE GetStreamCaps(int i,AM_MEDIA_TYPE**t,BYTE*c)override{if(i<0||i>1)return S_FALSE;if(!c)return E_POINTER;if(!t)return E_POINTER;*t=static_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));if(!*t)return E_OUTOFMEMORY;auto hr=VideoType(*t,i==1);if(FAILED(hr))return hr;auto v=reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(c);ZeroMemory(v,sizeof(*v));v->guid=FORMAT_VideoInfo;v->InputSize={640,480};v->MinOutputSize=v->MaxOutputSize=v->InputSize;v->MinFrameInterval=333333;v->MaxFrameInterval=2000000;v->MinBitsPerSecond=640*480*(i==1?24:32)*5;v->MaxBitsPerSecond=640*480*(i==1?24:32)*30;return S_OK;}
 HRESULT STDMETHODCALLTYPE Set(REFGUID,DWORD,LPVOID,DWORD,LPVOID,DWORD)override{return E_NOTIMPL;}
 HRESULT STDMETHODCALLTYPE Get(REFGUID g,DWORD id,LPVOID,DWORD,LPVOID data,DWORD size,DWORD*ret)override{if(g!=AMPROPSETID_Pin||id!=AMPROPERTY_PIN_CATEGORY)return E_PROP_ID_UNSUPPORTED;if(ret)*ret=sizeof(GUID);if(!data)return S_OK;if(size<sizeof(GUID))return E_UNEXPECTED;*static_cast<GUID*>(data)=PIN_CATEGORY_CAPTURE;return S_OK;}
 HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID g,DWORD id,DWORD*o)override{if(!o)return E_POINTER;*o=0;if(g!=AMPROPSETID_Pin||id!=AMPROPERTY_PIN_CATEGORY)return E_PROP_ID_UNSUPPORTED;*o=KSPROPERTY_SUPPORT_GET;return S_OK;}
 HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO*)override;HRESULT STDMETHODCALLTYPE QueryDirection(PIN_DIRECTION*d)override{if(!d)return E_POINTER;*d=PINDIR_OUTPUT;return S_OK;}HRESULT STDMETHODCALLTYPE QueryId(LPWSTR*o)override{if(!o)return E_POINTER;*o=static_cast<LPWSTR>(CoTaskMemAlloc(14));if(!*o)return E_OUTOFMEMORY;wcscpy_s(*o,7,L"Output");return S_OK;}
 HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE*t)override{return ValidType(t)?S_OK:S_FALSE;}HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes**o)override{if(!o)return E_POINTER;*o=new(std::nothrow)Types(rgb24,interval);return *o?S_OK:E_OUTOFMEMORY;}HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin**,ULONG*)override{return E_NOTIMPL;}HRESULT STDMETHODCALLTYPE EndOfStream()override{return E_UNEXPECTED;}HRESULT STDMETHODCALLTYPE BeginFlush()override{return S_OK;}HRESULT STDMETHODCALLTYPE EndFlush()override{return S_OK;}HRESULT STDMETHODCALLTYPE NewSegment(REFERENCE_TIME,REFERENCE_TIME,double)override{return S_OK;}
};
class Pins final:public IEnumPins { LONG refs=1;IPin*pin;ULONG index=0;public:explicit Pins(IPin*p):pin(p){++live_objects;pin->AddRef();}~Pins(){pin->Release();--live_objects;}HRESULT STDMETHODCALLTYPE QueryInterface(REFIID r,void**o)override{if(!o)return E_POINTER;*o=nullptr;if(r!=IID_IUnknown&&r!=IID_IEnumPins)return E_NOINTERFACE;*o=static_cast<IEnumPins*>(this);AddRef();return S_OK;}ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs);}ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs);if(!n)delete this;return n;}HRESULT STDMETHODCALLTYPE Next(ULONG n,IPin**o,ULONG*f)override{if(!o||(!f&&n!=1))return E_POINTER;if(f)*f=0;if(!n)return S_OK;if(index)return S_FALSE;*o=pin;pin->AddRef();++index;if(f)*f=1;return n==1?S_OK:S_FALSE;}HRESULT STDMETHODCALLTYPE Skip(ULONG n)override{if(!n)return S_OK;if(index)return S_FALSE;index=1;return n==1?S_OK:S_FALSE;}HRESULT STDMETHODCALLTYPE Reset()override{index=0;return S_OK;}HRESULT STDMETHODCALLTYPE Clone(IEnumPins**o)override{if(!o)return E_POINTER;auto p=new(std::nothrow)Pins(pin);if(!p)return E_OUTOFMEMORY;p->index=index;*o=p;return S_OK;}
};
class Filter final:public IBaseFilter,public IAMFilterMiscFlags { LONG refs=1;std::atomic<FILTER_STATE> state{State_Stopped};IReferenceClock*clock=nullptr;IFilterGraph*graph=nullptr;public:Pin pin;Filter():pin(this){++live_objects;}~Filter(){pin.StopDelivery();if(clock)clock->Release();--live_objects;}
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID r,void**o)override{if(!o)return E_POINTER;*o=nullptr;if(r==IID_IAMFilterMiscFlags)*o=static_cast<IAMFilterMiscFlags*>(this);else if(r==IID_IUnknown||r==IID_IPersist||r==IID_IMediaFilter||r==IID_IBaseFilter)*o=static_cast<IBaseFilter*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs);}ULONG STDMETHODCALLTYPE Release()override{auto n=InterlockedDecrement(&refs);if(!n)delete this;return n;}
 ULONG STDMETHODCALLTYPE GetMiscFlags()override{return AM_FILTER_MISC_FLAGS_IS_SOURCE;}
 HRESULT STDMETHODCALLTYPE GetClassID(CLSID*c)override{if(!c)return E_POINTER;*c=CLSID_BandHeartBeatDirectShow;return S_OK;}HRESULT STDMETHODCALLTYPE Stop()override{pin.StopDelivery();state=State_Stopped;return S_OK;}HRESULT STDMETHODCALLTYPE Pause()override{pin.PauseDelivery();state=State_Paused;return S_OK;}HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME)override{auto hr=pin.StartDelivery();state=FAILED(hr)?State_Stopped:State_Running;return hr;}HRESULT STDMETHODCALLTYPE GetState(DWORD,FILTER_STATE*s)override{if(!s)return E_POINTER;*s=state;return state==State_Paused?VFW_S_CANT_CUE:S_OK;}HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock*c)override{if(c)c->AddRef();if(clock)clock->Release();clock=c;return S_OK;}HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock**c)override{if(!c)return E_POINTER;*c=clock;if(clock)clock->AddRef();return S_OK;}HRESULT STDMETHODCALLTYPE EnumPins(IEnumPins**o)override{if(!o)return E_POINTER;*o=new(std::nothrow)Pins(&pin);return *o?S_OK:E_OUTOFMEMORY;}HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR id,IPin**o)override{if(!o||!id)return E_POINTER;*o=nullptr;if(wcscmp(id,L"Output"))return VFW_E_NOT_FOUND;*o=&pin;pin.AddRef();return S_OK;}HRESULT STDMETHODCALLTYPE QueryFilterInfo(FILTER_INFO*i)override{if(!i)return E_POINTER;ZeroMemory(i,sizeof(*i));wcscpy_s(i->achName,L"BandHeartBeat Virtual Camera");i->pGraph=graph;if(graph)graph->AddRef();return S_OK;}HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph*g,LPCWSTR)override{graph=g;return S_OK;}HRESULT STDMETHODCALLTYPE QueryVendorInfo(LPWSTR*o)override{if(o)*o=nullptr;return E_NOTIMPL;}
};
inline HRESULT Pin::QueryInterface(REFIID r,void**o){if(!o)return E_POINTER;*o=nullptr;if(r==IID_IAMDroppedFrames)*o=static_cast<IAMDroppedFrames*>(this);else if(r==IID_IAMStreamConfig)*o=static_cast<IAMStreamConfig*>(this);else if(r==__uuidof(IKsPropertySet))*o=static_cast<IKsPropertySet*>(this);else if(r==IID_IUnknown||r==IID_IPin)*o=static_cast<IPin*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}inline ULONG Pin::AddRef(){return owner->AddRef();}inline ULONG Pin::Release(){return owner->Release();}inline HRESULT Pin::QueryPinInfo(PIN_INFO*i){if(!i)return E_POINTER;ZeroMemory(i,sizeof(*i));i->pFilter=owner;owner->AddRef();i->dir=PINDIR_OUTPUT;wcscpy_s(i->achName,L"Output");return S_OK;}

inline HRESULT Pin::SetFormat(AM_MEDIA_TYPE*t){std::lock_guard<std::mutex>l(mutex);if(peer)return VFW_E_ALREADY_CONNECTED;if(!ValidType(t))return VFW_E_TYPE_NOT_ACCEPTED;rgb24=t->subtype==MEDIASUBTYPE_RGB24;interval=reinterpret_cast<const VIDEOINFOHEADER*>(t->pbFormat)->AvgTimePerFrame;return S_OK;}
inline HRESULT Pin::Connect(IPin*p,const AM_MEDIA_TYPE*t){
 if(!p)return E_POINTER;std::lock_guard<std::mutex>l(mutex);if(peer)return VFW_E_ALREADY_CONNECTED;
 AM_MEDIA_TYPE mt{};bool selected24=t?t->subtype==MEDIASUBTYPE_RGB24:rgb24;REFERENCE_TIME selected_interval=t&&t->pbFormat&&t->cbFormat>=sizeof(VIDEOINFOHEADER)?reinterpret_cast<const VIDEOINFOHEADER*>(t->pbFormat)->AvgTimePerFrame:interval;auto hr=VideoType(&mt,selected24,selected_interval);if(FAILED(hr))return hr;if(t&&!ValidType(t)){CoTaskMemFree(mt.pbFormat);return VFW_E_TYPE_NOT_ACCEPTED;}
 IMemInputPin*in=nullptr;IMemAllocator*a=nullptr;hr=p->QueryInterface(IID_IMemInputPin,(void**)&in);
 if(SUCCEEDED(hr)){hr=p->ReceiveConnection(this,&mt);if(FAILED(hr)&&!t&&!selected24){CoTaskMemFree(mt.pbFormat);selected24=true;VideoType(&mt,true,selected_interval);hr=p->ReceiveConnection(this,&mt);}}
 if(SUCCEEDED(hr)){hr=in->GetAllocator(&a);if(FAILED(hr))hr=CoCreateInstance(CLSID_MemoryAllocator,nullptr,CLSCTX_INPROC_SERVER,IID_IMemAllocator,(void**)&a);}
 ALLOCATOR_PROPERTIES request{2,640*480*4,1,0},actual{};
 if(SUCCEEDED(hr))hr=a->SetProperties(&request,&actual);
 if(SUCCEEDED(hr)&&(actual.cbBuffer<request.cbBuffer||actual.cBuffers<1))hr=E_FAIL;
 if(SUCCEEDED(hr))hr=in->NotifyAllocator(a,FALSE);
 if(SUCCEEDED(hr)){peer=p;p->AddRef();input=in;allocator=a;rgb24=selected24;interval=selected_interval;}else{if(a)a->Release();if(in)in->Release();p->Disconnect();}
 CoTaskMemFree(mt.pbFormat);return hr;
}
inline HRESULT Pin::Disconnect(){StopDelivery();std::lock_guard<std::mutex>l(mutex);if(!peer)return S_FALSE;peer->Release();peer=nullptr;input->Release();input=nullptr;allocator->Release();allocator=nullptr;return S_OK;}
inline void Pin::PauseDelivery(){std::lock_guard<std::recursive_mutex>guard(control);std::lock_guard<std::mutex>l(mutex);running=false;wake.notify_all();}
inline void Pin::StopDelivery(){
 std::lock_guard<std::recursive_mutex>guard(control);
 {std::lock_guard<std::mutex>l(mutex);quitting=true;running=false;wake.notify_all();if(peer)peer->BeginFlush();if(allocator)allocator->Decommit();}
 if(worker.joinable())worker.join();
 {std::lock_guard<std::mutex>l(mutex);if(peer)peer->EndFlush();}
}
inline HRESULT Pin::StartDelivery(){
 std::lock_guard<std::recursive_mutex>guard(control);
 std::lock_guard<std::mutex>l(mutex);if(!peer)return VFW_E_NOT_CONNECTED;if(worker.joinable()){running=true;wake.notify_all();return S_OK;}
 auto commit=allocator->Commit();if(FAILED(commit))return commit;quitting=false;running=true;
 try{std::vector<BYTE>initial_placeholder(640*480*4),initial_cached(640*480*4),initial_working(640*480*4);worker=std::thread([this,placeholder=std::move(initial_placeholder),cached=std::move(initial_cached),working=std::move(initial_working)]()mutable{
  CoInitializeEx(nullptr,COINIT_MULTITHREADED);REFERENCE_TIME time=0;bool first=true;auto deadline=std::chrono::steady_clock::now();
  bool have_good=false;uint64_t last_sequence=0;ULONGLONG last_good=0;{BYTE*p=placeholder.data();
    for(int y=0;y<480;y++)for(int x=0;x<640;x++){int i=(y*640+x)*4;bool mark=(x/40+y/40)%2;p[i]=mark?0x73:0x20;p[i+1]=mark?0x8b:0x20;p[i+2]=mark?0xff:0x20;p[i+3]=0xff;}
    HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=640;bi.bmiHeader.biHeight=480;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;void*bits=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);
    if(bitmap&&bits){auto old=SelectObject(dc,bitmap);memcpy(bits,p,640*480*4);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(255,255,255));TextOutW(dc,180,220,L"PLACEHOLDER - NO CAMERA",23);memcpy(p,bits,640*480*4);SelectObject(dc,old);}if(bitmap)DeleteObject(bitmap);DeleteDC(dc);
  }
  while(true){
   {std::unique_lock<std::mutex>l(mutex);wake.wait(l,[this]{return quitting||running;});if(quitting)break;}
   auto now=std::chrono::steady_clock::now();if(deadline<now-std::chrono::milliseconds(100))deadline=now;deadline+=std::chrono::nanoseconds(interval*100);
   IMediaSample*s=nullptr;HRESULT hr=allocator->GetBuffer(&s,nullptr,nullptr,0);if(FAILED(hr))break;
   BYTE*sample_data=nullptr;hr=s->GetPointer(&sample_data);BYTE*p=working.data();
   if(SUCCEEDED(hr)){
    // Fixed-size, versioned latest-frame IPC; mutex protects publication/copy.
    HANDLE map=OpenFileMappingW(FILE_MAP_READ,FALSE,BH_SHM_NAME);
    if(map){auto view=static_cast<const BYTE*>(MapViewOfFile(map,FILE_MAP_READ,0,0,sizeof(BhFrameHeader)+640*480*4));HANDLE gate=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,BH_FRAME_MUTEX);
     DWORD waited=(view&&gate)?WaitForSingleObject(gate,0):WAIT_TIMEOUT; if(waited==WAIT_OBJECT_0){auto h=reinterpret_cast<const BhFrameHeader*>(view);if(h->magic==0x42484247&&h->version==1&&h->width==640&&h->height==480&&h->stride==2560&&h->bytes==640*480*4){if(!have_good||h->sequence!=last_sequence){memcpy(cached.data(),view+sizeof(BhFrameHeader),h->bytes);last_sequence=h->sequence;last_good=GetTickCount64();have_good=true;}}else have_good=false;ReleaseMutex(gate);}else if(waited==WAIT_ABANDONED){ReleaseMutex(gate);}
     if(gate)CloseHandle(gate);if(view)UnmapViewOfFile(view);CloseHandle(map);}else have_good=false;
    memcpy(p,have_good&&GetTickCount64()-last_good<=2000?cached.data():placeholder.data(),640*480*4);
    if(!rgb24)memcpy(sample_data,p,640*480*4);
    if(rgb24)for(int i=0;i<640*480;i++){sample_data[i*3]=p[i*4];sample_data[i*3+1]=p[i*4+1];sample_data[i*3+2]=p[i*4+2];}
    REFERENCE_TIME end=time+interval;s->SetTime(&time,&end);s->SetActualDataLength(640*480*(rgb24?3:4));s->SetSyncPoint(TRUE);s->SetDiscontinuity(first);first=false;time=end;hr=input->Receive(s);if(hr==S_OK)++delivered;
   }
   s->Release();if(FAILED(hr))break;
   std::unique_lock<std::mutex>l(mutex);if(wake.wait_until(l,deadline,[this]{return quitting||!running;})&&quitting)break;
  }
  CoUninitialize();
 });}catch(...){allocator->Decommit();running=false;quitting=true;return E_OUTOFMEMORY;}return S_OK;
}
