# E_NOINTERFACE source contract investigation

Added an isolated `IMFMediaSourceEx` boundary to the placeholder source: QueryInterface now exposes that IID and implements GetSourceAttributes/GetStreamAttributes with empty attribute stores plus SetD3DManager for the CPU placeholder path. This is a compileable extension, not evidence that this was the missing activation IID.

VS2022/SDK build succeeded. Reversible CurrentUser registration returned S_OK. Activation after rebuilding still returned **Start 0x80004002 E_NOINTERFACE**. Thus IMFMediaSourceEx alone did not resolve the activation contract. No exact missing IID has yet been captured; adding more interfaces speculatively is not justified.

Cleanup: `activation-probe.exe --remove` returned `0xC00D36B2`, DllUnregisterServer returned S_OK, and the HKCU CLSID query confirmed absence. No physical camera, privacy setting, HKLM or Discord operation was used. Existing Rust/OBS release is untouched.

The next diagnostic must record requested QueryInterface IIDs in the source/class factory and compare them with the official Windows camera media-source sample, then implement the specific activation/stream attributes contract. This task has not established device enumeration or camera success.

Reference: [IMFMediaSourceEx](https://learn.microsoft.com/en-us/windows/win32/api/mfidl/nn-mfidl-imfmediasourceex), [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera).

## Summary

Implemented and built IMFMediaSourceEx support, but the authorized activation retry still fails E_NOINTERFACE. Registration was cleaned up; exact requested-IID tracing remains necessary before another source-side fix.
