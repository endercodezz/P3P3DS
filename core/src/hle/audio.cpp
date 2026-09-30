#include "p3p3ds/hle/audio.hpp"

#include <algorithm>
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

std::uint32_t AudioState::output(std::uint32_t index, std::uint32_t left, std::uint32_t right, std::uint64_t now) noexcept {
    // uOFW audio.c::sceAudioOutputBlocking/audioOutput and include/audio.h.
    if (left > 0xFFFFu || right > 0xFFFFu) return 0x8026000Bu; // INVALID_VOLUME
    if (index >= 8u) return 0x80260003u;                       // INVALID_CH
    auto &ch = channels_[index];
    if (!ch.reserved || ch.sample_count == 0u) return 0x80260001u; // NOT_INITIALIZED
    if (now < ch.slot_free_at) return 0x80260002u;                // OUTPUT_BUSY
    const std::uint64_t duration = (static_cast<std::uint64_t>(ch.sample_count) * 1000000u + kSampleRate - 1u) / kSampleRate;
    const std::uint64_t start = std::max(now, ch.play_end);
    ch.slot_free_at = start;
    ch.play_end = start + duration;
    ch.left_volume = left;
    ch.right_volume = right;
    ++ch.buffers;
    return ch.sample_count;
}

void register_audio_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    // sceAudioOutputBlocking(channel, volume, buffer): uOFW retries audioOutput
    // while OUTPUT_BUSY, waiting on the channel's completion event.
    runtime.register_hle("sceAudio",0x136CAF51u,
        [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) {
            auto &tm = kernel.threads();
            const auto channel = ctx.gpr[4], volume = ctx.gpr[5];
            auto result = kernel.audio().output(channel, volume, volume, tm.now());
            if (result == 0x80260002u) {
                // Retry at the stub once the queued buffer starts playing.
                WaitInfo wait{WaitType::Audio, static_cast<std::int32_t>(channel),
                              kernel.audio().channel_state(channel).slot_free_at};
                wait.retry = true;
                tm.block_current(rt, ctx, wait);
                return;
            }
            ctx.set_gpr(2, result);
        });
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
