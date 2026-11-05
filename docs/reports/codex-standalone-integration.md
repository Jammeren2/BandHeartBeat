# Standalone camera integration

Implemented a user-facing standalone package without an OBS requirement. Existing tested GNU/OBS and MF experiment artifacts were preserved. Source changes are confined to the native DirectShow/capture helpers, Rust tray integration, local telemetry endpoint and package instructions.

## Delivered pipeline

- `camera-producer.exe`: Media Foundation enumeration/SourceReader capture into fixed RGB32 640x480. Only our own BandHeartBeat virtual source is excluded. Friendly-name enumeration on this host lists **USB2.0 Camera** and **Iriun Webcam**; Iriun was enumerated, not capture-tested. A preferred native 640x480 mode is selected when available. The async callback retains only the latest sample; output pacing is independent of capture. IMF2DBuffer scanline/pitch is respected where available; published DIB rows are bottom-up to match the positive-height DirectShow type.
- Windows `FaceDetector` C++/WinRT runs on a separate latest-Gray8 worker (160x120 inputs, submissions limited to approximately 5 Hz). Runtime IsSupported/Gray8 checks pass here. The largest face drives a smoothed/clamped beside-face anchor. Empty or stale results use an explicitly labeled **NO FACE · CORNER** fallback. No OpenCV/ONNX model/runtime was added.
- GDI compositor draws an animated coral vector heart, BPM, LIVE/DISCONNECTED/DEMO and tracking state. Normal mode reads `/telemetry` from the same UI_DATA used by the Rust monitor/OBS SSE; it generates no readings. Mock mode explicitly labels a fixed 72 BPM demo. Heart timing illustrates BPM, not measured individual beats.
- DirectShow output now consistently advertises **640x480 RGB32 at nominal 15 fps** (666,667 in 100ns units), repeating the latest producer frame when the input is slower. Existing bounded header/mutex validation and fallback remain. The absent-producer checkerboard is labeled **PLACEHOLDER - NO CAMERA**.
- Native tray submenu lists camera friendly names. Selecting one starts the producer hidden with its index; no console/numeric prompt is required in the release UI. Explicit mock/start/stop and current-user install/remove actions are present. Producer singleton mutex prevents duplicate capture. Stop/Quit signals the producer event; SourceReader Flush/Shutdown breaks callback ownership before exit. Runtime failures produce a nonzero exit and stderr is surfaced in native status.

## Reversible registration

`camera-admin.exe --register` writes only our unique HKCU CLSID/InprocServer32 and VideoInputDeviceCategory Instance entry. It refuses to overwrite existing entries, preserves unrelated keys and needs no HKLM/admin. Unregister validates its own DLL path before deleting these two entries; absent entries are harmless. Package paths must remain stable until uninstall.

The authorized tests started with both entries absent. Registration returned **S_OK**, DirectShow category enumeration found **BandHeartBeat Virtual Camera**, moniker **BindToObject returned S_OK**, and the real Windows allocator/IMemInputPin sink received samples. Cleanup `--unregister` returned **S_OK** and both CLSID/category queries reported absent. No registration or capture process is left running by this agent.

## Actual tests

1. `cmd.exe /d /s /c directshow-camera\package-msvc.cmd`: clean installed VS2022/SDK build with `/O2 /MT`, plus isolated offline Rust MSVC release. No downloads/installations. `cargo fmt --check` and offline GNU parser/JSON tests: passed (2 tests).
2. `filter-probe.exe`: all class-factory/pin/RGB32, IAMStreamConfig/IKsPropertySet, actual allocator/Receive, multi-frame bytes/timestamps, stop/restart, invalid-header fallback and COM unload checks pass at the new 15 fps media interval.
3. `camera-producer.exe --self-test`: largest-face selection, clamping, smoothing/stale fallback and synthetic compositor pixel-change assertions pass. `--face-check`: IsSupported=1 and Gray8=1.
   Positive detector test: downloaded the public OpenCV sample `https://raw.githubusercontent.com/opencv/opencv/master/samples/data/lena.jpg` only into ignored `target/face-fixture/lena.jpg` (not distributed). `camera-producer.exe --face-fixture <absolute path>` uses Windows BitmapDecoder, the **same DownsampleGray function and FaceWorker** as capture, then the same placement/compositor. Observed: **PASS public face fixture: 1 bbox; anchor 450,199**. This proves the recognition path on that reference image, while actual-user webcam tracking quality remains unverified.
4. Packaged `BandHeartBeat.exe --demo`: `/telemetry` returned **72 0 1**. No capture started automatically (only safe camera enumeration). The owned test app was stopped afterward.
5. Full packaged mock transport: producer IPC sequence 10 contained BGRA **[0,239,35,255]** at the expected bottom-up gradient pixel. `camera-admin.exe --probe-producer` enumerated/bound the registered camera and verified those producer bytes through three actual Receive samples, not the fallback checkerboard. Stop was signaled and registration removed. Mock process logged 13 frames/1000 active ms before stopping; this is a short smoke count, not a benchmark.
6. Bounded real USB capture, no saved images: preferred native 640x480 **30 fps**, negotiated **30/1**, but **21 SourceReader callbacks and 21 published frames in 3031 active ms**, with **16 frames after the first second**. The callback count equals publication count, so this observed upstream capture/driver pipeline was approximately 7–8 unique fps despite the negotiated mode. This is explicitly disclosed in Russian package instructions; no measured 15 fps webcam claim is made. Face tracking state remained **0** during this short run: actual real-face bounding-box quality has **not** been verified.
7. `dumpbin /dependents`: native producer/filter depend on Windows system DLLs; MSVC C++ CRT is statically included. No third-party model DLL is required. All helpers are original SDK-interface code, with only the architectural UnityCapture MIT shared-memory pattern referenced; no upstream source copied.

## Package and commands

Folder: **D:\py\BandHeartBeat\dist\standalone**. Archive: **D:\py\BandHeartBeat\dist\BandHeartBeat-standalone.zip**. Files: BandHeartBeat.exe, BandHeartBeatDirectShow.dll, camera-producer.exe, camera-admin.exe, install-current-user.cmd, rollback.cmd and README-RU.md. Generated outputs are ignored by Git.

Frozen package sizes: app **1,125,376 B**, filter DLL **152,064 B**, admin **287,232 B**, producer **414,208 B**, archive **891,941 B**. The public test fixture is excluded from both folder and archive.

User flow: launch BandHeartBeat.exe; explicitly install current-user camera from tray; choose a friendly camera name; select BandHeartBeat Virtual Camera in the consuming app. Stop and uninstall are explicit tray actions; rollback.cmd is included. CLI diagnostics remain available for testing but are not required for normal camera selection.

## Limits and evidence boundaries

The short USB capture test proves frame acquisition/publication; it does not prove real-face tracking quality, visual orientation in every consumer, or live Discord-call compatibility. No Discord call/message/UI or privacy/HKLM setting was touched. No physical user images were saved or uploaded. Fixed RGB32 640x480 format negotiation may limit consumer compatibility. CPU/memory/latency and sustained throughput are unmeasured. Windows FaceAnalysis may be unavailable on other systems, in which case the labeled no-face corner fallback is used. Huawei Band 10 broadcast compatibility and real BLE telemetry remain unverified. Native menu code is built; final user GUI/Discord acceptance remains separate from this in-process transport validation.

## Summary

Built a standalone tray package with named camera selection, actual MF capture, asynchronous Windows face-analysis/compositing, shared real BPM, and a registered DirectShow source proven through sample delivery. Synthetic/transport/packaging checks pass and test registration is removed. Real-face quality and Discord compatibility remain unverified; the tested USB source supplied roughly 7–8 unique fps despite the nominal 15 fps virtual output.
