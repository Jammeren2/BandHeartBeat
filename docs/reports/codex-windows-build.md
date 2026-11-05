# Windows standalone EXE retry

The MSVC command remains blocked by missing `link.exe`, but the installed GNU Rust toolchain is viable. Command used:

`rustup run stable-x86_64-pc-windows-gnu cargo build --release`

Build succeeded. Artifact: `target\\release\\band-heart-beat.exe` (size: 3,168,719 bytes). This is a MinGW GNU-target executable; no Huawei hardware was available, so runtime BLE validation was not performed. The MSVC alternative remains `cargo build --release` from a Visual Studio Developer PowerShell.

