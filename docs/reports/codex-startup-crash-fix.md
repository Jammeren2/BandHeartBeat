# Desktop startup crash fix

The frozen package crashed during its first paint with process exit `0xC000041D`; Windows Application Error recorded `user32.dll` access violation `0xC0000005`. `/telemetry` never became usable. This acceptance rerun and fix were performed by the implementation author, **not an independent reviewer**.

The initial camera status is an empty string. `draw_text` converted it into an empty `Vec<u16>` and passed its dangling zero-length buffer pointer to `DrawTextW`. Skipping that empty call fixes startup. Disabling camera enumeration/BLE worker/timer during localization did not fix the failure; all temporary disabling was reverted. The final source change is only an early return when `text.is_empty()`, before allocating/selecting fonts or calling Win32 drawing functions. Native producer/filter behavior was not changed.

Validation:

- Debug MSVC build: demo remains alive and `/telemetry` returns `72 0 1`.
- Rebuilt standalone MSVC release: **three consecutive** fresh `--demo` starts remained alive, returned `72 0 1` with bounded HTTP timeout, and left no producer process running automatically. Only owned test app processes were terminated afterward.
- Re-extracted the replacement ZIP into `target/qa-startup-fixed-20261107` and repeated the bounded startup/telemetry/no-producer check successfully. The owned extracted-package process was closed and reaped within three seconds; no GUI automation was used.
- `cargo fmt --check`: passed. GNU offline tests and isolated MSVC offline tests: 2/2 passed on both toolchains. Some compiler processes were slow on this host but finished successfully.
- `cmd /c directshow-camera\package-msvc.cmd`: succeeded. No physical camera, registry/privacy/HKLM operation or Discord interaction was performed for this fix.

The replacement archive is `dist/BandHeartBeat-standalone.zip`, SHA256 **55ADF1F2108C6A8C31F3F7D72D48937E1382BE7DCC2F69C0578969545C70AD2C**. The previous frozen archive hash `442FAFA7...` remains a failed historical acceptance result; it must not be presented as the current package. This startup correction is not acceptance of the requested future native preview/drag-anchor/styles/scaling UX, live-face quality or Discord compatibility.

## Summary

Fixed the first-paint crash by avoiding DrawTextW on an empty text buffer. The rebuilt desktop package passes three real demo startup/telemetry checks; author validation is explicit and native-camera/Discord/UX acceptance remains separate.
