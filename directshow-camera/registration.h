#pragma once
#include <string>
inline HMODULE source_module=nullptr;
inline const wchar_t* class_key=L"Software\\Classes\\CLSID\\{5B4131AF-3298-47AA-9C12-33407A81914B}";
inline const wchar_t* category_key=L"Software\\Classes\\CLSID\\{860BB310-5D01-11D0-BD3B-00A0C911CE86}\\Instance\\{5B4131AF-3298-47AA-9C12-33407A81914B}";
inline const wchar_t* class_text=L"{5B4131AF-3298-47AA-9C12-33407A81914B}";
inline HRESULT StringValue(HKEY key,const wchar_t*name,const wchar_t*value){return HRESULT_FROM_WIN32(RegSetValueExW(key,name,0,REG_SZ,reinterpret_cast<const BYTE*>(value),static_cast<DWORD>((wcslen(value)+1)*sizeof(wchar_t))));}
inline bool Existing(HKEY root,const std::wstring&path){HKEY h;auto e=RegOpenKeyExW(root,path.c_str(),0,KEY_READ,&h);if(e==ERROR_SUCCESS){RegCloseKey(h);return true;}return e!=ERROR_FILE_NOT_FOUND;}
inline HRESULT RegisterCamera(){wchar_t path[32768];auto n=GetModuleFileNameW(source_module,path,32768);if(!n||n==32768)return E_FAIL;
 // Never overwrite even an identically named existing registration.
 if(Existing(HKEY_CURRENT_USER,class_key)||Existing(HKEY_CURRENT_USER,category_key))return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
 HKEY h=nullptr;auto e=RegCreateKeyExW(HKEY_CURRENT_USER,(std::wstring(class_key)+L"\\InprocServer32").c_str(),0,nullptr,0,KEY_WRITE,nullptr,&h,nullptr);if(e)return HRESULT_FROM_WIN32(e);
 auto hr=StringValue(h,nullptr,path);if(SUCCEEDED(hr))hr=StringValue(h,L"ThreadingModel",L"Both");RegCloseKey(h);
 if(SUCCEEDED(hr)){e=RegCreateKeyExW(HKEY_CURRENT_USER,category_key,0,nullptr,0,KEY_WRITE,nullptr,&h,nullptr);hr=HRESULT_FROM_WIN32(e);if(SUCCEEDED(hr)){hr=StringValue(h,L"CLSID",class_text);if(SUCCEEDED(hr))hr=StringValue(h,L"FriendlyName",L"BandHeartBeat Virtual Camera");RegCloseKey(h);}}
 if(FAILED(hr)){RegDeleteTreeW(HKEY_CURRENT_USER,category_key);RegDeleteTreeW(HKEY_CURRENT_USER,class_key);}return hr;}
inline HRESULT UnregisterCamera(){wchar_t own[32768],registered[32768];GetModuleFileNameW(source_module,own,32768);DWORD bytes=sizeof(registered);auto e=RegGetValueW(HKEY_CURRENT_USER,(std::wstring(class_key)+L"\\InprocServer32").c_str(),nullptr,RRF_RT_REG_SZ,nullptr,registered,&bytes);if(e==ERROR_FILE_NOT_FOUND&&!Existing(HKEY_CURRENT_USER,category_key))return S_OK;if(e||_wcsicmp(registered,own))return E_ACCESSDENIED;
 e=RegDeleteTreeW(HKEY_CURRENT_USER,category_key);if(e!=ERROR_SUCCESS&&e!=ERROR_FILE_NOT_FOUND)return HRESULT_FROM_WIN32(e);e=RegDeleteTreeW(HKEY_CURRENT_USER,class_key);return e==ERROR_FILE_NOT_FOUND?S_OK:HRESULT_FROM_WIN32(e);}
