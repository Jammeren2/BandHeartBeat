# Minimal Media Foundation source lifecycle

Extended the isolated `virtual-camera/` helper with a real COM object implementing the `IMFMediaSource` and inherited `IMFMediaEventGenerator` method boundary. No physical webcam, BLE telemetry, camera registration, privacy setting, or `MFCreateVirtualCamera` call is performed.

## Build and lifecycle check

```bat
cmd.exe /d /s /c virtual-camera\build-msvc.cmd
```

The script uses VS2022 17.14.44 x64 and SDK 10.0.26100.0. It builds the DLL against `ole32.lib`, `mfplat.lib`, and `mfsensorgroup.lib`, then builds the probe. Running from the output directory:

```bat
cd virtual-camera\out
source-probe.exe
```

Output:

```text
Source CLSID: {8EEC4A2D-7FA2-4B6E-9A4C-246DA8B46F11}
CreatePlaceholderSource HRESULT: 0x0
GetCharacteristics HRESULT: 0x0
Shutdown HRESULT: 0x0
No MFCreateVirtualCamera call; no registration, camera activation, or telemetry performed.
```

Latest isolated artifacts: `BandHeartBeatSource.dll` 118,272 bytes and `source-probe.exe` 296,960 bytes. They are generated under `virtual-camera/out` and intentionally not committed. Existing GNU/OBS artifacts were untouched.

## Implemented boundary

`PlaceholderSource` has COM reference counting and `QueryInterface` for `IUnknown`, `IMFMediaSource`, and `IMFMediaEventGenerator`. It returns live-source characteristics, supports Start/Pause/Stop/Shutdown state checks, and exposes all event-generator methods with explicit `E_NOTIMPL` until a stream exists. `Factory` implements `IClassFactory`; `DllGetClassObject` can create the source through a normal COM class-factory request. The probe loads the DLL in-process, calls the exported source constructor, queries characteristics, and shuts it down. This proves activation/lifetime ownership without system registration.

## Exact missing contract

This stage does **not** claim a valid virtual-camera stream or fixed frame yet. `CreatePresentationDescriptor` remains `E_NOTIMPL`, so `Start` cannot provide a presentation descriptor, no `IMFMediaStream` is exposed, and no BGRA/NV12 media type, sample timestamp, or deterministic placeholder frame is delivered. The next implementation must add one stream implementing `IMFMediaStream` and its event generator, construct a presentation descriptor with a negotiated video `IMFMediaType`, and queue bounded samples with 100-nanosecond timestamps. The source must then be registered/activated through the documented `{CLSID}` source ID of [`MFCreateVirtualCamera`](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera) in a separate opt-in experiment.

Current-user registry helpers remain deliberately `E_NOTIMPL`; no installer boundary is hidden. A future current-user/session implementation must handle `E_ACCESSDENIED` privacy failures and clean up registration. Until that stream and activation contract is complete, OBS Browser Source remains the working fallback and Discord visibility is unverified.

## Summary

Implemented and tested a real COM `IMFMediaSource` lifecycle/class-factory boundary in the isolated helper; deterministic frame/sample production and virtual-camera activation remain explicitly unimplemented. MSVC DLL/probe builds and in-process create/query/shutdown checks passed without camera or registry side effects.
