#include "p3p3ds/hle/audio.hpp"

#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>

namespace p3p3ds::hle {
namespace {
// references/uofw/include/audio.h
constexpr std::uint32_t kNotInitialized = 0x80260001u, kOutputBusy = 0x80260002u, kInvalidCh = 0x80260003u,
                        kNoFreeCh = 0x80260005u, kInvalidSize = 0x80260006u, kInvalidFormat = 0x80260007u,
                        kNotReserved = 0x80260008u, kInvalidFrequency = 0x8026000Au, kInvalidVolume = 0x8026000Bu,
                        kSrcBusy = 0x80268002u, kErrorInvalidSize = 0x80000104u; // common/errors.h

std::uint64_t duration_us(std::uint32_t samples) {
    return (static_cast<std::uint64_t>(samples) * 1000000u + AudioState::kSampleRate - 1u) / AudioState::kSampleRate;
}
} // namespace

std::uint32_t AudioState::reserve(std::int32_t channel, std::int32_t sample_count, std::int32_t format) noexcept {
    // uOFW audio.c::sceAudioChReserve and pspautotests audio/sceaudio/reserve.expected.
    if (channel < 0) {
        for (channel = 7; channel >= 0 && channels_[channel].reserved; --channel) {}
        if (channel < 0) return kNoFreeCh;
    } else if (channel >= 8 || channels_[channel].reserved) return kInvalidCh;
    if (sample_count <= 0 || sample_count > 0xFFC0 || (sample_count & 63) != 0) return kInvalidSize;
    if (format != 0 && format != 0x10) return kInvalidFormat;
    auto &ch = channels_[channel];
    ch = Channel{};
    ch.reserved = true;
    ch.sample_count = static_cast<std::uint32_t>(sample_count);
    ch.format = static_cast<std::uint32_t>(format);
    return static_cast<std::uint32_t>(channel);
}

std::uint32_t AudioState::release(std::uint32_t channel, std::uint64_t) noexcept {
    // uOFW sceAudioChRelease: NOT_RESERVED, then OUTPUT_BUSY while a thread blocks.
    if (channel >= 8u) return kInvalidCh;
    auto &ch = channels_[channel];
    if (!ch.reserved) return kNotReserved;
    if (ch.blocked != 0u) return kOutputBusy;
    ch.reserved = false;
    ch.sample_count = 0;
    return 0;
}

std::uint32_t AudioState::set_data_len(std::uint32_t channel, std::int32_t sample_count) noexcept {
    if (channel >= 8u) return kInvalidCh;
    if (sample_count <= 0 || (sample_count & 0x3F) != 0 || sample_count > 0xFFC0) return kInvalidSize;
    auto &ch = channels_[channel];
    if (ch.blocked != 0u) return kOutputBusy;
    if (!ch.reserved) return kNotInitialized;
    ch.sample_count = static_cast<std::uint32_t>(sample_count);
    return 0;
}

std::uint32_t AudioState::change_volume(std::uint32_t channel, std::int32_t left, std::int32_t right) noexcept {
    if (right > 0xFFFF || left > 0xFFFF) return kInvalidVolume;
    if (channel >= 8u) return kInvalidCh;
    auto &ch = channels_[channel];
    if (left >= 0) ch.left_volume = static_cast<std::uint32_t>(left);
    if (right >= 0) ch.right_volume = static_cast<std::uint32_t>(right);
    return 0;
}

std::uint32_t AudioState::change_config(std::uint32_t channel, std::int32_t format, std::uint64_t now) noexcept {
    // uOFW sceAudioChangeChannelConfig, SDK >= 2.00 branch (P3P compiles for 6.20):
    // busy only while a buffer is still queued behind the playing one.
    if (channel >= 8u) return kInvalidCh;
    auto &ch = channels_[channel];
    if (ch.blocked != 0u) return kOutputBusy;
    if (now < ch.play_end && now < ch.slot_free_at) return kOutputBusy;
    if (!ch.reserved) return kNotReserved;
    if (format != 0 && format != 0x10) return kInvalidFormat;
    ch.format = static_cast<std::uint32_t>(format);
    return 0;
}

std::uint32_t AudioState::rest_length(std::uint32_t channel, std::uint64_t now) const noexcept {
    // [INFERRED] samples not yet played across the playing and queued buffers.
    if (channel >= 8u) return kInvalidCh;
    const auto &ch = channels_[channel];
    if (now >= ch.play_end) return 0u;
    return static_cast<std::uint32_t>(((ch.play_end - now) * kSampleRate + 999999u) / 1000000u);
}

std::uint32_t AudioState::output(std::uint32_t index, std::int32_t left, std::int32_t right, std::uint32_t buffer,
                                 std::uint64_t now, const psprecomp::GuestMemory *memory) {
    auto &ch = channels_[index];
    if (!ch.reserved || ch.sample_count == 0u) return kNotInitialized;
    if (now < ch.slot_free_at) return kOutputBusy;
    const std::uint64_t start = std::max(now, ch.play_end);
    ch.slot_free_at = start;
    ch.play_end = start + duration_us(ch.sample_count);
    if (left >= 0) ch.left_volume = static_cast<std::uint32_t>(left);
    if (right >= 0) ch.right_volume = static_cast<std::uint32_t>(right);
    ++ch.buffers;
    if (sink_) {
        std::vector<std::int16_t> stereo(static_cast<std::size_t>(ch.sample_count) * 2u, 0);
        const bool mono = ch.format == 0x10u;
        const std::uint32_t bytes = ch.sample_count * (mono ? 2u : 4u);
        if (buffer != 0u && memory != nullptr && memory->contains(buffer, bytes)) {
            const auto scale = [](std::int32_t sample, std::uint32_t volume) {
                const auto v = (static_cast<std::int64_t>(sample) * volume) / 0x8000;
                return static_cast<std::int16_t>(std::clamp<std::int64_t>(v, -32768, 32767));
            };
            for (std::uint32_t i = 0; i < ch.sample_count; ++i) {
                const auto l = static_cast<std::int16_t>(memory->load16(buffer + i * (mono ? 2u : 4u)));
                const auto r = mono ? l : static_cast<std::int16_t>(memory->load16(buffer + i * 4u + 2u));
                stereo[i * 2u] = scale(l, ch.left_volume);
                stereo[i * 2u + 1u] = scale(r, ch.right_volume);
            }
        }
        sink_(index, start, stereo);
    }
    return ch.sample_count;
}

std::uint32_t AudioState::src_reserve(std::int32_t sample_count, std::int32_t frequency, std::int32_t channels) noexcept {
    // uOFW sceAudioSRCChReserve (Output2Reserve passes 44100 Hz, 2 channels).
    if (channels != 2 && channels != 4) return kErrorInvalidSize;
    if (channels == 4) return 0x80000003u;
    const std::int32_t half = (sample_count * channels) / 2;
    if (half < 17 || half > 4111) return kErrorInvalidSize;
    if (frequency != 0 && frequency != static_cast<std::int32_t>(kSampleRate)) {
        switch (frequency) {
        case 8000: case 11025: case 12000: case 16000: case 22050: case 24000: case 32000: case 48000: break;
        default: return kInvalidFrequency;
        }
    }
    auto &ch = channels_[kSrcChannel];
    if (ch.reserved) return kSrcBusy;
    ch = Channel{};
    ch.reserved = true;
    ch.sample_count = static_cast<std::uint32_t>(sample_count);
    ch.format = 0u;
    return 0;
}

std::uint32_t AudioState::src_release(std::uint64_t now) noexcept {
    auto &ch = channels_[kSrcChannel];
    if (!ch.reserved) return kNotReserved;
    if (now < ch.play_end) return kSrcBusy; // uOFW: buffers still pending in hardware
    ch.reserved = false;
    ch.sample_count = 0;
    return 0;
}

void register_audio_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    const auto reg = [&runtime](std::uint32_t nid, psprecomp::Runtime::HleFunction fn) {
        runtime.register_hle("sceAudio", nid, std::move(fn));
    };
    auto s = [](std::uint32_t v) { return static_cast<std::int32_t>(v); };

    reg(0x5EC81C55u, [&kernel, s](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceAudioChReserve
        const auto requested = ctx.gpr[4], count = ctx.gpr[5], format = ctx.gpr[6];
        const auto result = kernel.audio().reserve(s(requested), s(count), s(format));
        ctx.set_gpr(2, result);
        if (kernel.audio().record_telemetry())
            rt.event("audio_ch_reserve", {{"channel", requested}, {"samplecount", count}, {"format", format},
                {"result", result}, {"caller", ctx.gpr[31] - 8u}, {"return_pc", ctx.gpr[31]}});
    });
    reg(0x6FC46853u, [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceAudioChRelease
        ctx.set_gpr(2, kernel.audio().release(ctx.gpr[4], kernel.threads().now()));
    });
    reg(0xCB2E439Eu, [&kernel, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceAudioSetChannelDataLen
        ctx.set_gpr(2, kernel.audio().set_data_len(ctx.gpr[4], s(ctx.gpr[5])));
    });
    reg(0xB7E1D8E7u, [&kernel, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceAudioChangeChannelVolume
        ctx.set_gpr(2, kernel.audio().change_volume(ctx.gpr[4], s(ctx.gpr[5]), s(ctx.gpr[6])));
    });
    reg(0x95FD0C2Du, [&kernel, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceAudioChangeChannelConfig
        ctx.set_gpr(2, kernel.audio().change_config(ctx.gpr[4], s(ctx.gpr[5]), kernel.threads().now()));
    });
    reg(0xB011922Fu, [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceAudioGetChannelRestLength
        ctx.set_gpr(2, kernel.audio().rest_length(ctx.gpr[4], kernel.threads().now()));
    });

    // Output family: uOFW validates volume, then channel, then audioOutput().
    // Blocking variants wait (retrying at the stub) while OUTPUT_BUSY.
    const auto output = [&kernel, s](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx, std::uint32_t channel,
                                     std::uint32_t left, std::uint32_t right, std::uint32_t buffer, bool blocking) {
        if (s(left) > 0xFFFF || s(right) > 0xFFFF) { ctx.set_gpr(2, kInvalidVolume); return; }
        if (channel >= 8u) { ctx.set_gpr(2, kInvalidCh); return; }
        auto &audio = kernel.audio();
        auto &tm = kernel.threads();
        auto &ch = audio.channel_state(channel);
        if (ch.blocked != 0u && ch.blocked != static_cast<std::uint32_t>(tm.current_thread_id())) {
            ctx.set_gpr(2, kOutputBusy); // uOFW unk10: one blocked writer per channel
            return;
        }
        const auto result = audio.output(channel, s(left), s(right), buffer, tm.now(), &rt.memory());
        if (result == kOutputBusy && blocking) {
            ch.blocked = static_cast<std::uint32_t>(tm.current_thread_id());
            WaitInfo wait{WaitType::Audio, static_cast<std::int32_t>(channel), ch.slot_free_at};
            wait.retry = true;
            tm.block_current(rt, ctx, wait);
            return;
        }
        if (ch.blocked == static_cast<std::uint32_t>(tm.current_thread_id())) ch.blocked = 0u;
        ctx.set_gpr(2, result);
    };
    reg(0x136CAF51u, [output](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceAudioOutputBlocking
        output(rt, ctx, ctx.gpr[4], ctx.gpr[5], ctx.gpr[5], ctx.gpr[6], true);
    });
    reg(0x8C1009B2u, [output](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceAudioOutput
        output(rt, ctx, ctx.gpr[4], ctx.gpr[5], ctx.gpr[5], ctx.gpr[6], false);
    });
    reg(0x13F592BCu, [output](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceAudioOutputPannedBlocking
        output(rt, ctx, ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], true);
    });
    reg(0xE2D56B2Du, [output](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceAudioOutputPanned
        output(rt, ctx, ctx.gpr[4], ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], false);
    });

    // Output2 = SRC channel at 44.1 kHz stereo (uOFW sceAudioOutput2* wrappers).
    reg(0x01562BA3u, [&kernel, s](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceAudioOutput2Reserve
        ctx.set_gpr(2, kernel.audio().src_reserve(s(ctx.gpr[4]), 44100, 2));
    });
    reg(0x43196845u, [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceAudioOutput2Release
        ctx.set_gpr(2, kernel.audio().src_release(kernel.threads().now()));
    });
    reg(0x2D53F36Eu, [&kernel, s](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceAudioOutput2OutputBlocking
        const auto volume = ctx.gpr[4];
        if (volume > 0xFFFFFu) { ctx.set_gpr(2, kInvalidVolume); return; }
        auto &audio = kernel.audio();
        auto &tm = kernel.threads();
        auto &ch = audio.channel_state(AudioState::kSrcChannel);
        if (!ch.reserved) { ctx.set_gpr(2, kNotReserved); return; }
        const auto vol = std::min<std::uint32_t>(volume, 0xFFFFu);
        const auto result = audio.output(AudioState::kSrcChannel, s(vol), s(vol), ctx.gpr[5], tm.now(), &rt.memory());
        if (result == kOutputBusy) {
            WaitInfo wait{WaitType::Audio, static_cast<std::int32_t>(AudioState::kSrcChannel), ch.slot_free_at};
            wait.retry = true;
            tm.block_current(rt, ctx, wait);
            return;
        }
        ctx.set_gpr(2, result);
    });
}

} // namespace p3p3ds::hle
