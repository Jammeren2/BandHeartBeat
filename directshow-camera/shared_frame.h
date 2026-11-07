#pragma once
#include <windows.h>
#include <stdint.h>
#ifdef BH_ISOLATED_TEST
#define BH_SHM_NAME L"Local\\BandHeartBeatBGRAUxTest"
#define BH_READY_EVENT L"Local\\BandHeartBeatFrameReadyUxTest"
#define BH_FRAME_MUTEX L"Local\\BandHeartBeatFrameMutexUxTest"
#define BH_CAPTURE_STOP L"Local\\BandHeartBeatCaptureStopUxTest"
#define BH_CAPTURE_SINGLE L"Local\\BandHeartBeatCaptureWorkerUxTest"
#else
#define BH_SHM_NAME L"Local\\BandHeartBeatBGRA"
#define BH_READY_EVENT L"Local\\BandHeartBeatFrameReady"
#define BH_FRAME_MUTEX L"Local\\BandHeartBeatFrameMutex"
#define BH_CAPTURE_STOP L"Local\\BandHeartBeatCaptureStop"
#define BH_CAPTURE_SINGLE L"Local\\BandHeartBeatCaptureWorker"
#endif
struct BhFrameHeader { uint32_t magic=0x42484247,version=1,width=640,height=480,stride=2560,bytes=1228800; uint64_t sequence=0; };

static_assert(sizeof(BhFrameHeader)==32, "IPC ABI header must be 32 bytes");

