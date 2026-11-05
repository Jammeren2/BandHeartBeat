#pragma once
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfvirtualcamera.h>

// Stable source identity used in MFCreateVirtualCamera's {CLSID} sourceId.
// This prototype intentionally has no media-source activation yet.
// The eventual class must implement IMFMediaSource and IMFMediaEventGenerator,
// expose one IMFMediaStream with a negotiated video type, and produce samples.
inline constexpr CLSID CLSID_BandHeartBeatSource =
{ 0x8eec4a2d, 0x7fa2, 0x4b6e, { 0x9a, 0x4c, 0x24, 0x6d, 0xa8, 0xb4, 0x6f, 0x11 } };

// No registration is performed by this prototype. These are the explicit
// operations a future current-user installer/cleanup layer must implement.
