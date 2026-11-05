#pragma once
#include <windows.h>
#include <stdint.h>
#define BH_SHM_NAME L"Local\\BandHeartBeatBGRA"
#define BH_READY_EVENT L"Local\\BandHeartBeatFrameReady"
struct BhFrameHeader { uint32_t magic=0x42484247,width=640,height=480,stride=2560,bytes=1228800; uint64_t sequence=0; };
