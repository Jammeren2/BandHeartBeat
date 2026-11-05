# Current-user virtual-camera activation run

This was the explicitly authorized reversible activation test. Before changing anything, `reg query HKCU\Software\Classes\CLSID\{8EEC4A2D-7FA2-4B6E-9A4C-246DA8B46F11}` reported that the key did not exist. Passive `Get-PnpDevice -Class Camera` showed the existing USB2.0 and Iriun devices only; no BandHeartBeat device was present.

## Registration and activation

The built DLL was loaded with Python `ctypes` and its current-user `DllRegisterServer` export was called. It returned `HRESULT 0x00000000`. Registry verification showed:

```text
HKCU\Software\Classes\CLSID\{8EEC4A2D-7FA2-4B6E-9A4C-246DA8B46F11}\InprocServer32
  (Default) D:\py\BandHeartBeat\virtual-camera\out\BandHeartBeatSource.dll
  ThreadingModel Both
```

`activation-probe.exe --activate` called `MFCreateVirtualCamera` with SoftwareCameraSource, Session lifetime, CurrentUser access and friendly name `BandHeartBeat Virtual Camera`. `Start` failed with **HRESULT 0x800401F9 (`CO_E_ERRORINDLL`)**. No camera appeared in passive PnP enumeration. This is consistent with the source DLL not exposing a usable/registered `DllGetClassObject`/complete IMFMediaSource activation contract.

Cleanup was attempted immediately: `activation-probe.exe --remove` returned `0xC00D36B2` (no active virtual-camera object to remove), then `DllUnregisterServer` returned `S_OK`. A final registry query confirmed the CLSID key was absent, and PnP enumeration found no BandHeartBeat device. No HKLM key, physical camera, privacy setting, Discord client or OBS process was touched.

During diagnosis, a local-only export/DEF experiment confirmed the missing DLL entrypoint was a source defect; those temporary source/build changes were reverted and are not part of this report-only commit. The committed source remains unchanged by this task.

## Remaining blocker

Before retrying activation, the source DLL needs a stable exported COM class factory (`DllGetClassObject`) and a complete Media Foundation source/stream activation contract that the OS can instantiate from the CLSID. The current placeholder stream/descriptor and event implementation are not sufficient. Any future retry must repeat the HKCU baseline check, use explicit approval, and clean up registration/session state even on failure.

Official references: [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera), [IMFVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nn-mfvirtualcamera-imfvirtualcamera), and [COM registration](https://learn.microsoft.com/en-us/windows/win32/com/com-registry-keys).

## Summary

Current-user registration succeeded, but the authorized Session virtual-camera activation failed with `0x800401F9 CO_E_ERRORINDLL`; no device enumerated. Registration was fully removed and the CLSID key is absent. The source's COM activation/media-source contract remains the blocker; no camera or Discord success is claimed.
