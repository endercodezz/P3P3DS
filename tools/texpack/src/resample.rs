//! Downscaling for replacement art (packs ship 2x-6x upscales; the New 3DS
//! needs power-of-two textures of at most 1024 texels and a small budget).

use crate::pica::Image;

/// Largest power-of-two size with the image's aspect ratio that fits in the
/// image and in `max` on both axes. Pack images are k x (power-of-two texture),
/// so their aspect ratio is a power of two; others get the nearest fit.
pub fn target_size(width: usize, height: usize, max: usize) -> (usize, usize) {
    let floor_pow2 = |n: usize| if n <= 1 { 1 } else { 1usize << (usize::BITS - 1 - n.leading_zeros()) };
    // Aspect ratio as a power of two: width = height << shift (negative: taller).
    let shift = (width.max(1) as f64 / height.max(1) as f64).log2().round() as i32;
    let max = floor_pow2(max);
    let w_limit = floor_pow2(width).min(max);
    let h_limit = floor_pow2(height).min(max);
    let h_from_w = |w: usize| if shift >= 0 { w >> shift } else { w << -shift };
    let w_from_h = |h: usize| if shift >= 0 { h << shift } else { h >> -shift };
    let (w, h) = if h_from_w(w_limit) <= h_limit { (w_limit, h_from_w(w_limit)) } else { (w_from_h(h_limit), h_limit) };
    (w.max(1), h.max(1))
}

/// Area-average (box) resample with premultiplied alpha, so transparent texels
/// don't darken the edges of opaque ones.
pub fn resize(src: &Image, width: usize, height: usize) -> Image {
    if src.width == width && src.height == height {
        return Image { width, height, rgba: src.rgba.clone() };
    }
    let sx = src.width as f64 / width as f64;
    let sy = src.height as f64 / height as f64;
    let mut rgba = Vec::with_capacity(width * height * 4);
    for y in 0..height {
        let (y0, y1) = (y as f64 * sy, (y + 1) as f64 * sy);
        for x in 0..width {
            let (x0, x1) = (x as f64 * sx, (x + 1) as f64 * sx);
            let mut acc = [0f64; 4];
            let mut area = 0f64;
            let mut py = y0.floor() as usize;
            while (py as f64) < y1 && py < src.height {
                let wy = (y1.min(py as f64 + 1.0) - y0.max(py as f64)).max(0.0);
                let mut px = x0.floor() as usize;
                while (px as f64) < x1 && px < src.width {
                    let wx = (x1.min(px as f64 + 1.0) - x0.max(px as f64)).max(0.0);
                    let wgt = wx * wy;
                    let o = (py * src.width + px) * 4;
                    let a = src.rgba[o + 3] as f64;
                    acc[0] += src.rgba[o] as f64 * a * wgt;
                    acc[1] += src.rgba[o + 1] as f64 * a * wgt;
                    acc[2] += src.rgba[o + 2] as f64 * a * wgt;
                    acc[3] += a * wgt;
                    area += wgt;
                    px += 1;
                }
                py += 1;
            }
            let alpha = if area > 0.0 { acc[3] / area } else { 0.0 };
            let color = |c: f64| if acc[3] > 0.0 { (c / acc[3]).round().clamp(0.0, 255.0) as u8 } else { 0 };
            rgba.extend_from_slice(&[color(acc[0]), color(acc[1]), color(acc[2]), alpha.round().clamp(0.0, 255.0) as u8]);
        }
    }
    Image { width, height, rgba }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn target_sizes_of_pack_images() {
        assert_eq!(target_size(1536, 768, 512), (512, 256)); // 3x 512x256 backdrop
        assert_eq!(target_size(144, 144, 512), (128, 128)); // 4.5x 32x32 font glyph
        assert_eq!(target_size(768, 192, 1024), (512, 128)); // 3x 256x64
        assert_eq!(target_size(192, 768, 128), (32, 128));
        assert_eq!(target_size(64, 64, 512), (64, 64));
        assert_eq!(target_size(3, 3, 512), (2, 2));
    }

    #[test]
    fn box_filter_averages_and_keeps_alpha_edges_clean() {
        // 2x2 -> 1x1: red opaque, transparent black x3 -> red at quarter alpha.
        let src = Image { width: 2, height: 2, rgba: vec![255, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] };
        let out = resize(&src, 1, 1);
        assert_eq!(out.rgba, vec![255, 0, 0, 64]);
        // 3x1 -> 2x1 with fractional coverage: 10,20,30 -> 13.3, 26.7.
        let src = Image { width: 3, height: 1, rgba: vec![10, 10, 10, 255, 20, 20, 20, 255, 30, 30, 30, 255] };
        let out = resize(&src, 2, 1);
        assert_eq!(out.rgba, vec![13, 13, 13, 255, 27, 27, 27, 255]);
    }
}
