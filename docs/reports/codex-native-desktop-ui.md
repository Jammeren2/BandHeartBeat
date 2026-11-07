# Native desktop preview and controls

## Summary
Implemented a native Win32/GDI desktop interface with real 4:3 producer preview, camera selection, draggable badge, four rendered styles, size/BPM scale/offset/smoothing, face attachment and corner/hide fallback. Isolated producer pixel/control tests, native build, Rust tests and child shutdown passed. Live face tracking, manual GUI drag acceptance and Discord ingestion remain unverified.

## Implementation
`desktop.cpp` is a Windows-subsystem EXE, no console/browser runtime. Charcoal rounded panels and cyan accents follow the approved reference; identity remains BandHeartBeat. Actual format is fixed RGB32 640×480 nominal15fps, without illustrative 1280×720/82BPM claims. Preview reads the same bottom-up frame mapping used by the DirectShow filter, displayed at704×528; drag converts coordinates by640/704 and480/528. Source-owned controls/state are bounded/versioned, synchronized by named mutex. Producer validates/sanitizes controls and renders four styles into output pixels. Settings persist next to EXE.

No automatic production capture: select source then Start. Camera selection changes the next start. Stop clears preview; a frame older than2s is not displayed as current. Hide/minimize retains capture but stops preview paint/read; tray Exit stops its producer and owned backend. Enumeration/registration jobs are joined before exit. Backend bind failure blocks Start and reports occupied port. Processes are launched without console; only owned child handles are terminated if bounded shutdown expires. A single virtual output is supported; no fake selector options or tabs.

## Genuine checks
- `cmd /c directshow-camera\package-msvc.cmd`: VS2022/SDK x64 native DLL/producer/admin/desktop plus locked offline Rust MSVC release, success.
- `directshow-camera/out/controls-probe.exe`: PASS four distinct offline style hashes, bounded placement/offset/clamp,52-byte controls ABI; isolated real producer changes published frame hashes/position/size/fallback and exits cleanly. BH_ISOLATED_TEST maps/events/window class differ from production; test binaries excluded from package.
- `camera-producer-test.exe --self-test`: FaceDetector supported1, Gray8 supported1; synthetic bbox/stale fallback/compositor PASS.
- `cargo fmt --check`; `cargo test --offline --locked --target x86_64-pc-windows-msvc --target-dir target/standalone-build`:2 tests passed.
- Isolated desktop --mock-preview started; --quit exited within6s, no camera-producer-test process remained.
- Owned TCP listener occupied127.0.0.1:8765; newly launched backend exited1 within5s. Earlier attempt without listener found port free; owned backend was stopped, not counted as a conflict failure.
- Actual mock GUI inspected and screenshot captured at `docs/reports/screenshots/native-preview-mock.jpg`. Later recapture failed twice with computer-use 'foreground window did not report a process id'; no further input attempted. Manual drag/slider visual acceptance not claimed. Earlier user input detection prevented touching the user's app.

## Package
`dist/desktop-v2` contains BandHeartBeat.exe, band-backend.exe, camera-producer.exe, camera-admin.exe, BandHeartBeatDirectShow.dll, registration/rollback scripts and updated Russian README. Static native CRT; Windows provides MF/FaceAnalysis; no OpenCV/ONNX models. Package uses committed OBS overlay content; unrelated working-tree src/overlay.html modification was preserved and excluded from release build/commit.
Archive `dist/BandHeartBeat-desktop-v2.zip`:1,027,033bytes; SHA256 `0A4FFAFD15AB2F7D3B8933E5AD2DD416C6FE8E0F31183A120FDD2C10DD0591CF`.
Previous rollback retained at `dist/BandHeartBeat-startup-fixed-rollback.zip`, SHA256 `55ADF1F2108C6A8C31F3F7D72D48937E1382BE7DCC2F69C0578969545C70AD2C`.
Existing dist/standalone user app/processes/registration were not stopped, overwritten or controlled. Exit old version before starting new version; registration migration requires old rollback followed by new install. No physical camera, registry/privacy modification or Discord operation in final isolated checks. This task preserves existing transport; no new registered-filter regression performed against user-owned mapping.

## Limits
No measured CPU/GPU/RAM benchmark; native GDI and reduced-rate FaceDetector avoid browser/model runtime but do not prove a resource target. Live tracking quality, actual Discord compatibility and Huawei Band10 HR Broadcast are unconfirmed. Fixed-size desktop layout lacks keyboard/accessibility navigation and high-DPI layout acceptance. The four designs are functional native renderings, not exact bitmap copies of the reference.
