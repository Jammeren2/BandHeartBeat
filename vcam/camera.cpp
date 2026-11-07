// BandHeartBeat Camera: a DirectShow live source filter that republishes the
// frames composed by the BandHeartBeat app (see shared_frame.h).
//
// One output pin, 1280x720, RGB32 or RGB24, 5-30 fps. Registered per user by
// the app (crates/core/src/vcam.rs); no HKLM, no administrator rights.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dshow.h>
#include <dvdmedia.h>
#include <ks.h>
#include <ksmedia.h>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

#include "shared_frame.h"

// {8A3E5B1C-2F47-4D9B-A6C1-5E0B7D9F3A21}, must match crates/core/src/vcam.rs.
static const CLSID CLSID_BandHeartBeatCamera = {
    0x8a3e5b1c, 0x2f47, 0x4d9b, {0xa6, 0xc1, 0x5e, 0x0b, 0x7d, 0x9f, 0x3a, 0x21}};

namespace {

constexpr LONG kWidth = 1280;
constexpr LONG kHeight = 720;
constexpr REFERENCE_TIME kMinInterval = 333333;    // 30 fps
constexpr REFERENCE_TIME kMaxInterval = 2000000;   // 5 fps
constexpr wchar_t kPinName[] = L"Capture";
constexpr wchar_t kFilterName[] = L"BandHeartBeat Camera";

std::atomic<long> g_objects{0};
std::atomic<long> g_locks{0};

// ---------------------------------------------------------------------------
// Media types

void FreeMediaType(AM_MEDIA_TYPE& mt) {
    if (mt.pbFormat) CoTaskMemFree(mt.pbFormat);
    if (mt.pUnk) mt.pUnk->Release();
    mt.pbFormat = nullptr;
    mt.cbFormat = 0;
    mt.pUnk = nullptr;
}

void DeleteMediaType(AM_MEDIA_TYPE* mt) {
    if (!mt) return;
    FreeMediaType(*mt);
    CoTaskMemFree(mt);
}

HRESULT CopyMediaType(AM_MEDIA_TYPE* dst, const AM_MEDIA_TYPE& src) {
    *dst = src;
    if (src.cbFormat) {
        dst->pbFormat = static_cast<BYTE*>(CoTaskMemAlloc(src.cbFormat));
        if (!dst->pbFormat) {
            dst->cbFormat = 0;
            return E_OUTOFMEMORY;
        }
        memcpy(dst->pbFormat, src.pbFormat, src.cbFormat);
    }
    if (dst->pUnk) dst->pUnk->AddRef();
    return S_OK;
}

// Builds a bottom-up RGB VIDEOINFOHEADER type.
HRESULT MakeType(AM_MEDIA_TYPE* mt, WORD bits, REFERENCE_TIME interval) {
    ZeroMemory(mt, sizeof(*mt));
    auto* vih = static_cast<VIDEOINFOHEADER*>(CoTaskMemAlloc(sizeof(VIDEOINFOHEADER)));
    if (!vih) return E_OUTOFMEMORY;
    ZeroMemory(vih, sizeof(*vih));
    const DWORD image = DWORD(kWidth) * kHeight * (bits / 8);
    vih->AvgTimePerFrame = interval;
    vih->dwBitRate = DWORD(double(image) * 8 * 10000000.0 / interval);
    vih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    vih->bmiHeader.biWidth = kWidth;
    vih->bmiHeader.biHeight = kHeight;
    vih->bmiHeader.biPlanes = 1;
    vih->bmiHeader.biBitCount = bits;
    vih->bmiHeader.biCompression = BI_RGB;
    vih->bmiHeader.biSizeImage = image;
    mt->majortype = MEDIATYPE_Video;
    mt->subtype = bits == 32 ? MEDIASUBTYPE_RGB32 : MEDIASUBTYPE_RGB24;
    mt->bFixedSizeSamples = TRUE;
    mt->lSampleSize = image;
    mt->formattype = FORMAT_VideoInfo;
    mt->cbFormat = sizeof(VIDEOINFOHEADER);
    mt->pbFormat = reinterpret_cast<BYTE*>(vih);
    return S_OK;
}

constexpr WORD kBits[] = {32, 24};

// Accepts our size in either RGB depth with an interval in range.
// Returns the bit depth, or 0 if the type is not supported.
WORD Accepts(const AM_MEDIA_TYPE* mt) {
    if (!mt || mt->majortype != MEDIATYPE_Video || mt->formattype != FORMAT_VideoInfo ||
        mt->cbFormat < sizeof(VIDEOINFOHEADER) || !mt->pbFormat)
        return 0;
    const auto* vih = reinterpret_cast<const VIDEOINFOHEADER*>(mt->pbFormat);
    const auto& bmi = vih->bmiHeader;
    const WORD bits = mt->subtype == MEDIASUBTYPE_RGB32 ? 32 : mt->subtype == MEDIASUBTYPE_RGB24 ? 24 : 0;
    if (!bits || bmi.biBitCount != bits || bmi.biWidth != kWidth || std::abs(bmi.biHeight) != kHeight)
        return 0;
    if (vih->AvgTimePerFrame && (vih->AvgTimePerFrame < kMinInterval || vih->AvgTimePerFrame > kMaxInterval))
        return 0;
    return bits;
}

// ---------------------------------------------------------------------------
// Placeholder shown while the app is not running.

std::vector<uint8_t> MakePlaceholder() {
    std::vector<uint8_t> px(size_t(kWidth) * kHeight * 4);
    BITMAPINFO bi{};
    bi.bmiHeader = {sizeof(BITMAPINFOHEADER), kWidth, -kHeight, 1, 32, BI_RGB};
    void* bits = nullptr;
    HDC dc = CreateCompatibleDC(nullptr);
    HBITMAP bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dc || !bmp) {
        if (bmp) DeleteObject(bmp);
        if (dc) DeleteDC(dc);
        return px;
    }
    HGDIOBJ old = SelectObject(dc, bmp);
    RECT all{0, 0, kWidth, kHeight};
    HBRUSH bg = CreateSolidBrush(RGB(26, 22, 34));
    FillRect(dc, &all, bg);
    DeleteObject(bg);
    HFONT font = CreateFontW(-34, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(214, 210, 226));
    DrawTextW(dc, L"Запустите BandHeartBeat", -1, &all, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, oldFont);
    DeleteObject(font);
    GdiFlush();
    memcpy(px.data(), bits, px.size());
    SelectObject(dc, old);
    DeleteObject(bmp);
    DeleteDC(dc);
    return px;
}

// Converts a top-down BGRA frame into a bottom-up RGB32/RGB24 sample buffer.
void Convert(const uint8_t* bgra, BYTE* out, WORD bits) {
    const size_t srcRow = size_t(kWidth) * 4;
    const size_t dstRow = size_t(kWidth) * (bits / 8);  // 3840 and 5120 are DWORD aligned
    for (LONG y = 0; y < kHeight; ++y) {
        const uint8_t* s = bgra + srcRow * (kHeight - 1 - y);
        BYTE* d = out + dstRow * y;
        if (bits == 32) {
            memcpy(d, s, srcRow);
        } else {
            for (LONG x = 0; x < kWidth; ++x, s += 4, d += 3) {
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Enumerators

class Filter;

class EnumMediaTypes final : public IEnumMediaTypes {
public:
    // Lists the current depth first, so reconnections keep the configured format.
    EnumMediaTypes(REFERENCE_TIME interval, WORD first, ULONG pos = 0)
        : interval_(interval), first_(first), pos_(pos) {
        ++g_objects;
    }
    ~EnumMediaTypes() { --g_objects; }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IEnumMediaTypes) {
            *ppv = static_cast<IEnumMediaTypes*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG n = --refs_;
        if (!n) delete this;
        return n;
    }

    STDMETHODIMP Next(ULONG count, AM_MEDIA_TYPE** out, ULONG* fetched) override {
        if (!out) return E_POINTER;
        ULONG n = 0;
        while (n < count && pos_ < std::size(kBits)) {
            auto* mt = static_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
            const WORD bits = first_ == kBits[1] ? kBits[1 - pos_] : kBits[pos_];
            if (!mt || FAILED(MakeType(mt, bits, interval_))) {
                CoTaskMemFree(mt);
                break;
            }
            out[n++] = mt;
            ++pos_;
        }
        if (fetched) *fetched = n;
        return n == count ? S_OK : S_FALSE;
    }
    STDMETHODIMP Skip(ULONG count) override {
        pos_ += count;
        return pos_ <= std::size(kBits) ? S_OK : S_FALSE;
    }
    STDMETHODIMP Reset() override {
        pos_ = 0;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumMediaTypes** out) override {
        if (!out) return E_POINTER;
        *out = new (std::nothrow) EnumMediaTypes(interval_, first_, pos_);
        return *out ? S_OK : E_OUTOFMEMORY;
    }

private:
    std::atomic<ULONG> refs_{1};
    REFERENCE_TIME interval_;
    WORD first_;
    ULONG pos_;
};

class EnumPins final : public IEnumPins {
public:
    EnumPins(IPin* pin, ULONG pos = 0) : pin_(pin), pos_(pos) {
        pin_->AddRef();
        ++g_objects;
    }
    ~EnumPins() {
        pin_->Release();
        --g_objects;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IEnumPins) {
            *ppv = static_cast<IEnumPins*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG n = --refs_;
        if (!n) delete this;
        return n;
    }

    STDMETHODIMP Next(ULONG count, IPin** out, ULONG* fetched) override {
        if (!out) return E_POINTER;
        ULONG n = 0;
        if (count && pos_ == 0) {
            out[n++] = pin_;
            pin_->AddRef();
            pos_ = 1;
        }
        if (fetched) *fetched = n;
        return n == count ? S_OK : S_FALSE;
    }
    STDMETHODIMP Skip(ULONG count) override {
        pos_ += count;
        return pos_ <= 1 ? S_OK : S_FALSE;
    }
    STDMETHODIMP Reset() override {
        pos_ = 0;
        return S_OK;
    }
    STDMETHODIMP Clone(IEnumPins** out) override {
        if (!out) return E_POINTER;
        *out = new (std::nothrow) EnumPins(pin_, pos_);
        return *out ? S_OK : E_OUTOFMEMORY;
    }

private:
    std::atomic<ULONG> refs_{1};
    IPin* pin_;
    ULONG pos_;
};

// ---------------------------------------------------------------------------
// Output pin. Its lifetime is the filter's: AddRef/Release are forwarded.

class Pin final : public IPin, public IAMStreamConfig, public IKsPropertySet {
public:
    explicit Pin(Filter* filter) : filter_(filter) { MakeType(&format_, 32, kMinInterval); }
    ~Pin() {
        Disconnect();
        FreeMediaType(format_);
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IPin
    STDMETHODIMP Connect(IPin* receiver, const AM_MEDIA_TYPE* pmt) override {
        if (!receiver) return E_POINTER;
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (peer_) return VFW_E_ALREADY_CONNECTED;
        if (Running()) return VFW_E_NOT_STOPPED;

        // Try the caller's type, then the one set through IAMStreamConfig, then the rest.
        std::vector<AM_MEDIA_TYPE> candidates;
        AM_MEDIA_TYPE t{};
        if (pmt && Accepts(pmt) && SUCCEEDED(CopyMediaType(&t, *pmt))) candidates.push_back(t);
        if ((!pmt || pmt->subtype == GUID_NULL || pmt->subtype == format_.subtype) &&
            SUCCEEDED(CopyMediaType(&t, format_)))
            candidates.push_back(t);
        for (WORD bits : kBits) {
            if (pmt && pmt->subtype != GUID_NULL && pmt->subtype != (bits == 32 ? MEDIASUBTYPE_RGB32 : MEDIASUBTYPE_RGB24))
                continue;
            if (SUCCEEDED(MakeType(&t, bits, Interval()))) candidates.push_back(t);
        }
        HRESULT hr = VFW_E_NO_ACCEPTABLE_TYPES;
        for (auto& mt : candidates) {
            if (peer_ == nullptr && SUCCEEDED(receiver->ReceiveConnection(this, &mt))) {
                hr = CompleteConnect(receiver, mt);
                if (FAILED(hr)) receiver->Disconnect();
            }
        }
        for (auto& mt : candidates) FreeMediaType(mt);
        return hr;
    }
    STDMETHODIMP ReceiveConnection(IPin*, const AM_MEDIA_TYPE*) override { return E_UNEXPECTED; }
    STDMETHODIMP Disconnect() override {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (!peer_) return S_FALSE;
        if (allocator_) {
            allocator_->Decommit();
            allocator_->Release();
            allocator_ = nullptr;
        }
        if (input_) {
            input_->Release();
            input_ = nullptr;
        }
        peer_->Release();
        peer_ = nullptr;
        return S_OK;
    }
    STDMETHODIMP ConnectedTo(IPin** pin) override {
        if (!pin) return E_POINTER;
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        *pin = peer_;
        if (!peer_) return VFW_E_NOT_CONNECTED;
        peer_->AddRef();
        return S_OK;
    }
    STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE* mt) override {
        if (!mt) return E_POINTER;
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (!peer_) {
            ZeroMemory(mt, sizeof(*mt));
            return VFW_E_NOT_CONNECTED;
        }
        return CopyMediaType(mt, format_);
    }
    STDMETHODIMP QueryPinInfo(PIN_INFO* info) override;
    STDMETHODIMP QueryDirection(PIN_DIRECTION* dir) override {
        if (!dir) return E_POINTER;
        *dir = PINDIR_OUTPUT;
        return S_OK;
    }
    STDMETHODIMP QueryId(LPWSTR* id) override {
        if (!id) return E_POINTER;
        *id = static_cast<LPWSTR>(CoTaskMemAlloc(sizeof(kPinName)));
        if (!*id) return E_OUTOFMEMORY;
        memcpy(*id, kPinName, sizeof(kPinName));
        return S_OK;
    }
    STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* mt) override { return Accepts(mt) ? S_OK : S_FALSE; }
    STDMETHODIMP EnumMediaTypes(IEnumMediaTypes** out) override {
        if (!out) return E_POINTER;
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        *out = new (std::nothrow) ::EnumMediaTypes(Interval(), Bits());
        return *out ? S_OK : E_OUTOFMEMORY;
    }
    STDMETHODIMP QueryInternalConnections(IPin**, ULONG*) override { return E_NOTIMPL; }
    STDMETHODIMP EndOfStream() override { return E_UNEXPECTED; }
    STDMETHODIMP BeginFlush() override { return E_UNEXPECTED; }
    STDMETHODIMP EndFlush() override { return E_UNEXPECTED; }
    STDMETHODIMP NewSegment(REFERENCE_TIME, REFERENCE_TIME, double) override { return S_OK; }

    // IAMStreamConfig
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE* mt) override {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (!mt) return E_POINTER;
        if (!Accepts(mt)) return VFW_E_INVALIDMEDIATYPE;
        if (Running()) return VFW_E_NOT_STOPPED;
        if (peer_ && peer_->QueryAccept(mt) != S_OK) return VFW_E_INVALIDMEDIATYPE;
        AM_MEDIA_TYPE copy{};
        HRESULT hr = CopyMediaType(&copy, *mt);
        if (FAILED(hr)) return hr;
        auto* vih = reinterpret_cast<VIDEOINFOHEADER*>(copy.pbFormat);
        if (!vih->AvgTimePerFrame) vih->AvgTimePerFrame = kMinInterval;
        FreeMediaType(format_);
        format_ = copy;
        return S_OK;
    }
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE** out) override {
        if (!out) return E_POINTER;
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        *out = static_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
        if (!*out) return E_OUTOFMEMORY;
        HRESULT hr = CopyMediaType(*out, format_);
        if (FAILED(hr)) {
            CoTaskMemFree(*out);
            *out = nullptr;
        }
        return hr;
    }
    STDMETHODIMP GetNumberOfCapabilities(int* count, int* size) override {
        if (!count || !size) return E_POINTER;
        *count = int(std::size(kBits));
        *size = sizeof(VIDEO_STREAM_CONFIG_CAPS);
        return S_OK;
    }
    STDMETHODIMP GetStreamCaps(int index, AM_MEDIA_TYPE** out, BYTE* scc) override {
        if (!out || !scc) return E_POINTER;
        if (index < 0 || index >= int(std::size(kBits))) return S_FALSE;
        *out = static_cast<AM_MEDIA_TYPE*>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
        if (!*out) return E_OUTOFMEMORY;
        HRESULT hr = MakeType(*out, kBits[index], kMinInterval);
        if (FAILED(hr)) {
            CoTaskMemFree(*out);
            *out = nullptr;
            return hr;
        }
        auto* caps = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(scc);
        ZeroMemory(caps, sizeof(*caps));
        caps->guid = FORMAT_VideoInfo;
        caps->InputSize = caps->MinCroppingSize = caps->MaxCroppingSize = {kWidth, kHeight};
        caps->MinOutputSize = caps->MaxOutputSize = {kWidth, kHeight};
        caps->MinFrameInterval = kMinInterval;
        caps->MaxFrameInterval = kMaxInterval;
        const LONG bits = LONG(kWidth) * kHeight * kBits[index];
        caps->MinBitsPerSecond = LONG(double(bits) * 10000000.0 / kMaxInterval);
        caps->MaxBitsPerSecond = LONG(std::min(double(LONG_MAX), double(bits) * 10000000.0 / kMinInterval));
        return S_OK;
    }

    // IKsPropertySet: identifies this pin as the capture pin.
    STDMETHODIMP Set(REFGUID, DWORD, LPVOID, DWORD, LPVOID, DWORD) override { return E_NOTIMPL; }
    STDMETHODIMP Get(REFGUID set, DWORD id, LPVOID, DWORD, LPVOID data, DWORD size, DWORD* returned) override {
        if (set != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
        if (id != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
        if (returned) *returned = sizeof(GUID);
        if (!data) return returned ? S_OK : E_POINTER;
        if (size < sizeof(GUID)) return E_UNEXPECTED;
        *static_cast<GUID*>(data) = PIN_CATEGORY_CAPTURE;
        return S_OK;
    }
    STDMETHODIMP QuerySupported(REFGUID set, DWORD id, DWORD* support) override {
        if (set != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
        if (id != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
        if (support) *support = KSPROPERTY_SUPPORT_GET;
        return S_OK;
    }

    // Streaming control, called by the filter.
    HRESULT Commit() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return allocator_ ? allocator_->Commit() : VFW_E_NOT_CONNECTED;
    }
    void StartStreaming() {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        if (worker_.joinable() || !input_) return;
        stop_ = false;
        worker_ = std::thread([this] { Stream(); });
    }
    void StopStreaming() {
        std::thread worker;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            stop_ = true;
            if (allocator_) allocator_->Decommit();  // unblocks GetBuffer
            worker.swap(worker_);
        }
        if (worker.joinable()) worker.join();
    }

private:
    bool Running() const;

    REFERENCE_TIME Interval() const {
        auto* vih = reinterpret_cast<const VIDEOINFOHEADER*>(format_.pbFormat);
        return vih && vih->AvgTimePerFrame ? vih->AvgTimePerFrame : kMinInterval;
    }

    WORD Bits() const { return reinterpret_cast<const VIDEOINFOHEADER*>(format_.pbFormat)->bmiHeader.biBitCount; }

    HRESULT CompleteConnect(IPin* receiver, const AM_MEDIA_TYPE& mt) {
        IMemInputPin* input = nullptr;
        HRESULT hr = receiver->QueryInterface(IID_IMemInputPin, reinterpret_cast<void**>(&input));
        if (FAILED(hr)) return hr;

        ALLOCATOR_PROPERTIES want{}, got{};
        input->GetAllocatorRequirements(&want);
        want.cBuffers = std::max<long>(want.cBuffers, 3);
        want.cbBuffer = std::max<long>(want.cbBuffer, long(mt.lSampleSize));
        want.cbAlign = std::max<long>(want.cbAlign, 1);

        IMemAllocator* allocator = nullptr;
        hr = input->GetAllocator(&allocator);
        if (FAILED(hr) || FAILED(allocator->SetProperties(&want, &got)) || got.cbBuffer < long(mt.lSampleSize) ||
            FAILED(input->NotifyAllocator(allocator, FALSE))) {
            if (allocator) allocator->Release();
            allocator = nullptr;
            hr = CoCreateInstance(CLSID_MemoryAllocator, nullptr, CLSCTX_INPROC_SERVER, IID_IMemAllocator,
                                  reinterpret_cast<void**>(&allocator));
            if (SUCCEEDED(hr)) hr = allocator->SetProperties(&want, &got);
            if (SUCCEEDED(hr) && got.cbBuffer < long(mt.lSampleSize)) hr = VFW_E_SIZENOTSET;
            if (SUCCEEDED(hr)) hr = input->NotifyAllocator(allocator, FALSE);
        }
        if (FAILED(hr)) {
            if (allocator) allocator->Release();
            input->Release();
            return hr;
        }

        AM_MEDIA_TYPE copy{};
        hr = CopyMediaType(&copy, mt);
        if (FAILED(hr)) {
            allocator->Release();
            input->Release();
            return hr;
        }
        FreeMediaType(format_);
        format_ = copy;
        peer_ = receiver;
        peer_->AddRef();
        input_ = input;
        allocator_ = allocator;
        return S_OK;
    }

    // Delivers frames at the negotiated rate until stopped.
    void Stream() {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        IMemInputPin* input;
        IMemAllocator* allocator;
        REFERENCE_TIME interval;
        WORD bits;
        {
            std::lock_guard<std::recursive_mutex> lock(mutex_);
            input = input_;
            allocator = allocator_;
            input->AddRef();
            allocator->AddRef();
            interval = Interval();
            bits = Bits();
        }
        bhb::FrameReader reader;
        std::vector<uint8_t> frame(size_t(kWidth) * kHeight * 4);
        const std::vector<uint8_t> placeholder = MakePlaceholder();
        const DWORD image = DWORD(kWidth) * kHeight * (bits / 8);

        LARGE_INTEGER freq, start, now;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        for (LONGLONG n = 0; !stop_; ++n) {
            // Wait for this frame's slot; resync if we fell more than a frame behind.
            const LONGLONG due = start.QuadPart + n * interval * freq.QuadPart / 10000000;
            QueryPerformanceCounter(&now);
            if (now.QuadPart - due > interval * freq.QuadPart / 10000000) {
                start.QuadPart = now.QuadPart - n * interval * freq.QuadPart / 10000000;
            } else if (due > now.QuadPart) {
                Sleep(DWORD((due - now.QuadPart) * 1000 / freq.QuadPart));
            }

            IMediaSample* sample = nullptr;
            if (FAILED(allocator->GetBuffer(&sample, nullptr, nullptr, 0)) || !sample) break;
            BYTE* data = nullptr;
            if (SUCCEEDED(sample->GetPointer(&data)) && sample->GetSize() >= long(image)) {
                const uint8_t* src = reader.Read(kWidth, kHeight, frame.data()) ? frame.data() : placeholder.data();
                Convert(src, data, bits);
                REFERENCE_TIME t0 = n * interval, t1 = t0 + interval;
                sample->SetTime(&t0, &t1);
                sample->SetSyncPoint(TRUE);
                sample->SetActualDataLength(image);
                HRESULT hr = input->Receive(sample);
                sample->Release();
                if (hr != S_OK) break;  // S_FALSE or error: the sink wants no more
            } else {
                sample->Release();
            }
        }
        allocator->Release();
        input->Release();
        CoUninitialize();
    }

    Filter* filter_;
    std::recursive_mutex mutex_;
    AM_MEDIA_TYPE format_{};
    IPin* peer_ = nullptr;
    IMemInputPin* input_ = nullptr;
    IMemAllocator* allocator_ = nullptr;
    std::thread worker_;
    std::atomic<bool> stop_{false};
};

// ---------------------------------------------------------------------------
// Filter

class Filter final : public IBaseFilter, public IAMFilterMiscFlags {
public:
    Filter() : pin_(this) { ++g_objects; }
    ~Filter() {
        pin_.StopStreaming();
        if (clock_) clock_->Release();
        --g_objects;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IPersist || riid == IID_IMediaFilter || riid == IID_IBaseFilter)
            *ppv = static_cast<IBaseFilter*>(this);
        else if (riid == IID_IAMFilterMiscFlags)
            *ppv = static_cast<IAMFilterMiscFlags*>(this);
        else if (riid == IID_IAMStreamConfig)
            *ppv = static_cast<IAMStreamConfig*>(&pin_);
        else {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++refs_; }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG n = --refs_;
        if (!n) delete this;
        return n;
    }

    // IPersist / IMediaFilter
    STDMETHODIMP GetClassID(CLSID* id) override {
        if (!id) return E_POINTER;
        *id = CLSID_BandHeartBeatCamera;
        return S_OK;
    }
    STDMETHODIMP Stop() override {
        pin_.StopStreaming();
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = State_Stopped;
        return S_OK;
    }
    STDMETHODIMP Pause() override {
        if (Transition(State_Paused) == State_Stopped) pin_.Commit();
        return S_OK;
    }
    STDMETHODIMP Run(REFERENCE_TIME) override {
        if (Transition(State_Running) == State_Stopped) pin_.Commit();
        pin_.StartStreaming();
        return S_OK;
    }
    STDMETHODIMP GetState(DWORD, FILTER_STATE* state) override {
        if (!state) return E_POINTER;
        std::lock_guard<std::mutex> lock(mutex_);
        *state = state_;
        return state_ == State_Paused ? VFW_S_CANT_CUE : S_OK;  // live source
    }
    STDMETHODIMP SetSyncSource(IReferenceClock* clock) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (clock) clock->AddRef();
        if (clock_) clock_->Release();
        clock_ = clock;
        return S_OK;
    }
    STDMETHODIMP GetSyncSource(IReferenceClock** clock) override {
        if (!clock) return E_POINTER;
        std::lock_guard<std::mutex> lock(mutex_);
        *clock = clock_;
        if (clock_) clock_->AddRef();
        return S_OK;
    }

    // IBaseFilter
    STDMETHODIMP EnumPins(IEnumPins** out) override {
        if (!out) return E_POINTER;
        *out = new (std::nothrow) ::EnumPins(&pin_);
        return *out ? S_OK : E_OUTOFMEMORY;
    }
    STDMETHODIMP FindPin(LPCWSTR id, IPin** pin) override {
        if (!id || !pin) return E_POINTER;
        if (wcscmp(id, kPinName) != 0) {
            *pin = nullptr;
            return VFW_E_NOT_FOUND;
        }
        *pin = &pin_;
        pin_.AddRef();
        return S_OK;
    }
    STDMETHODIMP QueryFilterInfo(FILTER_INFO* info) override {
        if (!info) return E_POINTER;
        std::lock_guard<std::mutex> lock(mutex_);
        wcsncpy_s(info->achName, name_[0] ? name_ : kFilterName, _TRUNCATE);
        info->pGraph = graph_;
        if (graph_) graph_->AddRef();
        return S_OK;
    }
    STDMETHODIMP JoinFilterGraph(IFilterGraph* graph, LPCWSTR name) override {
        std::lock_guard<std::mutex> lock(mutex_);
        graph_ = graph;  // weak reference, as DirectShow requires
        wcsncpy_s(name_, name ? name : L"", _TRUNCATE);
        return S_OK;
    }
    STDMETHODIMP QueryVendorInfo(LPWSTR*) override { return E_NOTIMPL; }

    // IAMFilterMiscFlags
    STDMETHODIMP_(ULONG) GetMiscFlags() override { return AM_FILTER_MISC_FLAGS_IS_SOURCE; }

    // Sets the new state and returns the previous one. The pin is never called
    // with the filter lock held: the pin locks itself first, then asks State().
    FILTER_STATE Transition(FILTER_STATE next) {
        std::lock_guard<std::mutex> lock(mutex_);
        FILTER_STATE prev = state_;
        state_ = next;
        return prev;
    }

    FILTER_STATE State() {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }
    IBaseFilter* Base() { return this; }

private:
    std::atomic<ULONG> refs_{1};
    std::mutex mutex_;
    Pin pin_;
    FILTER_STATE state_ = State_Stopped;
    IReferenceClock* clock_ = nullptr;
    IFilterGraph* graph_ = nullptr;
    wchar_t name_[MAX_FILTER_NAME] = {};
};

STDMETHODIMP Pin::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IPin)
        *ppv = static_cast<IPin*>(this);
    else if (riid == IID_IAMStreamConfig)
        *ppv = static_cast<IAMStreamConfig*>(this);
    else if (riid == IID_IKsPropertySet)
        *ppv = static_cast<IKsPropertySet*>(this);
    else {
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}
STDMETHODIMP_(ULONG) Pin::AddRef() { return filter_->AddRef(); }
STDMETHODIMP_(ULONG) Pin::Release() { return filter_->Release(); }
bool Pin::Running() const { return filter_->State() != State_Stopped; }

STDMETHODIMP Pin::QueryPinInfo(PIN_INFO* info) {
    if (!info) return E_POINTER;
    info->pFilter = filter_->Base();
    info->pFilter->AddRef();
    info->dir = PINDIR_OUTPUT;
    wcsncpy_s(info->achName, kPinName, _TRUNCATE);
    return S_OK;
}

// ---------------------------------------------------------------------------
// Class factory

class Factory final : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return 2; }   // static object
    STDMETHODIMP_(ULONG) Release() override { return 1; }
    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = nullptr;
        if (outer) return CLASS_E_NOAGGREGATION;
        auto* filter = new (std::nothrow) Filter();
        if (!filter) return E_OUTOFMEMORY;
        HRESULT hr = filter->QueryInterface(riid, ppv);
        filter->Release();
        return hr;
    }
    STDMETHODIMP LockServer(BOOL lock) override {
        lock ? ++g_locks : --g_locks;
        return S_OK;
    }
};

Factory g_factory;

}  // namespace

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv) {
    if (clsid != CLSID_BandHeartBeatCamera) return CLASS_E_CLASSNOTAVAILABLE;
    return g_factory.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow() { return g_objects == 0 && g_locks == 0 ? S_OK : S_FALSE; }
