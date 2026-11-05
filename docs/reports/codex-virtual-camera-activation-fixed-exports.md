# Activation retry with corrected DLL exports

This was the authorized reversible CurrentUser activation test after the DEF export fix.

## Run and cleanup

Initial registry query showed no BandHeartBeat CLSID and passive camera enumeration showed no BandHeartBeat device. Calling the built DLL's `DllRegisterServer` via `ctypes` returned `0x00000000`; HKCU contained the expected InprocServer32 path and `ThreadingModel=Both`.

`activation-probe.exe --activate` then called `MFCreateVirtualCamera` with SoftwareCameraSource, Session lifetime, CurrentUser access and the BandHeartBeat source CLSID. `Start HRESULT: 0x80004002` (`E_NOINTERFACE`). No BandHeartBeat PnP camera appeared. This differs from the prior `CO_E_ERRORINDLL` and confirms the exported class factory was found, but the source does not expose an interface required by Media Foundation virtual-camera activation.

Cleanup was immediate: `activation-probe.exe --remove` returned `0xC00D36B2` (no active camera object), `DllUnregisterServer` returned `S_OK`, and final registry query reported the CLSID missing. Passive PnP camera enumeration found no BandHeartBeat entry. No physical camera, privacy setting, HKLM key, OBS, or Discord was touched.

## Next blocker

The source's class factory is now discoverable, but MF activation's `E_NOINTERFACE` requires implementing the exact source activation contract expected by the Windows software-camera pipeline—likely additional `IMFMediaSource`/`IMFMediaSourceEx` or stream/media-type interfaces and proper descriptor/event/sample binding. The current placeholder stream remains insufficient. Do not retry activation until a focused interface query/Media Foundation sample confirms the missing IID and source contract.

Official references: [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera), [IMFVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nn-mfvirtualcamera-imfvirtualcamera), [IMFMediaSource](https://learn.microsoft.com/en-us/windows/win32/api/mfidl/nn-mfidl-imfmediasource).

## Summary

With corrected exports registered under HKCU, activation reached the source but failed `Start` with `E_NOINTERFACE` and enumerated no device. Registration and session cleanup succeeded; the missing Media Foundation source activation interface contract is the next blocker.
