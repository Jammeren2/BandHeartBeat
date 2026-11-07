# Desktop V9 mirror and BLE controls

## Artifact
`dist/BandHeartBeat-desktop-v9.zip`:1,066,324 bytes; SHA256 `EAA557914C28F4F0FFD1599DD82CCA8237BF76C87DE7699DFD93AB57A5C78F6E`.

## Changes
CameraControls schema4/mapping V4 adds `mirror_horizontal=false`. Native toggle «Отразить камеру» writes the same controls producer reads. Producer mirrors the final BGRA frame after overlay composition; desktop preview mirrors its local copy once and restores it after painting, so preview and virtual output match without double mirroring. Tracking/drag coordinates remain unmirrored; only final image is flipped. Control probe observes distinct mirror on/off output hashes; controls/tracking/selftest pass.

BLE V8 discovery fallback remains: broad scan by advertised0x180D, case-insensitive local name HUAWEI Band10, or explicit `--ble-name`/`--ble-address`; connection requires discovered notify0x2A37. Name/address are candidate hints, not proof; no private address committed. V9 does not add a native search button; status is shown by backend candidate/error and explicit CLI start. Actual Band10 service, notifications and live Discord mirror remain unverified.

## Validation
VS2022/SDK native package, locked offline Rust MSVC tests (2 parser tests), fmt, extracted producer selftest, admin status, manifest8/no test namespaces PASS. No physical camera/BLE connection/pairing, registry changes, user process termination or Discord interaction. Prior 30fps/5–30 negotiation/cache/tracking tests remain unchanged. Camera mirror not live Discord tested.
