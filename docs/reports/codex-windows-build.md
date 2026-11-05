# Windows release build report

Attempted commands:

- `cargo build --release`
- `cargo build --release --target x86_64-pc-windows-gnu`

Both builds are blocked before linking because `link.exe` is not present. The active default target is `x86_64-pc-windows-msvc`; although the GNU target and GCC are installed, dependency build scripts still require the host MSVC linker in this environment and the GNU attempt reports the same `link.exe not found` error. No `.exe` artifact was produced, so artifact size/path are unavailable.

Recommended setup: install **Visual Studio 2022 Build Tools** with the *Desktop development with C++* workload (including MSVC v143 and Windows 10/11 SDK), open a *Developer PowerShell for VS 2022*, then run `cargo build --release`. The expected artifact is `target\\release\\band-heart-beat.exe`.

No BLE hardware was used or claimed for validation.
