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
        info_.topaddr = topaddr;
        info_.bufferwidth = bufferwidth;
        info_.pixelformat = pixelformat;
        info_.sync = sync;
        info_.active = (topaddr != 0);
        return 0;
    }

    [[nodiscard]] const DisplayFramebufInfo &info() const noexcept { return info_; }
    [[nodiscard]] bool has_framebuf() const noexcept { return info_.active; }
    [[nodiscard]] std::uint64_t vcount() noexcept { return ++info_.vcount; }

private:
    DisplayFramebufInfo info_;
};

void register_display_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
