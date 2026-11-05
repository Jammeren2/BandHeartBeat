# IMFStreamDescriptor binding review

The requested “descriptor stream -> IMFMediaStream” identity cannot be implemented through the public Media Foundation interfaces as phrased. `IMFPresentationDescriptor` contains `IMFStreamDescriptor` objects describing media types and selection; `IMFStreamDescriptor` exposes no `IMFMediaStream` pointer or setter. `IMFMediaSource::Start` receives the selected presentation descriptor, while the source is responsible for creating/dispatching its own `IMFMediaStream` and events. Therefore there is no supported public `QueryInterface` path from an `IMFStreamDescriptor` to a concrete stream whose pointer identity can be asserted.

The current source already owns `PlaceholderStream`, creates the matching selected RGB32 stream descriptor, starts/request-samples that object internally, and exposes its event queue. The probe's exported stream constructor is only a test seam; removing it would make the requested identity test impossible rather than more correct. No source changes were made in this review because adding a private `IMFStreamDescriptor` implementation would violate the SDK contract and would not be consumable by `MFCreatePresentationDescriptor`.

The correct next implementation is source-owned mapping: retain the selected stream descriptor and stream together in the source, validate the descriptor's media type/ID during `Start`, and expose stream events through the source's event-driven pipeline. A source-specific diagnostic interface can report the mapping for tests, but it is not a Media Foundation public interface and must not be mistaken for a descriptor binding.

Remaining activation blockers are unchanged: implement a complete source/stream event contract accepted by the Media Foundation virtual-camera source activation, register the source CLSID under an opt-in current-user/session installer boundary, then verify enumeration. No `MFCreateVirtualCamera` call, registration, physical camera, privacy setting, or Discord visibility was attempted.

Official references: [IMFStreamDescriptor](https://learn.microsoft.com/en-us/windows/win32/api/mfidl/nn-mfidl-imfstreamdescriptor), [IMFPresentationDescriptor](https://learn.microsoft.com/en-us/windows/win32/api/mfidl/nn-mfidl-imfpresentationdescriptor), and [IMFMediaSource](https://learn.microsoft.com/en-us/windows/win32/api/mfidl/nn-mfidl-imfmediasource).

## Summary

Verified that public Media Foundation descriptors describe stream metadata and selection but cannot carry or return an `IMFMediaStream` object. The requested pointer-identity acceptance path is therefore an API mismatch; source-owned mapping and a diagnostic seam are the implementable alternatives before virtual-camera activation.
