# BandHeartBeat - Project Instructions & Guidelines

## 1. Project Overview & Primary Objective
- **Project Name:** BandHeartBeat
- **Current Phase:** Proof of Concept & OBS Camera Overlay Integration
- **Core User Objective:** Render a real-time heart-rate overlay (pulsing coral heart, large legible BPM counter) **beside the user's head directly inside their Discord webcam/video call feed** on Windows. The native Win32 window is a local control surface, not the primary user goal.
- **Delivery Pipeline (Phase 1 MVP):**
  - **Telemetry:** Bluetooth SIG BLE GATT Heart Rate Service (`0x180D`/`0x2A37`) via `btleplug`.
  - **OBS Overlay:** Embedded loopback `std::net` HTTP/SSE server on `127.0.0.1:8765/overlay` providing transparent HTML5/SVG graphics for OBS Studio Browser Source (zero external web frameworks, no `axum`, no browser runtime in the binary).
  - **Discord Ingestion:** OBS Virtual Camera transmits the composited webcam + overlay feed into Discord video calls.
  - **Desktop Shell:** Compact native Win32 control window and tray icon (`Shell_NotifyIconW`) using the cached `windows` 0.61 crate.

## 2. Completed Commits & Release Artifacts
- **Commit `91f894b`:** *Add compact native Windows heart-rate monitor and tray* (Win32 window, tray menu, atomic retry guard, demo mode).
- **Commit `fceb27c`:** *Add transparent localhost OBS heart-rate overlay* (Embedded loopback `std::net` HTTP/SSE server, transparent SVG pulse animation).
- **Release Executable Artifact:** `target\release\band-heart-beat.exe` (exact size: `3,287,394` bytes) compiled via `rustup run stable-x86_64-pc-windows-gnu cargo build --release`. Statically links MinGW runtime; requires only native Windows system DLLs.

## 3. Toolchain & Environment Realities
- **Visual Studio 2022 Build Tools:** Installation on `D:\VSBuildTools` is completed and verified via `vswhere` (`BuildTools2022 17.14.44`, `isComplete: true`, `isLaunchable: true`, `isRebootRequired: false`).
- **Current Tested Artifact:** The GNU target release binary (`3,287,394` bytes) remains the tested artifact.

## 4. Strict Evidence & Fact-Reporting Boundaries
- **HUAWEI Band 10 Compatibility:** Formally **UNCONFIRMED / UNVERIFIED**. No official Huawei documentation or physical test confirms native HR Data Broadcast on Band 10. Generic BLE simulators (Nordic nRF Connect) and in-process mock streams test protocol fidelity, but do NOT prove Band 10 hardware compatibility.
- **End-to-End OBS/Discord Video Pipeline:** Formally **UNVERIFIED**. Real webcam compositing via OBS Virtual Camera into a live Discord video call has not been validated on physical camera hardware.
- **Performance Figures:** CPU (< 1.0%), RAM (< 30 MB), latency (< 100 ms), and battery life are **unmeasured engineering design targets**, not benchmarked facts.

## 5. Team Collaboration & Governance
- Team coordination is conducted via **Team MCP tools (preferred)** or the AionCore Team CLI (`"$AIONUI_HELPER_BIN" team ...`).
- Teammates must report task progress using `team_task_update` and message the leader via `team_send_message`.
- Teammates must not execute lead-only commands.
- Teammates must check incoming mailbox messages (`team_read_messages`) before and after task execution.
- Documentation-only tasks must not touch source code (`src/`), `README.md`, `Cargo.toml`, or commit changes while implementation agents work. Do not commit other agents' working tree changes.

## 6. Active Delegations & QA Workflow
- **Native UI QA:** Completed by Antigravity (`01a1ac15-a3be...`) in `docs/reports/antigravity-ui-qa.md`.
- **Overlay QA:** Completed by Antigravity (`01a1ac15-a312...`); HTTP/SSE, demo/offline states and route checks passed. See `docs/reports/antigravity-overlay-qa.md`. Browser/OBS/Discord rendering remains unverified.
- **MSVC build:** VS2022 Build Tools at `D:\VSBuildTools` compiled the application offline successfully. Isolated artifact: `target/msvc-verified/x86_64-pc-windows-msvc/release/band-heart-beat.exe` (1,004,032 bytes, not launched). The GNU release remains the runtime-tested artifact.
- **Physical Validation Gate:** Hardware validation with a physical HUAWEI Band 10 remains required before production claims can be made.
