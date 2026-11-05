#pragma once
// --texture-keys <file>: computes the PPSSPP-compatible key of every textured
// draw (core/include/p3p3ds/ge/texture_replacement.hpp) and checks it against
// a pack's keys (`p3p-texpack keys <pack dir>`), forwarding every call to the
// real renderer. Measures how much of a real pack the runtime would find.
#include "p3p3ds/ge/geometry.hpp"
#include "p3p3ds/ge/renderer.hpp"
#include "p3p3ds/ge/texture_replacement.hpp"
#include "psprecomp/guest_memory.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace p3p3ds::pc {

class TextureKeyProbe final : public ge::GeRenderer {
public:
    TextureKeyProbe(std::unique_ptr<ge::GeRenderer> inner, const std::string &keys_file) : inner_(std::move(inner)) {
        std::ifstream in(keys_file);
        if (!in) throw std::runtime_error("cannot read --texture-keys file " + keys_file);
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line == "# ignoreAddress") ignore_address_ = true;
            if (const auto key = ge::parse_replacement_key(line)) pack_.insert(*key);
        }
    }

    void draw(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, ge::Prim prim, std::uint32_t count, std::uint32_t vertex_address,
              std::uint32_t index_address) override {
        const auto layout = ge::vertex_layout(regs.reg[0x12]);
        if (ge::draw_is_textured(regs, layout)) probe(memory, regs, layout, count, vertex_address, index_address);
        inner_->draw(memory, regs, prim, count, vertex_address, index_address);
    }
    void transfer(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs) override { inner_->transfer(memory, regs); }
    void sync() override { inner_->sync(); }
    void drain() override { inner_->drain(); }
    [[nodiscard]] const ge::DrawStats &stats() const override { return inner_->stats(); }

    // Summary line plus, with `report_file`, one CSV row per distinct texture key.
    void report(const std::string &report_file) const {
        std::size_t matched_textures = 0;
        std::map<std::string, std::size_t> hits_by_kind;
        for (const auto &[k, t] : seen_) {
            if (!t.match_kind.empty()) ++matched_textures;
            ++hits_by_kind[t.match_kind.empty() ? "none" : t.match_kind];
        }
        std::printf("[TEXKEYS] pack_keys=%zu ignore_address=%d textured_draws=%llu matched_draws=%llu textures=%zu matched_textures=%zu "
                    "pack_keys_hit=%zu\n",
                    pack_.size(), ignore_address_ ? 1 : 0, static_cast<unsigned long long>(textured_draws_),
                    static_cast<unsigned long long>(matched_draws_), seen_.size(), matched_textures, pack_hit_.size());
        for (const auto &[kind, n] : hits_by_kind) std::printf("[TEXKEYS] match %s: %zu textures\n", kind.c_str(), n);
        if (report_file.empty()) return;
        std::ofstream out(report_file);
        out << "key,match,draws,address,format,width,height,bufw,clut_format,max_seen_v,through\n";
        for (const auto &[k, t] : seen_) {
            char key[32];
            std::snprintf(key, sizeof key, "%016llx%08x", static_cast<unsigned long long>(k.first), k.second);
            char row[200];
            std::snprintf(row, sizeof row, "%s,%s,%llu,%08x,%u,%u,%u,%u,%u,%u,%d\n", key, t.match_kind.empty() ? "none" : t.match_kind.c_str(),
                          static_cast<unsigned long long>(t.draws), t.address, t.format, t.width, t.height, t.bufw, t.clut_format,
                          t.max_seen_v, t.through ? 1 : 0);
            out << row;
        }
    }

private:
    struct Texture {
        std::uint64_t draws{};
        std::string match_kind;
        std::uint32_t address{}, format{}, width{}, height{}, bufw{}, clut_format{};
        std::uint16_t max_seen_v{};
        bool through{};
    };

    void probe(psprecomp::GuestMemory &memory, const ge::GeRegisters &regs, const ge::VertexLayout &layout, std::uint32_t count,
               std::uint32_t vertex_address, std::uint32_t index_address) {
        // The CLUT cache changes only on LOADCLUT, which updates clut_hash.
        if (regs.clut_hash != clut_hash_seen_) {
            clut_hash_seen_ = regs.clut_hash;
            clut_.load(reinterpret_cast<const std::uint8_t *>(regs.clut.data()), regs.clut_words * 4u);
        }
        const bool through = layout.through;
        std::uint16_t max_v = 0;
        if (through) {
            ge::ModelVertex v;
            for (std::uint32_t i = 0; i < count; ++i)
                if (ge::decode_model_vertex(memory, regs, layout, vertex_address, ge::vertex_index(memory, layout, index_address, i), v))
                    max_v = std::max<std::uint16_t>(max_v, static_cast<std::uint16_t>(std::clamp(v.uv[1], 0.0f, 65535.0f)));
        }
        const ge::ReplacementKey key = keys_.key(memory, regs, clut_, through, max_v);
        ++textured_draws_;
        auto &t = seen_[{ignore_address_ ? key.cachekey & 0xFFFFFFFFull : key.cachekey, key.hash}];
        if (t.draws++ == 0) {
            const auto info = ge::texture_info(regs);
            t.address = info.address;
            t.format = info.format;
            t.width = info.width;
            t.height = info.height;
            t.bufw = info.stride;
            t.clut_format = regs.reg[0xC5] & 0xFFFFFFu;
            t.through = through;
            if (const auto found = ge::find_replacement(pack_, key, ignore_address_)) {
                const std::uint64_t ck = ignore_address_ ? key.cachekey & 0xFFFFFFFFull : key.cachekey;
                t.match_kind = found->cachekey == ck && found->hash == key.hash ? "exact"
                               : found->hash == 0                               ? "clut_only"
                               : found->cachekey == 0                           ? "data_only"
                                                                                : "partial";
                pack_hit_.insert(*found);
            }
        }
        t.max_seen_v = std::max(t.max_seen_v, max_v);
        if (!t.match_kind.empty()) ++matched_draws_;
    }

    std::unique_ptr<ge::GeRenderer> inner_;
    ge::ReplacementKeySet pack_, pack_hit_;
    bool ignore_address_{};
    ge::TextureKeys keys_;
    ge::PpssppClut clut_;
    std::uint64_t clut_hash_seen_{};
    std::uint64_t textured_draws_{}, matched_draws_{};
    std::map<std::pair<std::uint64_t, std::uint32_t>, Texture> seen_;
};

} // namespace p3p3ds::pc
