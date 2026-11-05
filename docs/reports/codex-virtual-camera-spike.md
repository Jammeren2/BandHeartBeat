# Windows 11 virtual-camera API spike

This isolated spike checks the installed SDK/compiler boundary only. It does not activate a physical camera, call `MFCreateVirtualCamera`, register a virtual camera, alter privacy settings, or touch the existing release executable.

## What was built

`camera-spike/main.cpp` includes the Windows SDK's `mfvirtualcamera.h`, `mfapi.h`, and `mfreadwrite.h`, takes the address of `MFCreateVirtualCamera` to prove the declaration is visible, and links `mfsensorgroup.lib`, `mfplat.lib`, and `mfreadwrite.lib`. It prints the required source-ID/lifetime/access choices but deliberately does not invoke the API. `camera-spike/build-msvc.cmd` sets up the installed VS2022 x64 developer environment and writes to the isolated `camera-spike/out` directory.

Exact command:

```bat
cmd.exe /d /s /c camera-spike\build-msvc.cmd
```

Output: exit code 0. The resulting `camera-spike/out/camera-api-boundary.exe` is **220,160 bytes** and runs:

```text
MFCreateVirtualCamera declaration available; activation intentionally skipped.
Required sourceId form: {CLSID}; target lifetime: Session; access: CurrentUser.
```

This proves the installed Windows SDK and MSVC libraries expose the API surface. It does not prove camera enumeration or Discord visibility.

## API boundary and blocker

Official [`MFCreateVirtualCamera`](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera) requires a source ID in `{CLSID}` form for the custom Media Foundation source. The application must provide an `IMFMediaSource`/stream implementation that can be activated by that CLSID. The spike intentionally has no COM class, source DLL, media stream, registration, or frame producer, so it cannot create a camera yet. Implementing only the call in the current Rust EXE would not establish the source activation boundary and would be an unsupported single-EXE claim.

The official API targets Windows build 22000+, uses `mfsensorgroup.dll`, and supports session lifetime. `MFVirtualCameraAccess_CurrentUser` is the appropriate non-admin prototype scope; `AllUsers` requires administrator permission. Non-admin callers are limited to the documented camera categories. Webcam privacy denial can return `E_ACCESSDENIED`. Session shutdown/disposal removes the camera's enumeration/activation state. See [IMFVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nn-mfvirtualcamera-imfvirtualcamera) and [Media Foundation virtual cameras](https://learn.microsoft.com/en-us/windows/win32/medfound/virtual-camera).

Next spike should be a separate C++/WinRT or COM DLL implementing the smallest source/stream, registered only for the current user or session according to the documented model, then a read-only `MFEnumDeviceSources` harness. It must be tested with a synthetic frame before any physical camera or Discord test. The existing OBS overlay remains the safe working fallback.

## Summary

The installed Windows 11 SDK and MSVC successfully compile/link the `MFCreateVirtualCamera` API boundary in an isolated 220,160-byte helper. The required custom Media Foundation source/COM activation is still unimplemented; no camera registration or Discord visibility is claimed.
