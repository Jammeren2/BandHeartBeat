#![windows_subsystem = "windows"]
mod overlay;
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
static CAMERA_DIRTY: std::sync::atomic::AtomicBool = std::sync::atomic::AtomicBool::new(false);
static CAMERA: Mutex<Option<std::process::Child>> = Mutex::new(None);
static CAMERA_STATUS: Mutex<String> = Mutex::new(String::new());
static CAMERA_NAMES: Mutex<Vec<(usize, String)>> = Mutex::new(Vec::new());
static CAMERA_ERROR: Mutex<String> = Mutex::new(String::new());
const WM_TRAY: u32 = windows::Win32::UI::WindowsAndMessaging::WM_APP + 1;
const ID_TRAY_SHOW: usize = 1001;
const ID_TRAY_RETRY: usize = 1002;
const ID_TRAY_QUIT: usize = 1003;
const ID_TRAY_OVERLAY: usize = 1004;
const ID_CAMERA_START: usize = 1010;
const ID_CAMERA_STOP: usize = 1011;
const ID_CAMERA_DEMO: usize = 1012;
const ID_CAMERA_REGISTER: usize = 1013;
const ID_CAMERA_REMOVE: usize = 1014;

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
    let overlay_error = overlay::start().err();
    start_worker();
    camera_list();

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
                let mut changed = CAMERA_DIRTY.swap(false, std::sync::atomic::Ordering::AcqRel);
                if let Ok(mut camera) = CAMERA.lock() {
                    if let Some(child) = camera.as_mut() {
                        if let Ok(Some(code)) = child.try_wait() {
                            if let Ok(mut status) = CAMERA_STATUS.lock() {
                                *status = if code.success() {
                                    "Камера остановлена".into()
                                } else {
                                    let detail =
                                        CAMERA_ERROR.lock().map(|e| e.clone()).unwrap_or_default();
                                    format!("Ошибка камеры ({code}): {detail}")
                                };
                            }
                            *camera = None;
                            changed = true;
                        }
                    }
                }
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
                    if let Ok(status) = CAMERA_STATUS.lock() {
                        draw_text(hdc, 26, 210, 13, 400, 0x00a8adb8, &status);
                    }
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
                        let _ = ShowWindow(hwnd, SW_RESTORE);
                        let _ = SetForegroundWindow(hwnd);
                    }
                    ID_TRAY_RETRY => {
                        start_worker();
                    }
                    ID_TRAY_OVERLAY => {
                        let _ = windows::Win32::UI::Shell::ShellExecuteW(
                            Some(hwnd),
                            w!("open"),
                            w!("http://127.0.0.1:8765/overlay"),
                            None,
                            None,
                            SW_SHOWNORMAL,
                        );
                    }
                    ID_TRAY_QUIT => {
                        let _ = DestroyWindow(hwnd);
                    }
                    ID_CAMERA_START => camera_list(),
                    ID_CAMERA_DEMO => camera_start(0, true),
                    ID_CAMERA_STOP => camera_stop(),
                    ID_CAMERA_REGISTER => camera_admin("--register"),
                    ID_CAMERA_REMOVE => camera_admin("--unregister"),
                    id if (1100..1200).contains(&id) => camera_start(id - 1100, false),
                    _ => {}
                }
                return LRESULT(0);
            }
            WM_TRAY => {
                let event = (lparam.0 & 0xffff) as u32;
                if event == WM_LBUTTONUP {
                    let _ = ShowWindow(hwnd, SW_RESTORE);
                    let _ = SetForegroundWindow(hwnd);
                } else if event == WM_RBUTTONUP {
                    let Ok(menu) = CreatePopupMenu() else {
                        return LRESULT(0);
                    };
                    let _ = AppendMenuW(menu, MF_STRING, ID_TRAY_SHOW, w!("Показать"));
                    if let Ok(cameras) = CreatePopupMenu() {
                        if let Ok(names) = CAMERA_NAMES.lock() {
                            for (index, name) in names.iter().take(100) {
                                let text: Vec<u16> = name.encode_utf16().chain(Some(0)).collect();
                                let _ = AppendMenuW(
                                    cameras,
                                    MF_STRING,
                                    1100 + index,
                                    PCWSTR(text.as_ptr()),
                                );
                            }
                            if names.is_empty() {
                                let _ = AppendMenuW(
                                    cameras,
                                    MF_STRING | MF_GRAYED,
                                    0,
                                    w!("Нет доступных камер"),
                                );
                            }
                        }
                        let _ = AppendMenuW(
                            cameras,
                            MF_STRING,
                            ID_CAMERA_START,
                            w!("Обновить список камер"),
                        );
                        let _ = AppendMenuW(
                            menu,
                            MF_POPUP,
                            cameras.0 as usize,
                            w!("Камера: выбрать и запустить"),
                        );
                    }
                    let _ = AppendMenuW(
                        menu,
                        MF_STRING,
                        ID_CAMERA_DEMO,
                        w!("Камера: демо без веб-камеры"),
                    );
                    let _ = AppendMenuW(menu, MF_STRING, ID_CAMERA_STOP, w!("Камера: остановить"));
                    let _ = AppendMenuW(
                        menu,
                        MF_STRING,
                        ID_CAMERA_REGISTER,
                        w!("Установить виртуальную камеру (текущий пользователь)"),
                    );
                    let _ = AppendMenuW(
                        menu,
                        MF_STRING,
                        ID_CAMERA_REMOVE,
                        w!("Удалить виртуальную камеру"),
                    );
                    let _ =
                        AppendMenuW(menu, MF_STRING, ID_TRAY_OVERLAY, w!("Открыть оверлей OBS"));
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
                camera_stop();
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
            300,
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
        if let Some(error) = overlay_error {
            let message: Vec<u16> = format!("Оверлей недоступен: порт 127.0.0.1:8765 занят или заблокирован. Закройте другой экземпляр и перезапустите приложение.\n{error}\0").encode_utf16().collect();
            MessageBoxW(
                Some(hwnd),
                PCWSTR(message.as_ptr()),
                w!("BandHeartBeat · OBS"),
                MB_OK | MB_ICONWARNING,
            );
        }
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

fn camera_list() {
    use std::os::windows::process::CommandExt;
    std::thread::spawn(|| {
        let result = std::env::current_exe().and_then(|exe| {
            std::process::Command::new(exe.with_file_name("camera-producer.exe"))
                .arg("--list-machine")
                .creation_flags(0x08000000)
                .output()
        });
        if let Ok(output) = result {
            let names = String::from_utf8_lossy(&output.stdout)
                .lines()
                .filter_map(|line| {
                    let (index, name) = line.split_once('\t')?;
                    Some((index.parse::<usize>().ok()?, name.to_owned()))
                })
                .collect();
            if let Ok(mut list) = CAMERA_NAMES.lock() {
                *list = names;
            }
            CAMERA_DIRTY.store(true, std::sync::atomic::Ordering::Release);
        }
    });
}

fn camera_start(index: usize, demo: bool) {
    CAMERA_DIRTY.store(true, std::sync::atomic::Ordering::Release);
    use std::os::windows::process::CommandExt;
    let Ok(mut camera) = CAMERA.lock() else {
        return;
    };
    if camera.is_some() {
        return;
    }
    let result = std::env::current_exe().and_then(|exe| {
        let mut command = std::process::Command::new(exe.with_file_name("camera-producer.exe"));
        if demo {
            command.arg("--mock");
        } else {
            command.args(["--camera", &index.to_string()]);
        }
        command
            .stdout(std::process::Stdio::null())
            .stderr(std::process::Stdio::piped())
            .creation_flags(0x08000000)
            .spawn()
    });
    if let Ok(mut status) = CAMERA_STATUS.lock() {
        match result {
            Ok(mut child) => {
                if let Ok(mut error) = CAMERA_ERROR.lock() {
                    error.clear();
                }
                if let Some(mut stderr) = child.stderr.take() {
                    std::thread::spawn(move || {
                        use std::io::Read;
                        let mut buffer = [0; 512];
                        while let Ok(n) = stderr.read(&mut buffer) {
                            if n == 0 {
                                break;
                            }
                            if let Ok(mut error) = CAMERA_ERROR.lock() {
                                *error = String::from_utf8_lossy(&buffer[..n]).into_owned();
                            }
                        }
                    });
                }
                *camera = Some(child);
                *status = if demo {
                    "Камера: ДЕМО без веб-камеры".into()
                } else {
                    "Камера запущена".into()
                };
            }
            Err(error) => *status = format!("Не удалось запустить camera-producer.exe: {error}"),
        }
    }
}

fn camera_stop() {
    use std::os::windows::process::CommandExt;
    if let Ok(exe) = std::env::current_exe() {
        let _ = std::process::Command::new(exe.with_file_name("camera-producer.exe"))
            .arg("--stop")
            .creation_flags(0x08000000)
            .spawn();
    }
}

fn camera_admin(arg: &str) {
    use std::os::windows::process::CommandExt;
    let arg = arg.to_owned();
    std::thread::spawn(move || {
        let result = std::env::current_exe().and_then(|exe| {
            std::process::Command::new(exe.with_file_name("camera-admin.exe"))
                .arg(&arg)
                .creation_flags(0x08000000)
                .status()
        });
        let text = match result {
            Ok(code) if code.success() => "Операция виртуальной камеры выполнена".into(),
            Ok(code) => {
                format!("Ошибка регистрации ({code}). Запустите camera-admin.exe в консоли.")
            }
            Err(error) => format!("camera-admin.exe: {error}"),
        };
        if let Ok(mut status) = CAMERA_STATUS.lock() {
            *status = text;
            CAMERA_DIRTY.store(true, std::sync::atomic::Ordering::Release);
        }
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
    if text.is_empty() {
        return;
    }
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
