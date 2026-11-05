# Media source lifecycle binding

Bound a source-owned placeholder stream into the isolated source lifecycle. `PlaceholderSource` now owns/releases a `PlaceholderStream`; `Start` requests one deterministic sample from that stream, `Stop` stops it, and `Shutdown` drains/stops ownership before marking the source closed. The probe uses the source's public `CreatePresentationDescriptor`, `Start`, `Stop`, and `Shutdown` methods and separately exercises the exported stream boundary.

## Build and probe

```bat
cmd.exe /d /s /c virtual-camera\build-msvc.cmd
cd virtual-camera\out
source-probe.exe
```

Observed output includes `S_OK` for source creation, descriptor construction, source Start, stream creation, RequestSample, source Stop and Shutdown. The generated DLL/probe remain isolated under `virtual-camera/out` and are not committed. No physical camera, registry, privacy setting or `MFCreateVirtualCamera` activation was used.

## Remaining blocker

The stream is still not exposed through the `IMFStreamDescriptor` object returned by the presentation descriptor, and event-generator calls return `E_NOTIMPL`; the probe therefore cannot retrieve `MEStreamStarted`/`MEStreamSample` through public Media Foundation event queues. Start currently requests one sample internally as a lifecycle check. A production source needs a stream descriptor bound to the `IMFMediaStream`, source/stream `IMFMediaEventQueue` objects, event dispatch and a bounded `RequestSample` queue before MF virtual-camera activation. Discord visibility remains unverified; OBS is the fallback.

## Summary

Source ownership and Start/Stop/Shutdown lifecycle now drive the deterministic placeholder stream and pass in-process HRESULT checks. Public stream-descriptor binding and Media Foundation event delivery remain the exact next blocker; no virtual-camera or Discord claim is made.
