# Placeholder IMFMediaStream sample delivery

Added a bounded deterministic RGB32 placeholder stream to the isolated Media Foundation source. No physical camera, registry, privacy setting, BLE telemetry, or `MFCreateVirtualCamera` activation is used.

## Build and in-process probe

```bat
cmd.exe /d /s /c virtual-camera\build-msvc.cmd
cd virtual-camera\out
source-probe.exe
```

The installed VS2022/SDK build succeeds. The probe output includes:

```text
CreatePlaceholderSource HRESULT: 0x0
GetCharacteristics HRESULT: 0x0
CreatePresentationDescriptor HRESULT: 0x0
CreatePlaceholderStream HRESULT: 0x0
RequestSample HRESULT: 0x0
Shutdown HRESULT: 0x0
```

The stream implements COM QueryInterface/refcount, `GetMediaSource`/`GetStreamDescriptor` placeholders, event-generator method boundary, and `RequestSample`. Each request allocates one 320x240x4 `MFCreateMemoryBuffer`, fills deterministic blue/coral marker bytes, creates an `IMFSample`, attaches the buffer, sets an increasing 100-nanosecond timestamp and 20-ms duration, then releases it. The two-slot/unbounded queue issue is avoided by producing only on demand. Generated DLL/probe outputs remain isolated under `virtual-camera/out` and are untracked.

## Remaining blocker

The stream is not yet wired into the source's presentation descriptor and event queue: `IMFMediaStream::GetStreamDescriptor`/`GetMediaSource` and event-generator methods remain explicit placeholders, while source `Start`/`Stop` do not dispatch `MEStreamStarted`/`MEStreamSample` events. A production source still needs a shared stream object, descriptor binding, event queue (including end-of-stream), and a valid `IMFMediaSource::Start` path before MF virtual-camera activation can consume it. The current artifact proves buffer/sample construction and timestamps but does not prove virtual-camera enumeration or Discord visibility. OBS remains the fallback.

Official references: [IMFMediaStream](https://learn.microsoft.com/en-us/windows/win32/api/mfidl/nn-mfidl-imfmediastream), [MFCreateMemoryBuffer](https://learn.microsoft.com/en-us/windows/win32/api/mfapi/nf-mfapi-mfcreatememorybuffer), and [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera).

## Summary

Implemented and exercised a deterministic bounded RGB32 `IMFSample` producer with increasing timestamps through a COM `IMFMediaStream` boundary. Build and probe checks pass; descriptor/event binding and MF virtual-camera enumeration remain unimplemented and unclaimed.
