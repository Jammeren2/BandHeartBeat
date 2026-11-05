# Local transparent OBS overlay

Implemented an embedded HTML overlay at `http://127.0.0.1:8765/overlay` with a coral SVG heart, large tabular BPM, transparent background, Russian status and an explicit demo badge. CSS heartbeat duration derives from BPM and illustrates frequency; it does not represent measured individual beat timing. Disconnected telemetry displays a dash and stops animation; losing the SSE connection also clears the value. The browser has a four-second watchdog in case delivery silently stops.

The dependency-free HTTP/SSE server binds IPv4 loopback only. It serves two embedded routes and never reads host files. Request headers are bounded to 8 KiB, socket reads/writes have two-second timeouts, and at most eight client-handler threads exist. The accept loop blocks, SSE handlers sleep one second between snapshots, and disconnected clients release their slot after write failure. The server validates the loopback Host header and supplies no cross-origin read permission. Port binding errors produce a clear native warning rather than silently serving another app's page.

SSE uses the same `UI_DATA` mutex snapshot as native painting, including actual BLE status and the current BPM. There is no independent generated stream. Explicit `--demo` supplies the existing fixed example 72 and sets `demo:true, live:false`; ordinary mode does not generate readings. The native timer continues processing telemetry while hidden. Added a tray action to open the overlay URL and corrected Show to use `SW_RESTORE` after minimization.

README includes Russian steps for webcam plus Browser Source in OBS, manual positioning beside the head, Start Virtual Camera and selecting OBS Virtual Camera in Discord. No Discord calls/messages were made and no face tracking was added.

## Checks actually performed

- `cargo fmt --check`: passed.
- `rustup run stable-x86_64-pc-windows-gnu cargo build --release --offline`: passed.
- `rustup run stable-x86_64-pc-windows-gnu cargo test --offline`: passed, two tests (standard HR parsing and JSON control/quote escaping).
- Launched a separate release EXE copy with `--demo`; Python HTTP client verified `/overlay` returned 200 with embedded SVG and transparent CSS.
- `/events` produced valid JSON with `bpm:72, live:false, demo:true` and the correct Russian DEMO status.
- Restarted that copy without `--demo`; `/events` returned `bpm:0, live:false, demo:false` and a device-not-found status after scanning. No fabricated reading appeared.
- Requested `/../../Cargo.toml`: 404. Requested overlay with hostile Host: 403.
- All my overlay smoke-test processes were stopped after checks. The earlier user/root-launched native process was preserved; its old executable was renamed to `target/release/band-heart-beat-previous.exe` to permit rebuilding while it ran.

Final executable: `D:/py/BandHeartBeat/target/release/band-heart-beat.exe`, 3,287,394 bytes (about 3.14 MiB).

## Remaining verification

Actual OBS Browser Source composition, browser animation rendering, virtual camera and Discord video have not been exercised by this implementation agent. End-to-end hardware telemetry is unverified. Huawei Band 10 compatibility and its standard HR broadcast support remain explicitly unconfirmed. Performance targets are unmeasured. The one-second SSE interval plus native queue processing means this implementation does not claim sub-100 ms overlay latency. Port-warning rendering and the eight-client saturation limit were implemented but not interactively exercised.

## Summary

Delivered and built the embedded transparent OBS overlay backed by the native monitor's real shared telemetry, with truthful demo/disconnected states and bounded localhost SSE. Offline build, tests and HTTP/SSE checks passed. OBS/Discord composition and physical Band 10 compatibility remain unverified.
