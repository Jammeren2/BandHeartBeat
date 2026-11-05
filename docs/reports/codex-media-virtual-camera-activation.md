# Current-user virtual-camera activation harness

Added `virtual-camera/activation_probe.cpp`, an explicit activation/removal harness for the Windows 11 Media Foundation virtual-camera API. Default invocation is a safe dry run and performs no registration, activation, privacy change or device enumeration:

```bat
cmd.exe /d /s /c virtual-camera\build-msvc.cmd
cd virtual-camera\out
activation-probe.exe
```

The dry-run output states the intended call: `MFCreateVirtualCamera(SoftwareSource, Session, CurrentUser, BandHeartBeat Virtual Camera, {8EEC4A2D-7FA2-4B6E-9A4C-246DA8B46F11})`. The harness also accepts explicit `--activate` and `--remove` flags for a separately approved experiment. Those paths call `MFCreateVirtualCamera`, then `Start` or `Remove`, and always `Shutdown`/`Release`/`MFShutdown`; they were deliberately not run here. Failures print HRESULT and identify `E_ACCESSDENIED` as privacy, source-registration, category, or admin boundary.

Build succeeded with VS2022/SDK 10.0.26100.0. No registry helper is implemented: the source DLL's current-user registration exports remain `E_NOTIMPL`, so `--activate` is expected to fail until a properly registered source CLSID/COM class factory exists. No physical camera or Discord operation occurred. Generated output stays in `virtual-camera/out` and is untracked; Rust/OBS release artifacts are untouched.

Official references: [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera), [IMFVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nn-mfvirtualcamera-imfvirtualcamera), and [virtual camera overview](https://learn.microsoft.com/en-us/windows/win32/medfound/virtual-camera).

## Summary

Built a safe-by-default CurrentUser/session activation harness with explicit `--activate`/`--remove` cleanup paths and HRESULT diagnostics. Dry run passed; activation remains intentionally untested and blocked by the unregistered source COM class/Media Foundation stream contract.
