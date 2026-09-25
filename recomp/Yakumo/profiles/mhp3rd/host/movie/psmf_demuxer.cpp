#include "movie/psmf_demuxer.hpp"

#include <algorithm>

namespace mhp3rd::movie {
namespace {

constexpr std::uint8_t kPackStart = 0xBAu;
constexpr std::uint8_t kVideoStream = 0xE0u;
constexpr std::uint8_t kPrivateStream1 = 0xBDu;
// Private stream 1 payloads start with a sub-stream id and three more bytes.
constexpr std::size_t kPrivateHeader = 4u;
constexpr std::uint8_t kAccessUnitDelimiter = 9u;
constexpr std::uint32_t kAudioSampleRate = 44'100u;

[[nodiscard]] std::uint16_t be16(const std::uint8_t *bytes) noexcept {
    return static_cast<std::uint16_t>((bytes[0] << 8u) | bytes[1]);
}

[[nodiscard]] std::int64_t timestamp(const std::uint8_t *bytes) noexcept {
    return (static_cast<std::int64_t>((bytes[0] >> 1u) & 7u) << 30u) | (static_cast<std::int64_t>(bytes[1]) << 22u) |
           (static_cast<std::int64_t>(bytes[2] >> 1u) << 15u) | (static_cast<std::int64_t>(bytes[3]) << 7u) |
           static_cast<std::int64_t>(bytes[4] >> 1u);
}

} // namespace

void PsmfDemuxer::reset() { *this = PsmfDemuxer{}; }

bool PsmfDemuxer::push_pack(std::span<const std::uint8_t> pack) {
    if (pack.size() < 14u || pack[0] != 0u || pack[1] != 0u || pack[2] != 1u || pack[3] != kPackStart) return false;
    std::size_t offset = 14u + (pack[13] & 7u);
    while (offset + 6u <= pack.size() && pack[offset] == 0u && pack[offset + 1u] == 0u && pack[offset + 2u] == 1u) {
        const std::uint8_t stream = pack[offset + 3u];
        const std::size_t length = be16(&pack[offset + 4u]);
        const std::size_t body = offset + 6u;
        const std::size_t end = std::min(body + length, pack.size());
        offset = body + length;
        if ((stream != kVideoStream && stream != kPrivateStream1) || end < body + 3u) continue;
        // MPEG-2 PES header: flags, then the length of the optional fields.
        const std::uint8_t flags = pack[body + 1u];
        const std::size_t header = body + 3u + pack[body + 2u];
        if (header > end) continue;
        std::int64_t pts = -1;
        std::int64_t dts = -1;
        if ((flags & 0x80u) != 0u && body + 8u <= end) pts = timestamp(&pack[body + 3u]);
        if ((flags & 0x40u) != 0u && body + 13u <= end) dts = timestamp(&pack[body + 8u]);
        const auto payload = pack.subspan(header, end - header);
        if (stream == kVideoStream) {
            add_video(payload, pts, dts);
        } else if (payload.size() > kPrivateHeader && payload[0] == 0u) {
            add_audio(payload.subspan(kPrivateHeader), pts);
        }
    }
    split_video();
    split_audio();
    return true;
}

void PsmfDemuxer::end_of_stream() {
    ended_ = true;
    split_video();
}

void PsmfDemuxer::add_video(std::span<const std::uint8_t> payload, std::int64_t pts, std::int64_t dts) {
    if (pts >= 0) video_stamps_.push_back(Stamp{video_.size(), pts, dts >= 0 ? dts : pts});
    video_.insert(video_.end(), payload.begin(), payload.end());
}

void PsmfDemuxer::add_audio(std::span<const std::uint8_t> payload, std::int64_t pts) {
    if (pts >= 0) audio_stamps_.push_back(Stamp{audio_.size(), pts, pts});
    audio_.insert(audio_.end(), payload.begin(), payload.end());
}

void PsmfDemuxer::split_video() {
    // Find the delimiters (00 00 01 09) that arrived since the last scan.
    for (std::size_t i = video_scanned_; i + 3u < video_.size(); ++i) {
        if (video_[i] == 0u && video_[i + 1u] == 0u && video_[i + 2u] == 1u &&
            (video_[i + 3u] & 0x1Fu) == kAccessUnitDelimiter) {
            // Keep the leading zero of a four-byte start code with its picture.
            video_starts_.push_back(i > 0u && video_[i - 1u] == 0u ? i - 1u : i);
            i += 3u;
        }
    }
    video_scanned_ = video_.size() >= 3u ? video_.size() - 3u : 0u;

    std::size_t consumed = 0u;
    std::size_t index = 0u;
    while (index < video_starts_.size()) {
        const std::size_t start = video_starts_[index];
        std::size_t end{};
        if (index + 1u < video_starts_.size()) end = video_starts_[index + 1u];
        else if (ended_) end = video_.size();
        else break;

        AccessUnit unit;
        unit.data.assign(video_.begin() + static_cast<std::ptrdiff_t>(start), video_.begin() + static_cast<std::ptrdiff_t>(end));
        // A PES timestamp belongs to the first picture that starts in it.
        std::optional<Stamp> stamp;
        while (!video_stamps_.empty() && video_stamps_.front().offset <= start) {
            stamp = video_stamps_.front();
            video_stamps_.pop_front();
        }
        if (stamp) {
            unit.pts = stamp->pts;
            unit.dts = stamp->dts;
        } else if (last_video_pts_ >= 0) {
            unit.pts = last_video_pts_ + kVideoFrameTicks;
            unit.dts = last_video_dts_ + kVideoFrameTicks;
        }
        last_video_pts_ = unit.pts;
        last_video_dts_ = unit.dts;
        video_units_.push_back(std::move(unit));
        consumed = end;
        ++index;
    }
    if (consumed == 0u) return;
    video_.erase(video_.begin(), video_.begin() + static_cast<std::ptrdiff_t>(consumed));
    video_starts_.erase(video_starts_.begin(), video_starts_.begin() + static_cast<std::ptrdiff_t>(index));
    for (std::size_t &start : video_starts_) start -= consumed;
    for (Stamp &stamp : video_stamps_) stamp.offset = stamp.offset > consumed ? stamp.offset - consumed : 0u;
    video_scanned_ = video_scanned_ > consumed ? video_scanned_ - consumed : 0u;
}

void PsmfDemuxer::split_audio() {
    std::size_t consumed = 0u;
    while (consumed + kAtracFrameHeader <= audio_.size()) {
        const std::uint8_t *header = audio_.data() + consumed;
        if (header[0] != 0x0Fu || header[1] != 0xD0u) {
            // Lost sync: skip to the next frame header.
            ++consumed;
            continue;
        }
        const std::uint16_t parameters = be16(header + 2u);
        const std::size_t size = ((parameters & 0x3FFu) + 1u) * 8u + kAtracFrameHeader;
        if (consumed + size > audio_.size()) break;
        audio_frame_size_ = size;
        audio_channels_ = (parameters >> 10u) & 7u;

        AccessUnit unit;
        unit.data.assign(audio_.begin() + static_cast<std::ptrdiff_t>(consumed + kAtracFrameHeader),
                         audio_.begin() + static_cast<std::ptrdiff_t>(consumed + size));
        std::optional<Stamp> stamp;
        while (!audio_stamps_.empty() && audio_stamps_.front().offset <= consumed) {
            stamp = audio_stamps_.front();
            audio_stamps_.pop_front();
        }
        if (stamp) unit.pts = stamp->pts;
        else if (last_audio_pts_ >= 0)
            unit.pts = last_audio_pts_ + static_cast<std::int64_t>(kAtracFrameSamples) * 90'000 / kAudioSampleRate;
        unit.dts = unit.pts;
        last_audio_pts_ = unit.pts;
        audio_units_.push_back(std::move(unit));
        consumed += size;
    }
    if (consumed == 0u) return;
    audio_.erase(audio_.begin(), audio_.begin() + static_cast<std::ptrdiff_t>(consumed));
    for (Stamp &stamp : audio_stamps_) stamp.offset = stamp.offset > consumed ? stamp.offset - consumed : 0u;
}

std::optional<AccessUnit> PsmfDemuxer::pop_video() {
    if (video_units_.empty()) return std::nullopt;
    AccessUnit unit = std::move(video_units_.front());
    video_units_.pop_front();
    return unit;
}

std::optional<AccessUnit> PsmfDemuxer::pop_audio() {
    if (audio_units_.empty()) return std::nullopt;
    AccessUnit unit = std::move(audio_units_.front());
    audio_units_.pop_front();
    return unit;
}

bool PsmfDemuxer::video_ready() const { return !video_units_.empty(); }
bool PsmfDemuxer::audio_ready() const { return !audio_units_.empty(); }

} // namespace mhp3rd::movie
