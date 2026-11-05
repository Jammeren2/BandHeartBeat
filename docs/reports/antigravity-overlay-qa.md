# QA Report: Shared Telemetry OBS Overlay (`commit fceb27c`)

**Author:** Antigravity (`slot_id: 01a1ac15-a312-7b32-8071-fe3d440c2541`)  
**Task ID:** `01a1aca1-f56b-72d2-a5bd-d9a397d7f1e2`  
**Commit Inspected:** `fceb27c` (*Add transparent localhost OBS heart-rate overlay*)  
**Target Repository:** `D:\py\BandHeartBeat`  

---

## 1. Summary
The embedded loopback HTTP/SSE OBS overlay passes all automated, HTTP, security, and concurrency tests: it serves transparent SVG graphics on `127.0.0.1:8765/overlay` and streams truthful shared telemetry on `/events` (verifying both `demo:true` and offline `demo:false, bpm:0` states) with zero filesystem exposure and strict loopback validation. End-to-end video compositing via OBS Virtual Camera in a live Discord call and physical HUAWEI Band 10 hardware compatibility remain unverified.

---

## 2. Automated Verification & Code Quality

| Check | Target / Environment | Result | Details |
| :--- | :--- | :--- | :--- |
| **`cargo fmt --check`** | Standard Rustfmt | **PASS** | Working tree and new overlay modules conform strictly to formatting rules. |
| **`cargo test`** | `stable-x86_64-pc-windows-gnu` | **PASS** | 2/2 unit tests passed (`tests::decodes_standard_hr_measurements` and `overlay::tests::status_json_escapes_controls_and_quotes`). |
| **Binary Footprint** | Release executable | **PASS** | `target\release\band-heart-beat.exe` (3,287,394 bytes, ~3.13 MB) cleanly incorporates embedded HTML/SVG assets with zero runtime size bloat. |

---

## 3. Live HTTP & SSE Endpoint Verification

Live HTTP and Server-Sent Events (SSE) tests were conducted against the running instance on `127.0.0.1:8765`:

### 3.1 `GET /overlay` and `GET /` (HTML Web Interface)
- **Status Code:** `200 OK` (both `/overlay` and root `/` alias).
- **MIME & Headers:** `Content-Type: text/html; charset=utf-8`, `Cache-Control: no-store`, `X-Content-Type-Options: nosniff`, `Connection: close`.
- **Transparency:** Root and body declare `background: transparent!important; overflow: hidden`, ensuring perfect alpha blending inside OBS Studio Browser Source.
- **Graphic Assets:** Embedded inline vector SVG heart (`viewBox="0 0 64 64"`, fill `#ff8b73`), large tabular digits (`font-variant-numeric: tabular-nums`, size `92px`), and Segoe UI typography.
- **Dynamic Animation:** Pulse keyframe animation (`heartbeat`) calculates CSS cycle time dynamically via `style.setProperty('--beat', (60 / bpm) + 's')`.

### 3.2 `GET /events` (Real-Time Telemetry Stream)
- **Status Code:** `200 OK`.
- **MIME & Headers:** `Content-Type: text/event-stream`, `Cache-Control: no-store`, `X-Content-Type-Options: nosniff`.
- **Shared Telemetry Verification:**
  - **Demo Mode (`--demo`):** Stream emits valid JSON:  
    `data: {"bpm":72,"live":false,"demo":true,"status":"ДЕМО · без браслета"}`  
    The overlay displays `72`, sets heart rate pulse duration to `0.833s`, and displays the gold badge `ДЕМО · БЕЗ БРАСЛЕТА`.
  - **Normal / Hardware Search Mode:** Stream emits:  
    `data: {"bpm":0,"live":false,"demo":false,"status":"Браслет не найден · включите HR Broadcast"}`  
    The overlay displays an em-dash (`—`), stops the pulsing animation (opacity drops to `0.55`), hides the demo badge, and prints the exact device search status.
- **Watchdog & Offline Guard:** If SSE frames stop arriving for > 4.0 seconds, or if the socket errors out, client-side JavaScript automatically transitions the display to `'—'` and `'Нет связи с приложением'`.

---

## 4. Security & Robustness Verification

1. **Host Header Validation:**
   - Sending requests with a hostile Host header (`Host: attacker.com`) or foreign origin returned **`403 Forbidden`** (`Loopback Host required`).
2. **Zero Filesystem Exposure:**
   - Path traversal attempts (`GET /../../Cargo.toml`, `GET /etc/passwd`) returned **`404 Not Found`**.
   - The server exclusively serves memory-embedded bytes (`include_bytes!("overlay.html")`) and possesses zero filesystem read primitives.
3. **Client Limit & Saturation Handling:**
   - Server strictly enforces `clients.fetch_add(1) >= 8`.
   - Saturated connection test: Connected 8 concurrent SSE streams; the 9th connection was immediately dropped (0 bytes received) without blocking the accept loop or impacting existing clients.
   - Upon closing connections, client slots were immediately recycled and new connections succeeded.
4. **Occupied Port Handling:**
   - In `src/main.rs`, `overlay::start()` returns an `std::io::Result<()>`. If port `8765` is already bound by another process, the error is caught and displayed via a native Win32 warning dialog (`MessageBoxW`: *"Оверлей недоступен: порт 127.0.0.1:8765 занят..."*) rather than panicking or crashing.

---

## 5. Verification Boundaries & Honest Limitations

In strict adherence to project guidelines and facts:
1. **Physical Wearable Hardware:** **UNVERIFIED**. No physical HUAWEI Band 10 was connected; native "HR Data Broadcast" support on the Band 10 remains unconfirmed.
2. **End-to-End OBS/Discord Video Feed:** **UNVERIFIED**. Actual video compositing onto a physical webcam stream via OBS Virtual Camera and transmission into a Discord video call was not automated or verified (no browser automation, Discord control, or webcam hijacking was performed).
3. **Overlay Latency:** Due to the 1.0-second SSE broadcast interval and the 250 ms native timer polling loop, sub-100 ms telemetry latency is not claimed.

---

## Summary
The embedded loopback HTTP/SSE OBS overlay passes all automated, HTTP, security, and concurrency tests: it serves transparent SVG graphics on 127.0.0.1:8765/overlay and streams truthful shared telemetry on /events (verifying both demo:true and offline demo:false, bpm:0 states) with zero filesystem exposure and strict loopback validation. End-to-end video compositing via OBS Virtual Camera in a live Discord call and physical HUAWEI Band 10 hardware compatibility remain unverified.
