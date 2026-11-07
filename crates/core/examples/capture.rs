//! Opens the first camera, runs the full pipeline for a few seconds and saves
//! the composed 16:9 frame: `cargo run --release --example capture -- out.jpg [camera index]`.
use bhb_core::camera::{self, Camera};
use bhb_core::heart::Heart;
use bhb_core::pipeline::Pipeline;
use bhb_core::resample::fit_into;
use bhb_core::settings::{Fit, Settings};
use std::sync::{Arc, Mutex, RwLock};
use std::time::Duration;

fn main() -> anyhow::Result<()> {
    let out = std::env::args().nth(1).unwrap_or_else(|| "capture.jpg".into());
    let index: usize = std::env::args().nth(2).and_then(|s| s.parse().ok()).unwrap_or(0);
    camera::startup()?;
    let cams = camera::list()?;
    for c in &cams {
        println!("camera: {}", c.name);
    }
    let first = cams.get(index).ok_or_else(|| anyhow::anyhow!("no camera"))?;
    {
        let mut cam = Camera::open(&first.id)?;
        println!("native mode: {:?}", cam.mode);
        let mut frame = vec![0u8; 1280 * 720 * 4];
        for _ in 0..10 {
            cam.read(|src| fit_into(src, &mut frame, 1280, 720, Fit::Contain, false))?;
        }
    }
    let heart = Heart::default();
    heart.push(87);
    let settings = Arc::new(RwLock::new(Settings { camera: Some(first.id.clone()), ..Settings::default() }));
    let last = Arc::new(Mutex::new(Vec::new()));
    let sink = last.clone();
    let pipeline = Pipeline::start(settings, heart.clone(), Box::new(move |jpeg| *sink.lock().unwrap() = jpeg))?;
    for _ in 0..5 {
        std::thread::sleep(Duration::from_secs(1));
        heart.push(87);
        println!("status: {:?}", pipeline.status.lock().unwrap());
    }
    std::fs::write(&out, &*last.lock().unwrap())?;
    
    Ok(())
}
