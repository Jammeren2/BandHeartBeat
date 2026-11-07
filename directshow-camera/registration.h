#pragma once
#include "registration_store.h"
inline HMODULE source_module=nullptr;
inline HRESULT RegisterCamera(){wchar_t path[32768];auto n=GetModuleFileNameW(source_module,path,32768);if(!n||n==32768)return E_FAIL;return RegistrationStore{}.Install(path);}
inline HRESULT UnregisterCamera(){return RegistrationStore{}.Remove();}
