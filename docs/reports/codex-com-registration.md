# Current-user COM registration exports

Implemented idempotent HKCU registration helpers in the virtual-camera DLL. `DllRegisterServer` writes only:

```text
HKCU\Software\Classes\CLSID\{8EEC4A2D-7FA2-4B6E-9A4C-246DA8B46F11}\InprocServer32
  (Default)      REG_SZ = module path of the built BandHeartBeatSource.dll
  ThreadingModel REG_SZ = Both
```

`DllUnregisterServer` deletes that CLSID subtree under HKCU and treats a missing key as success. No HKLM/system registration or administrator elevation is used. These helpers are implemented but were not invoked in this task.

## Dry-run verification

```bat
cmd.exe /d /s /c virtual-camera\build-msvc.cmd
cd virtual-camera\out
activation-probe.exe
```

Build/link succeeds with VS2022/SDK and `advapi32.lib`. Default output prints the exact HKCU key/value preview and states that no registration, activation, privacy change, or device enumeration occurred. `--activate` and `--remove` remain explicit opt-in commands; neither was run. The activation harness still requires a properly registered source COM class and a complete Media Foundation stream before `MFCreateVirtualCamera` can succeed. `E_ACCESSDENIED` diagnostics remain in the harness.

The generated DLL/probes are isolated under `virtual-camera/out` and untracked. Rust/OBS release artifacts and user registry were untouched; no physical camera or Discord visibility was tested.

One-time user-visible commands, after separate approval, are `source DLL DllRegisterServer`/the equivalent current-user registration helper, then `activation-probe.exe --activate`. Rollback is `activation-probe.exe --remove` followed by `DllUnregisterServer`; these commands are intentionally not automated by default.

Official refs: [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera), [COM registration](https://learn.microsoft.com/en-us/windows/win32/com/com-registry-keys), and [IMFVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nn-mfvirtualcamera-imfvirtualcamera).

## Summary

Added safe idempotent HKCU `InprocServer32` registration/unregistration exports and a dry-run key preview. MSVC build passes; no registry or camera operation ran. Activation remains explicit and blocked by the incomplete source/stream contract.
