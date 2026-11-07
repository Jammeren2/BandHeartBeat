# Native Find Watch action

Added a real native «Найти часы» button in the desktop camera panel. It launches the bounded `band-backend.exe --ble-scan` helper asynchronously, keeps a separate persistent status line, disables concurrent search, and reports candidate names, service/characteristic readiness, timeout, Bluetooth adapter and connection errors. The helper scans up to8s, accepts advertised0x180D/name/address hints, then connects only to inspect services and requires notify0x2A37; it disconnects after readiness and does not subscribe for BPM or silently pair.

The prior mirror/schema4 behavior is preserved. Native source/build and Rust tests/fmt pass. No BLE scan or device connection was run by the agent; no user process/registry/camera/Discord action. Mock scan/no-device behavior is represented by helper error paths but cannot be exercised against a real adapter without starting the explicit button. Band10 support and live Discord remain unverified.
