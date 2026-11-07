//! Pixel helpers: aspect-aware bilinear scaling into the output frame and the
//! cheap downscales used for tracking and the UI preview.

use crate::settings::Fit;

/// A BGRA source image. `stride` may be negative for bottom-up buffers;
/// `row(y)` always returns the visual row `y`.
pub struct Source<'a> {
    pub data: &'a [u8],
    pub width: u32,
    pub height: u32,
    pub stride: isize,
    /// Byte offset of visual row 0 inside `data`.
    pub origin: usize,
}

impl Source<'_> {
    fn row(&self, y: u32) -> &[u8] {
        let start = (self.origin as isize + y as isize * self.stride) as usize;
        &self.data[start..start + self.width as usize * 4]
    }
}

/// Destination rectangle of a `sw`×`sh` image inside `dw`×`dh`.
/// Returns (x, y, w, h); for `Cover` the rectangle overflows the frame.
pub fn placement(sw: u32, sh: u32, dw: u32, dh: u32, fit: Fit) -> (i32, i32, u32, u32) {
    let sx = dw as f64 / sw as f64;
    let sy = dh as f64 / sh as f64;
    let s = match fit {
        Fit::Contain => sx.min(sy),
        Fit::Cover => sx.max(sy),
    };
    let w = (sw as f64 * s).round() as u32;
    let h = (sh as f64 * s).round() as u32;
    ((dw as i32 - w as i32) / 2, (dh as i32 - h as i32) / 2, w, h)
}

/// Scales `src` into the `dw`×`dh` BGRA buffer `dst`, preserving aspect ratio.
/// Uncovered area is filled with black.
pub fn fit_into(src: &Source, dst: &mut [u8], dw: u32, dh: u32, fit: Fit, mirror: bool) {
    let (ox, oy, w, h) = placement(src.width, src.height, dw, dh, fit);
    // Bilinear sample positions in 16.16 fixed point, one table per axis.
    let axis = |len: u32, src_len: u32| -> Vec<(u32, u32, u32)> {
        (0..len)
            .map(|i| {
                let f = ((i as f64 + 0.5) * src_len as f64 / len as f64 - 0.5).max(0.0);
                let i0 = (f as u32).min(src_len - 1);
                let i1 = (i0 + 1).min(src_len - 1);
                (i0, i1, ((f - i0 as f64) * 256.0) as u32)
            })
            .collect()
    };
    let xs = axis(w, src.width);
    let ys = axis(h, src.height);

    for y in 0..dh {
        let row = &mut dst[(y * dw * 4) as usize..((y + 1) * dw * 4) as usize];
        let sy = y as i32 - oy;
        if sy < 0 || sy >= h as i32 {
            row.fill(0);
            continue;
        }
        let (y0, y1, fy) = ys[sy as usize];
        let (r0, r1) = (src.row(y0), src.row(y1));
        for x in 0..dw {
            let out = &mut row[(x * 4) as usize..(x * 4 + 4) as usize];
            let vx = if mirror { dw - 1 - x } else { x } as i32 - ox;
            if vx < 0 || vx >= w as i32 {
                out.copy_from_slice(&[0, 0, 0, 255]);
                continue;
            }
            let (x0, x1, fx) = xs[vx as usize];
            let (a, b) = ((x0 * 4) as usize, (x1 * 4) as usize);
            for c in 0..3 {
                let top = r0[a + c] as u32 * (256 - fx) + r0[b + c] as u32 * fx;
                let bottom = r1[a + c] as u32 * (256 - fx) + r1[b + c] as u32 * fx;
                out[c] = ((top * (256 - fy) + bottom * fy) >> 16) as u8;
            }
            out[3] = 255;
        }
    }
}

/// Averages `factor`×`factor` blocks of a BGRA image into 8-bit luma.
pub fn gray_downscale(bgra: &[u8], w: u32, h: u32, factor: u32) -> (Vec<u8>, u32, u32) {
    let (gw, gh) = (w / factor, h / factor);
    let mut out = vec![0u8; (gw * gh) as usize];
    for gy in 0..gh {
        for gx in 0..gw {
            let mut sum = 0u32;
            for dy in 0..factor {
                let row = ((gy * factor + dy) * w + gx * factor) as usize * 4;
                for dx in 0..factor as usize {
                    let p = &bgra[row + dx * 4..];
                    sum += (p[0] as u32 * 29 + p[1] as u32 * 150 + p[2] as u32 * 77) >> 8;
                }
            }
            out[(gy * gw + gx) as usize] = (sum / (factor * factor)) as u8;
        }
    }
    (out, gw, gh)
}

/// Halves a BGRA image into packed RGB, for the preview JPEG.
pub fn half_rgb(bgra: &[u8], w: u32, h: u32) -> (Vec<u8>, u32, u32) {
    let (hw, hh) = (w / 2, h / 2);
    let mut out = vec![0u8; (hw * hh * 3) as usize];
    for y in 0..hh {
        for x in 0..hw {
            let a = ((2 * y * w + 2 * x) * 4) as usize;
            let b = a + (w * 4) as usize;
            let o = ((y * hw + x) * 3) as usize;
            for c in 0..3 {
                let s = bgra[a + c] as u32 + bgra[a + 4 + c] as u32 + bgra[b + c] as u32 + bgra[b + 4 + c] as u32;
                out[o + 2 - c] = (s / 4) as u8;
            }
        }
    }
    (out, hw, hh)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn contain_never_crops() {
        assert_eq!(placement(640, 480, 1280, 720, Fit::Contain), (160, 0, 960, 720));
        assert_eq!(placement(1920, 1080, 1280, 720, Fit::Contain), (0, 0, 1280, 720));
        assert_eq!(placement(640, 480, 1280, 720, Fit::Cover), (0, -120, 1280, 960));
    }

    #[test]
    fn scales_solid_color_and_letterboxes() {
        let src_px = [10u8, 20, 30, 255].repeat(4 * 3);
        let src = Source { data: &src_px, width: 4, height: 3, stride: 16, origin: 0 };
        let mut dst = vec![0u8; 16 * 9 * 4];
        fit_into(&src, &mut dst, 16, 9, Fit::Contain, false);
        assert_eq!(&dst[0..4], &[0, 0, 0, 255]); // left bar
        let mid = ((4 * 16 + 8) * 4) as usize;
        assert_eq!(&dst[mid..mid + 4], &[10, 20, 30, 255]);
    }

    #[test]
    fn bottom_up_source_reads_visual_rows() {
        // Two rows; the visual top row is stored last.
        let px = [[0u8, 0, 255, 255], [255, 0, 0, 255]].concat();
        let src = Source { data: &px, width: 1, height: 2, stride: -4, origin: 4 };
        let mut dst = vec![0u8; 2 * 4];
        fit_into(&src, &mut dst, 1, 2, Fit::Contain, false);
        assert_eq!(&dst[0..4], &[255, 0, 0, 255]);
    }
}
