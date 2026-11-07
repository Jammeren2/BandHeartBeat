# Desktop V11 bounded BLE helper

## Artifact
`dist/BandHeartBeat-desktop-v11.zip`:1,093,825 bytes; SHA256 `B2D7172B21A98B4AFBD3BFF81F166C096E53ECCE2FAB9670DDD7B3E4AA628FF9`.

## Fix
V10 found Huawei Band HR-193 then hung until generic Win32258. V11 separates post-discovery deadlines: connect8s, discover services8s, disconnect2s; native pipe safety timeout is30s and drains phase output before terminating only the owned helper. Candidate name remains persistent; failures identify connect/discovery timeout or missing notify2A37. No subscribe/pairing in scan mode.

## Validation
VS2022 native build, offline Rust tests2/fmt, extracted producer selftest/admin status, package archive generated. No BLE connection, physical camera, registry, user process or Discord action. HR-193/Band10 standard service and live Discord remain unverified.
