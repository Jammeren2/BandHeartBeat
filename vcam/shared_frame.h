// Shared-memory frame written by the BandHeartBeat app (crates/core/src/frame.rs).
// A 64-byte header followed by top-down BGRA pixels. `seq` is odd while the
// app is writing; readers retry until they copy a frame with an even, unchanged seq.
#pragma once
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>

namespace bhb {

constexpr wchar_t kFrameMapping[] = L"Local\\BandHeartBeat.Frame.v2";
constexpr uint32_t kMagic = 0x32424842;  // "BHB2"
constexpr size_t kHeaderSize = 64;
constexpr ULONGLONG kStaleMs = 2000;

struct FrameHeader {
    uint32_t magic;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    std::atomic<uint64_t> seq;
    std::atomic<uint64_t> tick;  // GetTickCount64() of the last complete frame
};
static_assert(sizeof(FrameHeader) <= kHeaderSize, "header must fit");

class FrameReader {
public:
    ~FrameReader() { Close(); }

    // Copies the latest frame (top-down BGRA) if it is `width`x`height` and
    // fresh. Returns false when the app is not running or the frame is stale.
    bool Read(uint32_t width, uint32_t height, uint8_t* dst) {
        if (!Open()) return false;
        auto* header = static_cast<FrameHeader*>(view_);
        if (header->magic != kMagic || header->width != width || header->height != height ||
            header->stride != width * 4)
            return false;
        const size_t bytes = size_t(width) * height * 4;
        const auto* pixels = static_cast<const uint8_t*>(view_) + kHeaderSize;
        for (int attempt = 0; attempt < 4; ++attempt) {
            uint64_t before = header->seq.load(std::memory_order_acquire);
            if (before & 1) { Sleep(1); continue; }
            memcpy(dst, pixels, bytes);
            std::atomic_thread_fence(std::memory_order_acquire);
            if (header->seq.load(std::memory_order_relaxed) == before)
                return GetTickCount64() - header->tick.load(std::memory_order_acquire) < kStaleMs;
        }
        return false;
    }

private:
    bool Open() {
        if (view_) return true;
        mapping_ = OpenFileMappingW(FILE_MAP_READ, FALSE, kFrameMapping);
        if (!mapping_) return false;
        view_ = MapViewOfFile(mapping_, FILE_MAP_READ, 0, 0, 0);
        if (!view_) Close();
        return view_ != nullptr;
    }

    void Close() {
        if (view_) UnmapViewOfFile(view_);
        if (mapping_) CloseHandle(mapping_);
        view_ = nullptr;
        mapping_ = nullptr;
    }

    HANDLE mapping_ = nullptr;
    void* view_ = nullptr;
};

}  // namespace bhb
