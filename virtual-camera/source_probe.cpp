#define WIN32_LEAN_AND_MEAN
#include "source_contract.h"
#include <objbase.h>
#include <iostream>

int main() {
    LPOLESTR value = nullptr;
    if (FAILED(StringFromCLSID(CLSID_BandHeartBeatSource, &value))) return 2;
    std::wcout << L"Source CLSID: " << value << L"\n";
    CoTaskMemFree(value);
    std::cout << "Current-user registration helper: not implemented (safe prototype).\n";
    std::cout << "No MFCreateVirtualCamera call; no camera registration or activation performed.\n";
    return 0;
}
