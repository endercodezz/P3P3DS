#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace psprecomp {

struct NidSymbol {
    std::string library;
    std::uint32_t nid{};
    std::string name;
};

class NidRegistry {
public:
    NidRegistry();
    void add(std::string library, std::uint32_t nid, std::string name);
    void load_csv(const std::filesystem::path &path);
    [[nodiscard]] std::optional<std::string> resolve(const std::string &library, std::uint32_t nid) const;
    [[nodiscard]] std::vector<NidSymbol> all() const;

private:
    [[nodiscard]] static std::string key(const std::string &library, std::uint32_t nid);
    std::unordered_map<std::string, NidSymbol> symbols_;
};

} // namespace psprecomp
