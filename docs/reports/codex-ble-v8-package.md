# Desktop V8 BLE discovery package

## Artifact
`dist/BandHeartBeat-desktop-v8.zip`:1,065,620 bytes; SHA256 `3150139D8AB7AC5FEB5E1DB6945705DCC79AC910E2610FE44E8C658A0BED1EC7`.
Folder `dist/desktop-v8`; extracted `target/desktop-v8-package-qa`; 8 release files, no UxTest/RegistrationQa namespace or test helper.

## Change
Rust BLE scan no longer filters exclusively on advertised0x180D. It considers advertised service, case-insensitive local name hint `HUAWEI Band 10`, or explicit runtime `--ble-name` / `--ble-address`. The supplied private MAC is absent from source/docs. Candidate selection still requires GATT discovery of notify characteristic0x2A37; name/address alone never yields BPM. `parse_hr` handles one/16-bit BPM, energy skip, and RR intervals.

## Checks
`cargo test --offline --locked` (MSVC):2 tests passed (BPM, malformed packets, energy/RR). `cargo fmt` passed. Package build via VS2022/SDK + locked offline Rust succeeded. Extracted producer self-test and admin status passed. Manifest8/no test namespaces passed. No BLE scan/connection/pairing performed by agent, no camera/runtime/registry/user process changes. Demo remains explicit72; one backend owns8765.

## User use and limits
Launch only one V8 instance. Demo: `BandHeartBeat.exe --demo`. Live hints: `BandHeartBeat.exe --ble-name "HUAWEI Band 10"` or a user-provided address. A candidate status is followed by one of: missing0x2A37, waiting for notifications, or live BPM. Windows pairing/Bluetooth presence does not prove GATT HR service. Band10 support and Discord/camera behavior remain unverified.
