#pragma once
// Cache of model vertices already converted for a renderer backend (the New
// 3DS shader layout), so a model drawn every frame from unchanged guest
// memory is not unpacked again. An entry keeps a copy of the exact guest
// bytes it was made from and is used only when they are still identical
// (memcmp, no hash), so CPU writes to vertex buffers are always seen.
// Sources that change on most uses (vertices animated by the game's CPU code)
// are marked volatile and left uncached for a while: copying them into the
// cache would only add work.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace p3p3ds::ge {

// What the converted vertices depend on besides the guest bytes: where they
// are, the vertex type (GE register 0x12), the referenced index range and,
// for vertices without a colour, the material colour that replaces it.
struct VertexCacheKey {
    std::uint32_t address{}, vertex_type{}, first{}, count{}, material{};
    bool operator==(const VertexCacheKey &) const = default;
};

struct VertexCacheKeyHash {
    std::size_t operator()(const VertexCacheKey &k) const noexcept {
        std::uint64_t h = 0x9E3779B97F4A7C15ull;
        for (const std::uint32_t v : {k.address, k.vertex_type, k.first, k.count, k.material})
            h = (h ^ v) * 0x100000001B3ull;
        return static_cast<std::size_t>(h ^ (h >> 29));
    }
};

struct VertexCacheStats {
    std::uint64_t hits{};           // converted vertices reused
    std::uint64_t misses{};         // converted now (new source or changed bytes)
    std::uint64_t volatile_skips{}; // not cached: the source changes too often
    std::uint64_t full{};           // not cached: budget used by recent sources
    std::size_t bytes{}, entries{};
};

class VertexCache {
public:
    static constexpr std::uint32_t kVolatileAfter = 3;    // content changes in a row
    static constexpr std::uint64_t kVolatileFrames = 120; // frames a volatile source stays uncached
    static constexpr std::size_t kEntryOverhead = 64;     // map node and bookkeeping, counted in the budget

    explicit VertexCache(std::size_t budget_bytes) : budget_(budget_bytes) {}

    // Converted vertices for `raw` (raw_size guest bytes): the cached copy when
    // the bytes are unchanged, else convert(out) fills out_size bytes, which
    // are kept. nullptr when the source is not cached (volatile or no room):
    // the caller converts directly. The result is valid until the next call.
    template <class Convert>
    const std::uint8_t *get(const VertexCacheKey &key, const std::uint8_t *raw, std::size_t raw_size, std::size_t out_size,
                            std::uint64_t frame, Convert &&convert) {
        if (const auto it = entries_.find(key); it != entries_.end()) {
            Entry &e = it->second;
            e.last_frame = frame;
            if (e.volatile_until > frame) { ++stats_.volatile_skips; return nullptr; }
            if (e.raw_size == raw_size && e.data.size() == raw_size + out_size &&
                std::memcmp(e.data.data(), raw, raw_size) == 0) {
                e.changes = 0;
                ++stats_.hits;
                return e.data.data() + raw_size;
            }
            if (!e.data.empty() && ++e.changes >= kVolatileAfter) {
                e.changes = 0;
                e.volatile_until = frame + kVolatileFrames;
                resize(e, 0, 0);
                ++stats_.volatile_skips;
                return nullptr;
            }
            const std::size_t total = raw_size + out_size;
            if (!fits(total > e.data.size() ? total - e.data.size() : 0u, frame)) { ++stats_.full; return nullptr; }
            ++stats_.misses;
            return fill(e, raw, raw_size, out_size, convert);
        }
        if (!fits(raw_size + out_size + kEntryOverhead, frame)) { ++stats_.full; return nullptr; }
        Entry &e = entries_[key];
        stats_.bytes += kEntryOverhead;
        stats_.entries = entries_.size();
        e.last_frame = frame;
        ++stats_.misses;
        return fill(e, raw, raw_size, out_size, convert);
    }

    void clear() {
        entries_.clear();
        stats_.bytes = 0;
        stats_.entries = 0;
    }
    [[nodiscard]] const VertexCacheStats &stats() const noexcept { return stats_; }

private:
    struct Entry {
        std::vector<std::uint8_t> data; // guest bytes, then the converted vertices
        std::size_t raw_size{};
        std::uint64_t last_frame{}, volatile_until{};
        std::uint32_t changes{};
    };

    void resize(Entry &e, std::size_t raw_size, std::size_t total) {
        stats_.bytes -= e.data.size();
        e.data.resize(total);
        if (total == 0u) e.data.shrink_to_fit();
        e.raw_size = raw_size;
        stats_.bytes += e.data.size();
    }

    template <class Convert>
    const std::uint8_t *fill(Entry &e, const std::uint8_t *raw, std::size_t raw_size, std::size_t out_size, Convert &convert) {
        resize(e, raw_size, raw_size + out_size);
        std::memcpy(e.data.data(), raw, raw_size);
        convert(e.data.data() + raw_size);
        return e.data.data() + raw_size;
    }

    // Room for `extra` bytes; over budget, sources not used in this or the
    // previous frame are dropped (at most once per frame).
    bool fits(std::size_t extra, std::uint64_t frame) {
        if (stats_.bytes + extra <= budget_) return true;
        if (evicted_frame_ == frame) return false;
        evicted_frame_ = frame;
        for (auto it = entries_.begin(); it != entries_.end();) {
            if (it->second.last_frame + 1u < frame) {
                stats_.bytes -= it->second.data.size() + kEntryOverhead;
                it = entries_.erase(it);
            } else {
                ++it;
            }
        }
        stats_.entries = entries_.size();
        return stats_.bytes + extra <= budget_;
    }

    std::unordered_map<VertexCacheKey, Entry, VertexCacheKeyHash> entries_;
    std::size_t budget_;
    std::uint64_t evicted_frame_{~0ull};
    VertexCacheStats stats_;
};

} // namespace p3p3ds::ge
