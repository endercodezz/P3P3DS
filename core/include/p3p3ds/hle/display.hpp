#pragma once

#include <cstdint>
#include <iostream>

namespace psprecomp {
class Runtime;
}

namespace p3p3ds {
class KernelState;
}

namespace p3p3ds::hle {

struct DisplayFramebufInfo {
    std::uint32_t topaddr{0};
    std::int32_t bufferwidth{512};
    std::int32_t pixelformat{3}; // 3 = PSP_DISPLAY_PIXEL_FORMAT_8888
    std::int32_t sync{1};
    std::int32_t mode{0};
    std::int32_t width{480};
    std::int32_t height{272};
    std::uint64_t vcount{0};
    bool active{false};
};

class DisplayManager {
public:
    DisplayManager() = default;

    int set_mode(int mode, int width, int height) noexcept {
        info_.mode = mode;
        info_.width = width;
        info_.height = height;
        return 0;
    }

    int set_framebuf(std::uint32_t topaddr, int bufferwidth, int pixelformat, int sync) noexcept {
        DisplayFramebufInfo requested = info_;
        requested.topaddr = topaddr;
        requested.bufferwidth = bufferwidth;
        requested.pixelformat = pixelformat;
        requested.sync = sync;
        requested.active = (topaddr != 0);
        if (sync == 0) {
            info_ = requested;
            pending_valid_ = false;
        } else {
            pending_ = requested;
            pending_valid_ = true;
        }
        return 0;
    }

    [[nodiscard]] const DisplayFramebufInfo &info() const noexcept { return pending_valid_ ? pending_ : info_; }
    [[nodiscard]] bool has_framebuf() const noexcept { return info_.active; }
    [[nodiscard]] std::uint64_t vcount() const noexcept { return info_.vcount; }
    [[nodiscard]] const DisplayFramebufInfo &framebuf(unsigned sync) const noexcept {
        return sync && pending_valid_ ? pending_ : info_;
    }
    void advance_vblank() noexcept {
        const auto next_vcount = info_.vcount + 1;
        if (pending_valid_) {
            info_ = pending_;
            pending_valid_ = false;
        }
        info_.vcount = next_vcount;
    }

private:
    DisplayFramebufInfo info_;
    DisplayFramebufInfo pending_;
    bool pending_valid_{};
};

void register_display_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
