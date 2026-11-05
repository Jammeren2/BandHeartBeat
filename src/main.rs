#![windows_subsystem = "windows"]
use anyhow::{Context, Result};
use btleplug::api::{
    Central, Characteristic, Manager as _, Peripheral as _, ScanFilter, ValueNotification,
};
use btleplug::platform::Manager;
use futures_util::StreamExt;
use std::sync::{mpsc, Mutex, OnceLock};
use std::thread;
use std::time::Duration;

const HEART_RATE_SERVICE: uuid::Uuid = uuid::uuid!("0000180d-0000-1000-8000-00805f9b34fb");
const HEART_RATE_MEASUREMENT: uuid::Uuid = uuid::uuid!("00002a37-0000-1000-8000-00805f9b34fb");

#[derive(Clone, Debug)]
enum UiMessage {
    Status(String, bool),
    Bpm(u16),
}

struct UiData {
    bpm: u16,
    status: String,
    live: bool,
}
static WORKING: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
static UI_TX: OnceLock<mpsc::Sender<UiMessage>> = OnceLock::new();
static UI_DATA: OnceLock<Mutex<UiData>> = OnceLock::new();
static UI_RX: OnceLock<Mutex<mpsc::Receiver<UiMessage>>> = OnceLock::new();
const WM_TRAY: u32 = windows::Win32::UI::WindowsAndMessaging::WM_APP + 1;
const ID_TRAY_SHOW: usize = 1001;
const ID_TRAY_RETRY: usize = 1002;
const ID_TRAY_QUIT: usize = 1003;

#[cfg(target_os = "windows")]
fn main() -> Result<()> {
    run_windows()
}

#[cfg(not(target_os = "windows"))]
#[tokio::main]
async fn main() -> Result<()> {
    run_ble(mpsc::channel().0, false).await
}

#[cfg(target_os = "windows")]
fn run_windows() -> Result<()> {
    use windows::core::{w, PCWSTR};
    use windows::Win32::Foundation::{HINSTANCE, HWND, LPARAM, LRESULT, POINT, WPARAM};
    use windows::Win32::Graphics::Gdi::{
        BeginPaint, CreateSolidBrush, DeleteObject, EndPaint, FillRect, InvalidateRect, SetBkMode,
        UpdateWindow, HBRUSH, PAINTSTRUCT, TRANSPARENT,
    };
    use windows::Win32::System::LibraryLoader::GetModuleHandleW;
    use windows::Win32::UI::Shell::{
        Shell_NotifyIconW, NIF_ICON, NIF_MESSAGE, NIF_TIP, NIM_ADD, NIM_DELETE, NOTIFYICONDATAW,
    };
    use windows::Win32::UI::WindowsAndMessaging::*;

    UI_DATA
        .set(Mutex::new(UiData {
            bpm: 0,
            status: "Поиск браслета…".into(),
            live: false,
        }))
        .ok();
    let (tx, rx) = mpsc::channel();
    UI_RX.set(Mutex::new(rx)).ok();
    UI_TX.set(tx).ok();
    start_worker();

    unsafe extern "system" fn wnd_proc(
        hwnd: HWND,
        msg: u32,
        wparam: WPARAM,
        lparam: LPARAM,
    ) -> LRESULT {
        use windows::Win32::Foundation::RECT;
        match msg {
            WM_CREATE => {
                SetTimer(Some(hwnd), 1, 500, None);
                return LRESULT(0);
            }
            WM_TIMER => {
                let mut changed = false;
                if let Some(rx) = UI_RX.get() {
                    if let Ok(rx) = rx.lock() {
                        if let Some(Ok(mut data)) = UI_DATA.get().map(|d| d.lock()) {
                            for msg in rx.try_iter() {
                                changed = true;
                                match msg {
                                    UiMessage::Status(s, live) => {
                                        data.status = s;
                                        data.live = live;
                                    }
                                    UiMessage::Bpm(v) => data.bpm = v,
                                }
                            }
                        }
                    }
                }
                if changed && IsWindowVisible(hwnd).as_bool() {
                    let _ = InvalidateRect(Some(hwnd), None, false);
                }
                return LRESULT(0);
            }
            WM_PAINT => {
                let mut ps = PAINTSTRUCT::default();
                let hdc = BeginPaint(hwnd, &mut ps);
                let mut rect = RECT::default();
                let _ = GetClientRect(hwnd, &mut rect);
                let bg = CreateSolidBrush(windows::Win32::Foundation::COLORREF(0x00261f1e));
                FillRect(hdc, &rect, HBRUSH(bg.0));
                let _ = DeleteObject(bg.into());
                SetBkMode(hdc, TRANSPARENT);
                if let Some(Ok(data)) = UI_DATA.get().map(|d| d.lock()) {
                    draw_text(hdc, 26, 20, 16, 600, 0x00e9edf2, "BAND HEARTBEAT");
                    draw_text(hdc, 26, 62, 52, 400, 0x00738bff, "♥");
                    draw_text(
                        hdc,
                        90,
                        48,
                        76,
                        600,
                        0x00e9edf2,
                        &if data.bpm == 0 {
                            "—".into()
                        } else {
                            data.bpm.to_string()
                        },
                    );
                    draw_text(hdc, 244, 98, 15, 400, 0x00a8adb8, "уд/мин");
                    draw_text(
                        hdc,
                        26,
                        144,
                        15,
                        400,
                        if data.live { 0x0075d69a } else { 0x00a8adb8 },
                        &data.status,
                    );
                    draw_text(
                        hdc,
                        26,
                        182,
                        13,
                        400,
                        0x00868c98,
                        "Закрытие окна сворачивает в трей",
                    );
                }
                let _ = EndPaint(hwnd, &ps);
                return LRESULT(0);
            }
            WM_SIZE if wparam.0 == SIZE_MINIMIZED as usize => {
                let _ = ShowWindow(hwnd, SW_HIDE);
                return LRESULT(0);
            }
            WM_CLOSE => {
                let _ = ShowWindow(hwnd, SW_HIDE);
                return LRESULT(0);
            }
            WM_COMMAND => {
                match (wparam.0 & 0xffff) as usize {
                    ID_TRAY_SHOW => {
                        let _ = ShowWindow(hwnd, SW_SHOW);
                        let _ = SetForegroundWindow(hwnd);
                    }
                    ID_TRAY_RETRY => {
                        start_worker();
                    }
                    ID_TRAY_QUIT => {
                        let _ = DestroyWindow(hwnd);
                    }
                    _ => {}
                }
                return LRESULT(0);
            }
            WM_TRAY => {
                let event = (lparam.0 & 0xffff) as u32;
                if event == WM_LBUTTONUP {
                    let _ = ShowWindow(hwnd, SW_SHOW);
                    let _ = SetForegroundWindow(hwnd);
                } else if event == WM_RBUTTONUP {
                    let Ok(menu) = CreatePopupMenu() else {
                        return LRESULT(0);
                    };
                    let _ = AppendMenuW(menu, MF_STRING, ID_TRAY_SHOW, w!("Показать"));
                    let _ =
                        AppendMenuW(menu, MF_STRING, ID_TRAY_RETRY, w!("Повторить подключение"));
                    let _ = AppendMenuW(menu, MF_SEPARATOR, 0, PCWSTR::null());
                    let _ = AppendMenuW(menu, MF_STRING, ID_TRAY_QUIT, w!("Выйти"));
                    let mut point = POINT::default();
                    let _ = GetCursorPos(&mut point);
                    let _ = SetForegroundWindow(hwnd);
                    let _ = TrackPopupMenu(
                        menu,
                        TPM_LEFTALIGN | TPM_BOTTOMALIGN,
                        point.x,
                        point.y,
                        None,
                        hwnd,
                        None,
                    );
                    let _ = DestroyMenu(menu);
                }
                return LRESULT(0);
            }
            WM_DESTROY => {
                let mut nid = NOTIFYICONDATAW::default();
                nid.cbSize = std::mem::size_of::<NOTIFYICONDATAW>() as u32;
                nid.hWnd = hwnd;
                nid.uID = 1;
                let _ = Shell_NotifyIconW(NIM_DELETE, &nid);
                PostQuitMessage(0);
                return LRESULT(0);
            }
            _ => {}
        }
        DefWindowProcW(hwnd, msg, wparam, lparam)
    }

    unsafe {
        let instance = HINSTANCE(GetModuleHandleW(None)?.0);
        let class = w!("BandHeartBeatWindow");
        let wc = WNDCLASSW {
            hInstance: instance,
            lpszClassName: class,
            lpfnWndProc: Some(wnd_proc),
            hCursor: LoadCursorW(None, IDC_ARROW)?,
            hbrBackground: HBRUSH(std::ptr::null_mut()),
            ..Default::default()
        };
        RegisterClassW(&wc);
        let hwnd = CreateWindowExW(
            WS_EX_APPWINDOW,
            class,
            w!("BandHeartBeat"),
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            390,
            260,
            None,
            None,
            Some(instance),
            None,
        )?;
        let mut nid = NOTIFYICONDATAW::default();
        nid.cbSize = std::mem::size_of::<NOTIFYICONDATAW>() as u32;
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid.uCallbackMessage = WM_TRAY;
        nid.hIcon = LoadIconW(None, IDI_APPLICATION)?;
        nid.szTip[..14].copy_from_slice(&[
            'B' as u16, 'a' as u16, 'n' as u16, 'd' as u16, 'H' as u16, 'e' as u16, 'a' as u16,
            'r' as u16, 't' as u16, 'B' as u16, 'e' as u16, 'a' as u16, 't' as u16, 0,
        ]);
        let _ = Shell_NotifyIconW(NIM_ADD, &nid);
        let _ = ShowWindow(hwnd, SW_SHOW);
        let _ = UpdateWindow(hwnd);
        let mut msg = MSG::default();
        while GetMessageW(&mut msg, None, 0, 0).0 > 0 {
            let _ = TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    Ok(())
}

async fn run_ble(tx: mpsc::Sender<UiMessage>, _demo: bool) -> Result<()> {
    let manager = Manager::new()
        .await
        .context("initialise Bluetooth manager")?;
    let adapters = manager
        .adapters()
        .await
        .context("enumerate Bluetooth adapters")?;
    let adapter = adapters
        .into_iter()
        .next()
        .context("no Bluetooth adapter found")?;
    let _ = tx.send(UiMessage::Status(
        "Сканирование HR Data Broadcast…".into(),
        false,
    ));
    adapter
        .start_scan(ScanFilter {
            services: vec![HEART_RATE_SERVICE],
        })
        .await?;
    tokio::time::sleep(Duration::from_secs(8)).await;
    let mut found = None;
    for p in adapter.peripherals().await? {
        if let Some(props) = p.properties().await? {
            if props.services.contains(&HEART_RATE_SERVICE) {
                found = Some(p);
                break;
            }
        }
    }
    adapter.stop_scan().await?;
    let peripheral = found.context("no HR device found; enable HR Data Broadcast")?;
    let _ = tx.send(UiMessage::Status("Подключение к браслету…".into(), false));
    peripheral.connect().await.context("connect to HR device")?;
    peripheral.discover_services().await?;
    let characteristic: Characteristic = peripheral
        .characteristics()
        .into_iter()
        .find(|c| c.uuid == HEART_RATE_MEASUREMENT)
        .context("missing 0x2A37")?;
    peripheral.subscribe(&characteristic).await?;
    let _ = tx.send(UiMessage::Status(
        "Подключено · ожидаем пульс".into(),
        false,
    ));
    let mut notifications = peripheral.notifications().await?;
    while let Ok(Some(ValueNotification { value, .. })) =
        tokio::time::timeout(Duration::from_secs(20), notifications.next()).await
    {
        if let Some(bpm) = parse_bpm(&value) {
            let _ = tx.send(UiMessage::Status("Подключено · прямой эфир".into(), true));
            let _ = tx.send(UiMessage::Bpm(bpm));
        }
    }
    let _ = peripheral.unsubscribe(&characteristic).await;
    let _ = peripheral.disconnect().await;
    anyhow::bail!("HR stream ended or no readings for 20 seconds")
}

fn parse_bpm(data: &[u8]) -> Option<u16> {
    let flags = *data.first()?;
    if flags & 1 == 0 {
        Some(*data.get(1)? as u16)
    } else {
        Some(u16::from_le_bytes([*data.get(1)?, *data.get(2)?]))
    }
}

fn start_worker() {
    use std::sync::atomic::Ordering;
    if WORKING.swap(true, Ordering::AcqRel) {
        return;
    }
    let Some(tx) = UI_TX.get().cloned() else {
        WORKING.store(false, Ordering::Release);
        return;
    };
    thread::spawn(move || {
        let _ = tx.send(UiMessage::Bpm(0));
        if std::env::args().any(|a| a == "--demo") {
            let _ = tx.send(UiMessage::Status("ДЕМО · без браслета".into(), false));
            let _ = tx.send(UiMessage::Bpm(72));
        } else {
            let result = tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
                .map_err(anyhow::Error::from)
                .and_then(|rt| rt.block_on(run_ble(tx.clone(), false)));
            let error = result
                .err()
                .map(|e| e.to_string())
                .unwrap_or_else(|| "Браслет отключён".into());
            let message = if error.contains("no HR device") {
                "Браслет не найден · включите HR Broadcast".to_string()
            } else if error.contains("no Bluetooth adapter") {
                "Bluetooth недоступен · проверьте адаптер".to_string()
            } else {
                format!("Ошибка: {error}")
            };
            let _ = tx.send(UiMessage::Bpm(0));
            let _ = tx.send(UiMessage::Status(message, false));
        }
        WORKING.store(false, Ordering::Release);
    });
}

unsafe fn draw_text(
    hdc: windows::Win32::Graphics::Gdi::HDC,
    x: i32,
    y: i32,
    size: i32,
    weight: i32,
    color: u32,
    text: &str,
) {
    use windows::core::w;
    use windows::Win32::Graphics::Gdi::*;
    let font = CreateFontW(
        -size,
        0,
        0,
        0,
        weight,
        0,
        0,
        0,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        0,
        w!("Segoe UI"),
    );
    let old = SelectObject(hdc, font.into());
    SetTextColor(hdc, windows::Win32::Foundation::COLORREF(color));
    let mut text: Vec<u16> = text.encode_utf16().collect();
    if size <= 16 {
        let mut bounds = windows::Win32::Foundation::RECT {
            left: x,
            top: y,
            right: 352,
            bottom: y + 36,
        };
        DrawTextW(hdc, &mut text, &mut bounds, DT_WORDBREAK | DT_NOPREFIX);
    } else {
        let _ = TextOutW(hdc, x, y, &text);
    }
    SelectObject(hdc, old);
    let _ = DeleteObject(font.into());
}

#[cfg(test)]
mod tests {
    use super::parse_bpm;
    #[test]
    fn decodes_standard_hr_measurements() {
        assert_eq!(parse_bpm(&[0, 72]), Some(72));
        assert_eq!(parse_bpm(&[1, 44, 1]), Some(300));
        assert_eq!(parse_bpm(&[]), None);
        assert_eq!(parse_bpm(&[0]), None);
        assert_eq!(parse_bpm(&[1, 44]), None);
    }
}
