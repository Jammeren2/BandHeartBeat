# DirectShow shared-memory camera prototype

Added an isolated `directshow-camera/` C++ helper based on the documented UnityCapture-style shared-memory pattern. The source defines a stable DirectShow CLSID, standard COM class-factory exports, and a bounded BGRA contract (`Local\\BandHeartBeatBGRA`, `Local\\BandHeartBeatFrameReady`, 640x480, 2-slot/latest-frame intent). It deliberately returns `E_NOTIMPL` from `IClassFactory::CreateInstance`: the full `IBaseFilter`/pin/allocator/`IAMStreamConfig`/`IKsPropertySet` graph is the explicit next implementation boundary, so no fake camera is registered.

## Build

```bat
cmd.exe /d /s /c directshow-camera\build-msvc.cmd
```

VS2022 Build Tools and SDK compilation/link succeeded. The output is isolated under `directshow-camera/out/BandHeartBeatDirectShow.dll` (generated, untracked). No physical camera, registry, privacy setting, HKLM or Discord operation occurred.

The contract follows the MIT-licensed UnityCapture architecture pattern for named shared memory + ready event; no UnityCapture source was copied. A production implementation must retain MIT attribution if code is later borrowed. Per-user registration should write HKCU `CLSID_VideoInputDeviceCategory`/CLSID entries and provide idempotent rollback; this prototype does not register.

## Remaining blocker

Implementing a Discord-visible source requires a real `IBaseFilter` with an output `IPin`, allocator/sample delivery from the shared BGRA mapping, `IAMStreamConfig` media negotiation, and `IKsPropertySet` capture-category response. Until those interfaces are implemented and a passive filter-enumeration probe succeeds, no virtual camera or Discord visibility is claimed. OBS remains the working fallback.

## Summary

Built the isolated DirectShow COM/shared-memory boundary with deterministic frame contract and successful MSVC link. The filter/pin/allocator and registration graph remain explicitly unimplemented; no camera or Discord claim is made.
