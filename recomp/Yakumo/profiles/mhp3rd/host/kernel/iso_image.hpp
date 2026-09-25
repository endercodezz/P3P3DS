#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mhp3rd {

// Read-only ISO9660 view of a PSP UMD image. Paths are case-insensitive and
// use '/' separators relative to the image root (e.g. "PSP_GAME/USRDIR/DATA.BIN").
class IsoImage {
public:
    static constexpr std::uint32_t kSectorSize = 2048u;

    struct Entry {
        std::uint32_t lba{};
        std::uint32_t size{};
        bool directory{};
    };

    explicit IsoImage(const std::filesystem::path &path);

    [[nodiscard]] std::optional<Entry> find(std::string path) const;
    [[nodiscard]] std::uint64_t size_bytes() const noexcept { return size_bytes_; }
    // Reads up to output.size() bytes at an absolute image offset.
    std::size_t read(std::uint64_t offset, std::span<std::uint8_t> output);
    [[nodiscard]] std::vector<std::string> list(std::string directory) const;

private:
    void scan_directory(std::uint32_t lba, std::uint32_t size, const std::string &prefix, int depth);
    [[nodiscard]] static std::string normalize(std::string path);

    std::ifstream file_;
    std::uint64_t size_bytes_{};
    std::map<std::string, Entry> entries_;
};

} // namespace mhp3rd
