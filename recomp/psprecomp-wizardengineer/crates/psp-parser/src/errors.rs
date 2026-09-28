use thiserror::Error;

#[derive(Debug, Error)]
pub enum ParseError {
    #[error("ELF parse error: {0}")]
    Elf(#[from] goblin::error::Error),
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),
    #[error("XML parse error: {0}")]
    Xml(#[from] roxmltree::Error),
    #[error("PRX format error: {message}")]
    Prx { message: String },
    #[error("Relocation error: {message}")]
    Reloc { message: String },
    #[error("NID error: {message}")]
    Nid { message: String },
    #[error(
        "encrypted ~PSP executable — this is not a plain ELF. Decrypt it first \
         (e.g. let PPSSPP run the game once, then take the decrypted BOOT.BIN \
         from its cache, or use a PSP to dump it); psprecomp has no PRX decryption"
    )]
    EncryptedPsp,
    #[error(
        "file begins with all zero bytes — likely a dummy/placeholder BOOT.BIN \
         (common in commercial dumps; the real code lives in the encrypted \
         EBOOT.BIN). Use a decrypted executable instead"
    )]
    DummyZeroes,
}
