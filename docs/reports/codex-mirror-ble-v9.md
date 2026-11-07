# Mirror and BLE search controls

## Summary
Added schema4 `mirror_horizontal` (default false) with V2 mapping names, native «Отразить камеру» toggle and producer final-frame BGRA mirror. Preview mirrors the same copied frame once after read, then restores its local buffer, so preview and virtual output match and no double mirror occurs. Added control-pixel mirror hash regression. BLE discovery fallback from V8 remains; explicit name/address hints select candidates, service0x180D or name/address candidate requires discovered notify2A37.

## Checks
VS2022 native build; controls-probe ABI/mirror hash, tracking probe, Rust BLE parser tests/fmt and package selftest/status/manifest checks pass. No physical camera, production IPC, registry or user process actions. Camera mirror not tested in live Discord. BLE search remains explicit CLI hint rather than a new native button; actual Band10 service/notifications remain unverified.
