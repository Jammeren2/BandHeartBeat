# Media Foundation event queue stage

Added `MFCreateEventQueue` ownership to both placeholder source and stream. Source Start/Stop/Shutdown now drive source queue events (`MESourceStarted`, `MESourceStopped`) and stream Start/Stop state. `RequestSample` queues a deterministic RGB32 sample as `MEStreamTick` (the SDK has no `MEStreamSample` enum; a production source must use the documented stream event semantics). Event generator methods delegate to the queues instead of returning `E_NOTIMPL`.

Build and in-process probe:

```bat
cmd.exe /d /s /c virtual-camera\build-msvc.cmd
cd virtual-camera\out
source-probe.exe
```

Build succeeds with VS2022/SDK 10.0.26100.0. Probe returns `S_OK` for source creation, presentation descriptor, source Start, stream creation, RequestSample, source Stop, and Shutdown. No camera, registry, privacy, BLE, `MFCreateVirtualCamera`, or Discord operation is performed. Generated output remains under `virtual-camera/out` and untracked.

## Remaining blocker

The source's `IMFStreamDescriptor` still does not expose the concrete `IMFMediaStream` instance; the probe uses the exported in-process stream constructor for lifecycle checks. A production implementation must bind that stream through the descriptor and validate event retrieval through public `IMFMediaEventGenerator` interfaces. It also needs correct end-of-stream and sample-request semantics before virtual-camera activation. OBS remains the working fallback and Discord visibility is unverified.

## Summary

Implemented source/stream Media Foundation event queues and deterministic sample event delivery, with successful isolated MSVC build and lifecycle probe. Descriptor-to-stream binding and production virtual-camera activation remain unimplemented; no camera or Discord claim is made.
