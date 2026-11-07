//! Head tracking with the built-in Windows face detector, and the smoothing
//! that turns detections into a calm overlay position.

use crate::settings::Placement;
use std::sync::mpsc::{sync_channel, Receiver, SyncSender, TrySendError};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};
use windows::Graphics::Imaging::{BitmapPixelFormat, SoftwareBitmap};
use windows::Media::FaceAnalysis::FaceDetector;
use windows::Security::Cryptography::CryptographicBuffer;

/// Face rectangle normalised to the output frame, 0..1.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct FaceBox {
    pub x: f32,
    pub y: f32,
    pub w: f32,
    pub h: f32,
}

type Latest = Arc<Mutex<Option<(Option<FaceBox>, Instant)>>>;

pub struct Tracker {
    tx: SyncSender<(Vec<u8>, u32, u32)>,
    latest: Latest,
}

impl Tracker {
    pub fn spawn() -> Self {
        let (tx, rx) = sync_channel(1);
        let latest = Latest::default();
        let out = latest.clone();
        std::thread::Builder::new()
            .name("face-detector".into())
            .spawn(move || {
                if let Err(e) = detect_loop(rx, out) {
                    log::warn!("face detector unavailable: {e:#}");
                }
            })
            .expect("spawn face detector");
        Self { tx, latest }
    }

    /// Offers a grayscale frame; dropped if the detector is still busy.
    pub fn submit(&self, gray: Vec<u8>, w: u32, h: u32) {
        if let Err(TrySendError::Disconnected(_)) = self.tx.try_send((gray, w, h)) {
            log::debug!("face detector stopped");
        }
    }

    /// Last detection result (`None` inside = looked, found nothing) and when.
    pub fn latest(&self) -> Option<(Option<FaceBox>, Instant)> {
        *self.latest.lock().unwrap()
    }
}

fn detect_loop(rx: Receiver<(Vec<u8>, u32, u32)>, out: Latest) -> windows::core::Result<()> {
    unsafe {
        let _ = windows::Win32::System::Com::CoInitializeEx(None, windows::Win32::System::Com::COINIT_MULTITHREADED);
    }
    let detector = FaceDetector::CreateAsync()?.get()?;
    for (gray, w, h) in rx {
        let buffer = CryptographicBuffer::CreateFromByteArray(&gray)?;
        let bitmap = SoftwareBitmap::CreateCopyFromBuffer(&buffer, BitmapPixelFormat::Gray8, w as i32, h as i32)?;
        let faces = detector.DetectFacesAsync(&bitmap)?.get()?;
        let best = faces
            .into_iter()
            .filter_map(|f| f.FaceBox().ok())
            .max_by_key(|b| b.Width * b.Height)
            .map(|b| FaceBox {
                x: b.X as f32 / w as f32,
                y: b.Y as f32 / h as f32,
                w: b.Width as f32 / w as f32,
                h: b.Height as f32 / h as f32,
            });
        *out.lock().unwrap() = Some((best, Instant::now()));
    }
    Ok(())
}

/// Keeps the last face for this long before easing back to the anchor.
const HOLD: Duration = Duration::from_secs(2);

/// Smoothly moves the overlay centre (in output pixels) towards its target.
pub struct Follower {
    pos: Option<(f32, f32)>,
    last_face: Option<(FaceBox, Instant)>,
    last_step: Instant,
}

impl Default for Follower {
    fn default() -> Self {
        Self { pos: None, last_face: None, last_step: Instant::now() }
    }
}

pub struct FollowInput {
    pub placement: Placement,
    pub anchor: (f32, f32),
    pub smoothing: f32,
    pub frame: (f32, f32),
    pub overlay: (f32, f32),
}

impl Follower {
    pub fn step(&mut self, detection: Option<(Option<FaceBox>, Instant)>, input: &FollowInput) -> (f32, f32) {
        let now = Instant::now();
        let dt = (now - self.last_step).as_secs_f32().min(0.25);
        self.last_step = now;
        let (fw, fh) = input.frame;
        let (ow, oh) = input.overlay;
        let anchor = (input.anchor.0 * fw, input.anchor.1 * fh);

        if let Some((Some(face), at)) = detection {
            if self.last_face.map_or(true, |(_, t)| at > t) {
                self.last_face = Some((face, at));
            }
        }
        let target = match (input.placement, self.last_face) {
            (Placement::Track, Some((face, at))) if now - at < HOLD => beside(&face, fw, fh, ow, oh),
            _ => anchor,
        };
        let target = clamp_inside(target, fw, fh, ow, oh);

        let pos = match (input.placement, self.pos) {
            (Placement::Fixed, _) | (_, None) => target,
            (Placement::Track, Some((x, y))) => {
                let tau = 0.04 + input.smoothing * 0.5;
                let k = 1.0 - (-dt / tau).exp();
                (x + (target.0 - x) * k, y + (target.1 - y) * k)
            }
        };
        self.pos = Some(pos);
        pos
    }
}

/// Overlay centre just right of the head at eye level, or left when there is no room.
fn beside(face: &FaceBox, fw: f32, fh: f32, ow: f32, oh: f32) -> (f32, f32) {
    let gap = face.w * fw * 0.15;
    let y = (face.y + face.h * 0.3) * fh;
    let right = (face.x + face.w) * fw + gap + ow / 2.0;
    if right + ow / 2.0 <= fw {
        (right, y)
    } else {
        (face.x * fw - gap - ow / 2.0, y.max(oh / 2.0))
    }
}

fn clamp_inside((x, y): (f32, f32), fw: f32, fh: f32, ow: f32, oh: f32) -> (f32, f32) {
    let m = 12.0;
    (
        x.clamp(ow / 2.0 + m, (fw - ow / 2.0 - m).max(ow / 2.0 + m)),
        y.clamp(oh / 2.0 + m, (fh - oh / 2.0 - m).max(oh / 2.0 + m)),
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    fn input(placement: Placement) -> FollowInput {
        FollowInput { placement, anchor: (0.8, 0.2), smoothing: 0.0, frame: (1280.0, 720.0), overlay: (200.0, 80.0) }
    }

    #[test]
    fn places_right_of_face_and_falls_back_to_anchor() {
        let mut f = Follower::default();
        let face = FaceBox { x: 0.4, y: 0.2, w: 0.2, h: 0.35 };
        let (x, _) = f.step(Some((Some(face), Instant::now())), &input(Placement::Track));
        assert!(x > 0.6 * 1280.0);
        let mut f = Follower::default();
        let (x, y) = f.step(None, &input(Placement::Track));
        assert_eq!((x, y), (1024.0, 144.0));
    }

    #[test]
    fn switches_side_near_right_edge_and_stays_inside() {
        let mut f = Follower::default();
        let face = FaceBox { x: 0.75, y: 0.3, w: 0.2, h: 0.3 };
        let (x, _) = f.step(Some((Some(face), Instant::now())), &input(Placement::Track));
        assert!(x < 0.75 * 1280.0);
        let (x, _) = f.step(None, &input(Placement::Fixed));
        assert!(x + 100.0 <= 1280.0);
    }
}
