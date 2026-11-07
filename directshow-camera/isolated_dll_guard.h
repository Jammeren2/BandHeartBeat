#pragma once
#include <windows.h>
#include <fstream>
#include <iterator>
#include <string>
#include "shared_frame.h"
inline bool IsolatedFrameDll(HMODULE module){wchar_t path[32768];if(!GetModuleFileNameW(module,path,32768))return false;std::ifstream file(path,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(file)),{});std::string expected(reinterpret_cast<const char*>(BH_SHM_NAME),wcslen(BH_SHM_NAME)*sizeof(wchar_t));return bytes.find(expected)!=std::string::npos;}
