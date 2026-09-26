#pragma once

#include <cstdint>
#include <vector>

namespace psprecomp {
class Runtime;
}

namespace p3p3ds {
class KernelState;
}

namespace p3p3ds::hle {

struct GeListInfo {
    std::uint32_t list_address{0};
    std::uint32_t stall_address{0};
    std::int32_t callback_id{0};
    std::uint32_t opt_param{0};
};

class GeManager {
public:
    static constexpr std::uint32_t kEdramBase = 0x04000000u;
    static constexpr std::uint32_t kEdramSize = 0x00200000u; // 2 MiB

    GeManager() = default;

    std::uint32_t edram_base() const noexcept { return kEdramBase; }
    std::uint32_t edram_size() const noexcept { return kEdramSize; }

    std::uint32_t enqueue_list(std::uint32_t list_addr, std::uint32_t stall_addr, int cb_id, std::uint32_t opt) {
        last_list_ = {list_addr, stall_addr, cb_id, opt};
        ++enqueue_count_;
        return 1u; // list id
    }

    [[nodiscard]] const GeListInfo &last_list() const noexcept { return last_list_; }
    [[nodiscard]] std::uint64_t enqueue_count() const noexcept { return enqueue_count_; }

private:
    GeListInfo last_list_;
    std::uint64_t enqueue_count_{0};
};

void register_ge_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
