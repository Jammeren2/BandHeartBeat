// Lists video capture devices the way WebRTC/Discord does (System Device
// Enumerator) and binds each to its filter: enum_test.exe
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dshow.h>
#include <cstdio>

int main() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    ICreateDevEnum* devenum = nullptr;
    IEnumMoniker* monikers = nullptr;
    if (FAILED(CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
                                reinterpret_cast<void**>(&devenum))) ||
        devenum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &monikers, 0) != S_OK) {
        printf("no video input devices\n");
        return 1;
    }
    IMoniker* m = nullptr;
    while (monikers->Next(1, &m, nullptr) == S_OK) {
        IPropertyBag* bag = nullptr;
        VARIANT name;
        VariantInit(&name);
        if (SUCCEEDED(m->BindToStorage(nullptr, nullptr, IID_IPropertyBag, reinterpret_cast<void**>(&bag))))
            bag->Read(L"FriendlyName", &name, nullptr);
        IBaseFilter* filter = nullptr;
        HRESULT hr = m->BindToObject(nullptr, nullptr, IID_IBaseFilter, reinterpret_cast<void**>(&filter));
        printf("%-32ls bind 0x%08lX\n", name.vt == VT_BSTR ? name.bstrVal : L"?", hr);
        if (filter) filter->Release();
        VariantClear(&name);
        if (bag) bag->Release();
        m->Release();
    }
    return 0;
}
