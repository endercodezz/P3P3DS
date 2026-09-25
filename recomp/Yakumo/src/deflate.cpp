#include "psprecomp/deflate.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace psprecomp {
namespace {

class BitReader {
public:
    BitReader(const GuestMemory &memory, std::uint32_t base) : memory_(memory), base_(base) {}

    [[nodiscard]] bool read(std::uint32_t count, std::uint32_t &value) {
        if (count > 24u || !ensure(count)) return false;
        const std::uint64_t mask = count == 0u ? 0u : ((std::uint64_t{1} << count) - 1u);
        value = static_cast<std::uint32_t>(bits_ & mask);
        bits_ >>= count;
        bit_count_ -= count;
        consumed_bits_ += count;
        return true;
    }

    [[nodiscard]] bool align_byte() {
        const std::uint32_t discard = static_cast<std::uint32_t>((8u - (consumed_bits_ & 7u)) & 7u);
        std::uint32_t ignored{};
        return discard == 0u || read(discard, ignored);
    }

    [[nodiscard]] std::uint32_t consumed_bytes() const noexcept {
        return static_cast<std::uint32_t>((consumed_bits_ + 7u) / 8u);
    }

private:
    [[nodiscard]] bool ensure(std::uint32_t count) {
        while (bit_count_ < count) {
            if (loaded_bytes_ == std::numeric_limits<std::uint32_t>::max() ||
                !memory_.contains(base_ + loaded_bytes_, 1u)) return false;
            bits_ |= static_cast<std::uint64_t>(memory_.aot_load8(base_ + loaded_bytes_)) << bit_count_;
            ++loaded_bytes_;
            bit_count_ += 8u;
        }
        return true;
    }

    const GuestMemory &memory_;
    std::uint32_t base_{};
    std::uint32_t loaded_bytes_{};
    std::uint64_t consumed_bits_{};
    std::uint64_t bits_{};
    std::uint32_t bit_count_{};
};

std::uint32_t reverse_code(std::uint32_t code, std::uint32_t length) noexcept {
    std::uint32_t reversed = 0u;
    for (std::uint32_t bit = 0u; bit < length; ++bit) {
        reversed = (reversed << 1u) | (code & 1u);
        code >>= 1u;
    }
    return reversed;
}

struct HuffmanTree {
    std::array<std::vector<std::int16_t>, 16> symbols_by_length{};
    std::uint32_t max_length{};

    [[nodiscard]] bool build(const std::vector<std::uint8_t> &lengths) {
        std::array<std::uint32_t, 16> counts{};
        for (const std::uint8_t length : lengths) {
            if (length > 15u) return false;
            if (length != 0u) ++counts[length];
        }
        max_length = 15u;
        while (max_length != 0u && counts[max_length] == 0u) --max_length;
        if (max_length == 0u) return false;

        std::int32_t remaining = 1;
        for (std::uint32_t length = 1u; length <= 15u; ++length) {
            remaining = remaining * 2 - static_cast<std::int32_t>(counts[length]);
            if (remaining < 0) return false; // oversubscribed code set
        }

        std::array<std::uint32_t, 16> next_code{};
        std::uint32_t code = 0u;
        for (std::uint32_t length = 1u; length <= 15u; ++length) {
            code = (code + counts[length - 1u]) << 1u;
            next_code[length] = code;
            symbols_by_length[length].assign(std::size_t{1} << length, -1);
        }
        for (std::size_t symbol = 0u; symbol < lengths.size(); ++symbol) {
            const std::uint32_t length = lengths[symbol];
            if (length == 0u) continue;
            const std::uint32_t reversed = reverse_code(next_code[length]++, length);
            symbols_by_length[length][reversed] = static_cast<std::int16_t>(symbol);
        }
        return true;
    }

    [[nodiscard]] bool decode(BitReader &reader, std::uint32_t &symbol) const {
        std::uint32_t code = 0u;
        for (std::uint32_t length = 1u; length <= max_length; ++length) {
            std::uint32_t bit{};
            if (!reader.read(1u, bit)) return false;
            code |= bit << (length - 1u);
            const auto &table = symbols_by_length[length];
            if (code < table.size() && table[code] >= 0) {
                symbol = static_cast<std::uint32_t>(table[code]);
                return true;
            }
        }
        return false;
    }
};

bool build_fixed_trees(HuffmanTree &literal_length, HuffmanTree &distance) {
    std::vector<std::uint8_t> literal_lengths(288u, 0u);
    std::fill(literal_lengths.begin() + 0, literal_lengths.begin() + 144, 8u);
    std::fill(literal_lengths.begin() + 144, literal_lengths.begin() + 256, 9u);
    std::fill(literal_lengths.begin() + 256, literal_lengths.begin() + 280, 7u);
    std::fill(literal_lengths.begin() + 280, literal_lengths.end(), 8u);
    std::vector<std::uint8_t> distance_lengths(32u, 5u);
    return literal_length.build(literal_lengths) && distance.build(distance_lengths);
}

bool build_dynamic_trees(BitReader &reader, HuffmanTree &literal_length, HuffmanTree &distance) {
    std::uint32_t hlit_bits{}, hdist_bits{}, hclen_bits{};
    if (!reader.read(5u, hlit_bits) || !reader.read(5u, hdist_bits) || !reader.read(4u, hclen_bits))
        return false;
    const std::uint32_t literal_count = hlit_bits + 257u;
    const std::uint32_t distance_count = hdist_bits + 1u;
    const std::uint32_t code_length_count = hclen_bits + 4u;
    if (literal_count > 286u || distance_count > 32u) return false;

    constexpr std::array<std::uint8_t, 19> order{
        16u, 17u, 18u, 0u, 8u, 7u, 9u, 6u, 10u, 5u, 11u, 4u, 12u, 3u, 13u, 2u, 14u, 1u, 15u};
    std::vector<std::uint8_t> code_lengths(19u, 0u);
    for (std::uint32_t index = 0u; index < code_length_count; ++index) {
        std::uint32_t length{};
        if (!reader.read(3u, length)) return false;
        code_lengths[order[index]] = static_cast<std::uint8_t>(length);
    }
    HuffmanTree code_length_tree;
    if (!code_length_tree.build(code_lengths)) return false;

    std::vector<std::uint8_t> combined;
    combined.reserve(literal_count + distance_count);
    while (combined.size() < literal_count + distance_count) {
        std::uint32_t symbol{};
        if (!code_length_tree.decode(reader, symbol)) return false;
        if (symbol <= 15u) {
            combined.push_back(static_cast<std::uint8_t>(symbol));
            continue;
        }
        std::uint32_t extra{};
        std::uint32_t repeat{};
        std::uint8_t value{};
        if (symbol == 16u) {
            if (combined.empty() || !reader.read(2u, extra)) return false;
            repeat = extra + 3u;
            value = combined.back();
        } else if (symbol == 17u) {
            if (!reader.read(3u, extra)) return false;
            repeat = extra + 3u;
        } else if (symbol == 18u) {
            if (!reader.read(7u, extra)) return false;
            repeat = extra + 11u;
        } else {
            return false;
        }
        if (combined.size() + repeat > literal_count + distance_count) return false;
        combined.insert(combined.end(), repeat, value);
    }

    std::vector<std::uint8_t> literal_lengths(combined.begin(), combined.begin() + literal_count);
    std::vector<std::uint8_t> distance_lengths(combined.begin() + literal_count, combined.end());
    if (literal_lengths.size() <= 256u || literal_lengths[256u] == 0u) return false;
    return literal_length.build(literal_lengths) && distance.build(distance_lengths);
}

constexpr std::array<std::uint16_t, 29> length_base{
    3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 13u, 15u, 17u, 19u, 23u, 27u,
    31u, 35u, 43u, 51u, 59u, 67u, 83u, 99u, 115u, 131u, 163u, 195u, 227u, 258u};
constexpr std::array<std::uint8_t, 29> length_extra{
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u, 1u, 1u, 1u, 2u, 2u, 2u,
    2u, 3u, 3u, 3u, 3u, 4u, 4u, 4u, 4u, 5u, 5u, 5u, 5u, 0u};
constexpr std::array<std::uint16_t, 30> distance_base{
    1u, 2u, 3u, 4u, 5u, 7u, 9u, 13u, 17u, 25u, 33u, 49u, 65u, 97u, 129u,
    193u, 257u, 385u, 513u, 769u, 1025u, 1537u, 2049u, 3073u, 4097u, 6145u,
    8193u, 12289u, 16385u, 24577u};
constexpr std::array<std::uint8_t, 30> distance_extra{
    0u, 0u, 0u, 0u, 1u, 1u, 2u, 2u, 3u, 3u, 4u, 4u, 5u, 5u, 6u,
    6u, 7u, 7u, 8u, 8u, 9u, 9u, 10u, 10u, 11u, 11u, 12u, 12u, 13u, 13u};

} // namespace

RawDeflateResult inflate_raw_deflate(GuestMemory &memory,
                                     std::uint32_t output_address,
                                     std::uint32_t output_capacity,
                                     std::uint32_t input_address) {
    BitReader reader(memory, input_address);
    std::uint32_t produced = 0u;
    bool final_block = false;

    while (!final_block) {
        std::uint32_t final_bit{}, block_type{};
        if (!reader.read(1u, final_bit) || !reader.read(2u, block_type))
            return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};
        final_block = final_bit != 0u;

        if (block_type == 0u) {
            if (!reader.align_byte())
                return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};
            std::uint32_t length{}, complement{};
            if (!reader.read(16u, length) || !reader.read(16u, complement) ||
                (length ^ 0xFFFFu) != complement)
                return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};
            if (length > output_capacity - produced ||
                !memory.contains(output_address + produced, length))
                return {RawDeflateStatus::OutputOverflow, produced, reader.consumed_bytes()};
            for (std::uint32_t index = 0u; index < length; ++index) {
                std::uint32_t byte{};
                if (!reader.read(8u, byte))
                    return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};
                memory.aot_store8(output_address + produced++, static_cast<std::uint8_t>(byte));
            }
            continue;
        }
        if (block_type == 3u)
            return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};

        HuffmanTree literal_length;
        HuffmanTree distance;
        const bool trees_ok = block_type == 1u
            ? build_fixed_trees(literal_length, distance)
            : build_dynamic_trees(reader, literal_length, distance);
        if (!trees_ok)
            return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};

        for (;;) {
            std::uint32_t symbol{};
            if (!literal_length.decode(reader, symbol))
                return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};
            if (symbol < 256u) {
                if (produced >= output_capacity || !memory.contains(output_address + produced, 1u))
                    return {RawDeflateStatus::OutputOverflow, produced, reader.consumed_bytes()};
                memory.aot_store8(output_address + produced++, static_cast<std::uint8_t>(symbol));
                continue;
            }
            if (symbol == 256u) break;
            if (symbol < 257u || symbol > 285u)
                return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};

            const std::uint32_t length_index = symbol - 257u;
            std::uint32_t length = length_base[length_index];
            std::uint32_t extra{};
            if (length_extra[length_index] != 0u) {
                if (!reader.read(length_extra[length_index], extra))
                    return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};
                length += extra;
            }

            std::uint32_t distance_symbol{};
            if (!distance.decode(reader, distance_symbol) || distance_symbol >= distance_base.size())
                return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};
            std::uint32_t match_distance = distance_base[distance_symbol];
            if (distance_extra[distance_symbol] != 0u) {
                if (!reader.read(distance_extra[distance_symbol], extra))
                    return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};
                match_distance += extra;
            }
            if (match_distance == 0u || match_distance > produced)
                return {RawDeflateStatus::InvalidData, produced, reader.consumed_bytes()};
            if (length > output_capacity - produced ||
                !memory.contains(output_address + produced, length))
                return {RawDeflateStatus::OutputOverflow, produced, reader.consumed_bytes()};
            memory.aot_copy_lz_match(output_address + produced,
                                     output_address + produced - match_distance,
                                     length);
            produced += length;
        }
    }

    return {RawDeflateStatus::Ok, produced, reader.consumed_bytes()};
}

} // namespace psprecomp
