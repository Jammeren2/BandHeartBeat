#include <dshow.h>
#include <iostream>
#include <string>
#include "test_sink.h"
#include "installed_camera.h"
#include "registration_store.h"
int wmain(int argc,wchar_t**argv){std::wstring cmd=argc>1?argv[1]:L"--dry-run";
 SetConsoleOutputCP(CP_UTF8);std::wstring prefix=L"Software\\Classes";
#ifdef BH_REGISTRY_TEST
 prefix=L"Software\\BandHeartBeat\\RegistrationQa";
#endif
 RegistrationStore store(prefix);wchar_t exe[32768];GetModuleFileNameW(nullptr,exe,32768);std::wstring current=exe;current.resize(current.find_last_of(L"\\")+1);current+=L"BandHeartBeatDirectShow.dll";
 if(cmd==L"--status"){std::wcout<<L"Installed DLL: "<<store.Read(store.cls+L"\\InprocServer32",nullptr)<<L"\nCurrent DLL: "<<current<<L"\n";return 0;}
 if(cmd==L"--dry-run"){std::cout<<"No changes. --register installs current build; --unregister removes only verified own records.\n";return 0;}
 if(cmd==L"--register"||cmd==L"--unregister"){HRESULT hr=cmd==L"--register"?store.Install(current):store.Remove();wchar_t*reason=nullptr;FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,HRESULT_FACILITY(hr)==FACILITY_WIN32?HRESULT_CODE(hr):static_cast<DWORD>(hr),0,reinterpret_cast<LPWSTR>(&reason),0,nullptr);std::wstring text=cmd+L" HRESULT 0x";wchar_t hex[16];swprintf_s(hex,L"%08X",static_cast<unsigned>(hr));text+=hex;text+=SUCCEEDED(hr)?L" success":L" failed";if(reason){text+=L": ";text+=reason;LocalFree(reason);}if(hr==HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))text+=L" Remove old registration, then Install.";if(hr==E_ACCESSDENIED)text+=L" Ownership not verified; foreign records preserved.";int size=WideCharToMultiByte(CP_UTF8,0,text.c_str(),-1,nullptr,0,nullptr,nullptr);std::string utf8(size,0);WideCharToMultiByte(CP_UTF8,0,text.c_str(),-1,utf8.data(),size,nullptr,nullptr);std::cout<<utf8.c_str()<<"\n";return FAILED(hr)?3:0;}
 if(cmd!=L"--probe"&&cmd!=L"--probe-producer")return 4;
 HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(FAILED(hr))return 5;ICreateDevEnum*devices=nullptr;IEnumMoniker*items=nullptr;IMoniker*m=nullptr;IBaseFilter*filter=nullptr;
 hr=CoCreateInstance(CLSID_SystemDeviceEnum,nullptr,CLSCTX_INPROC_SERVER,IID_ICreateDevEnum,(void**)&devices);if(SUCCEEDED(hr))hr=devices->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&items,0);
 if(hr==S_OK){while(items->Next(1,&m,nullptr)==S_OK){IPropertyBag*bag=nullptr;m->BindToStorage(nullptr,nullptr,IID_IPropertyBag,(void**)&bag);VARIANT value;VariantInit(&value);if(bag&&SUCCEEDED(bag->Read(L"FriendlyName",&value,nullptr))&&value.vt==VT_BSTR&&wcscmp(value.bstrVal,L"BandHeartBeat Virtual Camera")==0){std::wcout<<L"DirectShow category: BandHeartBeat Virtual Camera found\n";hr=m->BindToObject(nullptr,nullptr,IID_IBaseFilter,(void**)&filter);VariantClear(&value);bag->Release();m->Release();m=nullptr;break;}VariantClear(&value);if(bag)bag->Release();m->Release();m=nullptr;}}
 int result=6;if(filter){std::wcout<<L"BindToObject HRESULT S_OK\n";IPin*pin=nullptr;hr=filter->FindPin(L"Output",&pin);auto sink=new TestSink();sink->verifyPixel=false;sink->expectedInterval=0;sink->producerPattern=cmd==L"--probe-producer";if(SUCCEEDED(hr))hr=pin->Connect(sink,nullptr);if(SUCCEEDED(hr))hr=filter->Run(0);bool delivered=SUCCEEDED(hr)&&sink->Wait(3)&&sink->valid;filter->Stop();if(pin){pin->Disconnect();pin->Release();}sink->Disconnect();sink->Release();filter->Release();result=delivered?0:7;std::cout<<(delivered?"PASS registered moniker -> BindToObject -> allocator/Receive 3 samples\n":"FAIL registered sample delivery\n");}else std::wcerr<<L"Virtual source not found/bind failed HRESULT 0x"<<std::hex<<hr<<L"\n";
 if(items)items->Release();if(devices)devices->Release();CoUninitialize();return result;
}
