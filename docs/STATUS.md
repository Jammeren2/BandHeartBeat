> Current desktop V3 (2026-11-07, author validation): native Win32 preview/drag/settings plus four label-free GDI+ styles, optional «Без фона», V2 control IPC,2000ms last-detection hold then smooth bottom-right recovery. Final archive `dist/BandHeartBeat-desktop-v3.zip`,1,026,338 bytes, SHA256 `74A08A3B61001A06CF78158B1491E18371BE52A776916A820355CD8E7A6DF831`. Tracking,48background+20style renders, published-pixel/no-background probe and extracted mock producer passed. Final backend/UI lifecycle repeat not run: automatic approval review blocked compound process-control tests; earlier V3 checkpoint lifecycle passed. No live-face/Discord/Band10 compatibility or measured resource claim. See `docs/reports/codex-desktop-v3-integration.md`; current preview artifacts in `docs/reports/screenshots/desktop-v3-background-modes.png` and `desktop-v3-overlay-styles.png`. Older sections below describe historical checkpoints, not current UI scope.

> Startup correction (2026-11-07): historical archive 442FAFA7 / 891,941 bytes FAILED startup. Empty-text DrawTextW guard fixed the first-paint crash; replacement archive is 893,424 bytes, SHA256 55ADF1F2108C6A8C31F3F7D72D48937E1382BE7DCC2F69C0578969545C70AD2C. Author verified three packaged demo starts with telemetry72 0 1; this was not independent QA. Native preview/drag-anchor/style/scale UX remains future work and no Discord/live-face acceptance is claimed.

# Project Status - BandHeartBeat

## Current Phase: Standalone Startup Correction and Desktop UX Planning
**Date:** November 2026  
**Overall State:** Completed frozen standalone package delivery in commit `7d4137b` without requiring OBS Studio. Packaged archive `dist/BandHeartBeat-standalone.zip` (891,941 bytes, SHA256 `442FAFA7...`) contains the integrated companion app, DirectShow filter DLL, camera producer with native Windows FaceAnalysis, and current-user registration scripts. DirectShow mock transport was verified end-to-end through category moniker and BindToObject; test registry keys were cleaned up. Author acceptance found and fixed a startup failure; three replacement-package demo starts passed. Independent QA is not claimed. The original OBS-based application remains preserved as a working fallback.

---

## 1. Project Goal & Active Architecture

- **Primary Goal:** A **self-contained Windows companion application** that captures physical webcam frames directly, performs local head tracking via native Windows APIs, composites live BPM telemetry and an animated coral vector heart beside the detected head, and exposes its own DirectShow virtual camera for Discord **without requiring OBS Studio**.
- **Delivered Pipeline (Commit `7d4137b`):**
  1. **DirectShow Virtual Camera Filter (`BandHeartBeatDirectShow.dll`, 152,064 B):**
     - Exposes `IBaseFilter`, `IPin`, `IEnumPins`, `IEnumMediaTypes`, `IAMStreamConfig`, and `IKsPropertySet` (`PIN_CATEGORY_CAPTURE`).
     - Advertises fixed RGB32 640x480 at a nominal 15 fps (666,667 100ns units).
     - Paced worker thread uses Windows `CLSID_MemoryAllocator`, delivers samples via `IMemInputPin::Receive`, and reads bounded 32-byte shared-memory IPC (`Local\BandHeartBeatBGRA`).
  2. **Camera Producer & Face Tracking (`camera-producer.exe`, 414,208 B):**
     - Captures physical webcam video using Media Foundation `IMFSourceReader` into 640x480 RGB32 frames.
     - Local head tracking utilizes asynchronous Windows `FaceDetector` (C++/WinRT) operating at ~5 Hz on downsampled Gray8 frames. Tested and verified on public OpenCV face fixture (`PASS 1 bbox; anchor 450,199`; fixture not shipped).
     - Observed hardware USB capture rate during smoke testing was 21 frames in 3031 ms (~7–8 unique fps), repeated by DirectShow filter to sustain nominal 15 fps. Real-user camera face tracking quality remains unverified.
     - GDI compositor draws animated coral vector heart, BPM, tracking state, and status. Labeled fallback (`NO FACE · CORNER`) is used when no face is detected.
  3. **Reversible Per-User Registration (`camera-admin.exe`, 287,232 B):**
     - Writes only unique HKCU CLSID and `CLSID_VideoInputDeviceCategory` entries; requires zero administrative privileges or HKLM writes.
     - Verified end-to-end via moniker enumeration, `BindToObject`, and 3 actual `IMemInputPin::Receive` samples. Test registration was cleanly removed.
  4. **Desktop Shell & Tray Integration (`BandHeartBeat.exe`, 1,125,376 B):**
     - Windows GUI application with native friendly-name camera selection submenu (`USB2.0 Camera`, `Iriun Webcam`).
     - Launches hidden producer singleton; provides explicit Start/Stop and Install/Remove CurrentUser registration tray controls.
  5. **Preserved Retained Fallback (OBS Pipeline):**
     - The earlier OBS Browser Source + Virtual Camera application (commits `91f894b`, `fceb27c`) remains fully built and functional in `target/release` (GNU binary 3.29 MB) and `target/msvc-verified` (MSVC binary 1.00 MB).
  6. **Incomplete Diagnostic Experiment (Media Foundation Virtual Camera):**
     - Retained strictly as an incomplete diagnostic experiment that failed activation with `E_NOINTERFACE`; not on the product critical path.

---

## 2. Frozen Standalone Release Package (`dist/`)

- **Package Directory:** `D:\py\BandHeartBeat\dist\standalone`
- **Distribution Archive:** `D:\py\BandHeartBeat\dist\BandHeartBeat-standalone.zip`
  - **Archive Size:** `891,941` bytes
  - **SHA-256 Hash:** `442FAFA761DAB53C4F95DC44FF7D3E347032332106E326B19C6B1F3D90023EEA`
- **Archive Contents:**
  1. `BandHeartBeat.exe` (1,125,376 B) — Standalone desktop tray application.
  2. `BandHeartBeatDirectShow.dll` (152,064 B) — In-process DirectShow virtual camera filter.
  3. `camera-producer.exe` (414,208 B) — Webcam grabber, WinRT FaceDetector, and compositor.
  4. `camera-admin.exe` (287,232 B) — Current-user registration and diagnostic probe utility.
  5. `install-current-user.cmd` — Script for registering DirectShow filter in HKCU.
  6. `rollback.cmd` — Script for completely unregistering and removing HKCU entries.
  7. `README-RU.md` — Russian end-user setup instructions and operational boundaries.

---

## 3. Explicit Evidence Boundaries & Reality Check

1. **HUAWEI Band 10 Compatibility: UNVERIFIED / UNCONFIRMED.**  
   - Implements standard **Bluetooth SIG Heart Rate Service (`0x180D`/`0x2A37`)**.
   - Generic BLE tests, mock streams (`--demo`), and smartphone BLE simulators (Nordic nRF Connect) confirm protocol conformity, but **do not prove Band 10 hardware compatibility**. Official confirmation of HR Data Broadcast on Band 10 hardware remains absent.
2. **Head Tracking & Hardware Capture Reality:**  
   - Public reference face fixture (`lena.jpg`) successfully detected 1 face (anchor 450,199).
   - Real-world USB webcam smoke test captured 21 frames over 3031 ms (~7–8 unique fps); **zero real human faces were observed**. Real-user camera tracking quality remains unverified.
3. **Live Discord Video Ingestion: UNVERIFIED.**  
   - In-process moniker binding and sample delivery verified in isolated probes. Real-world video compositing and stability in an active Discord video call remain unverified on physical hardware.
4. **Performance Figures: UNMEASURED DESIGN TARGETS.**  
   - CPU (< 1.0%), RAM (< 30 MB), latency (< 100 ms), and battery life are **unmeasured engineering design targets**, not benchmarked facts.
5. **Git Repository Status:**  
   - Development is entirely local on branch `feature/ble-heart-rate-poc`; no remote repository or published PR exists.

---

## 4. Agent Registry & Delegations

| Role | Agent Name | Slot ID | Current Assignment / Status |
| :--- | :--- | :--- | :--- |
| Lead | Codex CLI | `01a1ac15-9fd7-7ae2-9603-1a70738d6808` | Active Team Lead |
| Teammate | Codex CLI | `01a1ac15-a1ce-7b12-bfcb-1fab7c73a2c0` | Completed commit `7d4137b` (Standalone integration) |
| Teammate | Antigravity | `01a1ac15-a3be-7881-8487-2faede13c827` | Active (`01a1ad0f...`: Standalone memory refresh) |
| Teammate | Antigravity | `01a1ac15-a312-7b32-8071-fe3d440c2541` | Active (`01a1ad31...`: Release archive independent QA) |
| Teammate | Antigravity | `01a1ac15-a34e-7262-9fd1-acd36212250b` | Research Teammate (Idle) |

---

## 5. Recent Task Board Milestones

| Task ID | Subject | Owner | Status | Outcome / Reality |
| :--- | :--- | :--- | :--- | :--- |
| `01a1ad0d...` | DirectShow filter & output pin | Codex CLI (`01a1ac15-a1ce...`) | Completed | Commit `9b0daf0`: IBaseFilter & RGB32 IPin. |
| `01a1ad11-2715...`| DirectShow real frame delivery | Codex CLI (`01a1ac15-a1ce...`) | Completed | Commit `a309e8f`: Real allocator & paced delivery. |
| `01a1ad17-24b7...`| QA DirectShow real frame delivery | Antigravity (`01a1ac15-a312...`) | Completed | Report `antigravity-directshow-delivery-qa.md`: PASSED. |
| `01a1ad17-22ca...`| Integrate webcam, tracking & camera | Codex CLI (`01a1ac15-a1ce...`) | Completed | Commit `7d4137b`: Built standalone tray package & archive. |
| `01a1ad0f...` | Refresh standalone project memory | Antigravity (`01a1ac15-a3be...`) | In Progress | Final project memory update across STATUS and CLAUDE. |
| `01a1ad31-a163...`| Accept frozen standalone release archive | Antigravity (`01a1ac15-a312...`) | In Progress | Independent release archive QA actively underway. |

---

## 6. Next Steps

1. **Independent Release Archive QA:** Teammate Antigravity (`01a1ac15-a312...`) completes independent evaluation of `dist/BandHeartBeat-standalone.zip` under task `01a1ad31...`.
2. **Preserve Tested Fallback:** Ensure original OBS-compatible binaries remain available and untouched in `target/release`.
3. **Physical Hardware Validation Gate:** Real-device validation with physical HUAWEI Band 10 and live Discord call acceptance remain required before production release claims.
