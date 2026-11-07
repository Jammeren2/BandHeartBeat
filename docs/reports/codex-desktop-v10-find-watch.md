# Desktop V10 native Find Watch

## Artifact
`dist/BandHeartBeat-desktop-v10.zip`:1,093,041 bytes; SHA256 `84FF9A56562F16098EAA25CC0ABD6BBCF6332D2670BF0CEAB2B1AE9BE88B04AC`.

## Native search
Desktop now has an always-visible «Найти часы» action. It asynchronously launches backend `--ble-scan`, bounded to8s discovery, displays persistent candidate/error status separately from camera status, and prevents concurrent scan. The scanner accepts advertised0x180D/name/address hints, reports candidate names without full addresses, connects only to inspect GATT, requires notify0x2A37, then disconnects. It does not subscribe to BPM or pair silently. Port ownership remains single backend at8765.

## Validation and limits
Rust tests2/fmt, native build, producer selftest, admin status, manifest8 and extracted package checks pass. No BLE scan/connection/pairing, camera, registry, user process or Discord action was performed. Mock/no-device behavior is represented by bounded helper error paths; actual Band10 service/2A37 and live Discord mirror remain unverified. V9 mirror/schema4 behavior is preserved. The task's full user flow can now be exercised by clicking Find Watch in V10; report candidate/missing characteristic/status. Band10 support remains unconfirmed.
