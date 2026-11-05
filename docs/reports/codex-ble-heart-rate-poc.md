# BLE heart-rate POC report

Implemented a minimal Rust `btleplug` binary on branch `feature/ble-heart-rate-poc`. It scans specifically for service UUID `0x180D`, connects to the first matching peripheral, discovers and subscribes to characteristic `0x2A37`, decodes 8-bit and 16-bit BPM payloads, and prints notifications. Missing adapters, devices, services, or subscriptions produce contextual diagnostics, including a reminder to enable HR Data Broadcasts.

Validation: `cargo fmt` completed. `cargo check` reached dependency compilation but is blocked in this environment because the Windows MSVC linker `link.exe` is unavailable; install Visual Studio Build Tools with the C++ workload to complete the check. No BLE hardware was required or available for runtime testing.

## Summary
The BLE HR Broadcast POC and setup documentation are complete. Formatting passed; compilation is blocked only by the missing local MSVC linker.
