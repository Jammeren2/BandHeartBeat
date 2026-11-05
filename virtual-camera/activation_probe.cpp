#define WIN32_LEAN_AND_MEAN
#include "source_contract.h"
#include <mfvirtualcamera.h>
#include <mfapi.h>
#include <iostream>
#include <string>

static const wchar_t *kSource = L"{8EEC4A2D-7FA2-4B6E-9A4C-246DA8B46F11}";

int wmain(int argc, wchar_t **argv) {
    bool activate = false, remove = false;
    for (int i=1;i<argc;i++) { activate |= wcscmp(argv[i],L"--activate")==0; remove |= wcscmp(argv[i],L"--remove")==0; }
    if (!activate && !remove) {
        std::wcout << L"DRY RUN: would call MFCreateVirtualCamera(SoftwareSource, Session, CurrentUser, ...).\n"
                      L"No registration, activation, privacy change, or device enumeration performed.\n"
                      L"Use --activate or --remove only after approving the source COM registration boundary.\n";
        return 0;
    }
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) { std::wcerr << L"MFStartup failed: 0x" << std::hex << hr << L"\n"; return 2; }
    IMFVirtualCamera *camera = nullptr;
    hr = MFCreateVirtualCamera(MFVirtualCameraType_SoftwareCameraSource, MFVirtualCameraLifetime_Session,
        MFVirtualCameraAccess_CurrentUser, L"BandHeartBeat Virtual Camera", kSource, nullptr, 0, &camera);
    if (FAILED(hr)) {
        std::wcerr << L"MFCreateVirtualCamera failed: 0x" << std::hex << hr << L"\n";
        if (hr == E_ACCESSDENIED) std::wcerr << L"Access denied: webcam privacy, source registration, or category/admin boundary.\n";
        MFShutdown(); return 3;
    }
    if (remove) { hr = camera->Remove(); std::wcout << L"Remove HRESULT: 0x" << std::hex << hr << L"\n"; }
    else { hr = camera->Start(nullptr); std::wcout << L"Start HRESULT: 0x" << std::hex << hr << L"\n"; }
    camera->Shutdown(); camera->Release(); MFShutdown(); return SUCCEEDED(hr) ? 0 : 4;
}
