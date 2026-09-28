//! Read-only virtual-address view over loaded (rebased, relocated) segments.
//!
//! After relocation, SceModuleInfo / libstub / libent pointer fields are final
//! virtual addresses (plan D6). This view lets those walkers read by VA
//! directly, killing the `vaddr_to_file_offset`-against-the-raw-file double
//! bookkeeping. File-offset reads remain only for ELF *structure*
//! (section/program-header tables).

use crate::errors::ParseError;

/// Read-only virtual-address view over loaded (rebased, relocated) segments.
pub struct LoadedImage<'a> {
    /// (segment base vaddr, segment bytes — length == p_memsz)
    segs: Vec<(u32, &'a [u8])>,
}

impl<'a> LoadedImage<'a> {
    /// Builds a view from parallel slices of segment base VAs and data buffers.
    ///
    /// `bases[i]` is the rebased virtual base of `datas[i]` (which must be
    /// memsz-sized, BSS included). Panics only on programmer error
    /// (mismatched lengths), never on binary content.
    pub fn new(bases: &[u32], datas: &'a [Vec<u8>]) -> Self {
        assert_eq!(
            bases.len(),
            datas.len(),
            "LoadedImage: bases and datas must be parallel arrays"
        );
        let segs = bases
            .iter()
            .copied()
            .zip(datas.iter().map(|d| d.as_slice()))
            .collect();
        Self { segs }
    }

    /// Little-endian u32 at `va`; `Err(ParseError::Prx)` when unmapped/truncated.
    pub fn read_u32(&self, va: u32) -> Result<u32, ParseError> {
        let bytes = self.read_bytes(va, 4)?;
        Ok(u32::from_le_bytes(
            bytes.try_into().expect("read_bytes returned 4 bytes"),
        ))
    }

    /// `len` bytes starting at `va`, fully contained in one segment.
    ///
    /// `Err(ParseError::Prx)` (with the offending VA) when `va` is unmapped or
    /// the range runs past the segment end.
    pub fn read_bytes(&self, va: u32, len: usize) -> Result<&'a [u8], ParseError> {
        let (off, data) = self.locate(va)?;
        let end = off
            .checked_add(len)
            .filter(|&end| end <= data.len())
            .ok_or_else(|| ParseError::Prx {
                message: format!(
                    "read of {len} bytes at virtual address 0x{va:08X} runs past the \
                     end of its segment"
                ),
            })?;
        Ok(&data[off..end])
    }

    /// NUL-terminated string at `va`, capped at `max` bytes.
    ///
    /// A string with no NUL within `max` bytes (or before the segment end) is
    /// truncated there. `Err(ParseError::Prx)` when `va` is unmapped.
    pub fn read_cstr(&self, va: u32, max: usize) -> Result<String, ParseError> {
        let (off, data) = self.locate(va)?;
        let avail = (data.len() - off).min(max);
        let slice = &data[off..off + avail];
        let end = slice.iter().position(|&b| b == 0).unwrap_or(avail);
        Ok(String::from_utf8_lossy(&slice[..end]).into_owned())
    }

    /// Resolves `va` to (offset within segment, segment bytes).
    fn locate(&self, va: u32) -> Result<(usize, &'a [u8]), ParseError> {
        for &(base, data) in &self.segs {
            if va >= base && ((va - base) as usize) < data.len() {
                return Ok(((va - base) as usize, data));
            }
        }
        Err(ParseError::Prx {
            message: format!("virtual address 0x{va:08X} is not mapped by any loaded segment"),
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn two_segment_image() -> (Vec<u32>, Vec<Vec<u8>>) {
        // Segment 0 at 0x08804000: u32 0xDEADBEEF, then "hacklink\0", padding.
        let mut seg0 = 0xDEAD_BEEFu32.to_le_bytes().to_vec();
        seg0.extend_from_slice(b"hacklink\0");
        seg0.resize(32, 0xAA);
        // Segment 1 at 0x09000000: u32 0x12345678, then NUL-less text to the end.
        let mut seg1 = 0x1234_5678u32.to_le_bytes().to_vec();
        seg1.extend_from_slice(b"abcdef");
        (vec![0x0880_4000, 0x0900_0000], vec![seg0, seg1])
    }

    #[test]
    fn read_u32_happy_and_cross_segment() {
        let (bases, datas) = two_segment_image();
        let img = LoadedImage::new(&bases, &datas);
        assert_eq!(img.read_u32(0x0880_4000).unwrap(), 0xDEAD_BEEF);
        assert_eq!(
            img.read_u32(0x0900_0000).unwrap(),
            0x1234_5678,
            "second segment"
        );
    }

    #[test]
    fn read_bytes_happy_and_truncated() {
        let (bases, datas) = two_segment_image();
        let img = LoadedImage::new(&bases, &datas);
        assert_eq!(img.read_bytes(0x0880_4004, 8).unwrap(), b"hacklink");
        let err = img.read_bytes(0x0900_0008, 16).unwrap_err();
        assert!(
            err.to_string().contains("0x09000008"),
            "error names the va: {err}"
        );
        assert!(
            err.to_string().contains("runs past"),
            "truncated read: {err}"
        );
    }

    #[test]
    fn unmapped_va_is_an_error_never_a_panic() {
        let (bases, datas) = two_segment_image();
        let img = LoadedImage::new(&bases, &datas);
        for va in [0u32, 0x0880_3FFF, 0x0880_4000 + 32, 0x0A00_0000] {
            let err = img.read_u32(va).unwrap_err();
            assert!(
                err.to_string().contains(&format!("0x{va:08X}")),
                "error must name the offending va: {err}"
            );
        }
        assert!(img.read_cstr(0x0700_0000, 64).is_err());
        assert!(img.read_bytes(0x0700_0000, 1).is_err());
    }

    #[test]
    fn read_cstr_nul_terminated_capped_and_segment_end() {
        let (bases, datas) = two_segment_image();
        let img = LoadedImage::new(&bases, &datas);
        assert_eq!(img.read_cstr(0x0880_4004, 64).unwrap(), "hacklink");
        assert_eq!(
            img.read_cstr(0x0880_4004, 4).unwrap(),
            "hack",
            "capped at max"
        );
        // No NUL before the segment end: truncates at the end, no panic.
        assert_eq!(img.read_cstr(0x0900_0004, 64).unwrap(), "abcdef");
    }
}
