# BandHeartBeat

Lightweight native Windows heart-rate monitor. Use a device broadcasting the standard Bluetooth SIG Heart Rate Service (0x180D), keep Windows Bluetooth on, then open `target/release/band-heart-beat.exe`.

The compact dark window shows real BLE heart-rate measurements. Closing or minimizing hides it in the notification area. Left-click its tray icon to show it; right-click for **Показать**, **Повторить подключение**, or **Выйти**. Retry starts a fresh scan after a failure; an active scan/connection is never duplicated. No readings for 20 seconds ends the connection and clears the value.

For an explicitly labeled UI preview without hardware: `band-heart-beat.exe --demo`. Demo shows a fixed example of 72 BPM and never claims a live connection. Normal mode never generates readings.

If no device is found, enable standard heart-rate broadcasting on a supported device and check that another host is not connected. **Huawei Band 10 compatibility and the availability of a broadcast setting are unconfirmed.**

## Build

Use Rust and Visual Studio C++ Build Tools: `cargo build --release`.
With the GNU Rust toolchain and MinGW installed: `rustup run stable-x86_64-pc-windows-gnu cargo build --release --offline`.
Both write `target/release/band-heart-beat.exe`. Run `cargo fmt --check` and `rustup run stable-x86_64-pc-windows-gnu cargo test --offline` for local checks.

No Huawei hardware was available for validation. This version provides the native monitor and tray; OBS/video output is not implemented.
