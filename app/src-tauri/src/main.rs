#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use bhb_core::ble::{Ble, BleConfig};
use bhb_core::camera::{self, CameraInfo};
use bhb_core::heart::{Heart, Link};
use bhb_core::pipeline::{self, Pipeline};
use bhb_core::settings::Settings;
use bhb_core::vcam;
use serde::Serialize;
use std::path::PathBuf;
use std::sync::{Arc, Mutex, RwLock};
use tauri::ipc::{Channel, InvokeResponseBody};
use tauri::menu::{Menu, MenuItem};
use tauri::tray::{MouseButton, MouseButtonState, TrayIconBuilder, TrayIconEvent};
use tauri::{Manager, State, WindowEvent};

type PreviewSink = Arc<Mutex<Option<Channel<InvokeResponseBody>>>>;

struct App {
    settings: Arc<RwLock<Settings>>,
    heart: Heart,
    ble: Ble,
    pipeline: Pipeline,
    preview: PreviewSink,
    dll: PathBuf,
    _runtime: tokio::runtime::Runtime,
}

#[derive(Serialize)]
struct Snapshot {
    settings: Settings,
    link: Link,
    bpm: Option<u16>,
    camera: pipeline::Status,
    vcam: vcam::Status,
}

type CmdResult<T> = Result<T, String>;

fn err(e: impl std::fmt::Display) -> String {
    e.to_string()
}

#[tauri::command]
fn snapshot(app: State<App>) -> Snapshot {
    Snapshot {
        settings: app.settings.read().unwrap().clone(),
        link: app.heart.link(),
        bpm: app.heart.bpm(),
        camera: app.pipeline.status.lock().unwrap().clone(),
        vcam: vcam::status(&app.dll),
    }
}

#[tauri::command]
fn cameras() -> CmdResult<Vec<CameraInfo>> {
    camera::list().map_err(err)
}

/// Replaces the settings; they are saved immediately and applied live.
#[tauri::command]
fn update_settings(app: State<App>, settings: Settings) -> CmdResult<Settings> {
    let settings = settings.clamped();
    let ble_changed = {
        let mut current = app.settings.write().unwrap();
        let changed = current.auto_scan != settings.auto_scan || current.last_device != settings.last_device;
        *current = settings.clone();
        changed
    };
    if ble_changed {
        app.ble.configure(BleConfig { enabled: settings.auto_scan, preferred: settings.last_device.clone() });
    }
    settings.save().map_err(err)?;
    Ok(settings)
}

#[tauri::command]
fn retry_watch(app: State<App>) {
    app.ble.retry();
}

#[tauri::command]
fn install_camera(app: State<App>) -> CmdResult<vcam::Status> {
    vcam::install(&app.dll).map_err(err)?;
    Ok(vcam::status(&app.dll))
}

#[tauri::command]
fn remove_camera(app: State<App>) -> CmdResult<vcam::Status> {
    vcam::remove().map_err(err)?;
    Ok(vcam::status(&app.dll))
}

/// The UI passes a channel that receives JPEG preview frames.
#[tauri::command]
fn watch_preview(app: State<App>, channel: Channel<InvokeResponseBody>) {
    *app.preview.lock().unwrap() = Some(channel);
}

fn camera_dll(handle: &tauri::AppHandle) -> PathBuf {
    let name = "BandHeartBeatCamera.dll";
    let bundled = handle.path().resource_dir().map(|d| d.join(name)).ok();
    match bundled {
        Some(p) if p.exists() => p,
        // `tauri dev` runs from target/: fall back to the DLL build output.
        _ => PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../vcam/out").join(name),
    }
}

fn start(handle: &tauri::AppHandle) -> anyhow::Result<App> {
    let settings = Settings::load();
    let shared = Arc::new(RwLock::new(settings.clone()));
    let heart = Heart::default();
    let runtime = tokio::runtime::Runtime::new()?;

    let remember = shared.clone();
    let ble = Ble::spawn(
        runtime.handle(),
        heart.clone(),
        BleConfig { enabled: settings.auto_scan, preferred: settings.last_device.clone() },
        move |address| {
            let mut s = remember.write().unwrap();
            if s.last_device.as_deref() != Some(address.as_str()) {
                s.last_device = Some(address);
                let _ = s.save();
            }
        },
    );

    let preview = PreviewSink::default();
    let sink = preview.clone();
    let pipeline = Pipeline::start(
        shared.clone(),
        heart.clone(),
        Box::new(move |jpeg| {
            if let Some(ch) = sink.lock().unwrap().as_ref() {
                let _ = ch.send(InvokeResponseBody::Raw(jpeg));
            }
        }),
    )?;

    // Keep the per-user registration pointing at this copy of the DLL.
    let bundled = camera_dll(handle);
    let dll = vcam::stage(&bundled).unwrap_or_else(|e| {
        eprintln!("camera DLL copy failed, registering it in place: {e:#}");
        bundled
    });
    if let Err(e) = vcam::remove_legacy() {
        eprintln!("old camera removal failed: {e:#}");
    }
    if vcam::status(&dll) != vcam::Status::Installed {
        if let Err(e) = vcam::install(&dll) {
            eprintln!("virtual camera registration failed: {e:#}");
        }
    }

    Ok(App { settings: shared, heart, ble, pipeline, preview, dll, _runtime: runtime })
}

fn show(handle: &tauri::AppHandle) {
    if let Some(w) = handle.get_webview_window("main") {
        let _ = w.show();
        let _ = w.unminimize();
        let _ = w.set_focus();
    }
}

fn main() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| {
            let state = start(app.handle())?;
            app.manage(state);

            let open = MenuItem::with_id(app, "open", "Открыть", true, None::<&str>)?;
            let quit = MenuItem::with_id(app, "quit", "Выйти", true, None::<&str>)?;
            TrayIconBuilder::new()
                .icon(app.default_window_icon().cloned().expect("bundled icon"))
                .tooltip("BandHeartBeat")
                .menu(&Menu::with_items(app, &[&open, &quit])?)
                .show_menu_on_left_click(false)
                .on_menu_event(|app, event| match event.id.as_ref() {
                    "open" => show(app),
                    "quit" => app.exit(0),
                    _ => {}
                })
                .on_tray_icon_event(|tray, event| {
                    if let TrayIconEvent::Click { button: MouseButton::Left, button_state: MouseButtonState::Up, .. } = event {
                        show(tray.app_handle());
                    }
                })
                .build(app)?;
            Ok(())
        })
        // Closing the window keeps the camera running; quit from the tray.
        .on_window_event(|window, event| {
            if let WindowEvent::CloseRequested { api, .. } = event {
                api.prevent_close();
                let _ = window.hide();
            }
        })
        .invoke_handler(tauri::generate_handler![
            snapshot,
            cameras,
            update_settings,
            retry_watch,
            install_camera,
            remove_camera,
            watch_preview
        ])
        .run(tauri::generate_context!())
        .expect("error while running BandHeartBeat");
}
