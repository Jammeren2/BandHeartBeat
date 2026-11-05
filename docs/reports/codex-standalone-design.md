# Standalone camera architecture review

This is a feasibility/design review only. No camera was activated, no virtual-camera registration was attempted, and no package or system setting was changed.

## Decision

The existing OBS Browser Source + OBS Virtual Camera path remains the working delivery. A direct Discord camera requires two separate pieces: a capture/compositor process and a camera device implementation that Discord can enumerate. A practical first standalone target is Windows 11 22H2+ (`MFCreateVirtualCamera`), with OBS retained as the Windows 10/fallback path. The Windows 11 API removes a kernel driver, but it does **not** turn an arbitrary Rust executable into a camera automatically: a virtual camera is backed by a Media Foundation media source and its COM activation/registration contract must be implemented and tested. Do not advertise this as a single portable EXE until that contract is demonstrated.

The existing local machine is Windows 11 24H2 build 26100 and has the Windows SDK headers/libraries (`mfvirtualcamera.h`, `mfsensorgroup.lib`), so it is a suitable development target. This is an environment fact, not proof that every user has the API.

## Official API findings

- [`FaceTracker`](https://learn.microsoft.com/en-us/uwp/api/windows.media.faceanalysis.facetracker) is a Windows.Media.FaceAnalysis WinRT API. It accepts a `VideoFrame`, processes frames asynchronously with `ProcessNextFrameAsync`, and returns detected face boxes/landmarks. Its `IsSupported` and `GetSupportedBitmapPixelFormats` checks must be used at runtime; do not assume every machine accepts Gray8 or NV12. [`FaceDetector`](https://learn.microsoft.com/en-us/uwp/api/windows.media.faceanalysis.facedetector) is the simpler face-box API. Either can consume app-owned converted frames; neither grants permission to silently capture a webcam.
- [`MFCreateSourceReaderFromMediaSource`](https://learn.microsoft.com/en-us/windows/win32/api/mfreadwrite/nf-mfreadwrite-mfcreatesourcereaderfrommediasource) and the [Source Reader](https://learn.microsoft.com/en-us/windows/win32/medfound/source-reader) API provide decoded webcam samples from an `IMFMediaSource`. Configure a small capture type (for example 640x480 NV12/YUY2), select a video stream, and use asynchronous callbacks or a bounded worker queue. `MFEnumDeviceSources` is the enumeration step; camera privacy/access failures must be surfaced.
- [`MFCreateVirtualCamera`](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera) is the Windows 11 Media Foundation virtual-camera entry point (minimum supported client Windows build 22000, `mfsensorgroup.dll`). Its parameters include lifetime (`MFVirtualCameraLifetime_Session` or system), access (`MFVirtualCameraAccess_CurrentUser`/`AllUsers`), device name, and a source CLSID. Session lifetime is the appropriate first experiment: disposing/shutting down the returned `IMFVirtualCamera` removes its enumerability and avoids persistent device registration. The API is Windows 11 only; use runtime feature detection and keep OBS fallback.
- A source CLSID still identifies the implementation of the `IMFMediaSource`/stream exposed by the virtual camera; the API requires the source ID in `{CLSID}` form. The official API does not state that arbitrary source COM code can be embedded as Rust data in one executable. Treat a custom source COM DLL or separately registered in-process component as required until a minimal official sample is built and enumerated by `MFEnumDeviceSources`. Current-user access can be created without administrator rights only with the allowed camera categories; `AllUsers` requires administrator permissions. Webcam privacy denial returns `E_ACCESSDENIED`. Persistent/system registration adds installation and privilege/uninstall concerns. A session/current-user proof-of-concept is lower risk but still needs COM activation and lifetime testing.
- [Media Foundation virtual camera overview](https://learn.microsoft.com/en-us/windows/win32/medfound/virtual-camera) and [`IMFVirtualCamera`](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nn-mfvirtualcamera-imfvirtualcamera) describe creation/activation and device lifetime. These APIs are not DirectShow `IBaseFilter` APIs; do not mix a DirectShow DLL design with the MF source contract without a deliberate bridge.

The FaceAnalysis APIs are Windows Runtime APIs. In Rust, `windows` crate projections may expose them, but the `VideoFrame`/`SoftwareBitmap` and async ABI surface should be validated in a tiny spike before committing the main app. A C++/WinRT helper DLL is a reasonable fallback if the Rust projection cannot safely marshal `VideoFrame` or `IAsyncOperation`. No OpenCV, ONNX Runtime, YuNet model, DirectML provider, or model-license obligation is needed if FaceDetector/FaceTracker meets the box/landmark requirement. If it does not, YuNet + ONNX Runtime becomes an explicit additional model/runtime dependency and must be reviewed separately; it is not a built-in Windows guarantee.

## Proposed modules and bounded data flow

```text
BLE worker/UI_DATA ───────────────┐
                                  v
IMFSourceReader -> capture ring (2 NV12 frames) -> detector worker (latest frame only)
                                  |                         |
                                  +-> compositor <-----------+ face box + EMA smoothing
                                         |
                                 virtual-camera source
```

1. `camera/enumerate.rs`: initialize Media Foundation, enumerate device names/properties with `MFEnumDeviceSources`; report no-camera/privacy errors without retry spin.
2. `camera/capture.rs`: create `IMFSourceReader`, request a bounded 640x480 NV12 (or YUY2) stream, and run one callback/worker. A two-slot ring drops stale frames instead of accumulating latency.
3. `tracking.rs`: downscale/convert the newest frame to the format required by FaceDetector/FaceTracker; return a face rectangle and optional landmarks. Keep the last valid rectangle for a short bounded grace period, then mark tracking lost. Apply EMA smoothing and clamp the heart anchor to frame bounds.
4. `composite.rs`: draw the heart/BPM using a premultiplied BGRA overlay (Direct2D or a small CPU alpha blend). Heart animation is visual BPM timing from shared telemetry; it is not a measured beat signal. If no face is present, use an explicit fixed corner fallback or disable the badge according to user setting; never claim tracking.
5. `virtual_camera.rs`: own the `IMFVirtualCamera` session object, expose the composited format through the custom Media Foundation source, and test enumeration in a separate small harness. Keep this boundary behind a Windows 11 feature gate.
6. `shell.rs`: preserve the existing native window/tray. Add camera state (`No camera`, `Permission denied`, `Tracking`, `Virtual camera active`) and a Stop Camera action. The existing OBS URL remains available as fallback.

The capture, detector, and compositor workers must have bounded channels (capacity 1–2) and a stop token. One frame is processed at a time; no busy polling. On camera disconnect, release COM/media resources and return to a truthful tray status. BLE `UI_DATA` remains the only BPM source. The virtual-camera output should use the same frame dimensions/framerate negotiated by the source reader and reject unsupported media types clearly.

## Deployment and fallback

Start with Windows 11 session/current-user scope. Detect API availability at runtime and show “Standalone camera requires Windows 11; use OBS overlay” on older systems. If camera enumeration, privacy, tracking, COM source activation, or virtual-camera enumeration fails, keep the native UI/tray running and offer the documented OBS Browser Source URL. Do not silently fall back to a fake camera or fake live BPM. A production all-user camera would require an installer, persistent registration/unregistration and likely elevation; that is a separate release track.

## Unknowns requiring a spike

1. Whether the Rust `windows` projection can construct the exact FaceAnalysis `VideoFrame` and await its WinRT operation without a helper DLL.
2. The minimum custom `IMFMediaSource`/`IMFMediaStream` implementation and source registration needed for `MFCreateVirtualCamera` to enumerate in Discord/WebRTC.
3. Supported source pixel formats/framerate negotiation on representative webcams and whether a D3D11 texture path or CPU BGRA conversion is more reliable on older GPUs.
4. Camera privacy behavior and COM/session teardown when Discord opens/closes the device repeatedly.
5. Actual CPU, memory, frame latency and battery impact. These remain unmeasured; no target is claimed as a benchmark.

## Summary

Windows 11 Media Foundation virtual cameras plus Source Reader and built-in FaceAnalysis are a credible standalone direction, but the custom media-source COM/registration boundary is still a required prototype and prevents claiming a portable single-EXE camera today. Build a small Windows 11 enumeration/source spike first, retain the tested OBS overlay as fallback, and use bounded latest-frame buffers with truthful camera/tracking failure states.
