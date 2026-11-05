# Project Status - BandHeartBeat

## Current Phase: Research Phase Completed / Proof of Concept
**Date:** November 2026  
**Overall State:** Architectural research successfully completed across wearable telemetry, video overlay pipeline, and runtime stack. Active implementation starting with BLE proof of concept.

## 1. Project Goal & Architectural Decisions
- **Project Goal:** Build a lightweight, low-CPU Windows companion application in Rust that streams real-time heart-rate telemetry from a HUAWEI Band 10 to a Discord webcam/video overlay and/or desktop HUD.
- **Architectural Decisions:**
  1. **Wearable Ingestion (HUAWEI Band 10):**
     - Use native **"HR Data Broadcast"** mode exposing standard Bluetooth SIG Heart Rate Service (`0x180D`) and Measurement Characteristic (`0x2A37`).
     - Avoid proprietary Huawei LPv2/HiChain3 protocols and Huawei Health Cloud APIs (which require enterprise vetting and do not support real-time streaming).
     - Standard GATT notifications deliver 1 Hz telemetry with < 100 ms latency and zero external credentials.
  2. **Discord Overlay Video Pipeline:**
     - **Phase 1 MVP:** Decoupled architecture using OBS Studio's built-in Browser Source and Virtual Camera. BandHeartBeat serves a local HTML5/WebSocket overlay (`http://127.0.0.1:8765/overlay`), allowing zero-driver delivery, rich CSS pulse animations, and complete stability across Windows 10 & 11.
     - **Phase 2 (Standalone):** DirectShow shared-memory virtual camera filter (`UnityCapture` / `obs-virtualcam` design) to eliminate the OBS dependency for non-streamers (< 30 MB RAM, < 1% CPU).
     - **Positioning:** Static anchor presets (Top-Right, Top-Left) for MVP (0% compute overhead); optional DirectML YuNet face tracking as a future enhancement.
     - **Platform Limitations Noted:** Discord lacks native camera overlay APIs and client mods violate TOS; transparent desktop windows do not render into Discord webcam feeds (only visible in full desktop screenshare).
  3. **Optimized Rust Technology Stack:**
     - **Runtime & Concurrency:** `tokio` (single-thread / current-thread runtime) with `tokio::sync::watch` for telemetry state.
     - **BLE Integration:** `btleplug` (v0.13) leveraging Windows Runtime (`Windows.Devices.Bluetooth`).
     - **Telemetry & Web Server:** `axum` (v0.8) embedded HTTP/WebSocket server serving embedded HTML/CSS assets (`rust-embed`).
     - **Desktop Shell:** `tray-icon` (v0.26) + native Win32 message pump for system tray controls, registry auto-start, and notification area status.
     - **Footprint Targets:** < 0.1% idle CPU, < 0.5% active streaming CPU, < 25 MB RAM, single self-contained Windows executable (< 8 MB).

## 2. Agent Registry
| Role | Agent Name | Slot ID | Current Status |
| --- | --- | --- | --- |
| Lead | Codex CLI | `01a1ac15-9fd7-7ae2-9603-1a70738d6808` | Active |
| Teammate | Codex CLI | `01a1ac15-a1ce-7b12-bfcb-1fab7c73a2c0` | Active (Implementing BLE POC) |
| Teammate | Antigravity | `01a1ac15-a3be-7881-8487-2faede13c827` | Active (Recording Status & Governance) |
| Teammate | Antigravity | `01a1ac15-a34e-7262-9fd1-acd36212250b` | Idle (Completed Rust Stack Research) |
| Teammate | Antigravity | `01a1ac15-a312-7b32-8071-fe3d440c2541` | Idle (Completed Huawei Band 10 Research) |

## 3. Task Board
| Task ID | Subject | Owner | Status |
| --- | --- | --- | --- |
| `01a1ac17-edce-7520-9da4-fd2d7d3765c9` | Restore project context | Antigravity (`01a1ac15-a312...`) | Completed |
| `01a1ac1d-9572-7af2-b40f-9263f074401c` | Bootstrap project memory files | Antigravity (`01a1ac15-a34e...`) | Completed |
| `01a1ac3f-a064-7c12-95d4-e47f6cf8f184` | Research Huawei Band 10 data access | Antigravity (`01a1ac15-a312...`) | Completed |
| `01a1ac3f-a25a-7800-93a2-96d197ca960a` | Research Discord camera overlay options | Antigravity (`01a1ac15-a3be...`) | Completed |
| `01a1ac3f-a458-7b71-a6fc-8f2d49f4bfc8` | Research optimized Rust stack | Antigravity (`01a1ac15-a34e...`) | Completed |
| `01a1ac52-a504-7241-b325-6697f41c8e4c` | Record research decisions in STATUS | Antigravity (`01a1ac15-a3be...`) | In Progress |
| `01a1ac54-f481-79b2-bdfc-cda08f0dfa42` | Implement Rust BLE heart-rate proof of concept | Codex CLI (`01a1ac15-a1ce...`) | In Progress |

## 4. Open Risks & Mitigations
1. **Band Battery Consumption:** Continuous optical PPG sensor operation and BLE broadcast reduces Huawei Band 10 battery life to ~12–24 hours. *Mitigation:* Document user requirement to enable broadcast only during sessions; app sends low battery notifications.
2. **Bluetooth Connection Drops & Reconnection:** Windows BLE connections can drop or timeout if signal is weak or adapter sleeps. *Mitigation:* Implement exponential backoff auto-reconnect logic in `btleplug` worker loop with tray status indication.
3. **Hardware Webcam Exclusivity (Phase 2):** Physical webcams on Windows generally cannot be opened by two processes simultaneously. *Mitigation:* For Phase 1 OBS MVP, OBS manages webcam ingestion; for Phase 2, provide clear in-app setup instructions designating the Virtual Camera in Discord.
4. **DirectShow COM Registration (Phase 2):** Installing custom DirectShow filters usually requires `regsvr32` admin elevation. *Mitigation:* Phase 1 uses zero-driver OBS Virtual Camera; Phase 2 can use per-user COM registration or standard installer.

## 5. Next Steps
1. **Product Owner Decision:** PO reviews and chooses between full video camera mode (OBS Virtual Camera / DirectShow) versus local-only desktop HUD mode.
2. **BLE Proof of Concept:** Codex CLI implements `feature/ble-heart-rate-poc` in Rust using `btleplug` to scan, connect, and stream GATT `0x180D`/`0x2A37` notifications with clear console diagnostics.
3. **Integration Scaffolding:** Upon PO confirmation, integrate the BLE ingestion stream with Axum WebSocket server and embed the HTML/CSS/JS overlay.
