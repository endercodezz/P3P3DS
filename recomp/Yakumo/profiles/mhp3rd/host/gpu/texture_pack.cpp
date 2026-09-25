#include "texture_pack.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <system_error>

#define XXH_INLINE_ALL
#include "xxhash/xxhash.h"

#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace mhp3rd::gpu {
namespace {

// Seeds the format uses: one for texture data, one for the palette.
constexpr std::uint32_t kDataSeed = 0xBACD7814u;
constexpr std::uint32_t kClutSeed = 0xC0108888u;
// Decoded images waiting for the renderer, at most.
constexpr std::size_t kMaxDecodedBytes = 256u * 1024u * 1024u;
// The largest image side the loader accepts. Every Vulkan device samples 4096;
// packs for this game stay well below it.
constexpr int kMaxImageSide = 8192;

} // namespace

bool texture_pack_trace() {
    static const bool trace = [] {
        const char *value = std::getenv("MHP3RD_TRACE_TEXTURE_PACK");
        return value != nullptr && *value != '\0' && std::strcmp(value, "0") != 0;
    }();
    return trace;
}

namespace {

std::string trim(std::string_view text) {
    std::size_t begin = 0u;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1u])) != 0) --end;
    return std::string(text.substr(begin, end - begin));
}

std::string lower(std::string text) {
    for (char &c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

bool parse_bool(const std::string &value) {
    const std::string v = lower(value);
    return v == "true" || v == "1" || v == "yes" || v == "on";
}

bool parse_uint(const std::string &text, std::uint32_t &value, int base = 10) {
    if (text.empty()) return false;
    char *end = nullptr;
    const unsigned long parsed = std::strtoul(text.c_str(), &end, base);
    if (end == text.c_str() || *end != '\0') return false;
    value = static_cast<std::uint32_t>(parsed);
    return true;
}

std::vector<std::string> split(const std::string &text, char separator) {
    std::vector<std::string> parts;
    std::string part;
    std::istringstream stream(text);
    while (std::getline(stream, part, separator)) parts.push_back(trim(part));
    return parts;
}

int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// A key as the ini writes it: up to 16 hex digits of cache key, up to 8 of
// data hash, then an optional _<mip level>. Shorter keys leave the rest zero.
bool parse_key(const std::string &text, TexturePackKey &key, int &level) {
    key = {};
    level = 0;
    std::size_t at = 0u;
    for (; at < text.size() && at < 16u && hex_digit(text[at]) >= 0; ++at)
        key.cache_key = key.cache_key << 4u | static_cast<std::uint64_t>(hex_digit(text[at]));
    if (at == 0u) return false;
    const std::size_t hash_start = at;
    for (; at < text.size() && at - hash_start < 8u && hex_digit(text[at]) >= 0; ++at)
        key.data_hash = key.data_hash << 4u | static_cast<std::uint32_t>(hex_digit(text[at]));
    if (at < text.size() && text[at] == '_') level = std::atoi(text.c_str() + at + 1u);
    return true;
}

// Bits per texel as the format hashes them, DXT included.
std::uint32_t hash_bits_per_texel(TextureFormat format) {
    switch (format) {
    case TextureFormat::Rgba5650:
    case TextureFormat::Rgba5551:
    case TextureFormat::Rgba4444:
    case TextureFormat::Clut16: return 16u;
    case TextureFormat::Rgba8888:
    case TextureFormat::Clut32: return 32u;
    case TextureFormat::Clut4:
    case TextureFormat::Dxt1: return 4u;
    case TextureFormat::Clut8:
    case TextureFormat::Dxt3:
    case TextureFormat::Dxt5: return 8u;
    default: return 0u;
    }
}

bool is_paletted(TextureFormat format) {
    return format == TextureFormat::Clut4 || format == TextureFormat::Clut8 || format == TextureFormat::Clut16 ||
           format == TextureFormat::Clut32;
}

std::uint32_t data_hash(const std::uint8_t *data, std::size_t size, TexturePackHash hash) {
    // xxh64 keys keep the low 32 bits of the 64-bit hash.
    return hash == TexturePackHash::Xxh32 ? XXH32(data, size, kDataSeed)
                                          : static_cast<std::uint32_t>(XXH64(data, size, kDataSeed));
}

bool is_hex_name(const std::string &stem) {
    const bool levelled = stem.size() >= 26u && stem.size() <= 27u && stem[24] == '_';
    if (stem.size() != 24u && !levelled) return false;
    for (std::size_t i = 0; i < 24u; ++i)
        if (hex_digit(stem[i]) < 0) return false;
    return true;
}

// Reads a whole file; empty on failure.
std::vector<std::uint8_t> read_file(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

} // namespace

std::string format_texture_pack_key(const TexturePackKey &key) {
    char text[32];
    std::snprintf(text, sizeof(text), "%016llx%08x", static_cast<unsigned long long>(key.cache_key),
                  static_cast<unsigned>(key.data_hash));
    return text;
}

bool compute_texture_pack_key(const GuestMemory &memory, const TextureState &texture, std::uint16_t max_seen_v,
                              const TexturePackOptions &options, TexturePackKey &key, std::uint32_t &covered_width,
                              std::uint32_t &covered_height) {
    const std::uint32_t bits = hash_bits_per_texel(texture.format);
    if (bits == 0u || texture.width == 0u || texture.height == 0u) return false;

    // The address the GE sees: 28 bits, 16-byte aligned.
    const std::uint32_t address = texture.address & 0x0FFFFFF0u;
    // The buffer width register keeps only whole 16-byte units; zero means
    // one unit.
    const std::uint32_t unit_texels = 128u / bits;
    std::uint32_t buffer_width = texture.buffer_width & 0x7FFu & ~(unit_texels - 1u);
    if (buffer_width == 0u) buffer_width = unit_texels;

    std::uint32_t width = texture.width;
    std::uint32_t height = texture.height;
    const std::uint64_t range_key = static_cast<std::uint64_t>(address) << 32u |
                                    static_cast<std::uint64_t>(width) << 16u | height;
    if (const auto range = options.hash_ranges.find(range_key); range != options.hash_ranges.end()) {
        // The pack replaces only this part of the texture.
        width = range->second.first;
        height = range->second.second;
        covered_width = width;
        covered_height = height;
    } else {
        covered_width = width;
        covered_height = height;
        // 512-tall textures drawn in through mode are hashed only down to the
        // lowest row a draw has used; their image still covers all of it.
        if (height == 512u && max_seen_v != 0u && max_seen_v < 512u) height = max_seen_v;
    }

    float reduce = 1.0f;
    if (options.reduce_hash) {
        const auto found = options.reduce_hash_ranges.find(width << 16u | height);
        reduce = found != options.reduce_hash_ranges.end() ? found->second : options.reduce_hash_default;
    }
    reduce = std::clamp(reduce, 0.0f, 1.0f);

    std::uint32_t hash = 0u;
    if (buffer_width <= width) {
        // Contiguous: the rows follow one another with no gap.
        const std::uint32_t texels = buffer_width * height + (width - buffer_width);
        std::uint32_t size = static_cast<std::uint32_t>(static_cast<float>(bits * texels / 8u) * reduce);
        if (texture.format == TextureFormat::Dxt1 && texture.width == 128u) {
            const std::uint32_t skip = texture.height == 64u    ? options.skip_last_dxt1_blocks_128x64
                                       : texture.height == 128u ? options.skip_last_dxt1_blocks_128x128
                                                                : 0u;
            size -= std::min(size, skip * 8u);
        }
        const std::uint8_t *data = memory.raw_pointer(address, size);
        if (data == nullptr) return false;
        hash = data_hash(data, size, options.hash);
    } else {
        // Rows narrower than the buffer: each row is hashed on its own and the
        // row hashes are folded together.
        const std::uint32_t row_bytes = static_cast<std::uint32_t>(static_cast<float>(bits * width / 8u) * reduce);
        const std::uint32_t stride = bits * buffer_width / 8u;
        const std::uint32_t span = height > 0u ? (height - 1u) * stride + row_bytes : 0u;
        const std::uint8_t *data = memory.raw_pointer(address, span);
        if (data == nullptr) return false;
        for (std::uint32_t row = 0; row < height; ++row)
            hash = (hash * 11u) ^ data_hash(data + static_cast<std::size_t>(row) * stride, row_bytes, options.hash);
    }

    // Dimension word: log2 height in bits 8-11, log2 width in bits 0-3.
    const std::uint32_t dimension = static_cast<std::uint32_t>(std::countr_zero(texture.height)) << 8u |
                                    static_cast<std::uint32_t>(std::countr_zero(texture.width));
    std::uint64_t cache_key = options.ignore_address ? 0u : static_cast<std::uint64_t>(address & 0x3FFFFFFFu) << 32u;
    cache_key |= dimension;
    if (is_paletted(texture.format)) {
        // The palette as the last CLUT load read it, plus the entries that the
        // index offset reaches beyond it, up to the most any load has read.
        const std::uint32_t entry_bytes = texture.clut_format == 3u ? 4u : 2u;
        const std::uint32_t base_bytes = ((texture.clut_format_word >> 16u) & 0x1Fu) * 16u * entry_bytes;
        const std::uint32_t clut_bytes = std::min(texture.clut_load_bytes + base_bytes, texture.clut_max_bytes);
        const std::uint8_t *clut = clut_bytes != 0u ? memory.raw_pointer(texture.clut_address & 0x0FFFFFF0u, clut_bytes)
                                                    : nullptr;
        std::uint32_t clut_hash = XXH32(clut, clut != nullptr ? clut_bytes : 0u, kClutSeed);
        // The CLUT format command word itself, command byte and all, is mixed in.
        clut_hash ^= texture.clut_format_word;
        cache_key ^= clut_hash;
    }
    key.cache_key = cache_key;
    key.data_hash = hash;
    return true;
}

std::unique_ptr<TexturePack> TexturePack::open(const std::filesystem::path &directory, const std::string &game_id,
                                               std::string &error) {
    std::unique_ptr<TexturePack> pack = parse(directory, game_id, error);
    if (!pack) return nullptr;
    const unsigned threads = std::clamp(std::thread::hardware_concurrency() / 4u, 1u, 2u);
    for (unsigned i = 0; i < threads; ++i) pack->loaders_.emplace_back([p = pack.get()] { p->loader_main(); });
    return pack;
}

bool TexturePack::inspect(const std::filesystem::path &directory, const std::string &game_id, TexturePackInfo &info,
                          std::string &error) {
    info = {};
    const std::unique_ptr<TexturePack> pack = parse(directory, game_id, error);
    if (!pack) return false;
    info.options = pack->options_;
    info.keys = pack->entries_.size();
    info.games = pack->games_;
    std::vector<std::string> files;
    files.reserve(pack->entries_.size());
    for (const auto &[key, name] : pack->entries_)
        if (!name.empty()) files.push_back(name);
    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());
    info.files = std::move(files);
    return true;
}

std::vector<std::string> texture_pack_games(const std::filesystem::path &ini) {
    std::vector<std::string> games;
    std::ifstream file(ini);
    std::string line;
    bool in_games = false;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[') {
            in_games = lower(line) == "[games]";
            continue;
        }
        const std::size_t equals = line.find('=');
        if (in_games && equals != std::string::npos) games.push_back(trim(line.substr(0, equals)));
    }
    return games;
}

std::unique_ptr<TexturePack> TexturePack::parse(const std::filesystem::path &directory, const std::string &game_id,
                                                std::string &error) {
    std::unique_ptr<TexturePack> pack(new TexturePack());
    pack->directory_ = directory;
    std::error_code ec;
    if (!std::filesystem::is_directory(directory, ec)) {
        error = "no texture pack folder at " + directory.string();
        return nullptr;
    }
    if (std::filesystem::exists(directory / "textures.zip", ec) && !std::filesystem::exists(directory / "textures.ini", ec)) {
        error = "zipped packs (textures.zip) are not supported; unpack it into " + directory.string();
        return nullptr;
    }
    const std::filesystem::path ini = directory / "textures.ini";
    if (std::filesystem::exists(ini, ec)) {
        if (!pack->load_ini(ini, false, error)) return nullptr;
        // [games] can name another ini with settings for one game.
        std::ifstream file(ini);
        std::string line;
        bool in_games = false;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            if (line.front() == '[') {
                in_games = lower(line) == "[games]";
                continue;
            }
            if (!in_games) continue;
            const std::size_t equals = line.find('=');
            if (equals == std::string::npos) continue;
            pack->games_.push_back(trim(line.substr(0, equals)));
            if (pack->games_.back() != game_id) continue;
            const std::string name = trim(line.substr(equals + 1u));
            if (name.empty() || name == "true" || name == "textures.ini") continue;
            if (name.find("..") != std::string::npos) {
                error = "textures.ini: [games] names a file outside the pack: " + name;
                return nullptr;
            }
            if (!pack->load_ini(directory / name, true, error)) return nullptr;
        }
    }
    pack->scan_hash_named_files();
    if (pack->entries_.empty()) {
        error = "no textures.ini and no hash-named images in " + directory.string();
        return nullptr;
    }
    return pack;
}

TexturePack::~TexturePack() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    wake_.notify_all();
    room_.notify_all();
    for (std::thread &loader : loaders_) loader.join();
}

bool TexturePack::load_ini(const std::filesystem::path &path, bool is_override, std::string &error) {
    std::ifstream file(path);
    if (!file) {
        error = "cannot read " + path.string();
        return false;
    }
    std::string section;
    std::string line;
    bool hash_given = false;
    std::size_t line_number = 0u;
    while (std::getline(file, line)) {
        ++line_number;
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[') {
            const std::size_t close = line.find(']');
            section = lower(line.substr(1u, close == std::string::npos ? std::string::npos : close - 1u));
            continue;
        }
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) continue;
        const std::string name = trim(line.substr(0, equals));
        const std::string value = trim(line.substr(equals + 1u));

        if (section == "options") {
            const std::string option = lower(name);
            if (option == "hash") {
                const std::string kind = lower(value);
                if (kind == "xxh64") options_.hash = TexturePackHash::Xxh64;
                else if (kind == "xxh32") options_.hash = TexturePackHash::Xxh32;
                else if (!value.empty() || !is_override) {
                    // "quick" was the format's first hash; packs made with it
                    // are rare, and it is not implemented here.
                    error = "textures.ini: hash = " + value + " is not supported (xxh64 and xxh32 are)";
                    return false;
                }
                hash_given = hash_given || !value.empty();
            } else if (option == "ignoreaddress") {
                options_.ignore_address = parse_bool(value);
            } else if (option == "reducehash") {
                options_.reduce_hash = parse_bool(value);
            } else if (option == "ignoremipmap") {
                options_.ignore_mipmap = parse_bool(value);
            } else if (option == "skiplastdxt1blocks128x64") {
                parse_uint(value, options_.skip_last_dxt1_blocks_128x64);
            } else if (option == "skiplastdxt1blocks128x128") {
                parse_uint(value, options_.skip_last_dxt1_blocks_128x128);
            }
            // version and video need nothing here.
        } else if (section == "hashes") {
            TexturePackKey key;
            int level = 0;
            if (!parse_key(name, key, level)) {
                std::cerr << "[texpack] " << path.filename().string() << ":" << line_number
                          << ": not a texture key: " << name << "\n";
                continue;
            }
            // Mip levels come from the base image; a pack's own are not used.
            if (level != 0) continue;
            std::string file_name = value;
            std::replace(file_name.begin(), file_name.end(), '\\', '/');
            if (file_name.find("..") != std::string::npos) {
                std::cerr << "[texpack] ignoring a path outside the pack: " << value << "\n";
                continue;
            }
            entries_[key] = file_name;
        } else if (section == "hashranges") {
            const std::vector<std::string> from = split(name, ',');
            const std::vector<std::string> to = split(value, ',');
            std::uint32_t address = 0u, width = 0u, height = 0u, new_width = 0u, new_height = 0u;
            std::string address_text = from.empty() ? std::string{} : from[0];
            if (address_text.rfind("0x", 0) == 0 || address_text.rfind("0X", 0) == 0) address_text.erase(0, 2);
            if (from.size() != 3u || to.size() != 2u || !parse_uint(address_text, address, 16) ||
                !parse_uint(from[1], width) || !parse_uint(from[2], height) || !parse_uint(to[0], new_width) ||
                !parse_uint(to[1], new_height) || new_width == 0u || new_height == 0u || new_width > width ||
                new_height > height) {
                std::cerr << "[texpack] ignoring hash range " << name << " = " << value << "\n";
                continue;
            }
            options_.hash_ranges[static_cast<std::uint64_t>(address) << 32u |
                                 static_cast<std::uint64_t>(width) << 16u | height] = {new_width, new_height};
        } else if (section == "filtering") {
            TexturePackKey key;
            int level = 0;
            if (!parse_key(name, key, level)) continue;
            const std::string mode = lower(value);
            if (mode == "nearest") filters_[key] = ReplacementFilter::Nearest;
            else if (mode == "linear") filters_[key] = ReplacementFilter::Linear;
            else if (mode == "auto") filters_[key] = ReplacementFilter::Auto;
        } else if (section == "reducehashranges") {
            const std::vector<std::string> size = split(name, ',');
            std::uint32_t width = 0u, height = 0u;
            char *end = nullptr;
            const float factor = std::strtof(value.c_str(), &end);
            if (size.size() != 2u || !parse_uint(size[0], width) || !parse_uint(size[1], height) || factor == 0.0f)
                continue;
            options_.reduce_hash_ranges[width << 16u | height] = factor;
        }
    }
    if (!hash_given && !is_override) {
        error = "textures.ini: [options] does not say which hash the pack uses";
        return false;
    }
    return true;
}

void TexturePack::scan_hash_named_files() {
    // Images named by their key may sit in the pack's top folder without an
    // ini line; an ini line for the same key wins.
    std::error_code ec;
    for (const auto &entry : std::filesystem::directory_iterator(directory_, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        const std::filesystem::path &path = entry.path();
        if (lower(path.extension().string()) != ".png") continue;
        const std::string stem = path.stem().string();
        if (!is_hex_name(stem)) continue;
        TexturePackKey key;
        int level = 0;
        if (!parse_key(stem, key, level) || level != 0) continue;
        entries_.try_emplace(key, path.filename().string());
    }
}

template <typename Map>
typename Map::const_iterator TexturePack::lookup(const Map &map, const TexturePackKey &key) const {
    // The exact key, then the wildcard forms, most specific first. A zero
    // half of a pack key matches anything.
    const std::uint64_t low = key.cache_key & 0xFFFFFFFFull;
    const std::uint64_t high = key.cache_key & ~0xFFFFFFFFull;
    const bool with_address = !options_.ignore_address;
    const TexturePackKey candidates[] = {
        key,
        {low, 0u},                  // dimension and palette only
        {key.cache_key, 0u},        // address, dimension and palette
        {low, key.data_hash},       // any address
        {high, key.data_hash},      // address and data, any palette
        {high, 0u},                 // address only
        {0u, key.data_hash},        // data only
    };
    const bool allowed[] = {true, true, with_address, true, with_address, with_address, true};
    for (std::size_t i = 0; i < std::size(candidates); ++i) {
        if (!allowed[i]) continue;
        if (const auto found = map.find(candidates[i]); found != map.end()) return found;
    }
    return map.end();
}

std::shared_ptr<Replacement> TexturePack::find(const GuestMemory &memory, const TextureState &texture,
                                               std::uint16_t max_seen_v) {
    TexturePackKey key;
    std::uint32_t covered_width = 0u, covered_height = 0u;
    if (!compute_texture_pack_key(memory, texture, max_seen_v, options_, key, covered_width, covered_height))
        return nullptr;
    const auto entry = lookup(entries_, key);
    const bool found = entry != entries_.end() && !entry->second.empty();
    if (texture_pack_trace()) {
        std::cout << "[texpack] 0x" << std::hex << texture.address << std::dec << " " << texture.width << "x"
                  << texture.height << " fmt=" << static_cast<int>(texture.format) << " key "
                  << format_texture_pack_key(key)
                  << (found                        ? " -> " + entry->second
                      : entry != entries_.end()    ? " (kept by the pack)"
                                                   : " (not in the pack)")
                  << "\n";
    }
    if (!found) return nullptr;
    auto [file, inserted] = files_.try_emplace(entry->second);
    if (inserted) {
        auto replacement = std::make_shared<Replacement>();
        replacement->name = entry->second;
        replacement->file = directory_ / std::filesystem::path(entry->second);
        replacement->texture_width = texture.width;
        replacement->texture_height = texture.height;
        replacement->covered_width = covered_width;
        replacement->covered_height = covered_height;
        auto filter = lookup(filters_, key);
        if (filter == filters_.end()) filter = filters_.find(TexturePackKey{});
        if (filter != filters_.end()) replacement->filter = filter->second;
        file->second = std::move(replacement);
    }
    return file->second;
}

void TexturePack::request(Replacement &replacement) {
    if (replacement.state.load(std::memory_order_acquire) != Replacement::State::Unloaded) return;
    replacement.state.store(Replacement::State::Queued, std::memory_order_release);
    {
        std::lock_guard lock(mutex_);
        queue_.push_back(&replacement);
    }
    wake_.notify_one();
}

void TexturePack::consumed(Replacement &replacement) {
    std::size_t freed = replacement.pixels.size();
    std::vector<std::uint8_t>().swap(replacement.pixels);
    replacement.state.store(Replacement::State::Resident, std::memory_order_release);
    {
        std::lock_guard lock(mutex_);
        decoded_bytes_ -= std::min(decoded_bytes_, freed);
    }
    room_.notify_all();
}

std::filesystem::path TexturePack::resolve(const std::string &name) {
    // Only the loader threads call this, one at a time under resolve_mutex_.
    if (!folded_scanned_) {
        folded_scanned_ = true;
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(directory_, ec);
             !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file(ec)) continue;
            const std::string relative = std::filesystem::relative(it->path(), directory_, ec).generic_string();
            folded_names_.emplace(lower(relative), it->path());
        }
    }
    const auto found = folded_names_.find(lower(name));
    return found != folded_names_.end() ? found->second : std::filesystem::path{};
}

void TexturePack::loader_main() {
    for (;;) {
        Replacement *replacement = nullptr;
        {
            std::unique_lock lock(mutex_);
            wake_.wait(lock, [&] { return stopping_ || !queue_.empty(); });
            if (stopping_) return;
            room_.wait(lock, [&] { return stopping_ || decoded_bytes_ < kMaxDecodedBytes; });
            if (stopping_) return;
            if (queue_.empty()) continue;
            replacement = queue_.front();
            queue_.pop_front();
        }
        decode(*replacement);
    }
}

void TexturePack::decode(Replacement &replacement) {
    std::vector<std::uint8_t> bytes = read_file(replacement.file);
    if (bytes.empty()) {
        std::filesystem::path folded;
        {
            std::lock_guard lock(resolve_mutex_);
            folded = resolve(replacement.name);
        }
        if (!folded.empty()) bytes = read_file(folded);
    }
    int width = 0, height = 0, channels = 0;
    stbi_uc *image = bytes.empty() ? nullptr
                                   : stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &width,
                                                           &height, &channels, 4);
    if (image == nullptr || width <= 0 || height <= 0 || width > kMaxImageSide || height > kMaxImageSide) {
        std::cerr << "[texpack] cannot use " << replacement.name << ": "
                  << (bytes.empty() ? "missing or unreadable"
                      : image == nullptr ? stbi_failure_reason()
                                         : "too large")
                  << "\n";
        if (image != nullptr) stbi_image_free(image);
        replacement.state.store(Replacement::State::Failed, std::memory_order_release);
        return;
    }
    // A [hashranges] image covers only part of the texture: pad it to the
    // size the whole texture would have at the same scale.
    std::uint32_t full_width = static_cast<std::uint32_t>(width);
    std::uint32_t full_height = static_cast<std::uint32_t>(height);
    if (replacement.covered_width != 0u && replacement.covered_width < replacement.texture_width)
        full_width = full_width * replacement.texture_width / replacement.covered_width;
    if (replacement.covered_height != 0u && replacement.covered_height < replacement.texture_height)
        full_height = full_height * replacement.texture_height / replacement.covered_height;
    full_width = std::min<std::uint32_t>(full_width, kMaxImageSide);
    full_height = std::min<std::uint32_t>(full_height, kMaxImageSide);
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(full_width) * full_height * 4u, 0u);
    for (int row = 0; row < height && static_cast<std::uint32_t>(row) < full_height; ++row)
        std::memcpy(pixels.data() + static_cast<std::size_t>(row) * full_width * 4u,
                    image + static_cast<std::size_t>(row) * static_cast<std::size_t>(width) * 4u,
                    static_cast<std::size_t>(std::min<std::uint32_t>(static_cast<std::uint32_t>(width), full_width)) * 4u);
    stbi_image_free(image);

    replacement.width = full_width;
    replacement.height = full_height;
    {
        std::lock_guard lock(mutex_);
        decoded_bytes_ += pixels.size();
    }
    replacement.pixels = std::move(pixels);
    if (texture_pack_trace())
        std::cout << "[texpack] decoded " << replacement.name << " " << full_width << "x" << full_height << "\n";
    replacement.state.store(Replacement::State::Decoded, std::memory_order_release);
}

TextureDumper::TextureDumper(std::filesystem::path directory) : directory_(std::move(directory)) {
    std::error_code ec;
    std::filesystem::create_directories(directory_, ec);
    const std::filesystem::path ini = directory_ / "textures.ini";
    if (!std::filesystem::exists(ini, ec)) {
        // Images named by their key need no ini line; this makes the folder a
        // pack as it is.
        std::ofstream out(ini);
        out << "[options]\nversion = 1\nhash = xxh64\nignoreAddress = true\n\n[hashes]\n";
    }
    writer_ = std::thread([this] { writer_main(); });
}

TextureDumper::~TextureDumper() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    wake_.notify_all();
    writer_.join();
}

void TextureDumper::dump(const GuestMemory &memory, const TextureState &texture, std::uint16_t max_seen_v,
                         const TexturePackOptions &options, const std::uint32_t *pixels) {
    TexturePackKey key;
    std::uint32_t covered_width = 0u, covered_height = 0u;
    if (!compute_texture_pack_key(memory, texture, max_seen_v, options, key, covered_width, covered_height)) return;
    const std::string name = format_texture_pack_key(key);
    std::lock_guard lock(mutex_);
    if (!written_.try_emplace(name, true).second) return;
    // Hundreds of textures load in the first seconds; drop rather than queue
    // without bound.
    if (jobs_.size() > 256u) return;
    Job job;
    job.path = directory_ / (name + ".png");
    job.width = covered_width;
    job.height = covered_height;
    job.pixels.resize(static_cast<std::size_t>(covered_width) * covered_height);
    for (std::uint32_t row = 0; row < covered_height; ++row)
        std::memcpy(job.pixels.data() + static_cast<std::size_t>(row) * covered_width,
                    pixels + static_cast<std::size_t>(row) * texture.width, covered_width * 4u);
    jobs_.push_back(std::move(job));
    wake_.notify_one();
}

void TextureDumper::writer_main() {
    for (;;) {
        Job job;
        {
            std::unique_lock lock(mutex_);
            wake_.wait(lock, [&] { return stopping_ || !jobs_.empty(); });
            if (jobs_.empty()) return;
            job = std::move(jobs_.front());
            jobs_.pop_front();
        }
        std::error_code ec;
        if (std::filesystem::exists(job.path, ec)) continue;
        int length = 0;
        unsigned char *png = stbi_write_png_to_mem(reinterpret_cast<const unsigned char *>(job.pixels.data()),
                                                   static_cast<int>(job.width * 4u), static_cast<int>(job.width),
                                                   static_cast<int>(job.height), 4, &length);
        if (png == nullptr) continue;
        std::ofstream out(job.path, std::ios::binary);
        out.write(reinterpret_cast<const char *>(png), length);
        STBIW_FREE(png);
    }
}

} // namespace mhp3rd::gpu
