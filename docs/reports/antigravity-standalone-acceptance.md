# Standalone archive acceptance

This report independently checked the frozen archive without rebuilding or changing its source.

## Archive and package integrity

- `dist/BandHeartBeat-standalone.zip`: **891,941 bytes**.
- SHA256: **442FAFA761DAB53C4F95DC44FF7D3E347032332106E326B19C6B1F3D90023EEA** (matches the requested frozen hash).
- Isolated extraction: `target/qa-standalone-acceptance-20261107`.
- Archive contains the expected `BandHeartBeat.exe`, DirectShow DLL, producer/admin helpers, Russian README and install/rollback scripts. No public face fixture is included.
- `dumpbin /dependents` shows the producer uses Windows Media Foundation/WinHTTP/WinRT/GDI system libraries; no MSVC CRT or third-party model DLL dependency was found. The C++ package is statically linked.

## Checks

- **PASS** `camera-producer.exe --self-test`: synthetic largest-face selection, clamp/stale fallback and compositor test.
- **PASS** `camera-producer.exe --face-check`: FaceDetector `IsSupported=1`, Gray8 supported.
- **PASS** Existing ignored public fixture `target/face-fixture/lena.jpg`: package producer reported `PASS public face fixture: 1 bbox; anchor 450,199; same Gray8 worker/compositor`.
- **PASS** `camera-producer.exe --list-machine`: friendly names were `USB2.0 Camera` and `Iriun Webcam`; the package selector path is name-based in native tray code and excludes only the product's own virtual source.
- **PASS** Native producer/DirectShow in-process checks from the frozen integration work: allocator/Receive samples, IAMStreamConfig, capture category, valid/invalid IPC fallback and unload/refcount checks. Current isolated `filter-probe.exe` was run successfully as part of package validation.
- **PASS** Frozen package archive hash and extraction contents.
- **NOT RUN / blocked** Package `BandHeartBeat.exe --demo` startup/telemetry: the executable crashed immediately with exit `0xC000041D`/`0xC0000005` in `user32.dll` on this QA session; `/telemetry` never became reachable. The same crash reproduced with `target/release/band-heart-beat.exe` and debug `cargo run -- --demo`, so this is an environment/source startup defect requiring a separate fix. No camera-producer child was started by the crashing app.
- **NOT RUN** GUI visual acceptance, named tray interaction, live webcam-face tracking quality, Discord call, physical camera end-to-end. No GUI/browser automation or Discord operation was performed.
- **CLEANUP PASS** No own registration was left installed; HKCU CLSID/category queries were absent. No unknown process was stopped. No privacy/HKLM settings changed.

The source/report review finds the major acceptance blocker above: desktop startup is currently not runnable in this session. The frozen integration report’s synthetic and transport evidence remains useful, but it does not establish a working user-facing desktop application until the `user32.dll` crash is fixed and the package is rebuilt.

## Summary

Archive identity, extraction, native helper dependencies, FaceDetector/fixture path, and DirectShow transport checks passed. Acceptance **failed for startup**: the frozen desktop executable crashes in `user32.dll` before telemetry/UI becomes available; live webcam-face and Discord compatibility remain unverified.
