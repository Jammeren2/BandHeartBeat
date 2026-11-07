# Desktop V3 final integration

## Summary
Built native desktop V3 with V2 controls, label-free four-style renderer, background-free toggle and2000ms timed face-loss hold/smooth recovery. Tracker/render/published-pixel tests and extracted bounded mock capture passed. Final backend/UI startup-lifecycle repeat was blocked by automatic approval review; no live tracking or Discord acceptance is claimed.

## Artifacts
- `dist/BandHeartBeat-desktop-v3.zip`:1,026,338 bytes; SHA256 `74A08A3B61001A06CF78158B1491E18371BE52A776916A820355CD8E7A6DF831`.
- Release folder `dist/desktop-v3`; extracted QA `target/desktop-v3-final-qa`.
- Current reviewed generated preview: `docs/reports/screenshots/desktop-v3-background-modes.png`; current styles/scales: `docs/reports/screenshots/desktop-v3-overlay-styles.png`. These are deterministic renderer contact sheets, not live camera screenshots. Visually reviewed no overlap/clipping, foreground-only mode retains the scene with readable glyph shadow; video cards have no BPM/tracking/connection captions.
- ZIP has8files: BandHeartBeat.exe, band-backend.exe, camera-producer.exe, camera-admin.exe, BandHeartBeatDirectShow.dll, README-RU.md, install-current-user.cmd, rollback.cmd. No test helpers or UTF16 UxTest strings in release binaries. V2 controls mapping production `Local\BandHeartBeatControlsV2`; isolated test `Local\BandHeartBeatControlsV2UxTest`. Frame transport ABI remains unchanged.

## Build and integration
Prerequisite commits tracking5e3de29, renderer3d89628/ce23a57, controls8ec6b0a. Built `cmd /c directshow-camera\package-msvc.cmd` and `build-msvc.cmd` using VS2022/SDK, static native CRT, GDI+ auto-link pragma. Locked offline Rust MSVC release built in target/standalone-build. Unrelated src/overlay.html working change preserved and excluded from backend artifact by temporarily building committed overlay bytes and restoring original bytes; no OBS fallback rewrite. Archive generated via Compress-Archive and extracted with Expand-Archive. Old release folders and rollback archives preserved.

Native toggle writes the same CameraControls read by producer/Compose; sanitization limits field0/1. Old settings version1 safely reset. Card visual180×60 centered inside stable180×86 anchor bounds; size/offset/manual clamps remain consistent with output state and preview hit target. Tracking mode1 alone is face-found; Holding2 shows recovery in desktop. Producer supplies actual detector completion timestamp;500ms stale bbox expiry does not extend2000ms grace. GDI+ video has only icon/number/dash and tinyDEMO for synthetic value. Full status stays in native app.

## Passed
- `tracking-probe.exe`:0/1900/2000/2100ms, exact timeout without jump, smooth convergence/reacquisition, cached actual timestamp, lowFPS equivalence, noface/manual/hide/clamp.
- `renderer-visual-check.cmd`:48 background-mode cases (4styles, light/dark, live/demo/disconnected), plus20style/scale cases. Exact unchanged empty pixels, visible foreground, alpha, distinct styles, bounds, hidden/held assertions.
- `controls-probe.exe`:56byte V2 ABI, four unique style render hashes; real isolated producer published distinct output hashes/position/size/fallback. New transparent toggle check samples a panel-only coordinate from actual shared output: foreground mode equals exact original gradient pixel, card mode differs. Clean producer stop.
- `camera-producer-test.exe --self-test`: synthetic bbox/stale/compositor; FaceDetector supported1/Gray8 supported1.
- Extracted final `camera-producer.exe --mock --seconds 3`:39published frames in3031ms,0physical callbacks, clean exit; extracted selftest passed.
- `cargo fmt --check` passed. Earlier unchanged Rust source2tests passed; not rerun unnecessarily.
- Earlier within this task, before background scope update: real DirectShow filter allocator/Receive6+samples, restart, timestamps, invalidIPC fallback and COM unload passed. Earlier V3 checkpoint extracted production-mapping mock progression, backend demo72 0 1 and desktop startup/noauto-capture/quit plus isolated owned-producer shutdown passed. These are checkpoint evidence, not final-archive lifecycle results.

## Not run / limits
Final compound backend/UI lifecycle-repeat scripts were rejected before execution by automatic approval review with only `blocked by policy`; did not bypass or retry equivalent process termination. Safe extracted mock/selftest/manifest checks completed separately. Final passive process inventory showed no related producer/backend/desktop processes left. No physical webcam activation, registration/privacy changes, user process termination or Discord interaction. No final GUI visual/drag/highDPI acceptance; earlier native mock screenshot belongs to old UI and is not presented as V3 preview. Live tracking quality, Discord ingestion and Huawei Band10 HR Broadcast unverified. CPU/GPU/RAM/latency unmeasured. This is author validation, not independent release acceptance.
