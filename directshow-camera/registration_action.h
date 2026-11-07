#pragma once
#include <windows.h>
#include <string>
enum class RegistrationAction {Install,Remove};
inline const wchar_t* RegistrationCommand(RegistrationAction a){return a==RegistrationAction::Install?L"--register":L"--unregister";}
inline std::wstring RegistrationResult(DWORD exit,DWORD launch,const std::wstring&output){if(launch){wchar_t*message=nullptr;FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,nullptr,launch,0,reinterpret_cast<LPWSTR>(&message),0,nullptr);std::wstring result=L"Ошибка запуска helper: Win32 "+std::to_wstring(launch);if(message){result+=L": ";result+=message;LocalFree(message);}return result;}if(output.empty())return L"Ошибка: helper не сообщил результат; exit="+std::to_wstring(exit);return std::wstring(exit?L"Ошибка: ":L"Готово: ")+output+L" (exit="+std::to_wstring(exit)+L")";}
