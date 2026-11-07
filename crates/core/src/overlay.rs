//! Heart-rate badge rendering. Drawn styles, SVG anime mascots and the user's
//! own image, all rendered with tiny-skia into a small premultiplied RGBA
//! pixmap that is then blended onto the BGRA frame.

use crate::girls::girl;
use crate::settings::OverlayStyle;
use ab_glyph::{Font, FontVec, PxScale, ScaleFont};
use anyhow::{Context, Result};
use std::cell::RefCell;
use std::collections::HashMap;
use tiny_skia::{
    Color, FillRule, LineCap, LineJoin, Paint, Path, PathBuilder, Pixmap, PixmapPaint, Stroke, Transform,
};

/// An anime mascot: SVG art in a 240x200 viewBox plus where the number goes.
struct Mascot {
    svg: &'static str,
    /// Number anchor in viewBox units; `center` aligns it horizontally on x.
    number: (f32, f32),
    center: bool,
    size: f32,
}

fn mascot(style: OverlayStyle) -> Option<Mascot> {
    Some(match style {
        OverlayStyle::Bunny => Mascot { svg: include_str!("../assets/mascots/bunny.svg"), number: (176.0, 145.0), center: true, size: 46.0 },
        OverlayStyle::Neko => Mascot { svg: include_str!("../assets/mascots/neko.svg"), number: (76.0, 180.0), center: false, size: 42.0 },
        OverlayStyle::Angel => Mascot { svg: include_str!("../assets/mascots/angel.svg"), number: (120.0, 166.0), center: true, size: 46.0 },
        _ => return None,
    })
}

/// Mascot art is 240x200 units drawn at this many pixels per unit at scale 1.
const MASCOT_UNIT: f32 = 0.75;
/// Anime girl badges are square, this many pixels at scale 1.
const GIRL_SIZE: f32 = 190.0;
/// Box the custom image is fitted into at scale 1; the number sits below it.
const CUSTOM_ART: (f32, f32) = (150.0, 130.0);

/// Rasterises SVG data, aspect-fitted and centred in a `w`x`h` pixmap.
fn rasterize_svg(data: &[u8], w: u32, h: u32) -> Option<Pixmap> {
    let tree = resvg::usvg::Tree::from_data(data, &resvg::usvg::Options::default()).ok()?;
    let size = tree.size();
    let mut pm = Pixmap::new(w.max(1), h.max(1))?;
    let k = (w as f32 / size.width()).min(h as f32 / size.height());
    let (tx, ty) = ((w as f32 - size.width() * k) / 2.0, (h as f32 - size.height() * k) / 2.0);
    resvg::render(&tree, Transform::from_row(k, 0.0, 0.0, k, tx, ty), &mut pm.as_mut());
    Some(pm)
}

/// Loads a PNG or SVG file fitted into `w`x`h`.
fn load_image(path: &str, w: u32, h: u32) -> Option<Pixmap> {
    let bytes = std::fs::read(path).ok()?;
    if path.to_lowercase().ends_with(".svg") {
        return rasterize_svg(&bytes, w, h);
    }
    let img = Pixmap::decode_png(&bytes).ok()?;
    let mut pm = Pixmap::new(w.max(1), h.max(1))?;
    let k = (w as f32 / img.width() as f32).min(h as f32 / img.height() as f32);
    let (tx, ty) = ((w as f32 - img.width() as f32 * k) / 2.0, (h as f32 - img.height() as f32 * k) / 2.0);
    let paint = PixmapPaint { quality: tiny_skia::FilterQuality::Bicubic, ..Default::default() };
    pm.draw_pixmap(0, 0, img.as_ref(), &paint, Transform::from_row(k, 0.0, 0.0, k, tx, ty), None);
    Some(pm)
}

fn coral() -> Color {
    rgb(0xFF, 0x6B, 0x6B)
}
fn teal() -> Color {
    rgb(0x2D, 0xD4, 0xBF)
}
fn white() -> Color {
    rgb(0xFF, 0xFF, 0xFF)
}
fn muted() -> Color {
    rgb(0x9A, 0xA3, 0xAF)
}
fn ink() -> Color {
    rgb(0x1B, 0x1D, 0x22)
}

fn rgb(r: u8, g: u8, b: u8) -> Color {
    Color::from_rgba8(r, g, b, 255)
}

fn alpha(c: Color, a: f32) -> Color {
    let mut c = c;
    c.set_alpha(a);
    c
}

pub struct Frame<'a> {
    pub bpm: Option<u16>,
    /// Last minute of readings, oldest first (card style).
    pub history: &'a [u16],
    pub style: OverlayStyle,
    pub scale: f32,
    pub animate: bool,
    /// Plate behind capsule, ECG and card; without it text gets a shadow.
    pub background: bool,
    /// Seconds since start; drives the beat and the ECG sweep.
    pub time: f32,
    /// PNG/SVG path for the custom style.
    pub custom_image: Option<&'a str>,
}

pub struct Renderer {
    bold: FontVec,
    mono: FontVec,
    /// Rasterised art by "source@size"; `None` remembers files that failed to load.
    art: RefCell<HashMap<String, Option<Pixmap>>>,
}

fn system_font(names: &[&str]) -> Result<FontVec> {
    let dir = std::env::var("WINDIR").unwrap_or_else(|_| "C:\\Windows".into());
    names
        .iter()
        .find_map(|n| std::fs::read(format!("{dir}\\Fonts\\{n}")).ok())
        .and_then(|bytes| FontVec::try_from_vec(bytes).ok())
        .with_context(|| format!("шрифт не найден: {names:?}"))
}

impl Renderer {
    pub fn new() -> Result<Self> {
        Ok(Self {
            bold: system_font(&["segoeuib.ttf", "seguisb.ttf", "arialbd.ttf"])?,
            mono: system_font(&["consolab.ttf", "consola.ttf", "cour.ttf"])?,
            art: RefCell::default(),
        })
    }

    /// Badge size in pixels at scale 1.
    fn base_size(style: OverlayStyle) -> (f32, f32) {
        match style {
            OverlayStyle::Capsule => (176.0, 68.0),
            OverlayStyle::Heart => (110.0, 128.0),
            OverlayStyle::Ecg => (196.0, 86.0),
            OverlayStyle::Ring => (112.0, 112.0),
            OverlayStyle::Minimal => (136.0, 72.0),
            OverlayStyle::Card => (190.0, 100.0),
            OverlayStyle::Bunny | OverlayStyle::Neko | OverlayStyle::Angel => (240.0 * MASCOT_UNIT, 200.0 * MASCOT_UNIT),
            OverlayStyle::Custom => (CUSTOM_ART.0, CUSTOM_ART.1 + 62.0),
            OverlayStyle::GirlFox | OverlayStyle::GirlMaid | OverlayStyle::GirlAngel | OverlayStyle::GirlNurse | OverlayStyle::GirlWitch | OverlayStyle::GirlSpace | OverlayStyle::GirlNeko | OverlayStyle::GirlSakura | OverlayStyle::GirlBunny | OverlayStyle::GirlSport => (GIRL_SIZE, GIRL_SIZE),
        }
    }

    pub fn size(style: OverlayStyle, scale: f32) -> (u32, u32) {
        let (w, h) = Self::base_size(style);
        ((w * scale).ceil() as u32, (h * scale).ceil() as u32)
    }

    pub fn render(&self, f: &Frame) -> Pixmap {
        let (w, h) = Self::size(f.style, f.scale);
        let mut pm = Pixmap::new(w.max(1), h.max(1)).expect("non-empty pixmap");
        let mut c = Canvas { pm: &mut pm, s: f.scale };
        let beat = if f.animate { beat_phase(f.bpm, f.time) } else { 0.0 };
        let live = f.bpm.is_some();
        let text = f.bpm.map_or("—".to_string(), |b| b.to_string());
        let accent = if live { coral() } else { muted() };

        if let Some(m) = mascot(f.style) {
            let key = format!("{:?}@{w}x{h}", f.style);
            let mut cache = self.art.borrow_mut();
            let art = cache.entry(key).or_insert_with(|| rasterize_svg(m.svg.as_bytes(), w, h));
            if let Some(art) = art {
                // A small hop on every beat.
                let hop = -3.0 * beat * f.scale;
                pm.draw_pixmap(0, 0, art.as_ref(), &PixmapPaint::default(), Transform::from_translate(0.0, hop), None);
            }
            let mut c = Canvas { pm: &mut pm, s: f.scale * MASCOT_UNIT };
            let size = m.size * (1.0 + 0.08 * beat);
            let align = if m.center { Align::Center } else { Align::Left };
            c.text(&self.bold, &text, size, m.number.0, m.number.1, white(), align, true);
            if !m.center {
                let tw = c.width(&self.bold, &text, size);
                c.text(&self.bold, "BPM", 16.0, m.number.0 + tw + 6.0, m.number.1, rgb(0xFF, 0x7A, 0xA8), Align::Left, false);
            }
            return pm;
        }

        if let Some(g) = girl(f.style) {
            let key = format!("{:?}@{w}", f.style);
            let mut cache = self.art.borrow_mut();
            let art = cache.entry(key).or_insert_with(|| {
                let img = Pixmap::decode_png(g.png).ok()?;
                let mut pm = Pixmap::new(w, h)?;
                let k = w as f32 / img.width() as f32;
                let paint = PixmapPaint { quality: tiny_skia::FilterQuality::Bicubic, ..Default::default() };
                pm.draw_pixmap(0, 0, img.as_ref(), &paint, Transform::from_scale(k, k), None);
                Some(pm)
            });
            if let Some(art) = art {
                let hop = -3.0 * beat * f.scale;
                pm.draw_pixmap(0, 0, art.as_ref(), &PixmapPaint::default(), Transform::from_translate(0.0, hop), None);
            }
            // Canvas units = image fractions x GIRL_SIZE.
            let mut c = Canvas { pm: &mut pm, s: f.scale };
            let size = g.size * GIRL_SIZE * (1.0 + 0.08 * beat);
            let (x, y) = (g.plate.0 * GIRL_SIZE, g.plate.1 * GIRL_SIZE - 3.0 * beat + size * 0.36);
            let color = if live { rgb(g.color.0, g.color.1, g.color.2) } else { alpha(rgb(g.color.0, g.color.1, g.color.2), 0.7) };
            c.text(&self.bold, &text, size, x, y, color, Align::Center, g.shadow);
            return pm;
        }

        if f.style == OverlayStyle::Custom {
            let (aw, ah) = ((CUSTOM_ART.0 * f.scale) as u32, (CUSTOM_ART.1 * f.scale) as u32);
            let path = f.custom_image.unwrap_or_default();
            let key = format!("{path}@{aw}x{ah}");
            let mut cache = self.art.borrow_mut();
            let art = cache.entry(key).or_insert_with(|| load_image(path, aw, ah));
            if let Some(art) = art {
                let hop = -3.0 * beat * f.scale;
                pm.draw_pixmap(0, 0, art.as_ref(), &PixmapPaint::default(), Transform::from_translate(0.0, hop), None);
            }
            let mut c = Canvas { pm: &mut pm, s: f.scale };
            let cw = CUSTOM_ART.0;
            if art.is_none() {
                c.rrect(cw / 2.0 - 55.0, 10.0, 110.0, 110.0, 20.0, alpha(rgb(10, 12, 16), 0.5), Some(alpha(white(), 0.4)));
                c.text(&self.bold, "?", 56.0, cw / 2.0, 86.0, alpha(white(), 0.7), Align::Center, false);
            }
            c.rrect(cw / 2.0 - 58.0, CUSTOM_ART.1 + 4.0, 116.0, 54.0, 27.0, alpha(rgb(10, 12, 16), 0.65), Some(alpha(white(), 0.2)));
            c.heart(cw / 2.0 - 30.0, CUSTOM_ART.1 + 31.0, 24.0 * (1.0 + 0.16 * beat), accent);
            c.text(&self.bold, &text, 34.0, cw / 2.0 - 12.0, CUSTOM_ART.1 + 43.0, white(), Align::Left, false);
            return pm;
        }

        match f.style {
            OverlayStyle::Capsule => {
                if f.background {
                    c.rrect(2.0, 2.0, 172.0, 64.0, 32.0, alpha(rgb(10, 12, 16), 0.6), Some(alpha(white(), 0.18)));
                    c.circle(34.0, 34.0, 23.0, alpha(coral(), 0.18));
                }
                c.heart(34.0, 35.0, 30.0 * (1.0 + 0.14 * beat), accent);
                c.text(&self.bold, &text, 40.0, 66.0, 48.0, white(), Align::Left, !f.background);
            }
            OverlayStyle::Heart => {
                let size = 84.0 * (1.0 + 0.12 * beat);
                c.heart(55.0, 46.0, size + 6.0, white());
                c.heart(55.0, 46.0, size, if live { rgb(0xFF, 0x4D, 0x6D) } else { muted() });
                c.text(&self.bold, &text, 38.0, 55.0, 120.0, white(), Align::Center, true);
            }
            OverlayStyle::Ecg => {
                let shadow = !f.background;
                if f.background {
                    c.rrect(2.0, 2.0, 192.0, 82.0, 14.0, alpha(rgb(6, 20, 18), 0.72), Some(alpha(teal(), 0.45)));
                } else {
                    c.ecg(15.0, 11.5, 168.0, 32.0, f, alpha(rgb(0, 0, 0), 0.5));
                }
                c.ecg(14.0, 10.0, 168.0, 32.0, f, if live { teal() } else { muted() });
                c.text(&self.mono, &text, 28.0, 14.0, 74.0, teal(), Align::Left, shadow);
                let bw = c.width(&self.mono, &text, 28.0);
                c.text(&self.mono, "BPM", 14.0, 20.0 + bw, 74.0, alpha(teal(), 0.8), Align::Left, shadow);
            }
            OverlayStyle::Ring => {
                c.circle(56.0, 56.0, 52.0, alpha(rgb(10, 12, 16), 0.6));
                c.arc(56.0, 56.0, 44.0, 1.0, alpha(white(), 0.16), 9.0);
                if let Some(b) = f.bpm {
                    let progress = ((b as f32 - 40.0) / 160.0).clamp(0.04, 1.0);
                    c.arc(56.0, 56.0, 44.0, progress, zone_color(f.bpm), 9.0 * (1.0 + 0.25 * beat));
                }
                c.text(&self.bold, &text, 34.0, 56.0, 68.0, white(), Align::Center, false);
            }
            OverlayStyle::Minimal => {
                c.text(&self.bold, &text, 60.0, 8.0, 58.0, white(), Align::Left, true);
                let tw = c.width(&self.bold, &text, 60.0);
                c.heart(tw + 26.0, 48.0, 22.0 * (1.0 + 0.2 * beat), accent);
            }
            OverlayStyle::Bunny | OverlayStyle::Neko | OverlayStyle::Angel | OverlayStyle::Custom => unreachable!(),
            OverlayStyle::GirlFox | OverlayStyle::GirlMaid | OverlayStyle::GirlAngel | OverlayStyle::GirlNurse | OverlayStyle::GirlWitch | OverlayStyle::GirlSpace | OverlayStyle::GirlNeko | OverlayStyle::GirlSakura | OverlayStyle::GirlBunny | OverlayStyle::GirlSport => unreachable!(),
            OverlayStyle::Card => {
                // Without the white plate the card switches to light text over video.
                let (fg, sub, line, shadow) = if f.background {
                    c.rrect(2.0, 2.0, 186.0, 96.0, 16.0, alpha(white(), 0.93), None);
                    (ink(), rgb(0x5F, 0x64, 0x6E), rgb(0xD9, 0x36, 0x3E), false)
                } else {
                    (white(), alpha(white(), 0.8), coral(), true)
                };
                c.heart(26.0, 34.0, 22.0 * (1.0 + 0.18 * beat), if live { line } else { muted() });
                c.text(&self.bold, &text, 32.0, 44.0, 45.0, fg, Align::Left, shadow);
                let tw = c.width(&self.bold, &text, 32.0);
                c.text(&self.bold, "BPM", 13.0, 50.0 + tw, 45.0, sub, Align::Left, shadow);
                if shadow {
                    c.sparkline(15.0, 59.5, 162.0, 30.0, f.history, alpha(rgb(0, 0, 0), 0.5));
                }
                c.sparkline(14.0, 58.0, 162.0, 30.0, f.history, line);
            }
        }
        pm
    }
}

/// 1 right at a heartbeat, decaying to 0 before the next one.
fn beat_phase(bpm: Option<u16>, time: f32) -> f32 {
    let Some(bpm) = bpm else { return 0.0 };
    let phase = (time * bpm as f32 / 60.0).fract();
    (-phase * 7.0).exp()
}

fn zone_color(bpm: Option<u16>) -> Color {
    match bpm {
        None => muted(),
        Some(b) if b < 100 => rgb(0x4A, 0xDE, 0x80),
        Some(b) if b < 130 => rgb(0xF9, 0x73, 0x16),
        Some(_) => rgb(0xEF, 0x44, 0x44),
    }
}

/// ECG-like waveform over one beat, phase 0..1 → amplitude -1..1.
fn ecg_wave(p: f32) -> f32 {
    let bump = |c: f32, w: f32, a: f32| a * (-((p - c) / w).powi(2)).exp();
    bump(0.18, 0.035, 0.18) - bump(0.30, 0.012, 0.25) + bump(0.33, 0.014, 1.0) - bump(0.36, 0.012, 0.35)
        + bump(0.56, 0.05, 0.3)
}

enum Align {
    Left,
    Center,
}

struct Canvas<'a> {
    pm: &'a mut Pixmap,
    s: f32,
}

impl Canvas<'_> {
    fn paint(color: Color) -> Paint<'static> {
        let mut p = Paint::default();
        p.set_color(color);
        p.anti_alias = true;
        p
    }

    fn fill(&mut self, path: &Path, color: Color) {
        self.pm.fill_path(path, &Self::paint(color), FillRule::Winding, Transform::from_scale(self.s, self.s), None);
    }

    fn stroke(&mut self, path: &Path, color: Color, width: f32) {
        let stroke = Stroke { width, line_cap: LineCap::Round, line_join: LineJoin::Round, ..Default::default() };
        self.pm.stroke_path(path, &Self::paint(color), &stroke, Transform::from_scale(self.s, self.s), None);
    }

    fn rrect(&mut self, x: f32, y: f32, w: f32, h: f32, r: f32, fill: Color, border: Option<Color>) {
        let r = r.min(w / 2.0).min(h / 2.0);
        let k = 0.5523 * r;
        let mut pb = PathBuilder::new();
        pb.move_to(x + r, y);
        pb.line_to(x + w - r, y);
        pb.cubic_to(x + w - r + k, y, x + w, y + r - k, x + w, y + r);
        pb.line_to(x + w, y + h - r);
        pb.cubic_to(x + w, y + h - r + k, x + w - r + k, y + h, x + w - r, y + h);
        pb.line_to(x + r, y + h);
        pb.cubic_to(x + r - k, y + h, x, y + h - r + k, x, y + h - r);
        pb.line_to(x, y + r);
        pb.cubic_to(x, y + r - k, x + r - k, y, x + r, y);
        pb.close();
        let path = pb.finish().unwrap();
        self.fill(&path, fill);
        if let Some(b) = border {
            self.stroke(&path, b, 1.5);
        }
    }

    fn circle(&mut self, cx: f32, cy: f32, r: f32, color: Color) {
        let path = PathBuilder::from_circle(cx, cy, r).unwrap();
        self.fill(&path, color);
    }

    /// Clockwise arc from 12 o'clock covering `fraction` of the circle.
    fn arc(&mut self, cx: f32, cy: f32, r: f32, fraction: f32, color: Color, width: f32) {
        let steps = (64.0 * fraction).ceil().max(2.0) as usize;
        let mut pb = PathBuilder::new();
        for i in 0..=steps {
            let a = -std::f32::consts::FRAC_PI_2 + std::f32::consts::TAU * fraction * i as f32 / steps as f32;
            let (x, y) = (cx + r * a.cos(), cy + r * a.sin());
            if i == 0 { pb.move_to(x, y) } else { pb.line_to(x, y) }
        }
        if let Some(path) = pb.finish() {
            self.stroke(&path, color, width);
        }
    }

    /// Heart centred on (cx, cy) with the given width.
    fn heart(&mut self, cx: f32, cy: f32, size: f32, color: Color) {
        let k = size / 22.0;
        let p = |x: f32, y: f32| (cx + (x - 12.0) * k, cy + (y - 12.5) * k);
        let mut pb = PathBuilder::new();
        let m = p(12.0, 21.0);
        pb.move_to(m.0, m.1);
        for [a, b, c] in [
            [(12.0, 21.0), (4.5, 16.4), (2.4, 11.8)],
            [(0.9, 8.5), (3.0, 4.5), (6.8, 4.5)],
            [(8.9, 4.5), (10.3, 5.6), (12.0, 7.5)],
            [(13.7, 5.6), (15.1, 4.5), (17.2, 4.5)],
            [(21.0, 4.5), (23.1, 8.5), (21.6, 11.8)],
            [(19.5, 16.4), (12.0, 21.0), (12.0, 21.0)],
        ] {
            let (a, b, c) = (p(a.0, a.1), p(b.0, b.1), p(c.0, c.1));
            pb.cubic_to(a.0, a.1, b.0, b.1, c.0, c.1);
        }
        pb.close();
        self.fill(&pb.finish().unwrap(), color);
    }

    fn ecg(&mut self, x: f32, y: f32, w: f32, h: f32, f: &Frame, color: Color) {
        let beats_across = 2.0;
        let shift = f.bpm.map_or(0.0, |b| f.time * b as f32 / 60.0);
        let mut pb = PathBuilder::new();
        for i in 0..=96 {
            let t = i as f32 / 96.0;
            let amp = if f.bpm.is_some() { ecg_wave((t * beats_across - shift).rem_euclid(1.0)) } else { 0.0 };
            let (px, py) = (x + t * w, y + h * 0.62 - amp * h * 0.55);
            if i == 0 { pb.move_to(px, py) } else { pb.line_to(px, py) }
        }
        self.stroke(&pb.finish().unwrap(), color, 2.2);
    }

    fn sparkline(&mut self, x: f32, y: f32, w: f32, h: f32, values: &[u16], color: Color) {
        if values.len() < 2 {
            let mut pb = PathBuilder::new();
            pb.move_to(x, y + h / 2.0);
            pb.line_to(x + w, y + h / 2.0);
            self.stroke(&pb.finish().unwrap(), alpha(color, 0.3), 2.0);
            return;
        }
        let lo = *values.iter().min().unwrap() as f32 - 3.0;
        let hi = *values.iter().max().unwrap() as f32 + 3.0;
        let mut pb = PathBuilder::new();
        for (i, v) in values.iter().enumerate() {
            let px = x + w * i as f32 / (values.len() - 1) as f32;
            let py = y + h - (*v as f32 - lo) / (hi - lo) * h;
            if i == 0 { pb.move_to(px, py) } else { pb.line_to(px, py) }
        }
        self.stroke(&pb.finish().unwrap(), color, 2.0);
    }

    fn width(&self, font: &FontVec, text: &str, size: f32) -> f32 {
        let f = font.as_scaled(PxScale::from(size * self.s));
        text.chars().map(|ch| f.h_advance(f.glyph_id(ch))).sum::<f32>() / self.s
    }

    /// Draws `text` with its baseline at `y` (unscaled units).
    #[allow(clippy::too_many_arguments)]
    fn text(&mut self, font: &FontVec, text: &str, size: f32, x: f32, y: f32, color: Color, align: Align, shadow: bool) {
        let x = match align {
            Align::Left => x,
            Align::Center => x - self.width(font, text, size) / 2.0,
        };
        if shadow {
            self.glyphs(font, text, size, x + 1.5, y + 2.0, alpha(rgb(0, 0, 0), 0.55));
        }
        self.glyphs(font, text, size, x, y, color);
    }

    fn glyphs(&mut self, font: &FontVec, text: &str, size: f32, x: f32, y: f32, color: Color) {
        let scaled = font.as_scaled(PxScale::from(size * self.s));
        let (pw, ph) = (self.pm.width() as i32, self.pm.height() as i32);
        let mut caret = x * self.s;
        let c = color.premultiply().to_color_u8();
        let pixels = self.pm.pixels_mut();
        for ch in text.chars() {
            let id = scaled.glyph_id(ch);
            let glyph = id.with_scale_and_position(scaled.scale(), ab_glyph::point(caret, y * self.s));
            caret += scaled.h_advance(id);
            let Some(outline) = font.outline_glyph(glyph) else { continue };
            let bounds = outline.px_bounds();
            outline.draw(|gx, gy, cov| {
                let (px, py) = (bounds.min.x as i32 + gx as i32, bounds.min.y as i32 + gy as i32);
                if px < 0 || py < 0 || px >= pw || py >= ph {
                    return;
                }
                let dst = &mut pixels[(py * pw + px) as usize];
                let a = cov.clamp(0.0, 1.0);
                let blend = |s: u8, d: u8, sa: u8| {
                    let s = s as f32 * a;
                    (s + d as f32 * (1.0 - sa as f32 * a / 255.0)).round().min(255.0) as u8
                };
                let (r, g, b, al) = (
                    blend(c.red(), dst.red(), c.alpha()),
                    blend(c.green(), dst.green(), c.alpha()),
                    blend(c.blue(), dst.blue(), c.alpha()),
                    blend(c.alpha(), dst.alpha(), c.alpha()),
                );
                if let Some(p) = tiny_skia::PremultipliedColorU8::from_rgba(r.min(al), g.min(al), b.min(al), al) {
                    *dst = p;
                }
            });
        }
    }
}

/// Blends a premultiplied RGBA pixmap centred at (cx, cy) onto a BGRA frame.
pub fn blend_onto(frame: &mut [u8], fw: u32, fh: u32, pm: &Pixmap, cx: f32, cy: f32) {
    let x0 = (cx - pm.width() as f32 / 2.0).round() as i32;
    let y0 = (cy - pm.height() as f32 / 2.0).round() as i32;
    let pw = pm.width() as i32;
    for (i, p) in pm.pixels().iter().enumerate() {
        let a = p.alpha() as u32;
        if a == 0 {
            continue;
        }
        let (x, y) = (x0 + i as i32 % pw, y0 + i as i32 / pw);
        if x < 0 || y < 0 || x >= fw as i32 || y >= fh as i32 {
            continue;
        }
        let o = ((y as u32 * fw + x as u32) * 4) as usize;
        let inv = 255 - a;
        frame[o] = (p.blue() as u32 + frame[o] as u32 * inv / 255) as u8;
        frame[o + 1] = (p.green() as u32 + frame[o + 1] as u32 * inv / 255) as u8;
        frame[o + 2] = (p.red() as u32 + frame[o + 2] as u32 * inv / 255) as u8;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn every_style_draws_something() {
        let r = Renderer::new().unwrap();
        for style in [
            OverlayStyle::Capsule,
            OverlayStyle::Heart,
            OverlayStyle::Ecg,
            OverlayStyle::Ring,
            OverlayStyle::Minimal,
            OverlayStyle::Card,
            OverlayStyle::Bunny,
            OverlayStyle::Neko,
            OverlayStyle::Angel,
            OverlayStyle::Custom,
            OverlayStyle::GirlFox,
            OverlayStyle::GirlMaid,
            OverlayStyle::GirlAngel,
            OverlayStyle::GirlNurse,
            OverlayStyle::GirlWitch,
            OverlayStyle::GirlSpace,
            OverlayStyle::GirlNeko,
            OverlayStyle::GirlSakura,
            OverlayStyle::GirlBunny,
            OverlayStyle::GirlSport,
        ] {
            for bpm in [Some(87), None] {
                for background in [true, false] {
                    let pm = r.render(&Frame { bpm, history: &[80, 85, 90], style, scale: 1.0, animate: true, background, time: 0.3, custom_image: None });
                    assert!(pm.pixels().iter().any(|p| p.alpha() > 200), "{style:?} {bpm:?}");
                }
            }
        }
    }
}
