// Smoke test: loads out\BandHeartBeatCamera.dll without registering it, sets a
// format through IAMStreamConfig like Chromium does, runs a real DirectShow
// graph into the Sample Grabber and checks the delivered samples.
//
//   graph_test.exe [24|32] [fps]
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dshow.h>
#include <cstdio>
#include <cstdlib>
#include <atomic>

// Sample Grabber interfaces (qedit.h is no longer in the SDK).
static const CLSID CLSID_SampleGrabber = {0xC1F400A0, 0x3F08, 0x11d3, {0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37}};
static const CLSID CLSID_NullRenderer = {0xC1F400A4, 0x3F08, 0x11d3, {0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37}};
static const IID IID_ISampleGrabber = {0x6B652FFF, 0x11FE, 0x4fce, {0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F}};
static const IID IID_ISampleGrabberCB = {0x0579154A, 0x2B53, 0x4994, {0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85}};
struct ISampleGrabberCB : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SampleCB(double, IMediaSample*) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB(double, BYTE*, long) = 0;
};
struct ISampleGrabber : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long*, long*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample**) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB*, long) = 0;
};

struct Counter : ISampleGrabberCB {
    std::atomic<int> samples{0};
    long size = 0;
    BYTE centre[4] = {};
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB) { *ppv = this; return S_OK; }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return 2; }
    STDMETHODIMP_(ULONG) Release() override { return 1; }
    STDMETHODIMP SampleCB(double, IMediaSample*) override { return S_OK; }
    STDMETHODIMP BufferCB(double, BYTE* data, long len) override {
        size = len;
        memcpy(centre, data + len / 2, 3);
        ++samples;
        return S_OK;
    }
};

#define CHECK(x) do { HRESULT hr_ = (x); if (FAILED(hr_)) { printf("FAIL %s: 0x%08lX\n", #x, hr_); return 1; } } while (0)

int main(int argc, char** argv) {
    const int bits = argc > 1 ? atoi(argv[1]) : 32;
    const int fps = argc > 2 ? atoi(argv[2]) : 30;
    CHECK(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    wcscpy_s(wcsrchr(path, L'\\') + 1, 64, L"BandHeartBeatCamera.dll");
    HMODULE dll = LoadLibraryW(path);
    if (!dll) { printf("FAIL LoadLibrary\n"); return 1; }
    auto get = reinterpret_cast<HRESULT(STDAPICALLTYPE*)(REFCLSID, REFIID, void**)>(GetProcAddress(dll, "DllGetClassObject"));
    const CLSID clsid = {0x8a3e5b1c, 0x2f47, 0x4d9b, {0xa6, 0xc1, 0x5e, 0x0b, 0x7d, 0x9f, 0x3a, 0x21}};
    IClassFactory* factory = nullptr;
    CHECK(get(clsid, IID_IClassFactory, reinterpret_cast<void**>(&factory)));
    IBaseFilter* source = nullptr;
    CHECK(factory->CreateInstance(nullptr, IID_IBaseFilter, reinterpret_cast<void**>(&source)));

    IEnumPins* pins = nullptr;
    IPin* out = nullptr;
    CHECK(source->EnumPins(&pins));
    CHECK(pins->Next(1, &out, nullptr));

    IKsPropertySet* ks = nullptr;
    GUID category{};
    DWORD got = 0;
    CHECK(out->QueryInterface(IID_IKsPropertySet, reinterpret_cast<void**>(&ks)));
    CHECK(ks->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, nullptr, 0, &category, sizeof(category), &got));
    printf("pin category capture: %s\n", category == PIN_CATEGORY_CAPTURE ? "yes" : "NO");

    IAMStreamConfig* config = nullptr;
    CHECK(out->QueryInterface(IID_IAMStreamConfig, reinterpret_cast<void**>(&config)));
    int count = 0, size = 0;
    CHECK(config->GetNumberOfCapabilities(&count, &size));
    AM_MEDIA_TYPE* chosen = nullptr;
    for (int i = 0; i < count; ++i) {
        AM_MEDIA_TYPE* mt = nullptr;
        VIDEO_STREAM_CONFIG_CAPS caps{};
        CHECK(config->GetStreamCaps(i, &mt, reinterpret_cast<BYTE*>(&caps)));
        auto* vih = reinterpret_cast<VIDEOINFOHEADER*>(mt->pbFormat);
        printf("cap %d: %ldx%ld %d-bit %.1f-%.1f fps\n", i, vih->bmiHeader.biWidth, vih->bmiHeader.biHeight,
               vih->bmiHeader.biBitCount, 1e7 / caps.MaxFrameInterval, 1e7 / caps.MinFrameInterval);
        if (vih->bmiHeader.biBitCount == bits) chosen = mt;
    }
    if (!chosen) { printf("FAIL no %d-bit format\n", bits); return 1; }
    reinterpret_cast<VIDEOINFOHEADER*>(chosen->pbFormat)->AvgTimePerFrame = 10000000 / fps;
    CHECK(config->SetFormat(chosen));
    AM_MEDIA_TYPE* current = nullptr;
    CHECK(config->GetFormat(&current));
    printf("configured: %d-bit\n", reinterpret_cast<VIDEOINFOHEADER*>(current->pbFormat)->bmiHeader.biBitCount);

    IGraphBuilder* graph = nullptr;
    IBaseFilter *grabberFilter = nullptr, *sink = nullptr;
    ISampleGrabber* grabber = nullptr;
    CHECK(CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, reinterpret_cast<void**>(&graph)));
    CHECK(CoCreateInstance(CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<void**>(&grabberFilter)));
    CHECK(CoCreateInstance(CLSID_NullRenderer, nullptr, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<void**>(&sink)));
    CHECK(grabberFilter->QueryInterface(IID_ISampleGrabber, reinterpret_cast<void**>(&grabber)));
    Counter counter;
    CHECK(grabber->SetCallback(&counter, 1));
    CHECK(graph->AddFilter(source, L"BandHeartBeat"));
    CHECK(graph->AddFilter(grabberFilter, L"Grabber"));
    CHECK(graph->AddFilter(sink, L"Null"));

    auto firstPin = [](IBaseFilter* f, PIN_DIRECTION dir) {
        IEnumPins* e = nullptr;
        IPin* p = nullptr;
        f->EnumPins(&e);
        while (e->Next(1, &p, nullptr) == S_OK) {
            PIN_DIRECTION d;
            p->QueryDirection(&d);
            if (d == dir) break;
            p->Release();
            p = nullptr;
        }
        e->Release();
        return p;
    };
    CHECK(graph->ConnectDirect(out, firstPin(grabberFilter, PINDIR_INPUT), nullptr));
    CHECK(graph->ConnectDirect(firstPin(grabberFilter, PINDIR_OUTPUT), firstPin(sink, PINDIR_INPUT), nullptr));

    AM_MEDIA_TYPE connected{};
    CHECK(out->ConnectionMediaType(&connected));
    printf("connected: %d-bit, %.1f fps\n", reinterpret_cast<VIDEOINFOHEADER*>(connected.pbFormat)->bmiHeader.biBitCount,
           1e7 / reinterpret_cast<VIDEOINFOHEADER*>(connected.pbFormat)->AvgTimePerFrame);

    IMediaControl* control = nullptr;
    CHECK(graph->QueryInterface(IID_IMediaControl, reinterpret_cast<void**>(&control)));
    CHECK(control->Run());
    Sleep(3000);
    CHECK(control->Stop());
    const int samples = counter.samples;
    printf("samples in 3 s: %d (%.1f fps), %ld bytes, centre BGR %u %u %u\n", samples, samples / 3.0, counter.size,
           counter.centre[0], counter.centre[1], counter.centre[2]);
    const bool ok = samples >= fps * 3 * 8 / 10 && counter.size == 1280 * 720 * bits / 8;

    // Second run after stop must work too (Discord re-opens cameras).
    counter.samples = 0;
    CHECK(control->Run());
    Sleep(1000);
    CHECK(control->Stop());
    printf("restart samples in 1 s: %d\n", int(counter.samples));
    const bool ok2 = counter.samples >= fps * 8 / 10;
    printf("%s\n", ok && ok2 ? "PASS" : "FAIL");
    return ok && ok2 ? 0 : 1;
}
