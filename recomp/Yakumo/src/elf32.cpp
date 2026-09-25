#include "psprecomp/elf32.hpp"
#include "psprecomp/common.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>

namespace psprecomp {
namespace {
#pragma pack(push, 1)
struct ElfHeader32 {
    std::uint8_t ident[16];
    std::uint16_t type;
    std::uint16_t machine;
    std::uint32_t version;
    std::uint32_t entry;
    std::uint32_t phoff;
    std::uint32_t shoff;
    std::uint32_t flags;
    std::uint16_t ehsize;
    std::uint16_t phentsize;
    std::uint16_t phnum;
    std::uint16_t shentsize;
    std::uint16_t shnum;
    std::uint16_t shstrndx;
};
struct ProgramHeader32 {
    std::uint32_t type, offset, vaddr, paddr, filesz, memsz, flags, align;
};
struct SectionHeader32 {
    std::uint32_t name, type, flags, addr, offset, size, link, info, addralign, entsize;
};
struct ModuleInfo32 {
    std::uint16_t attributes;
    std::uint8_t version[2];
    char name[28];
    std::uint32_t gp, ent_top, ent_end, stub_top, stub_end;
};
struct Rel32 {
    std::uint32_t offset;
    std::uint32_t info;
};
#pragma pack(pop)

static_assert(sizeof(ElfHeader32) == 52);
static_assert(sizeof(ProgramHeader32) == 32);
static_assert(sizeof(SectionHeader32) == 40);
static_assert(sizeof(ModuleInfo32) == 52);
static_assert(sizeof(Rel32) == 8);

constexpr std::uint32_t kPtLoad = 1u;
constexpr std::uint32_t kShtNobits = 8u;
constexpr std::uint32_t kRMips32 = 2u;
constexpr std::uint32_t kRMips26 = 4u;
constexpr std::uint32_t kRMipsHi16 = 5u;
constexpr std::uint32_t kRMipsLo16 = 6u;

std::uint32_t load_u32(const std::uint8_t *p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8u) |
           (static_cast<std::uint32_t>(p[2]) << 16u) | (static_cast<std::uint32_t>(p[3]) << 24u);
}

std::uint16_t relocated_hi(std::uint32_t value) {
    const auto signed_low = static_cast<std::int16_t>(value & 0xFFFFu);
    const auto corrected = value - static_cast<std::uint32_t>(static_cast<std::int32_t>(signed_low));
    return static_cast<std::uint16_t>((corrected >> 16u) & 0xFFFFu);
}
}

Elf32Image Elf32Image::from_file(const std::filesystem::path &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw Error("Cannot open PSP executable: " + path.string());
    in.seekg(0, std::ios::end);
    const auto end = in.tellg();
    if (end <= 0) throw Error("PSP executable is empty: " + path.string());
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!in) throw Error("Failed reading PSP executable: " + path.string());
    return from_bytes(std::move(bytes), path.string());
}

Elf32Image Elf32Image::from_bytes(std::vector<std::uint8_t> bytes, std::string source_name) {
    Elf32Image image;
    image.bytes_ = std::move(bytes);
    image.source_name_ = std::move(source_name);
    image.parse();
    return image;
}

std::span<const std::uint8_t> Elf32Image::checked_span(std::size_t offset, std::size_t length) const {
    if (offset > bytes_.size() || length > bytes_.size() - offset) {
        throw Error("Truncated ELF structure in " + source_name_);
    }
    return std::span<const std::uint8_t>(bytes_).subspan(offset, length);
}

void Elf32Image::parse() {
    if (bytes_.size() >= 4u && bytes_[0] == '~' && bytes_[1] == 'P' && bytes_[2] == 'S' && bytes_[3] == 'P') {
        throw Error("Encrypted/compressed ~PSP executable detected. Provide a decrypted ELF/PRX dump.");
    }
    if (bytes_.size() >= 4u && bytes_[0] == 0x00 && bytes_[1] == 'P' && bytes_[2] == 'B' && bytes_[3] == 'P') {
        throw Error("PBP container detected. Extract DATA.PSP or provide BOOT.BIN/decrypted EBOOT.BIN.");
    }

    ElfHeader32 header{};
    const auto header_bytes = checked_span(0u, sizeof(header));
    std::memcpy(&header, header_bytes.data(), sizeof(header));
    if (header.ident[0] != 0x7Fu || header.ident[1] != 'E' || header.ident[2] != 'L' || header.ident[3] != 'F') {
        throw Error("Not an ELF file: " + source_name_);
    }
    if (header.ident[4] != 1u || header.ident[5] != 1u) throw Error("PSPRecomp requires ELF32 little-endian input");
    if (header.machine != 8u) throw Error("ELF machine is not MIPS/Allegrex");
    if (header.phentsize != 0u && header.phentsize < sizeof(ProgramHeader32)) throw Error("Invalid ELF program header size");
    if (header.shentsize != 0u && header.shentsize < sizeof(SectionHeader32)) throw Error("Invalid ELF section header size");
    type_ = header.type;
    entry_ = header.entry;

    segments_.clear();
    for (std::uint16_t i = 0; i < header.phnum; ++i) {
        const std::size_t off = static_cast<std::size_t>(header.phoff) + static_cast<std::size_t>(i) * header.phentsize;
        ProgramHeader32 ph{};
        const auto data = checked_span(off, sizeof(ph));
        std::memcpy(&ph, data.data(), sizeof(ph));
        if (ph.filesz > 0u) (void)checked_span(ph.offset, ph.filesz);
        segments_.push_back({ph.type, ph.offset, ph.vaddr, ph.paddr, ph.filesz, ph.memsz, ph.flags, ph.align});
    }

    std::vector<SectionHeader32> raw_sections;
    raw_sections.reserve(header.shnum);
    for (std::uint16_t i = 0; i < header.shnum; ++i) {
        const std::size_t off = static_cast<std::size_t>(header.shoff) + static_cast<std::size_t>(i) * header.shentsize;
        SectionHeader32 sh{};
        const auto data = checked_span(off, sizeof(sh));
        std::memcpy(&sh, data.data(), sizeof(sh));
        if (sh.type != kShtNobits && sh.size > 0u) (void)checked_span(sh.offset, sh.size);
        raw_sections.push_back(sh);
    }

    std::span<const std::uint8_t> string_table;
    if (header.shstrndx < raw_sections.size()) {
        const auto &sh = raw_sections[header.shstrndx];
        string_table = checked_span(sh.offset, sh.size);
    }
    sections_.clear();
    for (const auto &sh : raw_sections) {
        std::string name;
        if (!string_table.empty() && sh.name < string_table.size()) {
            const char *start = reinterpret_cast<const char *>(string_table.data() + sh.name);
            const std::size_t max = string_table.size() - sh.name;
            const auto *end = static_cast<const char *>(std::memchr(start, '\0', max));
            if (end) name.assign(start, end);
        }
        sections_.push_back({name, sh.type, sh.flags, sh.addr, sh.offset, sh.size, sh.link, sh.info, sh.addralign, sh.entsize});
    }
}

std::uint32_t Elf32Image::entry() const noexcept { return entry_; }
std::uint32_t Elf32Image::runtime_entry(std::uint32_t load_base) const noexcept {
    return is_psp_prx() ? load_base + entry_ : entry_;
}
std::uint16_t Elf32Image::type() const noexcept { return type_; }
bool Elf32Image::is_psp_prx() const noexcept { return type_ == kElfTypePspPrx; }
const std::vector<std::uint8_t> &Elf32Image::bytes() const noexcept { return bytes_; }
const std::vector<ElfSegment> &Elf32Image::segments() const noexcept { return segments_; }
const std::vector<ElfSection> &Elf32Image::sections() const noexcept { return sections_; }
const std::string &Elf32Image::source_name() const noexcept { return source_name_; }

std::vector<std::uint32_t> Elf32Image::segment_runtime_addresses(std::uint32_t load_base) const {
    std::vector<std::uint32_t> result;
    result.reserve(segments_.size());
    for (const auto &segment : segments_) {
        result.push_back(is_psp_prx() ? load_base + segment.vaddr : segment.vaddr);
    }
    return result;
}

std::uint32_t Elf32Image::segment_runtime_address(std::size_t segment_index, std::uint32_t load_base) const {
    if (segment_index >= segments_.size()) throw Error("PSP segment index outside program header table");
    return is_psp_prx() ? load_base + segments_[segment_index].vaddr : segments_[segment_index].vaddr;
}

std::uint32_t Elf32Image::section_runtime_address(const ElfSection &section, std::uint32_t load_base) const noexcept {
    return is_psp_prx() ? load_base + section.address : section.address;
}

std::uint32_t Elf32Image::required_ram_size(std::uint32_t load_base) const {
    constexpr std::uint32_t kSmallRam = 32u * 1024u * 1024u;
    constexpr std::uint32_t kLargeRam = 64u * 1024u * 1024u;
    std::uint64_t image_end = 0u;
    for (std::size_t i = 0; i < segments_.size(); ++i) {
        const auto &segment = segments_[i];
        if (segment.type != kPtLoad) continue;
        const std::uint64_t start = GuestMemory::canonical(segment_runtime_address(i, load_base));
        image_end = std::max(image_end, start + segment.memory_size);
    }
    if (image_end <= static_cast<std::uint64_t>(GuestMemory::kPhysicalBase) + kSmallRam) return kSmallRam;
    if (image_end <= static_cast<std::uint64_t>(GuestMemory::kPhysicalBase) + kLargeRam) return kLargeRam;
    throw Error("ELF load image does not fit in 64 MiB PSP RAM");
}

void Elf32Image::load_into(GuestMemory &memory, std::uint32_t load_base) const {
    bool loaded = false;
    for (std::size_t i = 0; i < segments_.size(); ++i) {
        const auto &segment = segments_[i];
        if (segment.type != kPtLoad) continue;
        if (segment.memory_size < segment.file_size) throw Error("ELF PT_LOAD memsz is smaller than filesz");
        const std::uint32_t address = segment_runtime_address(i, load_base);
        const auto file_data = checked_span(segment.offset, segment.file_size);
        memory.copy_in(address, file_data);
        if (segment.memory_size > segment.file_size) {
            memory.zero(address + segment.file_size, segment.memory_size - segment.file_size);
        }
        loaded = true;
    }
    if (!loaded) throw Error("ELF contains no PT_LOAD segment");
}

RelocationStats Elf32Image::apply_relocations(GuestMemory &memory, std::uint32_t load_base) const {
    RelocationStats stats{};
    if (!is_psp_prx()) return stats;

    const auto segment_addresses = segment_runtime_addresses(load_base);
    for (const auto &section : sections_) {
        if (section.type != kSectionTypePspRel) continue;
        if ((section.size % sizeof(Rel32)) != 0u) throw Error("Malformed PSP relocation section " + section.name);
        const std::size_t count = section.size / sizeof(Rel32);
        const auto raw = checked_span(section.offset, section.size);
        std::vector<Rel32> rels(count);
        std::vector<std::uint32_t> original_ops(count);
        std::vector<bool> valid(count, true);
        std::memcpy(rels.data(), raw.data(), raw.size());

        for (std::size_t i = 0; i < count; ++i) {
            const std::uint32_t patch_segment = (rels[i].info >> 8u) & 0xFFu;
            const std::uint32_t type = rels[i].info & 0x0Fu;
            if (patch_segment >= segment_addresses.size()) {
                valid[i] = false;
                ++stats.invalid;
                continue;
            }
            const std::uint32_t patch_address = segment_addresses[patch_segment] + rels[i].offset;
            if (!memory.contains(patch_address, 4u) || ((patch_address & 3u) != 0u && type != kRMips32)) {
                valid[i] = false;
                ++stats.invalid;
                continue;
            }
            original_ops[i] = memory.load32(patch_address);
        }

        for (std::size_t i = 0; i < count; ++i) {
            ++stats.total;
            if (!valid[i]) continue;
            const std::uint32_t type = rels[i].info & 0x0Fu;
            const std::uint32_t patch_segment = (rels[i].info >> 8u) & 0xFFu;
            const std::uint32_t target_segment = (rels[i].info >> 16u) & 0xFFu;
            if (target_segment >= segment_addresses.size()) {
                ++stats.invalid;
                continue;
            }
            const std::uint32_t patch_address = segment_addresses[patch_segment] + rels[i].offset;
            const std::uint32_t relocate_to = segment_addresses[target_segment];
            std::uint32_t op = original_ops[i];

            switch (type) {
            case kRMips32:
                op += relocate_to;
                ++stats.r_mips_32;
                break;
            case kRMips26:
                op = (op & 0xFC000000u) |
                     (((op & 0x03FFFFFFu) + (relocate_to >> 2u)) & 0x03FFFFFFu);
                ++stats.r_mips_26;
                break;
            case kRMipsHi16: {
                bool paired = false;
                const std::uint32_t identity = rels[i].info >> 8u;
                for (std::size_t j = i + 1u; j < count; ++j) {
                    const std::uint32_t candidate_type = rels[j].info & 0x0Fu;
                    if (candidate_type == kRMipsHi16) continue;
                    if (candidate_type != kRMipsLo16 || (rels[j].info >> 8u) != identity || !valid[j]) continue;
                    const auto low = static_cast<std::int16_t>(original_ops[j] & 0xFFFFu);
                    std::uint32_t value = (op & 0xFFFFu) << 16u;
                    value += static_cast<std::uint32_t>(static_cast<std::int32_t>(low));
                    value += relocate_to;
                    op = (op & 0xFFFF0000u) | relocated_hi(value);
                    paired = true;
                    break;
                }
                if (!paired) {
                    ++stats.invalid;
                    continue;
                }
                ++stats.r_mips_hi16;
                break;
            }
            case kRMipsLo16:
                op = (op & 0xFFFF0000u) | ((op + relocate_to) & 0xFFFFu);
                ++stats.r_mips_lo16;
                break;
            default:
                ++stats.unsupported;
                continue;
            }
            memory.store32(patch_address, op);
        }
    }
    return stats;
}

RelocationStats Elf32Image::load_and_relocate(GuestMemory &memory, std::uint32_t load_base) const {
    load_into(memory, load_base);
    return apply_relocations(memory, load_base);
}

std::vector<PspRelocationSite> Elf32Image::relocation_sites(std::uint32_t load_base) const {
    std::vector<PspRelocationSite> sites;
    if (!is_psp_prx()) return sites;
    const auto segment_addresses = segment_runtime_addresses(load_base);
    for (const auto &section : sections_) {
        if (section.type != kSectionTypePspRel) continue;
        if ((section.size % sizeof(Rel32)) != 0u) throw Error("Malformed PSP relocation section " + section.name);
        const auto raw = checked_span(section.offset, section.size);
        const std::size_t count = section.size / sizeof(Rel32);
        for (std::size_t i = 0; i < count; ++i) {
            Rel32 rel{};
            std::memcpy(&rel, raw.data() + i * sizeof(Rel32), sizeof(Rel32));
            const std::uint32_t patch_segment = (rel.info >> 8u) & 0xFFu;
            const std::uint32_t target_segment = (rel.info >> 16u) & 0xFFu;
            if (patch_segment >= segment_addresses.size() || target_segment >= segment_addresses.size()) continue;
            sites.push_back({segment_addresses[patch_segment] + rel.offset,
                             rel.info & 0x0Fu, patch_segment, target_segment});
        }
    }
    return sites;
}

std::optional<PspModuleInfo> Elf32Image::find_module_info(const GuestMemory &memory, std::uint32_t load_base) const {
    const auto section_it = std::find_if(sections_.begin(), sections_.end(), [](const ElfSection &section) {
        return section.name == ".rodata.sceModuleInfo" || section.name == ".sceModuleInfo";
    });
    if (section_it == sections_.end() || section_it->size < sizeof(ModuleInfo32)) return std::nullopt;
    const std::uint32_t addr = section_runtime_address(*section_it, load_base);
    if (!memory.contains(addr, sizeof(ModuleInfo32))) return std::nullopt;

    char name[28]{};
    for (std::size_t i = 0; i < sizeof(name); ++i) {
        name[i] = static_cast<char>(memory.load8(addr + 4u + static_cast<std::uint32_t>(i)));
    }
    const auto nul = std::find(std::begin(name), std::end(name), '\0');
    return PspModuleInfo{
        memory.load16(addr), memory.load8(addr + 2u), memory.load8(addr + 3u), std::string(name, nul),
        memory.load32(addr + 32u), memory.load32(addr + 36u), memory.load32(addr + 40u),
        memory.load32(addr + 44u), memory.load32(addr + 48u), addr,
    };
}

std::vector<PspImport> Elf32Image::scan_imports(const GuestMemory &memory, const PspModuleInfo &module) const {
    std::vector<PspImport> imports;
    std::uint32_t cursor = module.stub_top;
    while (cursor < module.stub_end) {
        if (!memory.contains(cursor, 20u)) throw Error("Invalid PSP import stub table at " + hex32(cursor));
        const std::uint32_t libname = memory.load32(cursor);
        const std::uint8_t length_words = memory.load8(cursor + 8u);
        const std::uint16_t count = memory.load16(cursor + 10u);
        const std::uint32_t nid_table = memory.load32(cursor + 12u);
        const std::uint32_t stub_table = memory.load32(cursor + 16u);
        const std::uint32_t table_bytes = std::max<std::uint32_t>(20u, static_cast<std::uint32_t>(length_words) * 4u);
        if (table_bytes == 0u || cursor + table_bytes < cursor || cursor + table_bytes > module.stub_end) {
            throw Error("Corrupt PSP import table length at " + hex32(cursor));
        }
        const std::string library = memory.read_c_string(libname, 128u);
        if (!memory.contains(nid_table, static_cast<std::size_t>(count) * 4u)) throw Error("PSP import NID table outside RAM");
        if (!memory.contains(stub_table, static_cast<std::size_t>(count) * 8u)) throw Error("PSP import stub table outside RAM");
        for (std::uint16_t i = 0; i < count; ++i) {
            imports.push_back({library, memory.load32(nid_table + static_cast<std::uint32_t>(i) * 4u),
                               stub_table + static_cast<std::uint32_t>(i) * 8u});
        }
        cursor += table_bytes;
    }
    return imports;
}

std::uint32_t Elf32Image::read_word_at_vaddr(std::uint32_t address) const {
    for (const auto &segment : segments_) {
        if (segment.type != kPtLoad || address < segment.vaddr) continue;
        const std::uint64_t relative = static_cast<std::uint64_t>(address - segment.vaddr);
        if (relative + 4u <= segment.file_size) {
            const std::size_t off = static_cast<std::size_t>(segment.offset + relative);
            return load_u32(checked_span(off, 4u).data());
        }
    }
    throw Error("No file-backed instruction at " + hex32(address));
}

} // namespace psprecomp
