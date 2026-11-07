//! Renders every overlay style onto a test frame: `cargo run --example badges -- out.png`.
use bhb_core::overlay::{Frame, Renderer};
use bhb_core::settings::OverlayStyle::*;
use tiny_skia::{Pixmap, PixmapPaint, Transform};

fn main() {
    let out = std::env::args().nth(1).unwrap_or_else(|| "badges.png".into());
    let r = Renderer::new().unwrap();
    let styles = [Capsule, Heart, Ecg, Ring, Minimal, Card];
    let mut sheet = Pixmap::new(6 * 240, 3 * 160).unwrap();
    sheet.fill(tiny_skia::Color::from_rgba8(70, 78, 92, 255));
    let history: Vec<u16> = (0..40).map(|i| 80 + (i % 9) as u16 * 2).collect();
    for (col, style) in styles.into_iter().enumerate() {
        for (row, (bpm, scale)) in [(Some(87), 1.0), (None, 1.0), (Some(142), 0.7)].into_iter().enumerate() {
            let badge = r.render(&Frame { bpm, history: &history, style, scale, animate: true, time: 0.05 });
            sheet.draw_pixmap(col as i32 * 240 + 20, row as i32 * 160 + 16, badge.as_ref(), &PixmapPaint::default(), Transform::identity(), None);
        }
    }
    sheet.save_png(out).unwrap();
}
