# Standalone camera source prototype

Implemented and built an isolated Windows COM source boundary under `virtual-camera/`. It does not modify the Rust/OBS application, activate a physical camera, invoke `MFCreateVirtualCamera`, register a virtual camera, touch privacy settings, or claim Discord visibility.

## Artifacts and exact build

From `D:\py\BandHeartBeat`:

```bat
cmd.exe /d /s /c virtual-camera\build-msvc.cmd
virtual-camera\out\source-probe.exe
```

The script invokes `D:\VSBuildTools\Common7\Tools\VsDevCmd.bat -arch=x64 -host_arch=x64`, then MSVC 17.14.44 compiles with the Windows SDK 10.0.26100.0 and links `ole32.lib`, `mfplat.lib`, and `mfsensorgroup.lib`:

- `virtual-camera/out/BandHeartBeatSource.dll`: 103,424 bytes.
- `virtual-camera/out/source-probe.exe`: 238,080 bytes.

The probe output is:

```text
Source CLSID: {8EEC4A2D-7FA2-4B6E-9A4C-246DA8B46F11}
Current-user registration helper: not implemented (safe prototype).
No MFCreateVirtualCamera call; no camera registration or activation performed.
```

Generated binaries are intentionally left out of source control; rerun the command to reproduce them. The existing GNU/OBS release artifacts are untouched.

## What is implemented

`source_contract.h` defines a stable source CLSID and documents the future `IMFMediaSource`/`IMFMediaStream` contract. `source_dll.cpp` is a loadable COM in-process DLL boundary with `DllGetClassObject`, `DllCanUnloadNow`, and explicit `DllRegisterServer`/`DllUnregisterServer` exports. It returns `CLASS_E_CLASSNOTAVAILABLE`/`E_NOTIMPL` deliberately because no media source is exposed yet. `source_probe.cpp` formats the `{CLSID}` source ID and reports that registration/activation are intentionally skipped.

This is an explicit placeholder, not a fake live frame producer. It has no webcam access, no BPM generation, and no independent telemetry. It proves the isolated DLL/EXE build boundary and source identity only.

## Missing implementation and installer boundary

The next safe stage must implement a real COM class factory and `IMFMediaSource` plus `IMFMediaEventGenerator`, one media stream, media-type negotiation, sample delivery, and shutdown semantics. The media source must be activatable by the source CLSID passed to [`MFCreateVirtualCamera`](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera). A future current-user registration layer would write only the documented per-user COM entries and clean them up; this prototype does not write registry state. A persistent or `AllUsers` camera needs an installer and administrator boundary (`AllUsers` requires admin); session/current-user scope is the safer first activation experiment. Privacy denial must be surfaced as `E_ACCESSDENIED`.

The source still needs a bounded frame producer (two latest-frame slots, no unbounded queue), likely fed by a separate `IMFSourceReader` capture worker. Face tracking should use runtime-checked `FaceTracker`/`FaceDetector` supported bitmap formats, with explicit no-camera/no-face states. The existing OBS overlay remains the fallback until source activation and enumeration are demonstrated.

Official references: [`MFCreateVirtualCamera`](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera), [`IMFVirtualCamera`](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nn-mfvirtualcamera-imfvirtualcamera), [virtual camera overview](https://learn.microsoft.com/en-us/windows/win32/medfound/virtual-camera), and [`FaceTracker`](https://learn.microsoft.com/en-us/uwp/api/windows.media.faceanalysis.facetracker).

## Summary

Built an isolated 103,424-byte COM DLL and 238,080-byte probe that compile/link the Windows 11 virtual-camera boundary and expose a stable source CLSID without registering or activating anything. The real IMFMediaSource/class-factory/frame path remains the explicit next blocker; no camera or Discord visibility is claimed.
