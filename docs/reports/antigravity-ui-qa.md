# QA Report: Native Windows UI & Release Executable Artifact

**Author:** Antigravity (`slot_id: 01a1ac15-a3be-7881-8487-2faede13c827`)  
**Task ID:** `01a1ac9c-703a-7950-ba93-0382caec90aa`  
**Date:** November 2026  
**Status:** Completed  
**Target Repository:** `D:\py\BandHeartBeat`  
**Baseline Git Commit:** `91f894b`  

---

## 1. Executive Summary

This report delivers independent quality assurance for the native Windows companion application and release artifact (`band-heart-beat.exe`) developed on branch `feature/ble-heart-rate-poc`. 

Independent verification confirmed that the codebase passes `cargo fmt --check` and `cargo test --offline` with zero errors. Inspection of the Win32 message loop, GDI resource handling, and state synchronization confirmed robust tray behavior, correct `--demo` execution, thread-safe BLE retry guarding, and leak-free GDI cleanup. The compiled GNU release executable was verified as a standalone 3.13 MB binary linking exclusively against standard Windows system DLLs with zero external MinGW runtime dependencies. No physical hardware tests were performed; all performance metrics remain unmeasured design targets.

---

## 2. Automated Test & Code Formatting Results

Independent validation commands were executed on the Windows test environment using the offline GNU toolchain:

| Check | Command | Result | Details |
| :--- | :--- | :--- | :--- |
| **Code Formatting** | `cargo fmt --check` | **PASS** | Exit code `0`. Zero style discrepancies. |
| **Unit Tests** | `rustup run stable-x86_64-pc-windows-gnu cargo test --offline` | **PASS** | Exit code `0`. 2 passed, 0 failed: <br>• `tests::decodes_standard_hr_measurements`: Validates 8-bit, 16-bit, and malformed/truncated GATT payloads.<br>• `overlay::tests::status_json_escapes_controls_and_quotes`: Validates JSON string escaping for SSE streams. |

---

## 3. Release Artifact & Portability Verification

### 3.1 Artifact Metadata
- **File Path:** `target\release\band-heart-beat.exe`
- **File Size:** `3,287,401` bytes (~3.13 MiB / 3.28 MB)
- **SHA-256 Checksum:** `0DF0C195D7F2E8FA416BAFC843376A5704FD907D74ED70B4A108EDA9D9B28B39`
- **Subsystem:** Windows GUI (`#![windows_subsystem = "windows"]`) — launches silently without an attached console window.

### 3.2 Portable Runtime DLL Dependency Audit
Analysis of the PE Import Directory Table verified that the binary contains **zero external MinGW or third-party DLL dependencies**:
- **System Imports:** `KERNEL32.dll`, `user32.dll`, `gdi32.dll`, `shell32.dll`, `ole32.dll`, `oleaut32.dll`, `WS2_32.dll`, `USERENV.dll`, `ntdll.dll`, `bcryptprimitives.dll`.
- **C Runtime Forwarders:** `api-ms-win-crt-*` (Windows Universal CRT, pre-installed on Windows 10/11).
- **WinRT Forwarders:** `api-ms-win-core-winrt-*` (Windows Runtime Bluetooth infrastructure).
- **MinGW Runtime:** The GNU toolchain statically links `libgcc` and compiler runtime routines. `libgcc_s_dw2-1.dll` and `libwinpthread-1.dll` are **not required**. The binary runs cleanly on clean Windows 10/11 systems without installing MinGW.

---

## 4. Source Code Architecture & Surface Inspection

### 4.1 Window Lifecycle & System Tray Behavior
- **Minimize Behavior (`WM_SIZE`):** Intercepts `SIZE_MINIMIZED` and calls `ShowWindow(hwnd, SW_HIDE)`. The window disappears from the taskbar while the process continues running.
- **Close Behavior (`WM_CLOSE`):** Intercepts the window title bar close button (`X`) and executes `ShowWindow(hwnd, SW_HIDE)`, preserving background telemetry.
- **Tray Left-Click (`WM_TRAY` / `WM_LBUTTONUP`):** Executes `ShowWindow(hwnd, SW_RESTORE)` and `SetForegroundWindow(hwnd)` to restore focus.
- **Tray Right-Click (`WM_TRAY` / `WM_RBUTTONUP`):** Displays a Win32 popup menu (`CreatePopupMenu`) offering `Show` ("Показать"), `Open OBS overlay` ("Открыть оверлей OBS"), `Retry` ("Повторить подключение"), and `Quit` ("Выйти"). Calls `TrackPopupMenu` followed immediately by `DestroyMenu(menu)`.
- **Clean Exit (`WM_DESTROY`):** Deletes the notification icon via `Shell_NotifyIconW(NIM_DELETE, &nid)` and triggers `PostQuitMessage(0)`, exiting the message pump.

### 4.2 GDI Resource Management & Memory Safety
- **Brush Management:** In `WM_PAINT`, `CreateSolidBrush(0x00261f1e)` is paired with `DeleteObject(bg.into())` on every paint cycle.
- **Font Management:** In `draw_text`, `CreateFontW(...)` selects the font, renders text, restores the original DC object via `SelectObject(hdc, old)`, and immediately calls `DeleteObject(font.into())`.
- **Painting Overhead:** The 500 ms timer checks the MPSC channel (`UI_RX.try_iter()`). `InvalidateRect` is executed **only if telemetry data changed AND the window is visible** (`IsWindowVisible(hwnd)`). When minimized or hidden in the tray, redraws are entirely skipped, consuming 0% GPU/GDI rendering cycles.

### 4.3 Truthful `--demo` Mode
- Evaluates `std::env::args().any(|a| a == "--demo")`.
- When enabled, immediately broadcasts:
  - BPM: `72`
  - Status: `"ДЕМО · без браслета"`
- Does not invoke Bluetooth adapters, WinRT managers, or over-the-air scans.
- Clearly flags simulated operation to the user in both the window and status bar.

### 4.4 Disconnected & Error State Handling
- **Initial State:** BPM initialized to `0` (rendered as an em-dash `"—"` rather than a misleading zero); status set to `"Поиск браслета…"`.
- **Visual Feedback:** Status text renders in muted gray (`0x00a8adb8`) when disconnected or searching, switching to green (`0x0075d69a`) only when actively receiving live GATT notifications (`data.live == true`).
- **Adapter Missing:** Reports `"Bluetooth недоступен · проверьте адаптер"`.
- **Device Not Found:** Reports `"Браслет не найден · включите HR Broadcast"`.
- **Session Timeout:** If notifications cease for 20 seconds, `tokio::time::timeout` drops the peripheral connection, resets BPM to `0`, and transitions status to `"Браслет отключён"`.

### 4.5 Worker Concurrency & Retry Guard
- An atomic flag `static WORKING: AtomicBool` guards the worker thread.
- `WORKING.swap(true, Ordering::AcqRel)` ensures that if a scan or connection worker is active, repeated clicks on `Retry` ("Повторить подключение") are safely ignored.
- The flag is reset to `false` via `Ordering::Release` once the worker finishes or terminates, preventing worker thread leaks or concurrent BLE adapter contention.

---

## 5. Identified Limitations & Operational Boundaries

1. **Hardware Verification Unperformed:**  
   Zero physical heart-rate wearables (HUAWEI Band 10 or generic BLE monitors) were available. Compatibility with physical HUAWEI Band 10 hardware remains formally **unconfirmed**.
2. **Performance Metrics Unmeasured:**  
   Performance specifications (e.g., $< 1.0\%$ CPU, $< 30\text{ MB}$ RAM, $< 100\text{ ms}$ latency) are design targets, not empirically measured figures on target hardware.
3. **No Automatic Background Reconnection:**  
   When a session disconnects after the 20-second timeout, the app does not enter an infinite reconnect loop; the user must click "Повторить подключение" in the tray context menu or restart the app.
4. **Standard DPI Virtualization:**  
   Uses standard Windows GDI font rendering with default DPI virtualization; no custom Per-Monitor V2 high-DPI scaling matrix is embedded.
5. **English Error Diagnostic Passthrough:**  
   Unexpected WinRT subsystem errors (e.g. `Access is denied`) are passed through as raw formatted error strings without full Russian localization.
6. **Tray Icon Artwork:**  
   Uses the system default application icon (`IDI_APPLICATION`); custom taskbar branding icon is not embedded.

---

## 6. QA Checklist & Verification Matrix

| Verification Item | Status | Evidence / Notes |
| :--- | :--- | :--- |
| `cargo fmt --check` | **PASS** | Codebase conforms to standard Rust formatting. |
| `cargo test --offline` | **PASS** | Parser and SSE escaping tests pass cleanly. |
| Executable Existence | **PASS** | `target\release\band-heart-beat.exe` exists (3,287,401 bytes). |
| SHA-256 Checksum | **VERIFIED** | `0DF0C195D7F2E8FA416BAFC843376A5704FD907D74ED70B4A108EDA9D9B28B39`. |
| Portability / DLL Audit | **PASS** | Only standard Windows DLLs; no external MinGW/GNU runtime requirements. |
| Hide-to-Tray on Minimize | **PASS** | `WM_SIZE` (`SIZE_MINIMIZED`) invokes `ShowWindow(hwnd, SW_HIDE)`. |
| Hide-to-Tray on Close | **PASS** | `WM_CLOSE` invokes `ShowWindow(hwnd, SW_HIDE)`. |
| Tray Context Menu | **PASS** | `WM_RBUTTONUP` tracks popup menu (`Show`, `Overlay`, `Retry`, `Quit`). |
| GDI Leak Prevention | **PASS** | All created fonts and brushes deleted via `DeleteObject`. |
| Idle Redraw Efficiency | **PASS** | Paints only when telemetry changes AND window is visible. |
| Truthful `--demo` Mode | **PASS** | Displays explicit "ДЕМО" status with 72 BPM; bypasses Bluetooth. |
| Disconnected State Display | **PASS** | Displays `"—"` when BPM is 0; muted status text. |
| Retry Thread Guard | **PASS** | `WORKING` atomic flag prevents duplicate worker spawning. |
| Physical Hardware Verification | **UNPERFORMED** | No physical wearable connected; Band 10 status unconfirmed. |

---

## 7. Summary

Independent QA verified that `band-heart-beat.exe` (3.13 MB, SHA-256 `0DF0C195...`) passes all offline tests and `cargo fmt`, linking exclusively to native Windows system DLLs with zero external MinGW runtime dependencies. The Win32 implementation demonstrates leak-free GDI handling, efficient event-driven painting, safe atomic retry guards, and truthful `--demo` and disconnected state displays. Physical hardware tests were unperformed and HUAWEI Band 10 compatibility remains unconfirmed.
