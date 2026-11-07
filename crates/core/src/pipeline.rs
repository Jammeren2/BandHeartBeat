//! The frame loop: camera → 16:9 output → head tracking → overlay → virtual
//! camera, plus a downscaled JPEG preview for the UI.

use crate::camera::{self, Camera, Mode};
use crate::frame::FrameWriter;
use crate::heart::Heart;
use crate::overlay::{blend_onto, Frame, Renderer};
use crate::resample::{fit_into, gray_downscale, half_rgb};
use crate::settings::Settings;
use crate::tracking::{FollowInput, Follower, Tracker};
use crate::{OUT_H, OUT_W};
use anyhow::Result;
use serde::Serialize;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, RwLock};
use std::thread::JoinHandle;
use std::time::{Duration, Instant};

#[derive(Clone, Debug, Default, Serialize)]
pub struct Status {
    pub camera: Option<String>,
    pub mode: Option<Mode>,
    pub fps: f32,
    pub face: bool,
    /// Latest face box (x, y, w, h) in 0..1 output coordinates, for the UI.
    pub face_box: Option<[f32; 4]>,
    pub error: Option<String>,
}

pub type Preview = Box<dyn Fn(Vec<u8>) + Send>;

pub struct Pipeline {
    stop: Arc<AtomicBool>,
    thread: Option<JoinHandle<()>>,
    pub status: Arc<Mutex<Status>>,
}

const TRACK_EVERY: u32 = 3;
const PREVIEW_EVERY: u32 = 2;
const NO_CAMERA_INTERVAL: Duration = Duration::from_millis(66);
const REOPEN_DELAY: Duration = Duration::from_secs(2);

impl Pipeline {
    pub fn start(settings: Arc<RwLock<Settings>>, heart: Heart, preview: Preview) -> Result<Self> {
        let writer = FrameWriter::create(OUT_W, OUT_H)?;
        let renderer = Renderer::new()?;
        let stop = Arc::new(AtomicBool::new(false));
        let status = Arc::new(Mutex::new(Status::default()));
        let worker = Worker {
            settings,
            heart,
            preview,
            writer,
            renderer,
            status: status.clone(),
            stop: stop.clone(),
        };
        let thread = std::thread::Builder::new().name("pipeline".into()).spawn(move || worker.run())?;
        Ok(Self { stop, thread: Some(thread), status })
    }
}

impl Drop for Pipeline {
    fn drop(&mut self) {
        self.stop.store(true, Ordering::Relaxed);
        if let Some(t) = self.thread.take() {
            let _ = t.join();
        }
    }
}

struct Worker {
    settings: Arc<RwLock<Settings>>,
    heart: Heart,
    preview: Preview,
    writer: FrameWriter,
    renderer: Renderer,
    status: Arc<Mutex<Status>>,
    stop: Arc<AtomicBool>,
}

impl Worker {
    fn run(mut self) {
        unsafe {
            let _ = windows::Win32::System::Com::CoInitializeEx(None, windows::Win32::System::Com::COINIT_MULTITHREADED);
        }
        if let Err(e) = camera::startup() {
            self.status.lock().unwrap().error = Some(format!("{e:#}"));
        }
        let tracker = Tracker::spawn();
        let mut follower = Follower::default();
        let mut frame = vec![0u8; (OUT_W * OUT_H * 4) as usize];
        let idle = idle_background();
        let mut cam: Option<(String, Camera)> = None;
        let mut retry_at = Instant::now();
        let started = Instant::now();
        let mut n: u32 = 0;
        let mut fps_window = (Instant::now(), 0u32);

        while !self.stop.load(Ordering::Relaxed) {
            let s = self.settings.read().unwrap().clone();

            if cam.as_ref().map(|(id, _)| id) != s.camera.as_ref() {
                cam = None;
                self.set_camera(None, None);
                if let Some(id) = &s.camera {
                    if Instant::now() >= retry_at {
                        match Camera::open(id) {
                            Ok(c) => {
                                self.set_camera(Some(id.clone()), Some(c.mode));
                                cam = Some((id.clone(), c));
                            }
                            Err(e) => {
                                self.fail(e);
                                retry_at = Instant::now() + REOPEN_DELAY;
                            }
                        }
                    }
                }
            }

            let got_frame = match &mut cam {
                Some((_, c)) => match c.read(|src| fit_into(src, &mut frame, OUT_W, OUT_H, s.fit, s.mirror)) {
                    Ok(r) => r.is_some(),
                    Err(e) => {
                        self.fail(e);
                        cam = None;
                        retry_at = Instant::now() + REOPEN_DELAY;
                        continue;
                    }
                },
                None => {
                    std::thread::sleep(NO_CAMERA_INTERVAL);
                    frame.copy_from_slice(&idle);
                    true
                }
            };
            if !got_frame {
                continue;
            }
            n = n.wrapping_add(1);

            let tracking = cam.is_some() && s.placement == crate::settings::Placement::Track;
            if tracking && n % TRACK_EVERY == 0 {
                let (gray, w, h) = gray_downscale(&frame, OUT_W, OUT_H, 4);
                tracker.submit(gray, w, h);
            }
            let detection = if tracking { tracker.latest() } else { None };

            let bpm = self.heart.bpm();
            let history = self.heart.history();
            let badge = self.renderer.render(&Frame {
                bpm,
                history: &history,
                style: s.style,
                scale: s.size,
                animate: s.pulse_animation,
                background: s.badge_background,
                time: started.elapsed().as_secs_f32(),
            });
            let (cx, cy) = follower.step(
                detection,
                &FollowInput {
                    placement: s.placement,
                    anchor: s.anchor,
                    smoothing: s.smoothing,
                    frame: (OUT_W as f32, OUT_H as f32),
                    overlay: (badge.width() as f32, badge.height() as f32),
                    face_point: s.face_point,
                    point_offset: s.point_offset,
                },
            );
            blend_onto(&mut frame, OUT_W, OUT_H, &badge, cx, cy);
            self.writer.write(&frame);

            if n % PREVIEW_EVERY == 0 {
                let (rgb, w, h) = half_rgb(&frame, OUT_W, OUT_H);
                let mut jpeg = Vec::with_capacity(64 * 1024);
                let enc = jpeg_encoder::Encoder::new(&mut jpeg, 78);
                if enc.encode(&rgb, w as u16, h as u16, jpeg_encoder::ColorType::Rgb).is_ok() {
                    (self.preview)(jpeg);
                }
            }

            {
                let fresh = detection.and_then(|(f, at)| f.filter(|_| at.elapsed() < Duration::from_secs(1)));
                self.status.lock().unwrap().face_box = fresh.map(|f| [f.x, f.y, f.w, f.h]);
            }
            fps_window.1 += 1;
            let elapsed = fps_window.0.elapsed();
            if elapsed >= Duration::from_secs(1) {
                let mut st = self.status.lock().unwrap();
                st.fps = if cam.is_some() { fps_window.1 as f32 / elapsed.as_secs_f32() } else { 0.0 };
                st.face = detection.is_some_and(|(f, at)| f.is_some() && at.elapsed() < Duration::from_secs(1));
                fps_window = (Instant::now(), 0);
            }
        }
    }

    fn set_camera(&self, id: Option<String>, mode: Option<Mode>) {
        let mut st = self.status.lock().unwrap();
        st.camera = id;
        st.mode = mode;
        if mode.is_some() {
            st.error = None;
        }
    }

    fn fail(&self, e: anyhow::Error) {
        log::warn!("camera: {e:#}");
        self.status.lock().unwrap().error = Some(format!("{e:#}"));
    }
}

/// Frame shown while no camera runs: a dark vertical gradient.
fn idle_background() -> Vec<u8> {
    let mut v = vec![0u8; (OUT_W * OUT_H * 4) as usize];
    for y in 0..OUT_H {
        let t = y as f32 / OUT_H as f32;
        let px = [(28.0 - 12.0 * t) as u8, (22.0 - 10.0 * t) as u8, (26.0 - 12.0 * t) as u8, 255];
        for x in 0..OUT_W {
            let o = ((y * OUT_W + x) * 4) as usize;
            v[o..o + 4].copy_from_slice(&px);
        }
    }
    v
}
