#include "audio/atrac_decoder.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#if defined(MHP3RD_HAS_FFMPEG)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
}
#endif

namespace mhp3rd::audio {

#if defined(MHP3RD_HAS_FFMPEG)

struct AtracDecoder::Impl {
    AtracCodec codec{AtracCodec::Atrac3};
    unsigned channels{};
    AVCodecContext *context{};
    AVPacket *packet{};
    AVFrame *frame{};

    ~Impl() { close(); }

    void close() {
        avcodec_free_context(&context);
        av_packet_free(&packet);
        av_frame_free(&frame);
    }
};

namespace {

[[nodiscard]] std::int16_t to_pcm16(float value) noexcept {
    const float scaled = std::nearbyint(value * 32768.0f);
    return static_cast<std::int16_t>(std::clamp(scaled, -32768.0f, 32767.0f));
}

} // namespace

AtracDecoder::AtracDecoder() : impl_(std::make_unique<Impl>()) {}
AtracDecoder::~AtracDecoder() = default;

bool AtracDecoder::available() noexcept { return true; }

bool AtracDecoder::open(AtracCodec codec, unsigned channels, unsigned block_align,
                        std::span<const std::uint8_t> extradata) {
    Impl &impl = *impl_;
    impl.close();
    if (channels == 0u || channels > 2u || block_align == 0u) return false;
    const AVCodec *decoder = avcodec_find_decoder(codec == AtracCodec::Atrac3 ? AV_CODEC_ID_ATRAC3 : AV_CODEC_ID_ATRAC3P);
    if (decoder == nullptr) return false;
    impl.context = avcodec_alloc_context3(decoder);
    impl.packet = av_packet_alloc();
    impl.frame = av_frame_alloc();
    if (impl.context == nullptr || impl.packet == nullptr || impl.frame == nullptr) {
        impl.close();
        return false;
    }
    av_channel_layout_default(&impl.context->ch_layout, static_cast<int>(channels));
    impl.context->sample_rate = 44'100;
    impl.context->block_align = static_cast<int>(block_align);
    if (!extradata.empty()) {
        auto *copy = static_cast<std::uint8_t *>(av_mallocz(extradata.size() + AV_INPUT_BUFFER_PADDING_SIZE));
        if (copy == nullptr) {
            impl.close();
            return false;
        }
        std::memcpy(copy, extradata.data(), extradata.size());
        impl.context->extradata = copy;
        impl.context->extradata_size = static_cast<int>(extradata.size());
    }
    if (avcodec_open2(impl.context, decoder, nullptr) < 0) {
        impl.close();
        return false;
    }
    impl.codec = codec;
    impl.channels = channels;
    return true;
}

bool AtracDecoder::is_open() const noexcept { return impl_->context != nullptr; }
AtracCodec AtracDecoder::codec() const noexcept { return impl_->codec; }

std::size_t AtracDecoder::decode(std::span<const std::uint8_t> frame, std::int16_t *out) {
    Impl &impl = *impl_;
    if (impl.context == nullptr || frame.empty()) return 0u;
    av_packet_unref(impl.packet);
    if (av_new_packet(impl.packet, static_cast<int>(frame.size())) < 0) return 0u;
    std::memcpy(impl.packet->data, frame.data(), frame.size());
    if (avcodec_send_packet(impl.context, impl.packet) < 0) return 0u;
    if (avcodec_receive_frame(impl.context, impl.frame) < 0) return 0u;

    const std::size_t limit = atrac_frame_samples(impl.codec);
    const auto samples = std::min<std::size_t>(static_cast<std::size_t>(impl.frame->nb_samples), limit);
    const bool planar = impl.frame->format == AV_SAMPLE_FMT_FLTP;
    const bool packed = impl.frame->format == AV_SAMPLE_FMT_FLT;
    if (!planar && !packed) {
        av_frame_unref(impl.frame);
        return 0u;
    }
    const unsigned channels = static_cast<unsigned>(impl.frame->ch_layout.nb_channels);
    for (std::size_t i = 0; i < samples; ++i) {
        float left{};
        float right{};
        if (planar) {
            left = reinterpret_cast<const float *>(impl.frame->extended_data[0])[i];
            right = channels > 1u ? reinterpret_cast<const float *>(impl.frame->extended_data[1])[i] : left;
        } else {
            const auto *interleaved = reinterpret_cast<const float *>(impl.frame->extended_data[0]);
            left = interleaved[i * channels];
            right = channels > 1u ? interleaved[i * channels + 1u] : left;
        }
        out[i * 2u] = to_pcm16(left);
        out[i * 2u + 1u] = to_pcm16(right);
    }
    av_frame_unref(impl.frame);
    return samples;
}

void AtracDecoder::reset() {
    if (impl_->context != nullptr) avcodec_flush_buffers(impl_->context);
}

#else // !MHP3RD_HAS_FFMPEG

struct AtracDecoder::Impl {};

AtracDecoder::AtracDecoder() : impl_(std::make_unique<Impl>()) {}
AtracDecoder::~AtracDecoder() = default;

bool AtracDecoder::available() noexcept { return false; }
bool AtracDecoder::open(AtracCodec, unsigned, unsigned, std::span<const std::uint8_t>) { return false; }
bool AtracDecoder::is_open() const noexcept { return false; }
AtracCodec AtracDecoder::codec() const noexcept { return AtracCodec::Atrac3; }
std::size_t AtracDecoder::decode(std::span<const std::uint8_t>, std::int16_t *) { return 0u; }
void AtracDecoder::reset() {}

#endif

} // namespace mhp3rd::audio
