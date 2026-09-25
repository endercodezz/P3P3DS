#include "movie/avc_decoder.hpp"

#include <cstring>

#if defined(MHP3RD_HAS_FFMPEG)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixfmt.h>
}
#endif

namespace mhp3rd::movie {

#if defined(MHP3RD_HAS_FFMPEG)

struct AvcDecoder::Impl {
    AVCodecContext *context{};
    AVPacket *packet{};
    AVFrame *frame{};
    bool draining{};

    ~Impl() { close(); }

    void close() {
        avcodec_free_context(&context);
        av_packet_free(&packet);
        av_frame_free(&frame);
        draining = false;
    }

    // Copies the frame FFmpeg holds into `picture`, dropping row padding.
    bool take(Picture &picture) {
        if (avcodec_receive_frame(context, frame) < 0) return false;
        const auto format = static_cast<AVPixelFormat>(frame->format);
        if (format != AV_PIX_FMT_YUV420P && format != AV_PIX_FMT_YUVJ420P) {
            av_frame_unref(frame);
            return false;
        }
        picture.width = static_cast<std::uint32_t>(frame->width);
        picture.height = static_cast<std::uint32_t>(frame->height);
        picture.full_range = format == AV_PIX_FMT_YUVJ420P || frame->color_range == AVCOL_RANGE_JPEG;
        const auto copy_plane = [&](int plane, std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t> &out) {
            out.resize(static_cast<std::size_t>(width) * height);
            for (std::uint32_t row = 0; row < height; ++row)
                std::memcpy(out.data() + static_cast<std::size_t>(row) * width,
                            frame->data[plane] + static_cast<std::ptrdiff_t>(row) * frame->linesize[plane], width);
        };
        copy_plane(0, picture.width, picture.height, picture.y);
        copy_plane(1, (picture.width + 1u) / 2u, (picture.height + 1u) / 2u, picture.cb);
        copy_plane(2, (picture.width + 1u) / 2u, (picture.height + 1u) / 2u, picture.cr);
        av_frame_unref(frame);
        return true;
    }
};

AvcDecoder::AvcDecoder() : impl_(std::make_unique<Impl>()) {}
AvcDecoder::~AvcDecoder() = default;

bool AvcDecoder::available() noexcept { return avcodec_find_decoder(AV_CODEC_ID_H264) != nullptr; }

bool AvcDecoder::open() {
    Impl &impl = *impl_;
    impl.close();
    const AVCodec *decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (decoder == nullptr) return false;
    impl.context = avcodec_alloc_context3(decoder);
    impl.packet = av_packet_alloc();
    impl.frame = av_frame_alloc();
    if (impl.context == nullptr || impl.packet == nullptr || impl.frame == nullptr) {
        impl.close();
        return false;
    }
    // One picture in, one picture out, in the calling thread: the guest
    // expects each decode call to finish its picture.
    impl.context->thread_count = 1;
    impl.context->flags |= AV_CODEC_FLAG_LOW_DELAY;
    if (avcodec_open2(impl.context, decoder, nullptr) < 0) {
        impl.close();
        return false;
    }
    return true;
}

void AvcDecoder::close() { impl_->close(); }
bool AvcDecoder::is_open() const noexcept { return impl_->context != nullptr; }

bool AvcDecoder::decode(std::span<const std::uint8_t> unit, Picture &picture) {
    Impl &impl = *impl_;
    if (impl.context == nullptr || unit.empty() || impl.draining) return false;
    av_packet_unref(impl.packet);
    if (av_new_packet(impl.packet, static_cast<int>(unit.size())) < 0) return false;
    std::memcpy(impl.packet->data, unit.data(), unit.size());
    if (avcodec_send_packet(impl.context, impl.packet) < 0) return false;
    return impl.take(picture);
}

bool AvcDecoder::drain(Picture &picture) {
    Impl &impl = *impl_;
    if (impl.context == nullptr) return false;
    if (!impl.draining) {
        impl.draining = true;
        avcodec_send_packet(impl.context, nullptr);
    }
    return impl.take(picture);
}

void AvcDecoder::reset() {
    Impl &impl = *impl_;
    if (impl.context == nullptr) return;
    avcodec_flush_buffers(impl.context);
    impl.draining = false;
}

#else // !MHP3RD_HAS_FFMPEG

struct AvcDecoder::Impl {};

AvcDecoder::AvcDecoder() : impl_(std::make_unique<Impl>()) {}
AvcDecoder::~AvcDecoder() = default;

bool AvcDecoder::available() noexcept { return false; }
bool AvcDecoder::open() { return false; }
void AvcDecoder::close() {}
bool AvcDecoder::is_open() const noexcept { return false; }
bool AvcDecoder::decode(std::span<const std::uint8_t>, Picture &) { return false; }
bool AvcDecoder::drain(Picture &) { return false; }
void AvcDecoder::reset() {}

#endif

} // namespace mhp3rd::movie
