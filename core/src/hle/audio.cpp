#include "p3p3ds/hle/audio.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

namespace p3p3ds::hle {
std::uint32_t AudioState::reserve(std::int32_t channel, std::int32_t sample_count, std::int32_t format) noexcept {
    // uOFW audio.c::sceAudioChReserve and pspautotests audio/sceaudio/reserve.expected.
    if (channel < 0) {
        for (channel=7;channel>=0 && channels_[channel].reserved;--channel) {}
        if (channel<0) return 0x80260005u; // No free channel.
    } else if (channel>=8 || channels_[channel].reserved) return 0x80260003u;
    if (sample_count<=0 || sample_count>0xFFC0 || (sample_count&63)!=0) return 0x80260006u;
    if (format!=0 && format!=0x10) return 0x80260007u;
    channels_[channel]={true,static_cast<std::uint32_t>(sample_count),static_cast<std::uint32_t>(format)};
    return static_cast<std::uint32_t>(channel);
}

void register_audio_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    runtime.register_hle("sceAudio",0x5EC81C55u,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            const auto requested=ctx.gpr[4], count=ctx.gpr[5], format=ctx.gpr[6];
            const auto result=kernel.audio().reserve(static_cast<std::int32_t>(requested),
                static_cast<std::int32_t>(count),static_cast<std::int32_t>(format));
            ctx.set_gpr(2,result);
            if (kernel.audio().record_telemetry())
                rt.event("audio_ch_reserve",{{"channel",requested},{"samplecount",count},
                    {"format",format},{"result",result},{"caller",ctx.gpr[31]-8u},
                    {"return_pc",ctx.gpr[31]}});
        });
}
}
