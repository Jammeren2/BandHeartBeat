# Native Windows monitor and tray

Implemented a compact Win32/GDI window with charcoal background, coral heart, large Segoe UI BPM, Russian status/units, and wrapped diagnostics. Windows GUI subsystem prevents an unwanted console. Close and minimize hide to the notification area; left-click shows the window and the context menu exposes Show, Retry and Quit.

The BLE worker uses a current-thread Tokio runtime. An atomic guard prevents concurrent connection attempts. Retry starts a new scan after the previous attempt finishes; errors clear the displayed BPM. Live status begins only on a decoded notification. A 20-second notification timeout disconnects stale sessions. Scanning is stopped before connecting. Explicit --demo uses a fixed 72 BPM and a visible DEMO label without opening Bluetooth.

Painting occurs only after telemetry changes and only while visible. The message pump blocks normally; a 500 ms timer drains queued updates, without animation or busy polling. Font/brush resources are released after painting.

## Validation

- cargo fmt --check: passed.
- rustup run stable-x86_64-pc-windows-gnu cargo build --release --offline: passed.
- rustup run stable-x86_64-pc-windows-gnu cargo test --offline: passed, one parser test covers 8-bit, 16-bit and truncated measurements.
- Computer Use screenshot of the running release --demo: verified correct Russian text, coral heart, large 72, separated unit and DEMO status. Initial mojibake was found and repaired before delivery.
- Leader independently confirmed the corrected layout screenshot. Final tray interaction/no-device smoke checks are handed to the leader; I stopped test processes to avoid duplicate windows.
- Artifact: D:/py/BandHeartBeat/target/release/band-heart-beat.exe, 3,272,579 bytes (about 3.12 MiB).

## Limits

Huawei Band 10 support and the presence of a standard HR broadcast setting remain unconfirmed; no physical heart-rate device was available. No claim of real-device validation. This is the monitor/tray release, without OBS/video integration, custom tray artwork, settings or automatic reconnection. Retry is deliberately ignored while the single worker is active. Standard DPI virtualization is used; no system DPI settings changed. Errors from the BLE backend may include English diagnostic text.

## Summary

Delivered and built the lightweight native Windows UI/tray executable with explicit demo, single-worker retry and visible error states. Offline build and parser tests passed; corrected layout was visually inspected. Hardware compatibility remains unverified.
