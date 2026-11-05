//! p3p-texpack: PPSSPP texture packs -> P3P3DS (New 3DS) textures. Host tool.
//!
//!   p3p-texpack inspect <pack dir>
//!   p3p-texpack keys <pack dir>
//!   p3p-texpack encode <image.png> --format <fmt> --out <file>
//!
//! `encode` writes PICA200 texture data (no header), the same bytes as
//! `tex3ds -r -z none -f <fmt>` after its 4-byte header. Exit codes: 0 ok,
//! 1 error, 2 usage.

mod pack;
mod pica;

use std::fs::File;
use std::io::BufReader;
use std::path::{Path, PathBuf};
use std::process::ExitCode;

const USAGE: &str = "usage: p3p-texpack inspect <pack dir>\n       p3p-texpack keys <pack dir>\n       p3p-texpack encode <image.png> --format <fmt> --out <file>\n\
                     formats: rgba8 rgb8 rgb565 rgba5551 rgba4";

/// Largest source image accepted (packs ship upscales; the 3DS limit applies at encode).
const MAX_SOURCE_BYTES: usize = 512 * 1024 * 1024;

struct PngInfo {
    width: u32,
    height: u32,
    color: png::ColorType,
    depth: png::BitDepth,
}

fn png_info(path: &Path) -> Result<PngInfo, String> {
    let file = File::open(path).map_err(|e| format!("{}: {e}", path.display()))?;
    let decoder = png::Decoder::new(BufReader::new(file));
    let reader = decoder.read_info().map_err(|e| format!("{}: {e}", path.display()))?;
    let info = reader.info();
    Ok(PngInfo { width: info.width, height: info.height, color: info.color_type, depth: info.bit_depth })
}

fn load_png(path: &Path) -> Result<pica::Image, String> {
    let file = File::open(path).map_err(|e| format!("{}: {e}", path.display()))?;
    let mut decoder = png::Decoder::new_with_limits(BufReader::new(file), png::Limits { bytes: MAX_SOURCE_BYTES });
    decoder.set_transformations(png::Transformations::normalize_to_color8());
    let mut reader = decoder.read_info().map_err(|e| format!("{}: {e}", path.display()))?;
    let mut buf = vec![0; reader.output_buffer_size()];
    let frame = reader.next_frame(&mut buf).map_err(|e| format!("{}: {e}", path.display()))?;
    let (w, h) = (frame.width as usize, frame.height as usize);
    let src = &buf[..frame.buffer_size()];
    let mut rgba = Vec::with_capacity(w * h * 4);
    match frame.color_type {
        png::ColorType::Rgba => rgba.extend_from_slice(src),
        png::ColorType::Rgb => src.as_chunks::<3>().0.iter().for_each(|&[r, g, b]| rgba.extend_from_slice(&[r, g, b, 255])),
        png::ColorType::GrayscaleAlpha => src.as_chunks::<2>().0.iter().for_each(|&[g, a]| rgba.extend_from_slice(&[g, g, g, a])),
        png::ColorType::Grayscale => src.iter().for_each(|&g| rgba.extend_from_slice(&[g, g, g, 255])),
        png::ColorType::Indexed => return Err(format!("{}: palette not expanded", path.display())),
    }
    Ok(pica::Image { width: w, height: h, rgba })
}

fn inspect(dir: &Path) -> Result<(), String> {
    let pack = pack::load(dir).map_err(|e| e.to_string())?;
    let o = &pack.options;
    println!("hash={:?} ignoreAddress={} reduceHash={} ignoreMipmap={} version={}",
             o.hash.unwrap(), o.ignore_address, o.reduce_hash, o.ignore_mipmap,
             o.version.map_or("-".to_string(), |v| v.to_string()));
    let (mut files, mut skipped, mut missing, mut unsupported, mut oversize, mut bad) = (0, 0, 0, 0, 0, 0);
    for (key, entry) in &pack.entries {
        for (level, file) in &entry.levels {
            let Some(file) = file else { skipped += 1; continue };
            files += 1;
            let path = dir.join(file);
            if !path.is_file() {
                missing += 1;
                println!("missing  {key}_{level} {file}");
                continue;
            }
            if !file.to_ascii_lowercase().ends_with(".png") {
                unsupported += 1;
                println!("format   {key}_{level} {file} (only PNG is supported)");
                continue;
            }
            match png_info(&path) {
                Ok(i) => {
                    let (tw, th) = (pica::texture_dim(i.width as usize), pica::texture_dim(i.height as usize));
                    if tw > pica::MAX_SIZE || th > pica::MAX_SIZE {
                        oversize += 1;
                        println!("large    {key}_{level} {file} {}x{} {:?}/{:?} (needs downscaling for the 3DS)",
                                 i.width, i.height, i.color, i.depth);
                    }
                }
                Err(e) => {
                    bad += 1;
                    println!("bad      {key}_{level} {e}");
                }
            }
        }
    }
    for w in &pack.warnings {
        println!("warning  {w}");
    }
    for (name, lines) in &pack.other_sections {
        println!("section  [{name}] {lines} lines (not used yet)");
    }
    println!("entries={} files={files} no_replace={skipped} missing={missing} unsupported={unsupported} \
              over_1024={oversize} unreadable={bad} warnings={}",
             pack.entries.len(), pack.warnings.len());
    Ok(())
}

/// One `<16 hex><8 hex>` key per line (all mip levels and "do not replace"
/// entries included): the list the PC runner checks with --texture-keys.
fn keys(dir: &Path) -> Result<(), String> {
    let pack = pack::load(dir).map_err(|e| e.to_string())?;
    if pack.options.ignore_address {
        println!("# ignoreAddress");
    }
    for key in pack.entries.keys() {
        println!("{key}");
    }
    Ok(())
}

fn encode(input: &Path, format: pica::Format, out: &Path) -> Result<(), String> {
    let image = load_png(input)?;
    let data = pica::encode(&image, format).map_err(|e| format!("{}: {e}", input.display()))?;
    std::fs::write(out, &data).map_err(|e| format!("{}: {e}", out.display()))?;
    println!("{}x{} -> {}x{} {format} {} bytes", image.width, image.height,
             pica::texture_dim(image.width), pica::texture_dim(image.height), data.len());
    Ok(())
}

fn run(args: &[String]) -> Result<Result<(), String>, ()> {
    match args {
        [cmd, dir] if cmd == "inspect" => Ok(inspect(Path::new(dir))),
        [cmd, dir] if cmd == "keys" => Ok(keys(Path::new(dir))),
        [cmd, input, rest @ ..] if cmd == "encode" => {
            let (mut format, mut out) = (None, None);
            let mut it = rest.iter();
            while let Some(a) = it.next() {
                match (a.as_str(), it.next()) {
                    ("--format", Some(f)) => format = Some(pica::Format::parse(f).ok_or(())?),
                    ("--out", Some(o)) => out = Some(PathBuf::from(o)),
                    _ => return Err(()),
                }
            }
            Ok(encode(Path::new(input), format.ok_or(())?, &out.ok_or(())?))
        }
        _ => Err(()),
    }
}

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().skip(1).collect();
    match run(&args) {
        Ok(Ok(())) => ExitCode::SUCCESS,
        Ok(Err(e)) => {
            eprintln!("error: {e}");
            ExitCode::from(1)
        }
        Err(()) => {
            eprintln!("{USAGE}");
            ExitCode::from(2)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Every fixture image, every format: byte-identical to tex3ds
    /// (tests/fixtures/make_fixtures.py regenerates the references).
    #[test]
    fn matches_tex3ds_fixtures() {
        let dir = Path::new(env!("CARGO_MANIFEST_DIR")).join("tests/fixtures");
        let mut checked = 0;
        let mut failures = Vec::new();
        let mut pngs: Vec<_> = std::fs::read_dir(&dir).unwrap().map(|e| e.unwrap().path())
            .filter(|p| p.extension().is_some_and(|e| e == "png")).collect();
        pngs.sort();
        for png in &pngs {
            let image = load_png(png).unwrap();
            for format in pica::Format::ALL {
                let reference = std::fs::read(png.with_extension(format!("{format}.tex"))).unwrap();
                let ours = pica::encode(&image, format).unwrap();
                checked += 1;
                if ours != reference {
                    let at = ours.iter().zip(&reference).position(|(a, b)| a != b).unwrap_or(ours.len().min(reference.len()));
                    failures.push(format!("{} {format}: differs at byte {at} (ours {} bytes, tex3ds {})",
                                          png.file_name().unwrap().to_string_lossy(), ours.len(), reference.len()));
                }
            }
        }
        assert!(checked >= 40, "only {checked} fixture checks");
        assert!(failures.is_empty(), "{}", failures.join("\n"));
    }

    #[test]
    fn rejects_malformed_png() {
        let dir = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../build/texpack/test-input");
        std::fs::create_dir_all(&dir).unwrap();
        let good = std::fs::read(Path::new(env!("CARGO_MANIFEST_DIR")).join("tests/fixtures/pattern_16x8.png")).unwrap();
        let cases: [(&str, Vec<u8>); 3] = [
            ("empty.png", vec![]),
            ("truncated.png", good[..good.len() / 2].to_vec()),
            ("not_png.png", b"textures.ini".to_vec()),
        ];
        for (name, bytes) in cases {
            let path = dir.join(name);
            std::fs::write(&path, bytes).unwrap();
            assert!(load_png(&path).is_err(), "{name} was accepted");
        }
    }
}
