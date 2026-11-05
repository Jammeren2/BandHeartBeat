# Requested IID trace during virtual-camera activation

Added temporary isolated QueryInterface logging to the source and reran the authorized CurrentUser/session activation test. The source logged:

```text
QI 7FEE9E9A-4A89-47A6
```

The SDK maps `{7FEE9E9A-4A89-47A6-899C-B6A53A70FB67}` to **`IMFActivate`** (`Mfobjects.idl`, `IMFActivate Interface`). This explains `Start HRESULT 0x80004002 (E_NOINTERFACE)`: the source class factory currently creates an `IMFMediaSource`, but Media Foundation's software-camera activation path requests an `IMFActivate` object from the registered source CLSID. `IMFMediaSourceEx` support was added and did not change the result.

Registration returned `S_OK`; activation returned `E_NOINTERFACE`; no BandHeartBeat PnP camera appeared. Cleanup returned `--remove 0xC00D36B2`, `DllUnregisterServer S_OK`, and the HKCU CLSID was absent afterward. No physical camera, privacy, HKLM, OBS or Discord operation occurred.

The temporary file trace writes only to `C:\Temp\BandHeartBeat-QI.log` during this diagnostic build and must not remain in a production binary. The next correct implementation is an `IMFActivate` class with the documented activation/property methods that creates the media source, or an official Windows software-camera source pattern; do not cast an `IClassFactory` vtable to `IMFActivate`. This report intentionally does not add an unsafe interface shim.

Official references: [IMFActivate](https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/nn-mfobjects-imfactivate), [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera), and Windows SDK `Mfobjects.idl`.

## Summary

Tracing identified the exact requested IID as `IMFActivate` (`7FEE9E9A-4A89-47A6-899C-B6A53A70FB67`). The source lacks this activation object, causing E_NOINTERFACE; registration/session cleanup succeeded and no camera enumerated. A proper IMFActivate implementation is the next blocker.
