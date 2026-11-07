# BLE telemetry diagnosis

## Findings (2026-11-07)
The local telemetry endpoint is alive but reports `0 0 0` (`GET http://127.0.0.1:8765/telemetry`, HTTP200). This means no BPM and `live=false`; it is not evidence that the watch is connected. The packaged backend's BLE worker starts in non-demo mode, scans eight seconds for devices advertising standard Heart Rate Service `0000180d-0000-1000-8000-00805f9b34fb`, then connects, discovers `00002a37-0000-1000-8000-00805f9b34fb`, subscribes notifications, and publishes BPM. If no service is found it surfaces “Браслет не найден · включите HR Broadcast”.

Read-only Windows checks show Bluetooth LE enumerator and Realtek Bluetooth 5.3 adapter present/OK, but no present PnP device whose friendly name identifies Band/ Huawei/Watch/Heart. This distinguishes an available adapter / paired computer entry from a discoverable GATT HR service. No pairing, connection, notification subscription or privacy setting was initiated. No watch address/name is recorded.

No BandHeartBeat process was attached to the current query; the endpoint response therefore likely came from another backend instance or a stale/user process. The backend can only own port8765 once; a second GUI reports an occupied-port startup error. Demo mode is explicit (`--demo`) and publishes 72 BPM / demo flag, so `0 0 0` is the truthful disconnected state.

## Protocol/code audit
`run_ble` uses btleplug `Central::start_scan(ScanFilter{services:[0x180D]})`, selects the first peripheral whose advertised properties contain0x180D, connects, discovers services, finds0x2A37, subscribes, and waits up to20s per notification. The parser correctly handles Heart Rate Measurement flags: bit0 clear = one-byte BPM; bit0 set = little-endian uint16. Unit tests pass `[0,72]→72`, `[1,44,1]→300`, malformed packets→None. No Huawei-specific broadcast UUID or vendor protocol is implemented. The code assumes the watch exposes the standard GATT service and active notifications; Windows Bluetooth pairing alone does not satisfy either assumption.

Huawei Band 10 support is **UNCONFIRMED**. No local evidence proves HR Data Broadcast or standard0x180D exposure, and no official Band10 capability source is present in this workspace. A paired device may expose only vendor services, require Huawei Health/phone relay, or remain non-connectable to generic GATT clients.

## Tests
`rustup run stable-x86_64-pc-windows-msvc cargo test --offline --locked`:2 tests passed (BPM parser and overlay JSON escaping). Telemetry endpoint read returned HTTP200 `0 0 0`. No BLE device data, watch images/files, connection attempts, pairing, notification writes, Discord actions, or process termination were performed.

## Next user step
On the same machine, run exactly one V7 backend/GUI instance and confirm its native status. Use explicit `--demo` to verify the UI path (expected72/demo). For live mode, the watch must advertise a generic readable0x180D service with notify-enabled0x2A37; Windows Settings showing “paired” is insufficient. If it does, collect the app's displayed error after the eight-second scan/20-second notification wait. If it does not, generic BandHeartBeat cannot read it without a documented Huawei/vendor transport or a phone relay; Band10 compatibility must remain unclaimed. Do not infer success from the localhost server being reachable.

Parent virtual-camera artifact issue is separate from BLE telemetry and is not changed by this diagnosis.
