//! `build`: PPSSPP pack -> New 3DS pack (two files in the output directory).
//!
//! index.bin (little endian):
//!   0  "P3TXPACK"            magic
//!   8  u32 version = 1
//!   12 u32 flags             bit 0: ignoreAddress (keys carry no address)
//!   16 u32 entry count
//!   20 u32 reserved = 0
//!   24 entries, 32 bytes each, sorted by (cachekey, hash):
//!      u64 cachekey, u32 hash, u32 data offset in textures.bin,
//!      u32 data size, u16 width, u16 height (power of two, 8..1024),
//!      u8 format (citro3d GPU_TEXCOLOR: 0 RGBA8, 2 RGBA5551, 3 RGB565, 4 RGBA4),
//!      u8 reserved, u16 reserved, u32 reserved
//!   An entry with width 0 means "keep the original texture" (an empty or `|`
//!   file in textures.ini).
//! textures.bin: PICA200 texture data (pica.rs), each entry 128-byte aligned,
//!   rows bottom-up as the P3P3DS 3DS renderer uploads textures: image row y
//!   is memory row h - 1 - y (platform/3ds/gpu_renderer.cpp, tiled_row).
//!
//! The replacement covers the whole PSP texture: the renderer samples it with
//! the PSP texture's normalised coordinates, so any power-of-two size with the
//! same aspect ratio works.

use crate::pack::{Key, Pack};
use crate::pica::{self, Format, Image};
use crate::resample;
use std::fs::File;
use std::io::{BufWriter, Write};
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Mutex;

pub const MAGIC: &[u8; 8] = b"P3TXPACK";
pub const VERSION: u32 = 1;
pub const ENTRY_SIZE: usize = 32;
const ALIGN: usize = 128;

pub struct Options {
    pub max_size: usize,
    /// None: pick per texture from its alpha channel.
    pub format: Option<Format>,
}

/// citro3d GPU_TEXCOLOR value of a format.
pub fn gpu_format(format: Format) -> u8 {
    match format {
        Format::Rgba8 => 0,
        Format::Rgb8 => 1,
        Format::Rgba5551 => 2,
        Format::Rgb565 => 3,
        Format::Rgba4 => 4,
    }
}

/// Opaque -> RGB565, alpha only 0/255 -> RGBA5551, otherwise RGBA4.
pub fn auto_format(image: &Image) -> Format {
    let (mut partial, mut transparent) = (false, false);
    for a in image.rgba.iter().skip(3).step_by(4) {
        match a {
            255 => {}
            0 => transparent = true,
            _ => {
                partial = true;
                break;
            }
        }
    }
    if partial {
        Format::Rgba4
    } else if transparent {
        Format::Rgba5551
    } else {
        Format::Rgb565
    }
}

/// Image row y becomes row h - 1 - y.
pub fn flip_vertical(image: Image) -> Image {
    let row = image.width * 4;
    let rgba = image.rgba.chunks_exact(row).rev().flatten().copied().collect();
    Image { rgba, ..image }
}

struct Built {
    key: Key,
    width: u16,
    height: u16,
    format: u8,
    data: Vec<u8>,
}

pub struct Summary {
    pub entries: usize,
    pub keep_original: usize,
    pub skipped: Vec<String>,
    pub data_bytes: u64,
    pub by_format: [usize; 5],
}

fn build_one(dir: &Path, key: Key, file: &str, options: &Options, load: &dyn Fn(&Path) -> Result<Image, String>) -> Result<Built, String> {
    let image = load(&dir.join(file))?;
    let (w, h) = resample::target_size(image.width, image.height, options.max_size);
    let (w, h) = (w.max(pica::MIN_SIZE), h.max(pica::MIN_SIZE));
    let scaled = flip_vertical(resample::resize(&image, w, h));
    let format = options.format.unwrap_or_else(|| auto_format(&scaled));
    let data = pica::encode(&scaled, format).map_err(|e| format!("{file}: {e}"))?;
    Ok(Built { key, width: w as u16, height: h as u16, format: gpu_format(format), data })
}

/// Converts level 0 of every entry. `load` decodes a PNG (main.rs).
pub fn build(dir: &Path, pack: &Pack, out: &Path, options: &Options, load: &(dyn Fn(&Path) -> Result<Image, String> + Sync)) -> Result<Summary, String> {
    std::fs::create_dir_all(out).map_err(|e| format!("{}: {e}", out.display()))?;
    let jobs: Vec<(Key, Option<String>)> = pack.entries.iter().filter_map(|(k, e)| e.levels.get(&0).map(|f| (*k, f.clone()))).collect();
    let next = AtomicUsize::new(0);
    let results: Mutex<Vec<Option<Result<Built, String>>>> = Mutex::new((0..jobs.len()).map(|_| None).collect());
    let threads = std::thread::available_parallelism().map_or(4, |n| n.get());
    std::thread::scope(|s| {
        for _ in 0..threads {
            s.spawn(|| loop {
                let i = next.fetch_add(1, Ordering::Relaxed);
                if i >= jobs.len() {
                    break;
                }
                let (key, file) = &jobs[i];
                let r = match file {
                    Some(f) => build_one(dir, *key, f, options, load),
                    None => Ok(Built { key: *key, width: 0, height: 0, format: 0, data: Vec::new() }),
                };
                results.lock().unwrap()[i] = Some(r);
            });
        }
    });
    let mut built = Vec::new();
    let mut summary = Summary { entries: 0, keep_original: 0, skipped: Vec::new(), data_bytes: 0, by_format: [0; 5] };
    for r in results.into_inner().unwrap().into_iter().flatten() {
        match r {
            Ok(b) => built.push(b),
            Err(e) => summary.skipped.push(e),
        }
    }
    built.sort_by_key(|b| (b.key.cachekey, b.key.hash));

    let data_path: PathBuf = out.join("textures.bin");
    let mut data = BufWriter::new(File::create(&data_path).map_err(|e| format!("{}: {e}", data_path.display()))?);
    let mut index = Vec::with_capacity(24 + built.len() * ENTRY_SIZE);
    index.extend_from_slice(MAGIC);
    index.extend_from_slice(&VERSION.to_le_bytes());
    index.extend_from_slice(&(pack.options.ignore_address as u32).to_le_bytes());
    index.extend_from_slice(&(built.len() as u32).to_le_bytes());
    index.extend_from_slice(&0u32.to_le_bytes());
    let mut offset: u64 = 0;
    for b in &built {
        if offset > u32::MAX as u64 {
            return Err("textures.bin would exceed 4 GiB; lower --max-size".into());
        }
        index.extend_from_slice(&b.key.cachekey.to_le_bytes());
        index.extend_from_slice(&b.key.hash.to_le_bytes());
        index.extend_from_slice(&(offset as u32).to_le_bytes());
        index.extend_from_slice(&(b.data.len() as u32).to_le_bytes());
        index.extend_from_slice(&b.width.to_le_bytes());
        index.extend_from_slice(&b.height.to_le_bytes());
        index.extend_from_slice(&[b.format, 0, 0, 0]);
        index.extend_from_slice(&0u32.to_le_bytes());
        if b.width == 0 {
            summary.keep_original += 1;
            continue;
        }
        data.write_all(&b.data).map_err(|e| e.to_string())?;
        let pad = (ALIGN - b.data.len() % ALIGN) % ALIGN;
        data.write_all(&vec![0; pad]).map_err(|e| e.to_string())?;
        offset += (b.data.len() + pad) as u64;
        summary.by_format[b.format.min(4) as usize] += 1;
    }
    data.flush().map_err(|e| e.to_string())?;
    std::fs::write(out.join("index.bin"), &index).map_err(|e| format!("{}: {e}", out.display()))?;
    summary.entries = built.len();
    summary.data_bytes = offset;
    Ok(summary)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn flips_rows() {
        let img = Image { width: 1, height: 3, rgba: vec![1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3] };
        assert_eq!(flip_vertical(img).rgba, vec![3, 3, 3, 3, 2, 2, 2, 2, 1, 1, 1, 1]);
    }

    #[test]
    fn auto_format_by_alpha() {
        let img = |a: [u8; 2]| Image { width: 2, height: 1, rgba: vec![1, 2, 3, a[0], 4, 5, 6, a[1]] };
        assert_eq!(auto_format(&img([255, 255])), Format::Rgb565);
        assert_eq!(auto_format(&img([255, 0])), Format::Rgba5551);
        assert_eq!(auto_format(&img([255, 128])), Format::Rgba4);
    }

    #[test]
    fn writes_sorted_index_and_aligned_data() {
        let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../build/texpack/test-build");
        let _ = std::fs::remove_dir_all(&root);
        let src = root.join("pack");
        std::fs::create_dir_all(&src).unwrap();
        let fixtures = Path::new(env!("CARGO_MANIFEST_DIR")).join("tests/fixtures");
        std::fs::copy(fixtures.join("all_values_16x16.png"), src.join("b.png")).unwrap();
        std::fs::copy(fixtures.join("pattern_16x8.png"), src.join("a.png")).unwrap();
        let ini = "[options]\nhash = xxh64\nignoreAddress = true\n[hashes]\n\
                   00000000000000ff00000002 = b.png\n0000000000000001aaaaaaaa = a.png\n000000000000000500000000 = |\n";
        let pack = crate::pack::parse_ini(ini, &[]).unwrap();
        let out = root.join("out");
        let options = Options { max_size: 8, format: Some(Format::Rgba8) };
        let summary = build(&src, &pack, &out, &options, &crate::load_png).unwrap();
        assert_eq!((summary.entries, summary.keep_original), (3, 1));
        let index = std::fs::read(out.join("index.bin")).unwrap();
        assert_eq!(&index[..8], MAGIC);
        assert_eq!(u32::from_le_bytes(index[12..16].try_into().unwrap()), 1);
        assert_eq!(u32::from_le_bytes(index[16..20].try_into().unwrap()), 3);
        assert_eq!(index.len(), 24 + 3 * ENTRY_SIZE);
        let entry = |i: usize| &index[24 + i * ENTRY_SIZE..24 + (i + 1) * ENTRY_SIZE];
        let ck = |i: usize| u64::from_le_bytes(entry(i)[..8].try_into().unwrap());
        assert_eq!((ck(0), ck(1), ck(2)), (1, 5, 0xff));
        // a.png 16x8 -> 8x8 at max 8 (aspect 2:1 -> 8x4, raised to the 8 minimum).
        let e0 = entry(0);
        assert_eq!(u16::from_le_bytes([e0[20], e0[21]]), 8);
        assert_eq!(u32::from_le_bytes(e0[16..20].try_into().unwrap()), 8 * 8 * 4);
        let e1 = entry(1);
        assert_eq!(u16::from_le_bytes([e1[20], e1[21]]), 0, "keep-original entry has no size");
        let e2 = entry(2);
        assert_eq!(u32::from_le_bytes(e2[12..16].try_into().unwrap()), 256, "second texture starts on the next 128-byte boundary");
        let data = std::fs::read(out.join("textures.bin")).unwrap();
        assert_eq!(data.len(), 512);
        // Stored bottom-up: the first entry equals encode() of the flipped, resized image.
        let a = crate::load_png(&src.join("a.png")).unwrap();
        let expect = pica::encode(&flip_vertical(resample::resize(&a, 8, 8)), Format::Rgba8).unwrap();
        assert_eq!(&data[..256], &expect[..]);
    }
}
