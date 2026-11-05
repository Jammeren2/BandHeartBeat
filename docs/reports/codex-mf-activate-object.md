# IMFActivate contract investigation

The requested `IMFActivate` interface is not a small three-method shim. Windows SDK `mfobjects.h` declares `IMFActivate : public IMFAttributes`; therefore a valid object must implement the complete IMFAttributes vtable in addition to `ActivateObject`, `ShutdownObject`, and `DetachObject`. Returning an `IMFMediaSource` from `IClassFactory::CreateInstance` cannot satisfy the software-camera activation path, which requested IID `{7FEE9E9A-4A89-47A6-899C-B6A53A70FB67}`.

I did not add an unsafe vtable cast or partial fake object. The current source remains unchanged and the existing isolated probe/cleanup evidence is preserved. Implementing a correct object requires either delegating every IMFAttributes method to an internal `IMFAttributes` store (created by `MFCreateAttributes`) or using a documented COM aggregation pattern, then implementing `ActivateObject(IID_IMFMediaSource)`, `ShutdownObject`, and `DetachObject` with explicit ownership. The store must carry the source CLSID/media-source attributes expected by `MFCreateVirtualCamera`.

No activation was attempted in this task because the source cannot safely expose the required interface yet. The prior authorized run established registration succeeds, Start returns `E_NOINTERFACE`, no PnP camera appears, and cleanup removes the HKCU CLSID. Physical cameras, privacy, HKLM, OBS and Discord remain untouched.

Official references: [IMFActivate](https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/nn-mfobjects-imfactivate), [MFCreateAttributes](https://learn.microsoft.com/en-us/windows/win32/api/mfapi/nf-mfapi-mfcreateattributes), and [MFCreateVirtualCamera](https://learn.microsoft.com/en-us/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera).

## Summary

Confirmed the next blocker is a complete `IMFActivate` plus inherited `IMFAttributes` implementation, not a single missing method. Avoided unsafe casts and source changes; prior registration/activation failure and cleanup remain documented.
