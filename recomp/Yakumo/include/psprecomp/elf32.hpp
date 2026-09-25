#pragma once

#include "psprecomp/guest_memory.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace psprecomp {

inline constexpr std::uint16_t kElfTypePspPrx = 0xFFA0u;
inline constexpr std::uint32_t kDefaultPspUserLoadBase = 0x08804000u;
inline constexpr std::uint32_t kSectionTypePspRel = 0x700000A0u;

struct ElfSegment {
    std::uint32_t type{};
    std::uint32_t offset{};
    std::uint32_t vaddr{};
    std::uint32_t paddr{};
    std::uint32_t file_size{};
    std::uint32_t memory_size{};
    std::uint32_t flags{};
    std::uint32_t alignment{};
};

struct ElfSection {
    std::string name;
    std::uint32_t type{};
    std::uint32_t flags{};
    std::uint32_t address{};
    std::uint32_t offset{};
    std::uint32_t size{};
    std::uint32_t link{};
    std::uint32_t info{};
    std::uint32_t alignment{};
    std::uint32_t entry_size{};
};

struct PspModuleInfo {
    std::uint16_t attributes{};
    std::uint8_t minor_version{};
    std::uint8_t major_version{};
    std::string name;
    std::uint32_t gp{};
    std::uint32_t ent_top{};
    std::uint32_t ent_end{};
    std::uint32_t stub_top{};
    std::uint32_t stub_end{};
    std::uint32_t address{};
};

struct PspImport {
    std::string library;
    std::uint32_t nid{};
    std::uint32_t stub_address{};
};


struct PspRelocationSite {
    std::uint32_t patch_address{};
    std::uint32_t type{};
    std::uint32_t patch_segment{};
    std::uint32_t target_segment{};
};

struct RelocationStats {
    std::uint32_t total{};
    std::uint32_t r_mips_32{};
    std::uint32_t r_mips_26{};
    std::uint32_t r_mips_hi16{};
    std::uint32_t r_mips_lo16{};
    std::uint32_t unsupported{};
    std::uint32_t invalid{};
};

class Elf32Image {
public:
    static Elf32Image from_file(const std::filesystem::path &path);
    static Elf32Image from_bytes(std::vector<std::uint8_t> bytes, std::string source_name = "<memory>");

    [[nodiscard]] std::uint32_t entry() const noexcept;
    [[nodiscard]] std::uint32_t runtime_entry(std::uint32_t load_base = kDefaultPspUserLoadBase) const noexcept;
    [[nodiscard]] std::uint16_t type() const noexcept;
    [[nodiscard]] bool is_psp_prx() const noexcept;
    [[nodiscard]] const std::vector<std::uint8_t> &bytes() const noexcept;
    [[nodiscard]] const std::vector<ElfSegment> &segments() const noexcept;
    [[nodiscard]] const std::vector<ElfSection> &sections() const noexcept;
    [[nodiscard]] const std::string &source_name() const noexcept;

    [[nodiscard]] std::uint32_t segment_runtime_address(std::size_t segment_index,
                                                        std::uint32_t load_base = kDefaultPspUserLoadBase) const;
    [[nodiscard]] std::uint32_t section_runtime_address(const ElfSection &section,
                                                        std::uint32_t load_base = kDefaultPspUserLoadBase) const noexcept;

    // Smallest supported guest RAM (32 or 64 MiB) that holds every PT_LOAD
    // segment including its BSS. Extended-memory titles such as PS3 PSP
    // remasters place BSS past the classic 0x0A000000 user RAM limit.
    [[nodiscard]] std::uint32_t required_ram_size(std::uint32_t load_base = kDefaultPspUserLoadBase) const;

    void load_into(GuestMemory &memory, std::uint32_t load_base = kDefaultPspUserLoadBase) const;
    [[nodiscard]] RelocationStats apply_relocations(GuestMemory &memory,
                                                    std::uint32_t load_base = kDefaultPspUserLoadBase) const;
    [[nodiscard]] RelocationStats load_and_relocate(GuestMemory &memory,
                                                   std::uint32_t load_base = kDefaultPspUserLoadBase) const;
    [[nodiscard]] std::vector<PspRelocationSite> relocation_sites(
        std::uint32_t load_base = kDefaultPspUserLoadBase) const;

    [[nodiscard]] std::optional<PspModuleInfo> find_module_info(
        const GuestMemory &memory, std::uint32_t load_base = kDefaultPspUserLoadBase) const;
    [[nodiscard]] std::vector<PspImport> scan_imports(const GuestMemory &memory,
                                                      const PspModuleInfo &module) const;
    [[nodiscard]] std::uint32_t read_word_at_vaddr(std::uint32_t address) const;

private:
    void parse();
    [[nodiscard]] std::span<const std::uint8_t> checked_span(std::size_t offset, std::size_t length) const;
    [[nodiscard]] std::vector<std::uint32_t> segment_runtime_addresses(std::uint32_t load_base) const;

    std::vector<std::uint8_t> bytes_;
    std::string source_name_;
    std::uint16_t type_{};
    std::uint32_t entry_{};
    std::vector<ElfSegment> segments_;
    std::vector<ElfSection> sections_;
};

} // namespace psprecomp
