#pragma once

// PARAM.SFO: the key/value table the PSP keeps beside every save.
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mhp3rd::savedata {

class ParamSfo {
public:
    struct Entry {
        std::uint16_t format{};  // 0x0004 binary, 0x0204 UTF-8 string, 0x0404 integer
        std::uint32_t max_length{};
        std::vector<std::uint8_t> data;  // exactly the used bytes (strings include their NUL)
    };

    static constexpr std::uint16_t kBinary = 0x0004u;
    static constexpr std::uint16_t kString = 0x0204u;
    static constexpr std::uint16_t kInteger = 0x0404u;

    [[nodiscard]] static std::optional<ParamSfo> parse(std::span<const std::uint8_t> bytes);
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;

    void set_string(const std::string &key, const std::string &value, std::uint32_t max_length);
    void set_integer(const std::string &key, std::uint32_t value);
    void set_binary(const std::string &key, std::vector<std::uint8_t> value, std::uint32_t max_length);

    [[nodiscard]] std::optional<std::string> string(const std::string &key) const;
    [[nodiscard]] std::optional<std::uint32_t> integer(const std::string &key) const;
    [[nodiscard]] const std::vector<std::uint8_t> *binary(const std::string &key) const;

    // Offset of a value's data in the serialized file, for the hashes that
    // cover the file itself.
    [[nodiscard]] std::optional<std::size_t> data_offset(const std::string &key) const;

private:
    std::map<std::string, Entry> entries_;  // PSP files keep keys sorted
};

} // namespace mhp3rd::savedata
