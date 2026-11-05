# Placeholder Media Foundation stream stage

Extended the isolated COM source from `b79667a` with a real presentation descriptor. The source now constructs one selected video stream descriptor using SDK helpers (`MFCreateMediaType`, `MFCreateStreamDescriptor`, `MFCreatePresentationDescriptor`) and negotiates a deterministic RGB32 type: 320x240, fixed low-rate placeholder intent. It remains clearly a placeholder and does not access physical hardware or BLE telemetry.

Build and test:

```bat
cmd.exe /d /s /c virtual-camera\build-msvc.cmd
cd virtual-camera\out
source-probe.exe
```

The VS2022/SDK build succeeded. The in-process probe loaded `BandHeartBeatSource.dll`, created the COM source, returned `S_OK` from `GetCharacteristics` and `CreatePresentationDescriptor`, then returned `S_OK` from `Shutdown`. It did not call `MFCreateVirtualCamera`, write registry state, change privacy, or activate a camera. Generated outputs remain isolated under `virtual-camera/out` and untracked.

## Remaining exact blocker

The descriptor currently has no `IMFMediaStream` object. `Start`, `Stop`, `Pause`, and event methods still return `E_NOTIMPL` (or a shutdown error), and no `IMFSample`/`IMFMediaBuffer`, timestamp, or stream event is delivered. A valid source for MF virtual-camera activation still requires implementing `IMFMediaStream` plus its `IMFMediaEventGenerator`, wiring a stream object into the stream descriptor, handling `RequestSample`, producing bounded RGB32/NV12 buffers and 100-nanosecond timestamps, and delivering start/stop/end-of-stream events. The next stage must test those methods through the probe before any virtual-camera registration experiment.

The existing source CLSID/class factory and current-user registration helpers remain unchanged and registration is intentionally disabled. Discord visibility is not claimed. OBS remains the working fallback.

Official references: [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera), [IMFMediaStream](https://learn.microsoft.com/en-us/windows/win32/api/mfidl/nn-mfidl-imfmediastream), [MFCreateMediaType](https://learn.microsoft.com/en-us/windows/win32/api/mfapi/nf-mfapi-mfcreatemediatype).

## Summary

Added and tested a selected RGB32 presentation descriptor for the placeholder source. COM source creation, descriptor construction and shutdown pass in-process; the IMFMediaStream/sample/event contract remains the explicit blocker before virtual-camera activation.
