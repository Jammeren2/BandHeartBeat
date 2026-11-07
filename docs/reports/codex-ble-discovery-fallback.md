# BLE discovery fallback

Rust BLE discovery now scans without a service-only filter and accepts candidates advertising 0x180D, matching an explicit `--ble-name`/`--ble-address`, or whose local name contains `HUAWEI Band 10`. A candidate is never treated as heart-rate support by name alone: the backend still discovers services and requires notify characteristic 0x2A37 before subscribing.

The private address from the supplied Python reference is not in source or docs. `parse_hr` handles one/16-bit BPM, skips Energy Expended (flag 0x08), and decodes RR intervals (flag 0x10). Offline MSVC Rust tests pass (2 tests), including malformed packets and RR/Energy cases; `cargo fmt` applied. No BLE connection, pairing, privacy change, camera/runtime change, or package rebuild was performed.

Cause: the prior `ScanFilter { services: [0x180D] }` excluded a watch that advertises by name/address while exposing 0x2A37 only after connection. Run one app instance with `--ble-name "HUAWEI Band 10"` or a user-supplied address and inspect candidate/missing-characteristic/no-notification status. Band10 support remains unconfirmed.
