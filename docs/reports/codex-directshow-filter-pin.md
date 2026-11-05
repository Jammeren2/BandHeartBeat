# DirectShow filter and output pin

Implemented real SDK C++ interfaces in `directshow-camera/filter.h`: `IBaseFilter` (including IPersist/IMediaFilter), one output `IPin`, `IEnumPins`, and `IEnumMediaTypes`. The class factory now creates the filter and rejects aggregation. The embedded pin's AddRef/Release retain the owning filter, so a pin/enumerator cannot outlive its parent. FindPin and EnumPins return the same pin identity. RGB32 metadata describes 640x480, 32-bit BI_RGB, fixed 1,228,800-byte samples and a 5 fps frame duration. Filter Run/Pause/Stop and reference-clock ownership are implemented.

The SDK compiler checks all interface signatures through `override`; no partial vtable or interface reinterpret-cast is used. VIDEOINFOHEADER interpretation uses the SDK's documented AM_MEDIA_TYPE format block layout. Registration remains `E_NOTIMPL`.

## Actual checks

From repository root: `cmd.exe /d /s /c directshow-camera\build-msvc.cmd` succeeded with VS2022/Windows SDK. Then, from `directshow-camera/out`, `filter-probe.exe` returned exit 0:

```text
PASS class factory, IBaseFilter, EnumPins/FindPin identity, output IPin, RGB32 640x480 at 5fps
PASS Run/Pause/Stop and COM release; no registration/camera access
```

The probe loads the DLL/GetProcAddress, calls DllGetClassObject/CreateInstance, enumerates the output pin, verifies direction, QueryPinInfo parent identity, FindPin identity, and video format dimensions/duration. Every tested HRESULT must equal S_OK, and failures return nonzero. Artifacts: `directshow-camera/out/BandHeartBeatDirectShow.dll` 124,416 bytes; `filter-probe.exe` 283,648 bytes.

## Remaining boundary

Connect/allocator negotiation and IMemInputPin Receive/sample scheduling are unimplemented. IAMStreamConfig, IKsPropertySet capture category, shared-memory reading/synchronization, per-user registration/rollback, and production thread safety/unload accounting remain required. The named BGRA contract is retained but not consumed yet. QueryAccept currently validates major/subtype only and will need full format validation when connecting. No physical camera, registry, privacy, HKLM or Discord was accessed. Rust/OBS/MF experiments are untouched. No camera-enumeration or Discord visibility claim is made.

This is original SDK-interface code; no UnityCapture code was copied. The architectural shared-memory pattern reference remains MIT UnityCapture; any future copied code must retain its upstream license.

## Summary

Built and exercised an actual IBaseFilter/output IPin with RGB32 enumeration, COM identity/lifetime and state transitions. In-process probe passes; allocator/sample delivery and capture-device interfaces/registration remain the next implementation boundary.
