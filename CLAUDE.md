> Current desktop V3 (2026-11-07, author validation): native Win32 preview/drag/settings plus four label-free GDI+ styles, optional «Без фона», V2 control IPC,2000ms last-detection hold then smooth bottom-right recovery. Final archive `dist/BandHeartBeat-desktop-v3.zip`,1,026,338 bytes, SHA256 `74A08A3B61001A06CF78158B1491E18371BE52A776916A820355CD8E7A6DF831`. Tracking,48background+20style renders, published-pixel/no-background probe and extracted mock producer passed. Final backend/UI lifecycle repeat not run: automatic approval review blocked compound process-control tests; earlier V3 checkpoint lifecycle passed. No live-face/Discord/Band10 compatibility or measured resource claim. See `docs/reports/codex-desktop-v3-integration.md`; current preview artifacts in `docs/reports/screenshots/desktop-v3-background-modes.png` and `desktop-v3-overlay-styles.png`. Older sections below describe historical checkpoints, not current UI scope.

> Startup correction (2026-11-07): historical archive 442FAFA7 / 891,941 bytes FAILED startup. Empty-text DrawTextW guard fixed the first-paint crash; replacement archive is 893,424 bytes, SHA256 55ADF1F2108C6A8C31F3F7D72D48937E1382BE7DCC2F69C0578969545C70AD2C. Author verified three packaged demo starts with telemetry72 0 1; this was not independent QA. Native preview/drag-anchor/style/scale UX remains future work and no Discord/live-face acceptance is claimed.

# BandHeartBeat - Project Instructions & Guidelines

## 1. Project Overview & Primary Objective
- **Project Name:** BandHeartBeat
- **Current Phase:** Standalone Startup Correction and Desktop UX Planning
- **Core Target:** A **self-contained Windows companion application** that directly captures the user's physical webcam, performs local head tracking via native Windows APIs, composites live BPM telemetry and an animated coral heart beside the detected head, and exposes its own virtual camera for Discord **without requiring OBS Studio**.
- **Retained Fallback:** The existing OBS-based HTTP/SSE application (commits `91f894b`, `fceb27c`) remains the built, working fallback in `target/release` (GNU binary 3.29 MB) and `target/msvc-verified` (MSVC binary 1.00 MB). Its end-to-end integration into a live Discord video call remains unverified on physical hardware.

## 2. Frozen Standalone Package (Commit `7d4137b`)
- **Distribution Package:**
  - Folder: `dist/standalone`
  - Archive: `dist/BandHeartBeat-standalone.zip` (exact size: `891,941` bytes, SHA256: `442FAFA761DAB53C4F95DC44FF7D3E347032332106E326B19C6B1F3D90023EEA`).
  - Contents: `BandHeartBeat.exe`, `BandHeartBeatDirectShow.dll`, `camera-producer.exe`, `camera-admin.exe`, `install-current-user.cmd`, `rollback.cmd`, and `README-RU.md`.
- **Pipeline Details:**
  - **DirectShow Virtual Camera:** In-process filter (`BandHeartBeatDirectShow.dll`, 152,064 B) advertising nominal 15 fps 640x480 RGB32, delivering paced samples via `CLSID_MemoryAllocator` and `IMemInputPin::Receive`.
  - **Camera Producer & Face Tracking:** Media Foundation capture into 640x480 RGB32; asynchronous WinRT `FaceDetector` worker operating at ~5 Hz on downsampled Gray8 frames.
  - **Capture Reality:** Observed USB webcam capture was 21 frames in 3031 ms (~7–8 unique fps), repeated by DirectShow filter to maintain nominal 15 fps. Real-user camera face tracking quality remains unverified.
  - **Reference Face Test:** Public OpenCV reference fixture (`lena.jpg`) successfully detected 1 face (anchor 450,199); test fixture is excluded from distribution.
  - **Per-User Registration:** `camera-admin.exe` writes only unique HKCU entries (`CLSID_VideoInputDeviceCategory`). Verified end-to-end through category moniker `BindToObject` and 3 actual samples; test registration was cleanly removed.
- **Incomplete Experiment — Media Foundation Virtual Camera:**
  - Windows 11 `MFCreateVirtualCamera` source boundary failed activation with **`Start 0x80004002 E_NOINTERFACE`**; retained strictly as an incomplete diagnostic experiment.

## 3. Strict Evidence & Fact-Reporting Boundaries
- **HUAWEI Band 10 Compatibility:** Formally **UNCONFIRMED / UNVERIFIED**. No official Huawei documentation or physical test confirms native HR Data Broadcast on Band 10. Generic BLE simulators (Nordic nRF Connect) and in-process mock streams test protocol fidelity, but do NOT prove Band 10 hardware compatibility.
- **Real-World Discord Ingestion:** Real video compositing and stability in a live Discord video call remain unverified on physical hardware.
- **Performance Figures:** CPU (< 1.0%), RAM (< 30 MB), latency (< 100 ms), and battery life are **unmeasured engineering design targets**, not benchmarked facts.
- **Git Repository Status:** All development is local on branch `feature/ble-heart-rate-poc`; no remote repository or published PR exists.

## 4. Team Collaboration, Permissions & Safety
- Team coordination is conducted via **Team MCP tools (preferred)** or the AionCore Team CLI (`"$AIONUI_HELPER_BIN" team ...`).
- Teammates must report task progress using `team_task_update` and message the leader via `team_send_message`.
- Teammates must not execute lead-only commands.
- Teammates must check incoming mailbox messages (`team_read_messages`) before and after task execution.
- Documentation-only tasks must not touch source code, build scripts, or commit changes while implementation agents work. Do not commit other agents' working tree changes.
- **Safety Rules:**
  - Per-user (`CurrentUser`) virtual camera testing authorization persists from user approval.
  - Never alter Windows camera privacy settings or HKLM registry.
  - Never initiate autonomous Discord calls or send Discord messages.

## 5. Active Delegations & Next Steps
- **Release Archive QA:** In progress by teammate Antigravity (`01a1ac15-a312...`) under task `01a1ad31-a163-7b71-9e06-ed97f8a1d7ad`.
- **Memory Refresh:** Completed by Antigravity (`01a1ac15-a3be...`) in `docs/reports/antigravity-standalone-status.md`.
