#define WIN32_LEAN_AND_MEAN
#include "source_contract.h"
#include <objbase.h>
#include <iostream>

int main() {
    MFStartup(MF_VERSION);
    LPOLESTR value = nullptr;
    if (FAILED(StringFromCLSID(CLSID_BandHeartBeatSource, &value))) return 2;
    std::wcout << L"Source CLSID: " << value << L"\n";
    CoTaskMemFree(value);
    HMODULE dll = LoadLibraryW(L"BandHeartBeatSource.dll");
    if (!dll) { std::cout << "LoadLibrary failed (run from virtual-camera/out).\n"; return 3; }
    using CreateFn = HRESULT (__stdcall *)(IMFMediaSource **);
    auto create = reinterpret_cast<CreateFn>(GetProcAddress(dll, "CreatePlaceholderSource"));
    IMFMediaSource *source = nullptr;
    HRESULT hr = create ? create(&source) : E_POINTER;
    std::cout << "CreatePlaceholderSource HRESULT: 0x" << std::hex << hr << std::dec << "\n";
    if (SUCCEEDED(hr)) {
        DWORD characteristics = 0; std::cout << "GetCharacteristics HRESULT: 0x" << std::hex << source->GetCharacteristics(&characteristics) << std::dec << "\n";
        IMFPresentationDescriptor *pd = nullptr;
        HRESULT descriptor = source->CreatePresentationDescriptor(&pd);
        std::cout << "CreatePresentationDescriptor HRESULT: 0x" << std::hex << descriptor << std::dec << "\n";
        if (SUCCEEDED(descriptor)) pd->Release();
        using StreamFn = HRESULT (__stdcall *)(IMFMediaStream **);
        auto stream_create = reinterpret_cast<StreamFn>(GetProcAddress(dll, "CreatePlaceholderStream"));
        IMFMediaStream *stream = nullptr; HRESULT sh = stream_create ? stream_create(&stream) : E_POINTER;
        std::cout << "CreatePlaceholderStream HRESULT: 0x" << std::hex << sh << std::dec << "\n";
        if (SUCCEEDED(sh)) { HRESULT sample = stream->RequestSample(nullptr); std::cout << "RequestSample HRESULT: 0x" << std::hex << sample << std::dec << "\n"; stream->Release(); }
        std::cout << "Shutdown HRESULT: 0x" << std::hex << source->Shutdown() << std::dec << "\n";
        source->Release();
    }
    FreeLibrary(dll);
    MFShutdown();
    std::cout << "No MFCreateVirtualCamera call; no registration, camera activation, or telemetry performed.\n";
    return SUCCEEDED(hr) ? 0 : 4;
}
