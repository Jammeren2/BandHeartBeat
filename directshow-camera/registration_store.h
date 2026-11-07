#pragma once
#include <windows.h>
#include <string>
struct RegistrationStore {
 std::wstring cls,category;
 static constexpr const wchar_t* id=L"{5B4131AF-3298-47AA-9C12-33407A81914B}";
 explicit RegistrationStore(std::wstring prefix=L"Software\\Classes"){cls=prefix+L"\\CLSID\\"+id;category=prefix+L"\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\"+id;}
 bool Exists(const std::wstring&p)const{HKEY h=nullptr;auto e=RegOpenKeyExW(HKEY_CURRENT_USER,p.c_str(),0,KEY_READ,&h);if(h)RegCloseKey(h);return e!=ERROR_FILE_NOT_FOUND&&e!=ERROR_PATH_NOT_FOUND;}
 std::wstring Read(const std::wstring&p,const wchar_t*n)const{DWORD size=0;if(RegGetValueW(HKEY_CURRENT_USER,p.c_str(),n,RRF_RT_REG_SZ,nullptr,nullptr,&size))return {};std::wstring s(size/sizeof(wchar_t),0);if(RegGetValueW(HKEY_CURRENT_USER,p.c_str(),n,RRF_RT_REG_SZ,nullptr,s.data(),&size))return {};while(!s.empty()&&!s.back())s.pop_back();return s;}
 bool Any()const{return Exists(cls)||Exists(category);}
 bool Owned()const{auto marker=Read(cls,L"Product"),guid=Read(category,L"CLSID"),name=Read(category,L"FriendlyName"),threading=Read(cls+L"\\InprocServer32",L"ThreadingModel"),path=Read(cls+L"\\InprocServer32",nullptr);auto slash=path.find_last_of(L"\\/");auto base=path.substr(slash==std::wstring::npos?0:slash+1);if((!marker.empty()&&marker!=L"BandHeartBeat")||(!guid.empty()&&_wcsicmp(guid.c_str(),id))||(!name.empty()&&name!=L"BandHeartBeat Virtual Camera")||(!threading.empty()&&threading!=L"Both")||(!base.empty()&&_wcsicmp(base.c_str(),L"BandHeartBeatDirectShow.dll")))return false;return marker==L"BandHeartBeat"||!guid.empty()||name==L"BandHeartBeat Virtual Camera"||!base.empty();}
 HRESULT Put(const std::wstring&p,const wchar_t*n,const std::wstring&value)const{HKEY h=nullptr;auto e=RegCreateKeyExW(HKEY_CURRENT_USER,p.c_str(),0,nullptr,0,KEY_WRITE,nullptr,&h,nullptr);if(e)return HRESULT_FROM_WIN32(e);e=RegSetValueExW(h,n,0,REG_SZ,reinterpret_cast<const BYTE*>(value.c_str()),static_cast<DWORD>((value.size()+1)*sizeof(wchar_t)));RegCloseKey(h);return HRESULT_FROM_WIN32(e);}
 HRESULT Remove()const{if(!Any())return S_OK;if(!Owned())return E_ACCESSDENIED;auto e=RegDeleteTreeW(HKEY_CURRENT_USER,category.c_str());if(e&&e!=ERROR_FILE_NOT_FOUND&&e!=ERROR_PATH_NOT_FOUND)return HRESULT_FROM_WIN32(e);e=RegDeleteTreeW(HKEY_CURRENT_USER,cls.c_str());return e==ERROR_FILE_NOT_FOUND||e==ERROR_PATH_NOT_FOUND?S_OK:HRESULT_FROM_WIN32(e);}
 HRESULT Install(const std::wstring&dll)const{if(Any())return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);if(GetFileAttributesW(dll.c_str())==INVALID_FILE_ATTRIBUTES)return HRESULT_FROM_WIN32(GetLastError());HRESULT hr=Put(cls,L"Product",L"BandHeartBeat");if(SUCCEEDED(hr))hr=Put(cls+L"\\InprocServer32",nullptr,dll);if(SUCCEEDED(hr))hr=Put(cls+L"\\InprocServer32",L"ThreadingModel",L"Both");if(SUCCEEDED(hr))hr=Put(category,L"CLSID",id);if(SUCCEEDED(hr))hr=Put(category,L"FriendlyName",L"BandHeartBeat Virtual Camera");if(FAILED(hr))Remove();return hr;}
};
