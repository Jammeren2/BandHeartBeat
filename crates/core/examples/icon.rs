//! Draws the 1024px app icon source: `cargo run --example icon -- icon.png`,
//! then `npx tauri icon icon.png` inside `app/`.
use tiny_skia::*;

fn main() {
    let out = std::env::args().nth(1).unwrap_or_else(|| "icon.png".into());
    let mut pm = Pixmap::new(1024, 1024).unwrap();
    let bg = PathBuilder::from_rect(Rect::from_xywh(64.0, 64.0, 896.0, 896.0).unwrap());
    let mut paint = Paint::default();
    paint.anti_alias = true;
    paint.shader = LinearGradient::new(
        Point::from_xy(64.0, 64.0),
        Point::from_xy(960.0, 960.0),
        vec![
            GradientStop::new(0.0, Color::from_rgba8(0xFF, 0x8A, 0x9B, 255)),
            GradientStop::new(1.0, Color::from_rgba8(0xE0, 0x3E, 0x5C, 255)),
        ],
        SpreadMode::Pad,
        Transform::identity(),
    )
    .unwrap();
    // Rounded square via a clip mask.
    let mut mask = Mask::new(1024, 1024).unwrap();
    let r = 220.0;
    let mut pb = PathBuilder::new();
    pb.push_rect(Rect::from_xywh(64.0 + r, 64.0, 896.0 - 2.0 * r, 896.0).unwrap());
    pb.push_rect(Rect::from_xywh(64.0, 64.0 + r, 896.0, 896.0 - 2.0 * r).unwrap());
    for (cx, cy) in [(64.0 + r, 64.0 + r), (960.0 - r, 64.0 + r), (64.0 + r, 960.0 - r), (960.0 - r, 960.0 - r)] {
        pb.push_circle(cx, cy, r);
    }
    mask.fill_path(&pb.finish().unwrap(), FillRule::Winding, true, Transform::identity());
    pm.fill_path(&bg, &paint, FillRule::Winding, Transform::identity(), Some(&mask));

    // Heart, from the same 24-unit outline the overlay uses.
    let k = 26.0;
    let p = |x: f32, y: f32| (512.0 + (x - 12.0) * k, 540.0 + (y - 12.5) * k);
    let mut hb = PathBuilder::new();
    let m = p(12.0, 21.0);
    hb.move_to(m.0, m.1);
    for [a, b, c] in [
        [(12.0, 21.0), (4.5, 16.4), (2.4, 11.8)],
        [(0.9, 8.5), (3.0, 4.5), (6.8, 4.5)],
        [(8.9, 4.5), (10.3, 5.6), (12.0, 7.5)],
        [(13.7, 5.6), (15.1, 4.5), (17.2, 4.5)],
        [(21.0, 4.5), (23.1, 8.5), (21.6, 11.8)],
        [(19.5, 16.4), (12.0, 21.0), (12.0, 21.0)],
    ] {
        let (a, b, c) = (p(a.0, a.1), p(b.0, b.1), p(c.0, c.1));
        hb.cubic_to(a.0, a.1, b.0, b.1, c.0, c.1);
    }
    hb.close();
    let heart = hb.finish().unwrap();
    let mut white = Paint::default();
    white.anti_alias = true;
    white.set_color(Color::WHITE);
    pm.fill_path(&heart, &white, FillRule::Winding, Transform::identity(), None);
    // ECG stroke across the heart.
    let mut ecg = PathBuilder::new();
    for (i, (x, y)) in [(300.0, 520.0), (430.0, 520.0), (470.0, 440.0), (530.0, 620.0), (570.0, 520.0), (724.0, 520.0)].iter().enumerate() {
        if i == 0 { ecg.move_to(*x, *y) } else { ecg.line_to(*x, *y) }
    }
    let mut coral = Paint::default();
    coral.anti_alias = true;
    coral.set_color(Color::from_rgba8(0xE0, 0x3E, 0x5C, 255));
    let stroke = Stroke { width: 34.0, line_cap: LineCap::Round, line_join: LineJoin::Round, ..Default::default() };
    pm.stroke_path(&ecg.finish().unwrap(), &coral, &stroke, Transform::identity(), None);
    pm.save_png(out).unwrap();
}
