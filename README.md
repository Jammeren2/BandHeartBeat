# BandHeartBeat BLE heart-rate POC

This Windows-focused proof of concept scans for the Bluetooth SIG Heart Rate Service (`0x180D`), connects to a matching peripheral, subscribes to Heart Rate Measurement (`0x2A37`), and prints BPM notifications.

## Run

1. On the Band 10, open **Settings > HR Data Broadcasts** and enable broadcasting. Keep Bluetooth enabled on Windows.
2. Install Rust and the Visual Studio C++ Build Tools (MSVC linker).
3. Run `cargo run` and wait for the scan. Stop with Ctrl+C.

If no device is found, confirm HR Data Broadcasts is enabled and that another host is not connected. The mode may disconnect the Huawei Health phone companion while active.
## Build

From a Windows terminal with the Visual Studio C++ Build Tools installed, run `cargo build --release`. The executable is written to `target\\release\\band-heart-beat.exe`.
