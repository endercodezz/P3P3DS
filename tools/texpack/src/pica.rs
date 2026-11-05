//! PICA200 texture data as citro3d loads it: 8x8 tiles in row-major order from
//! the top of the image, Morton (Z) order inside a tile, little-endian texels.
//! Checked byte for byte against devkitPro `tex3ds -r -z none` (tests/fixtures).

use std::fmt;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Format {
    Rgba8,
    Rgb8,
    Rgb565,
    Rgba5551,
    Rgba4,
}

impl Format {
    #[cfg(test)]
    pub const ALL: [Format; 5] = [Format::Rgba8, Format::Rgb8, Format::Rgb565, Format::Rgba5551, Format::Rgba4];

    pub fn parse(name: &str) -> Option<Format> {
        match name.to_ascii_lowercase().as_str() {
            "rgba8" | "rgba" | "rgba8888" => Some(Format::Rgba8),
            "rgb8" | "rgb" | "rgb888" => Some(Format::Rgb8),
            "rgb565" => Some(Format::Rgb565),
            "rgba5551" => Some(Format::Rgba5551),
            "rgba4" | "rgba444" | "rgba4444" => Some(Format::Rgba4),
            _ => None,
        }
    }

    pub fn name(self) -> &'static str {
        match self {
            Format::Rgba8 => "rgba8",
            Format::Rgb8 => "rgb8",
            Format::Rgb565 => "rgb565",
            Format::Rgba5551 => "rgba5551",
            Format::Rgba4 => "rgba4",
        }
    }

    pub fn bytes_per_texel(self) -> usize {
        match self {
            Format::Rgba8 => 4,
            Format::Rgb8 => 3,
            Format::Rgb565 | Format::Rgba5551 | Format::Rgba4 => 2,
        }
    }
}

impl fmt::Display for Format {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.name())
    }
}

/// An RGBA8 image, rows from the top.
pub struct Image {
    pub width: usize,
    pub height: usize,
    pub rgba: Vec<u8>,
}

pub const MIN_SIZE: usize = 8;
pub const MAX_SIZE: usize = 1024;

/// Texture size the PICA200 accepts for an image dimension: the next power of
/// two, at least 8.
pub fn texture_dim(n: usize) -> usize {
    n.max(MIN_SIZE).next_power_of_two()
}

/// The top `bits` bits of an 8-bit channel (tex3ds truncates; fixtures).
fn quantize(v: u8, bits: u32) -> u16 {
    (v >> (8 - bits)) as u16
}

fn texel(format: Format, p: [u8; 4], out: &mut Vec<u8>) {
    // tex3ds stores fully transparent texels as transparent black (fixtures).
    let [r, g, b, a] = if p[3] == 0 { [0; 4] } else { p };
    match format {
        Format::Rgba8 => out.extend_from_slice(&[a, b, g, r]),
        Format::Rgb8 => out.extend_from_slice(&[b, g, r]),
        Format::Rgb565 => {
            let v = quantize(r, 5) << 11 | quantize(g, 6) << 5 | quantize(b, 5);
            out.extend_from_slice(&v.to_le_bytes());
        }
        Format::Rgba5551 => {
            let v = quantize(r, 5) << 11 | quantize(g, 5) << 6 | quantize(b, 5) << 1 | quantize(a, 1);
            out.extend_from_slice(&v.to_le_bytes());
        }
        Format::Rgba4 => {
            let v = quantize(r, 4) << 12 | quantize(g, 4) << 8 | quantize(b, 4) << 4 | quantize(a, 4);
            out.extend_from_slice(&v.to_le_bytes());
        }
    }
}

/// Position of texel (x, y) of an 8x8 tile in Morton order (x in the low bit).
fn morton(x: usize, y: usize) -> usize {
    let mut i = 0;
    for bit in 0..3 {
        i |= ((x >> bit) & 1) << (2 * bit);
        i |= ((y >> bit) & 1) << (2 * bit + 1);
    }
    i
}

#[derive(Debug)]
pub enum EncodeError {
    TooLarge { width: usize, height: usize },
    Empty,
}

impl fmt::Display for EncodeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            EncodeError::TooLarge { width, height } => {
                write!(f, "{width}x{height} is larger than the PICA200 limit of {MAX_SIZE}x{MAX_SIZE}")
            }
            EncodeError::Empty => f.write_str("image has no pixels"),
        }
    }
}

/// Encodes `image` padded to the texture size (transparent black outside it).
pub fn encode(image: &Image, format: Format) -> Result<Vec<u8>, EncodeError> {
    if image.width == 0 || image.height == 0 {
        return Err(EncodeError::Empty);
    }
    let (tw, th) = (texture_dim(image.width), texture_dim(image.height));
    if tw > MAX_SIZE || th > MAX_SIZE {
        return Err(EncodeError::TooLarge { width: image.width, height: image.height });
    }
    let bpp = format.bytes_per_texel();
    let mut out = Vec::with_capacity(tw * th * bpp);
    let mut tile = vec![[0u8; 4]; 64];
    for ty in (0..th).step_by(8) {
        for tx in (0..tw).step_by(8) {
            for y in 0..8 {
                for x in 0..8 {
                    let (px, py) = (tx + x, ty + y);
                    tile[morton(x, y)] = if px < image.width && py < image.height {
                        let o = (py * image.width + px) * 4;
                        [image.rgba[o], image.rgba[o + 1], image.rgba[o + 2], image.rgba[o + 3]]
                    } else {
                        [0; 4]
                    };
                }
            }
            for &p in &tile {
                texel(format, p, &mut out);
            }
        }
    }
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn morton_order_of_first_texels() {
        assert_eq!(morton(0, 0), 0);
        assert_eq!(morton(1, 0), 1);
        assert_eq!(morton(0, 1), 2);
        assert_eq!(morton(1, 1), 3);
        assert_eq!(morton(2, 0), 4);
        assert_eq!(morton(7, 7), 63);
    }

    #[test]
    fn quantize_ends() {
        assert_eq!(quantize(0, 5), 0);
        assert_eq!(quantize(255, 5), 31);
        assert_eq!(quantize(255, 1), 1);
        assert_eq!(quantize(127, 1), 0);
        assert_eq!(quantize(128, 1), 1);
    }

    #[test]
    fn rejects_oversized_and_empty() {
        let big = Image { width: 1025, height: 8, rgba: vec![0; 1025 * 8 * 4] };
        assert!(matches!(encode(&big, Format::Rgba8), Err(EncodeError::TooLarge { .. })));
        let empty = Image { width: 0, height: 0, rgba: vec![] };
        assert!(matches!(encode(&empty, Format::Rgba8), Err(EncodeError::Empty)));
    }

    #[test]
    fn pads_to_power_of_two() {
        let img = Image { width: 9, height: 3, rgba: vec![255; 9 * 3 * 4] };
        assert_eq!(encode(&img, Format::Rgb565).unwrap().len(), 16 * 8 * 2);
    }
}
