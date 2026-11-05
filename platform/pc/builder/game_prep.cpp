// See game_prep.hpp.
#include "game_prep.hpp"
#include "psp_eboot.hpp"

#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace fs = std::filesystem;

namespace p3p3ds::builder {

std::vector<std::uint8_t> read_source(const std::shared_ptr<const vfs::Source> &src) {
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(src->size()));
    if (!bytes.empty()) src->read(0, bytes.data(), bytes.size());
    return bytes;
}

std::string sfo_string(const std::vector<std::uint8_t> &d, const std::string &key) {
    auto le16 = [&](std::size_t o) { return static_cast<std::uint32_t>(d[o] | (d[o + 1] << 8)); };
    auto le32 = [&](std::size_t o) { return d[o] | (d[o + 1] << 8) | (d[o + 2] << 16) | (static_cast<std::uint32_t>(d[o + 3]) << 24); };
    if (d.size() < 20 || std::memcmp(d.data(), "\0PSF", 4) != 0) return {};
    const std::uint32_t keys = le32(8), values = le32(12), count = le32(16);
    for (std::uint32_t i = 0; i < count && 20 + i * 16 + 16 <= d.size(); ++i) {
        const std::size_t e = 20 + i * 16;
        const std::size_t name_at = keys + le16(e);
        if (name_at >= d.size()) continue;
        const std::string name(reinterpret_cast<const char *>(&d[name_at]), strnlen(reinterpret_cast<const char *>(&d[name_at]), d.size() - name_at));
        if (name != key) continue;
        const std::size_t at = values + le32(e + 12), len = le32(e + 4);
        if (at + len > d.size()) return {};
        return std::string(reinterpret_cast<const char *>(&d[at]), strnlen(reinterpret_cast<const char *>(&d[at]), len));
    }
    return {};
}

namespace {
std::string trim(std::string s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
} // namespace

PreparedExecutable prepare_executable(const fs::path &iso, const fs::path &decrypted_override) {
    {
        std::ifstream probe(iso, std::ios::binary);
        if (!probe) throw std::runtime_error("Cannot open ISO file: " + iso.string());
        char pvd[2048];
        probe.seekg(16 * 2048);
        probe.read(pvd, sizeof pvd);
        if (probe.gcount() != static_cast<std::streamsize>(sizeof pvd) || std::memcmp(pvd + 1, "CD001", 5) != 0)
            throw std::runtime_error("Not a valid ISO9660 disc image (CD001 identifier not found at sector 16)");
    }
    vfs::IsoFileSystem fsys(iso);
    PreparedExecutable out;

    const auto sfo_file = fsys.open("PSP_GAME/PARAM.SFO");
    if (!sfo_file) throw std::runtime_error("PARAM.SFO not found inside ISO (not a valid PSP game disc)");
    const auto sfo = read_source(sfo_file);
    if (sfo.size() < 20 || std::memcmp(sfo.data(), "\0PSF", 4) != 0) throw std::runtime_error("Invalid PARAM.SFO magic");
    for (char c : trim(sfo_string(sfo, "DISC_ID")))
        if (c != '-') out.disc_id += c;
    out.title = trim(sfo_string(sfo, "TITLE"));
    out.version = trim(sfo_string(sfo, "DISC_VERSION"));
    if (out.version.empty()) out.version = trim(sfo_string(sfo, "APP_VER"));
    if (out.version.empty()) out.version = "1.00";
    if (out.disc_id != kSupportedDiscId)
        throw std::runtime_error("Unsupported game disc ID: " + out.disc_id +
                                 ". P3P3DS currently supports only Persona 3 Portable USA (ULUS-10512).");

    if (!decrypted_override.empty()) {
        std::ifstream in(decrypted_override, std::ios::binary);
        if (!in) throw std::runtime_error("Supplied decrypted EBOOT not found: " + decrypted_override.string());
        out.elf.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        out.from_override = true;
    } else {
        const auto eboot_file = fsys.open("PSP_GAME/SYSDIR/EBOOT.BIN");
        if (!eboot_file) throw std::runtime_error("EBOOT.BIN not found inside ISO");
        const auto eboot = read_source(eboot_file);
        out.eboot_size = eboot.size();
        out.eboot_sha256 = sha256_hex(eboot);
        out.elf = decrypt_eboot(eboot);
    }

    out.elf_sha256 = sha256_hex(out.elf);
    if (out.elf_sha256 != kExpectedElfSha256)
        throw std::runtime_error(std::string("Executable SHA-256 mismatch!\n  Expected: ") + kExpectedElfSha256 + "\n  Actual:   " +
                                 out.elf_sha256 + "\nPlease verify that your disc is an untampered retail copy of ULUS-10512.");
    return out;
}

} // namespace p3p3ds::builder
