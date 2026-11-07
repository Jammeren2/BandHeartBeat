# BLE HR-193 helper timeout fix

The V10 screenshot showed candidate `HUAWEI Band HR-193` with service180D, followed by generic Win32258 after the helper's 10s parent timeout. The scan had completed; post-discovery `connect` or `discover_services` could block inside btleplug/WinRT. The helper now applies separate8s connect and8s service-discovery deadlines,2s disconnect cleanup, and prints phase-specific candidate/rejection output. The native pipe reader allows30s for the owned helper and preserves phase output; only the helper is terminated on its deadline. No subscription/BPM notification or silent pairing occurs in scan mode.

Candidate still requires discovered notify-capable0x2A37. If HR-193 exposes180D but not2A37, output is `candidate rejected: missing notify 0x2A37`; if WinRT blocks, output names `connect timeout after 8s` or `discover services timeout after 8s`. Candidate name remains visible in native persistent status. No full address is printed by default.

Offline Rust tests (2) and fmt pass. No physical BLE connection or process/registry/camera/Discord action was run by the agent. Band10/HR-193 service support remains unconfirmed.
