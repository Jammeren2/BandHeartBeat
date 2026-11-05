# DirectShow allocator and frame delivery milestone

Implemented functional output-pin delivery to a minimal honest DirectShow sink. This milestone uses the actual Windows `CLSID_MemoryAllocator`, `IMemAllocator::SetProperties/Commit/GetBuffer`, downstream `IMemInputPin::NotifyAllocator/Receive`, and SDK `IMediaSample` objects. It does not register a camera or use a physical webcam.

## Behavior

Connect validates the fixed RGB32/VideoInfo format, connects the downstream pin, obtains/falls back to the Windows memory allocator, requests two 1,228,800-byte buffers and checks actual properties. The worker sends a deterministic coral/dark checkerboard at a nominal 5 fps using condition-variable timed waits, with actual buffer length, sync-point/discontinuity flags and increasing 100-nanosecond timestamps. Stop signals the worker, flushes downstream, decommits the allocator to unblock GetBuffer, joins the worker and ends flush. Restart recommits the allocator and starts a new timestamp segment. Pause gates delivery without a busy loop. Pin references retain their owning filter; factory/filter/enumerator objects and server locks participate in `DllCanUnloadNow` accounting.

IAMStreamConfig supplies one fixed supported RGB32 640x480 format and VIDEO_STREAM_CONFIG_CAPS. SetFormat rejects a connected pin. IKsPropertySet exposes AMPROPSETID_Pin/AMPROPERTY_PIN_CATEGORY = PIN_CATEGORY_CAPTURE. This is a discovery interface, not proof of Discord enumeration.

## Bounded IPC

`shared_frame.h` defines a compile-time-checked 32-byte header: magic, version, width, height, stride, byte count (six uint32 fields), then uint64 sequence. Pixel bytes follow at offset 32. Reader opens `Local\BandHeartBeatBGRA` and `Local\BandHeartBeatFrameMutex`, maps exactly header plus one 640x480x4 frame, takes the mutex without waiting, validates magic/version/dimensions/stride/size, and copies at most that frame. Missing mapping/mutex, invalid header or unavailable publication falls back to the placeholder. An abandoned mutex is released and the frame rejected. The existing ready-event name remains reserved; delivery is paced by the filter. Rust has not yet been wired as a writer.

## Exact validation

```bat
cmd.exe /d /s /c directshow-camera\build-msvc.cmd
cd directshow-camera\out
filter-probe.exe
```

Installed VS2022/SDK build succeeds without warnings. Probe exit code 0:

```text
PASS class factory, IBaseFilter, EnumPins/FindPin identity, output IPin, RGB32 640x480 at 5fps
PASS IAMStreamConfig and IKsPropertySet capture category
PASS valid shared-memory frame and invalid-version placeholder fallback
PASS real allocator/Receive: 6+ samples, bytes, timestamp progression, Stop/restart/Disconnect
PASS Run/Pause/Stop and COM release; no registration/camera access
```

The sink asserts multiple actual Receive calls, negotiated format, sample lengths/bytes, increasing timestamps and frame duration across stop/restart. The probe publishes a valid shared-memory frame and then corrupts its version, verifying copied bytes and placeholder fallback. It checks DLL unload returns S_FALSE while objects live and S_OK after releasing them. This is the explicitly permitted minimal sink test, not an end-to-end registered camera or third-party graph test.

Artifacts are generated in `directshow-camera/out`: BandHeartBeatDirectShow.dll and filter-probe.exe (approximately 151 KiB and 295 KiB respectively; rebuild determines exact size). Outputs are ignored; the previously tracked generated DLL is removed only from the Git index and its on-disk binary remains available. Explicit source/report paths are committed, leaving unrelated staged documentation untouched.

## Limits

No CurrentUser registration, Windows camera enumeration, physical capture, face tracking, Rust IPC producer, privacy/HKLM change or Discord interaction was performed. Registration exports remain disabled. Host application compatibility, high-contention graph shutdown and performance require later validation; this probe verifies the isolated allocator/Receive contract. Existing Rust/OBS/MF artifacts remain intact. All implementation is original SDK-interface code; no upstream UnityCapture code was copied.

## Summary

The DirectShow output pin now genuinely negotiates an allocator and delivers paced samples to a real IMemInputPin sink. Build and multi-frame, restart, IPC-validation, discovery-interface and unload tests pass; registered-camera and standalone webcam/head-tracking integration remain future milestones.
