# BandHeartBeat — project notes

Rewrite on branch `rewrite/tauri` (2026-11-07). The previous Win32/OBS/DirectShow
implementation (desktop v2–v11) lives only in git history on
`feature/ble-heart-rate-poc`; nothing from it is built or shipped any more.

## Layout
- `crates/core` — all logic, Rust. One process: BLE → `heart`, camera → `pipeline`
  → `overlay` → `frame` (shared memory `Local\BandHeartBeat.Frame.v2`).
- `app` — Tauri 2 shell + Svelte 5 UI (design: A+B mockup — left nav, framed 16:9
  preview, glass side panel). UI polls `snapshot`; preview JPEGs come over a Channel.
- `vcam` — DirectShow source filter DLL, CLSID `{8A3E5B1C-2F47-4D9B-A6C1-5E0B7D9F3A21}`,
  1280×720 RGB32/RGB24, 5–30 fps. Shared header format must match `frame.rs`.

## Rules
- Output is always 16:9 1280×720; cameras are fitted (contain by default), never cropped silently.
- Settings persist automatically to `%APPDATA%\BandHeartBeat\settings.json`.
- Keep code readable: normal formatting, no one-line files, tests next to logic.
- Verify with `cargo test -p bhb-core`, `vcam\test.cmd`, `npm run check` in `app`.

## Evidence boundaries
- HUAWEI Band 10 is the only device tested (user confirmed live BPM in Discord, 2026-11-07).
  Everything else in README's device list is "presumed", not tested.
- Discord delivery confirmed by the user on 2026-11-07; tracking quality is user-judged.
- CPU/RAM/latency numbers are unmeasured unless a report says otherwise.

## Safety
- Virtual camera registration is per-user HKCU only (approved). Never touch HKLM
  or Windows camera privacy settings.
- Never start Discord calls or send Discord messages.
