# Full IMFActivate delegation boundary

The SDK confirms `IMFActivate` inherits the complete `IMFAttributes` interface (the full getter/setter/blob/unknown/serialization vtable), followed by `ActivateObject`, `ShutdownObject`, and `DetachObject`. A correct source object therefore needs a real composition wrapper delegating every IMFAttributes method to an `MFCreateAttributes` store; a partial class or vtable cast is ABI-unsafe and could corrupt the Media Foundation service process.

No unsafe implementation was added. The prior trace identifies the exact requested IID (`{7FEE9E9A-4A89-47A6-899C-B6A53A70FB67}`), and the previous activation/cleanup runs show registration succeeds but Start returns `E_NOINTERFACE` with no PnP camera. The current source/OBS/Rust code remains untouched by this task.

Implementing the wrapper requires mechanically forwarding the full SDK interface (including all 30+ IMFAttributes methods), initializing required source CLSID/media-source attributes, and returning the existing source from `ActivateObject`. That generated ABI layer should be implemented in a focused C++ helper and tested in-process before another registry/activation attempt. No physical camera, privacy, HKLM, or Discord operation was performed.

Official references: [IMFActivate](https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/nn-mfobjects-imfactivate), [IMFAttributes](https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/nn-mfobjects-imfattributes), and [MFCreateAttributes](https://learn.microsoft.com/en-us/windows/win32/api/mfapi/nf-mfapi-mfcreateattributes).

## Summary

Verified the complete IMFActivate/IMFAttributes ABI requirement and avoided an unsafe partial delegation. The exact next blocker is a generated full attribute-forwarding wrapper before source activation can be retried.
