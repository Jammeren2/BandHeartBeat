# BandHeartBeat - Project Instructions & Guidelines

## 1. Project Overview & Current Phase
- **Project Name:** BandHeartBeat
- **Current Phase:** Research Phase Completed / Proof of Concept
- **Active Goal:** Build a lightweight, low-CPU Windows companion application in Rust to stream Huawei Band 10 heart-rate telemetry into Discord webcam/video feeds and desktop overlays.
- **Architectural Decisions:** Standard BLE GATT HR Data Broadcast (`0x180D`/`0x2A37`), OBS Virtual Camera MVP, and a minimal Rust stack (`btleplug`, `tokio`, `axum`, `tray-icon`).

## 2. Team & Agent Registry
- **Team Name:** HeartBeat
- **Lead Agent:** Codex CLI (`slot_id: 01a1ac15-9fd7-7ae2-9603-1a70738d6808`)
- **Teammates:**
  - Antigravity (`slot_id: 01a1ac15-a34e-7262-9fd1-acd36212250b`)
  - Codex CLI (`slot_id: 01a1ac15-a1ce-7b12-bfcb-1fab7c73a2c0`)
  - Antigravity (`slot_id: 01a1ac15-a312-7b32-8071-fe3d440c2541`)
  - Antigravity (`slot_id: 01a1ac15-a3be-7881-8487-2faede13c827`)

## 3. Team Collaboration & Governance
- Team coordination is managed exclusively via the AionCore Team CLI (`"$AIONUI_HELPER_BIN" team ...`).
- Teammates must report task progress using `team_task_update` and message the leader via `team_send_message`.
- Teammates must not execute lead-only commands.
- Teammates must check incoming mailbox messages before and after task execution.

## 4. Open Risks & Blockers
- **Blockers:** Awaiting Product Owner selection between video camera overlay mode vs. local desktop HUD mode.
- **Risks:** Band 10 battery drain in continuous broadcast mode, BLE connection drop handling, and webcam exclusivity in video workflows.

## 5. Next Steps
- Teammate Codex CLI implements minimal Rust BLE heart-rate proof of concept (`feature/ble-heart-rate-poc`).
- Await Product Owner decision on video mode vs. local mode to scaffold the Axum WebSocket overlay and tray shell.
