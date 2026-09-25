// sceAtrac3plus: the game's streamed music. Each track is a whole ATRAC3 WAVE
// file the game has already read into guest memory; the library decodes it one
// frame per call into 16-bit stereo PCM, which the game's own decode threads
// then hand to sceAudio. Frames are decoded with FFmpeg (audio/atrac_decoder),
// so without it nothing here is bound and the imports stay logging stubs.
//
// Sample positions follow the library's convention: position 0 is the first
// sample the encoder was given. The decoded stream starts earlier, by the
// encoder delay recorded in the `fact` chunk plus the decoder's own delay, and
// loop points in the `smpl` chunk are counted including the encoder delay.
#include "hle_common.hpp"

#include "audio/atrac_decoder.hpp"

#include "psprecomp/common.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

namespace mhp3rd {
namespace {

namespace atrac_error {
inline constexpr std::uint32_t kParamFail = 0x80630001u;
inline constexpr std::uint32_t kNoAtracId = 0x80630003u;
inline constexpr std::uint32_t kBadCodecType = 0x80630004u;
inline constexpr std::uint32_t kBadAtracId = 0x80630005u;
inline constexpr std::uint32_t kUnknownFormat = 0x80630006u;
inline constexpr std::uint32_t kBadCodecParam = 0x80630008u;
inline constexpr std::uint32_t kNoData = 0x80630010u;
inline constexpr std::uint32_t kSizeTooSmall = 0x80630011u;
inline constexpr std::uint32_t kBadSample = 0x80630015u;
inline constexpr std::uint32_t kNoLoopInformation = 0x80630021u;
inline constexpr std::uint32_t kAllDataDecoded = 0x80630024u;
} // namespace atrac_error

constexpr std::uint16_t kFormatAtrac3 = 0x0270u;
constexpr std::uint16_t kFormatExtensible = 0xFFFEu;
// Samples the decoder itself delays its output by, per codec.
constexpr std::uint32_t kAtrac3DecoderDelay = 69u;
constexpr std::uint32_t kAtrac3PlusDecoderDelay = 368u;
// "Every byte of the file is in the buffer", reported instead of a frame count.
constexpr std::int32_t kRemainAllDataOnMemory = -1;
constexpr std::size_t kMaxAtracIds = 6u;

bool trace_atrac() {
    static const bool enabled = std::getenv("MHP3RD_TRACE_ATRAC") != nullptr;
    return enabled;
}

void trace(const std::string &line) {
    if (trace_atrac()) std::cerr << "[atrac] " << line << "\n";
}

// What SetData learns from the WAVE header.
struct TrackInfo {
    audio::AtracCodec codec{audio::AtracCodec::Atrac3};
    std::uint32_t channels{};
    std::uint32_t block_align{};
    std::vector<std::uint8_t> extradata;
    std::uint32_t file_size{};
    std::uint32_t data_offset{};
    std::uint32_t data_size{};
    std::int32_t end_sample{};    // last playable position
    std::int32_t loop_start{-1};  // positions, -1 without a loop
    std::int32_t loop_end{-1};
    std::uint32_t skip{};         // decoded samples before position 0
};

struct AtracContext {
    TrackInfo track;
    std::uint32_t buffer{};
    std::uint32_t buffer_size{};
    std::int32_t loop_num{};
    std::int32_t position{};  // next position DecodeData returns
    audio::AtracDecoder decoder;
    // The frame the decoder produces next if fed sequentially, and the last
    // frame it produced, kept because a call rarely consumes a whole frame.
    std::int64_t next_frame{};
    std::int64_t cached_frame{-1};
    std::vector<std::int16_t> cached;
};

std::array<std::unique_ptr<AtracContext>, kMaxAtracIds> &contexts() {
    static std::array<std::unique_ptr<AtracContext>, kMaxAtracIds> table;
    return table;
}

AtracContext *find_context(std::uint32_t id) {
    return id < kMaxAtracIds ? contexts()[id].get() : nullptr;
}

// Unknown ids and released ids fail differently on hardware.
std::uint32_t context_error(std::uint32_t id) {
    return id < kMaxAtracIds ? atrac_error::kNoData : atrac_error::kBadAtracId;
}

std::uint32_t read_le32(const std::vector<std::uint8_t> &bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
           (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
           (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

std::uint16_t read_le16(const std::vector<std::uint8_t> &bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1u] << 8u));
}

// Parses the RIFF WAVE header at the start of the guest buffer. Returns an
// error code on failure.
std::optional<std::uint32_t> parse_header(const psprecomp::GuestMemory &memory, std::uint32_t buffer,
                                          std::uint32_t buffer_size, TrackInfo &info) {
    if (buffer_size < 12u) return atrac_error::kSizeTooSmall;
    // The header is small; everything up to the data chunk fits in far less.
    const std::uint32_t header_bytes = std::min<std::uint32_t>(buffer_size, 0x1000u);
    std::vector<std::uint8_t> bytes(header_bytes);
    memory.copy_out(buffer, bytes);
    if (read_le32(bytes, 0u) != 0x46464952u || read_le32(bytes, 8u) != 0x45564157u)  // "RIFF", "WAVE"
        return atrac_error::kUnknownFormat;
    info.file_size = read_le32(bytes, 4u) + 8u;

    bool have_format = false;
    std::optional<std::uint32_t> fact_samples;
    std::uint32_t fact_offset = 0u;
    std::optional<std::pair<std::uint32_t, std::uint32_t>> loop;
    std::size_t offset = 12u;
    while (offset + 8u <= bytes.size()) {
        const std::uint32_t id = read_le32(bytes, offset);
        const std::uint32_t size = read_le32(bytes, offset + 4u);
        const std::size_t body = offset + 8u;
        if (id == 0x61746164u) {  // "data"
            info.data_offset = static_cast<std::uint32_t>(body);
            info.data_size = size;
            break;
        }
        if (body + size > bytes.size()) return atrac_error::kSizeTooSmall;
        if (id == 0x20746D66u && size >= 16u) {  // "fmt "
            const std::uint16_t tag = read_le16(bytes, body);
            info.channels = read_le16(bytes, body + 2u);
            info.block_align = read_le16(bytes, body + 12u);
            if (tag == kFormatAtrac3) {
                info.codec = audio::AtracCodec::Atrac3;
                if (size >= 18u + 14u) info.extradata.assign(bytes.begin() + static_cast<std::ptrdiff_t>(body + 18u),
                                                              bytes.begin() + static_cast<std::ptrdiff_t>(body + 32u));
            } else if (tag == kFormatExtensible) {
                info.codec = audio::AtracCodec::Atrac3Plus;
            } else {
                return atrac_error::kBadCodecType;
            }
            have_format = true;
        } else if (id == 0x74636166u && size >= 4u) {  // "fact"
            fact_samples = read_le32(bytes, body);
            if (size >= 8u) fact_offset = read_le32(bytes, body + 4u);
        } else if (id == 0x6C706D73u && size >= 36u) {  // "smpl"
            const std::uint32_t loops = read_le32(bytes, body + 28u);
            if (loops > 0u && size >= 36u + 24u) loop = {read_le32(bytes, body + 36u + 8u), read_le32(bytes, body + 36u + 12u)};
        }
        offset = body + size + (size & 1u);
    }
    if (!have_format || info.data_offset == 0u) return atrac_error::kUnknownFormat;
    if (info.channels == 0u || info.channels > 2u || info.block_align == 0u) return atrac_error::kBadCodecParam;

    const std::uint32_t frame_samples = static_cast<std::uint32_t>(audio::atrac_frame_samples(info.codec));
    info.skip = fact_offset + (info.codec == audio::AtracCodec::Atrac3 ? kAtrac3DecoderDelay : kAtrac3PlusDecoderDelay);
    const std::uint64_t decoded = static_cast<std::uint64_t>(info.data_size / info.block_align) * frame_samples;
    const std::uint64_t playable = decoded > info.skip ? decoded - info.skip : 0u;
    const std::uint64_t samples = fact_samples ? std::min<std::uint64_t>(*fact_samples, playable) : playable;
    if (samples == 0u) return atrac_error::kUnknownFormat;
    info.end_sample = static_cast<std::int32_t>(samples - 1u);
    if (loop && loop->first >= fact_offset && loop->second >= loop->first) {
        info.loop_start = static_cast<std::int32_t>(loop->first - fact_offset);
        info.loop_end = std::min(static_cast<std::int32_t>(loop->second - fact_offset), info.end_sample);
    }
    return std::nullopt;
}

bool looping(const AtracContext &context) {
    return context.track.loop_start >= 0 && context.loop_num != 0;
}

// Decodes stream frame `frame` into the cache, reseeding the decoder when the
// request is not the frame that follows the last one it decoded.
bool load_frame(const psprecomp::GuestMemory &memory, AtracContext &context, std::int64_t frame) {
    if (frame == context.cached_frame) return true;
    const TrackInfo &track = context.track;
    const std::size_t frame_samples = audio::atrac_frame_samples(track.codec);
    context.cached.assign(frame_samples * 2u, 0);
    std::vector<std::uint8_t> bytes(track.block_align);
    const auto decode = [&](std::int64_t index) {
        const std::uint64_t offset = track.data_offset + static_cast<std::uint64_t>(index) * track.block_align;
        if (offset + track.block_align > std::min(context.buffer_size, track.data_offset + track.data_size)) return false;
        memory.copy_out(context.buffer + static_cast<std::uint32_t>(offset), bytes);
        return context.decoder.decode(bytes, context.cached.data()) != 0u;
    };
    if (frame != context.next_frame) {
        // A frame's output depends on the ones before it: after a seek, two
        // frames of warm-up make the decoder's state, and so its output,
        // identical to decoding straight through.
        context.decoder.reset();
        for (std::int64_t warm_up = std::max<std::int64_t>(frame - 2, 0); warm_up < frame; ++warm_up)
            (void)decode(warm_up);
    }
    const bool decoded = decode(frame);
    if (!decoded) std::fill(context.cached.begin(), context.cached.end(), std::int16_t{0});
    context.next_frame = frame + 1;
    context.cached_frame = frame;
    return decoded;
}

void release_context(std::uint32_t id) {
    if (id < kMaxAtracIds) contexts()[id].reset();
}

void write_s32(psprecomp::GuestMemory &memory, std::uint32_t address, std::int32_t value) {
    if (address != 0u) memory.store32(address, static_cast<std::uint32_t>(value));
}

void finish_traced(AllegrexContext &ctx, const char *name, std::uint32_t result, const std::string &details = {}) {
    if (trace_atrac()) {
        std::ostringstream line;
        line << name << "(" << psprecomp::hex32(arg(ctx, 0)) << ", " << psprecomp::hex32(arg(ctx, 1)) << ", "
             << psprecomp::hex32(arg(ctx, 2)) << ", " << psprecomp::hex32(arg(ctx, 3)) << ") -> "
             << psprecomp::hex32(result);
        if (!details.empty()) line << " " << details;
        trace(line.str());
    }
    kernel().finish(ctx, result);
}

void register_atrac_functions(HleRegistrar &hle) {
    // sceAtracSetDataAndGetID(buffer, bufferSize) -> atracID
    hle.add("sceAtrac3plus", "sceAtracSetDataAndGetID", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t buffer = arg(ctx, 0);
        const std::uint32_t buffer_size = arg(ctx, 1);
        auto context = std::make_unique<AtracContext>();
        if (auto failed = parse_header(rt.memory(), buffer, buffer_size, context->track)) {
            finish_traced(ctx, "sceAtracSetDataAndGetID", *failed, "bad header");
            return;
        }
        const TrackInfo &track = context->track;
        if (buffer_size < track.file_size)
            log_once("atrac-partial", "[atrac] a track does not fit its buffer; streaming is not implemented");
        if (!context->decoder.open(track.codec, track.channels, track.block_align, track.extradata)) {
            finish_traced(ctx, "sceAtracSetDataAndGetID", atrac_error::kBadCodecParam, "decoder refused the stream");
            return;
        }
        auto &table = contexts();
        const auto slot = std::find_if(table.begin(), table.end(), [](const auto &entry) { return !entry; });
        if (slot == table.end()) {
            finish_traced(ctx, "sceAtracSetDataAndGetID", atrac_error::kNoAtracId);
            return;
        }
        context->buffer = buffer;
        context->buffer_size = buffer_size;
        const auto id = static_cast<std::uint32_t>(slot - table.begin());
        std::ostringstream details;
        details << "file=" << track.file_size << " align=" << track.block_align << " channels=" << track.channels
                << " end=" << track.end_sample << " loop=" << track.loop_start << ".." << track.loop_end
                << " skip=" << track.skip;
        *slot = std::move(context);
        finish_traced(ctx, "sceAtracSetDataAndGetID", id, details.str());
    });

    hle.add("sceAtrac3plus", "sceAtracReleaseAtracID", [](Runtime &, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        if (find_context(id) == nullptr) {
            finish_traced(ctx, "sceAtracReleaseAtracID", context_error(id));
            return;
        }
        release_context(id);
        finish_traced(ctx, "sceAtracReleaseAtracID", 0u);
    });

    // sceAtracDecodeData(id, outSamples, outCount, outEnd, outRemainFrame):
    // one frame, or what is left of it before the loop end or the track end.
    hle.add("sceAtrac3plus", "sceAtracDecodeData", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        AtracContext *context = find_context(id);
        if (context == nullptr) {
            finish_traced(ctx, "sceAtracDecodeData", context_error(id));
            return;
        }
        auto &memory = rt.memory();
        const std::uint32_t output = arg(ctx, 1);
        const std::uint32_t count_address = arg(ctx, 2);
        const std::uint32_t end_address = arg(ctx, 3);
        const std::uint32_t remain_address = arg(ctx, 4);
        const TrackInfo &track = context->track;
        write_s32(memory, remain_address, kRemainAllDataOnMemory);
        if (context->position > track.end_sample) {
            write_s32(memory, count_address, 0);
            write_s32(memory, end_address, 1);
            finish_traced(ctx, "sceAtracDecodeData", atrac_error::kAllDataDecoded);
            return;
        }

        // Past the loop end already (the loop count was set late), play out.
        const bool loops = looping(*context) && context->position <= track.loop_end;
        const std::int32_t stop = loops ? track.loop_end : track.end_sample;
        const auto frame_samples = static_cast<std::int64_t>(audio::atrac_frame_samples(track.codec));
        const std::int64_t decoded_index = static_cast<std::int64_t>(context->position) + track.skip;
        const std::int64_t frame = decoded_index / frame_samples;
        const std::int64_t first = decoded_index % frame_samples;
        const auto count = static_cast<std::int32_t>(
            std::min<std::int64_t>(frame_samples - first, static_cast<std::int64_t>(stop) - context->position + 1));
        const bool decoded = load_frame(memory, *context, frame);
        if (output != 0u) {
            if (std::uint8_t *destination = memory.raw_pointer(output, static_cast<std::size_t>(count) * 4u)) {
                const std::int16_t *source = context->cached.data() + first * 2;
                for (std::int32_t i = 0; i < count * 2; ++i) {
                    const auto value = static_cast<std::uint16_t>(source[i]);
                    destination[i * 2] = static_cast<std::uint8_t>(value);
                    destination[i * 2 + 1] = static_cast<std::uint8_t>(value >> 8u);
                }
            }
        }
        const std::int32_t played = context->position;
        context->position += count;
        if (context->position > stop && loops) {
            context->position = track.loop_start;
            if (context->loop_num > 0) --context->loop_num;
        }
        const bool ended = context->position > track.end_sample;
        write_s32(memory, count_address, count);
        write_s32(memory, end_address, ended ? 1 : 0);
        if (trace_atrac()) {
            std::ostringstream details;
            details << "at=" << played << " count=" << count << " next=" << context->position
                    << (decoded ? "" : " (silent)") << (ended ? " end" : "");
            finish_traced(ctx, "sceAtracDecodeData", 0u, details.str());
            return;
        }
        kernel().finish(ctx, 0u);
    });

    // sceAtracGetRemainFrame(id, outRemainFrame)
    hle.add("sceAtrac3plus", "sceAtracGetRemainFrame", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        if (find_context(id) == nullptr) {
            finish_traced(ctx, "sceAtracGetRemainFrame", context_error(id));
            return;
        }
        write_s32(rt.memory(), arg(ctx, 1), kRemainAllDataOnMemory);
        finish_traced(ctx, "sceAtracGetRemainFrame", 0u);
    });

    // sceAtracGetSoundSample(id, outEndSample, outLoopStart, outLoopEnd)
    hle.add("sceAtrac3plus", "sceAtracGetSoundSample", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        const AtracContext *context = find_context(id);
        if (context == nullptr) {
            finish_traced(ctx, "sceAtracGetSoundSample", context_error(id));
            return;
        }
        write_s32(rt.memory(), arg(ctx, 1), context->track.end_sample);
        write_s32(rt.memory(), arg(ctx, 2), context->track.loop_start);
        write_s32(rt.memory(), arg(ctx, 3), context->track.loop_end);
        finish_traced(ctx, "sceAtracGetSoundSample", 0u);
    });

    // sceAtracGetBitrate(id, outKbps)
    hle.add("sceAtrac3plus", "sceAtracGetBitrate", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        const AtracContext *context = find_context(id);
        if (context == nullptr) {
            finish_traced(ctx, "sceAtracGetBitrate", context_error(id));
            return;
        }
        // Bytes per frame times frames per second, in the library's rounding.
        std::uint32_t bitrate = context->track.block_align * 352'800u / 1000u;
        if (context->track.codec == audio::AtracCodec::Atrac3) bitrate = (bitrate + 511u) >> 10u;
        else bitrate = ((bitrate >> 11u) + 8u) & 0xFFFFFFF0u;
        rt.memory().store32(arg(ctx, 1), bitrate);
        finish_traced(ctx, "sceAtracGetBitrate", 0u, "kbps=" + std::to_string(bitrate));
    });

    // sceAtracSetLoopNum(id, loops): -1 loops forever.
    hle.add("sceAtrac3plus", "sceAtracSetLoopNum", [](Runtime &, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        AtracContext *context = find_context(id);
        if (context == nullptr) {
            finish_traced(ctx, "sceAtracSetLoopNum", context_error(id));
            return;
        }
        if (context->track.loop_start < 0) {
            finish_traced(ctx, "sceAtracSetLoopNum", atrac_error::kNoLoopInformation);
            return;
        }
        context->loop_num = static_cast<std::int32_t>(arg(ctx, 1));
        finish_traced(ctx, "sceAtracSetLoopNum", 0u);
    });

    // sceAtracGetLoopStatus(id, outLoopNum, outLoopStatus)
    hle.add("sceAtrac3plus", "sceAtracGetLoopStatus", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        const AtracContext *context = find_context(id);
        if (context == nullptr) {
            finish_traced(ctx, "sceAtracGetLoopStatus", context_error(id));
            return;
        }
        write_s32(rt.memory(), arg(ctx, 1), context->loop_num);
        write_s32(rt.memory(), arg(ctx, 2), looping(*context) ? 1 : 0);
        finish_traced(ctx, "sceAtracGetLoopStatus", 0u);
    });

    // sceAtracGetNextDecodePosition(id, outSample)
    hle.add("sceAtrac3plus", "sceAtracGetNextDecodePosition", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        const AtracContext *context = find_context(id);
        if (context == nullptr) {
            finish_traced(ctx, "sceAtracGetNextDecodePosition", context_error(id));
            return;
        }
        if (context->position > context->track.end_sample) {
            finish_traced(ctx, "sceAtracGetNextDecodePosition", atrac_error::kAllDataDecoded);
            return;
        }
        write_s32(rt.memory(), arg(ctx, 1), context->position);
        finish_traced(ctx, "sceAtracGetNextDecodePosition", 0u, "position=" + std::to_string(context->position));
    });

    // sceAtracGetStreamDataInfo(id, outWritePointer, outWritableBytes, outReadOffset).
    // The whole file is in memory, so there is never room to add data.
    hle.add("sceAtrac3plus", "sceAtracGetStreamDataInfo", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        const AtracContext *context = find_context(id);
        if (context == nullptr) {
            finish_traced(ctx, "sceAtracGetStreamDataInfo", context_error(id));
            return;
        }
        auto &memory = rt.memory();
        if (arg(ctx, 1) != 0u) memory.store32(arg(ctx, 1), context->buffer);
        if (arg(ctx, 2) != 0u) memory.store32(arg(ctx, 2), 0u);
        if (arg(ctx, 3) != 0u) memory.store32(arg(ctx, 3), context->track.file_size);
        finish_traced(ctx, "sceAtracGetStreamDataInfo", 0u);
    });

    // sceAtracGetBufferInfoForResetting(id, sample, outBufferInfo): where data
    // for a restart at `sample` would have to go. Nothing, with the whole file
    // in memory. The structure is two {writePointer, writableBytes,
    // minWriteBytes, readOffset} records, for the first and second buffer.
    hle.add("sceAtrac3plus", "sceAtracGetBufferInfoForResetting", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        const AtracContext *context = find_context(id);
        if (context == nullptr) {
            finish_traced(ctx, "sceAtracGetBufferInfoForResetting", context_error(id));
            return;
        }
        const auto sample = static_cast<std::int32_t>(arg(ctx, 1));
        const std::uint32_t info = arg(ctx, 2);
        if (info == 0u) {
            finish_traced(ctx, "sceAtracGetBufferInfoForResetting", atrac_error::kParamFail);
            return;
        }
        if (sample < 0 || sample > context->track.end_sample) {
            finish_traced(ctx, "sceAtracGetBufferInfoForResetting", atrac_error::kBadSample);
            return;
        }
        auto &memory = rt.memory();
        const std::array<std::uint32_t, 8> fields = {context->buffer, 0u, 0u, context->track.file_size,
                                                     context->buffer, 0u, 0u, 0u};
        for (std::size_t i = 0; i < fields.size(); ++i) memory.store32(info + static_cast<std::uint32_t>(i * 4u), fields[i]);
        finish_traced(ctx, "sceAtracGetBufferInfoForResetting", 0u);
    });

    // sceAtracResetPlayPosition(id, sample, bytesWrittenFirst, bytesWrittenSecond)
    hle.add("sceAtrac3plus", "sceAtracResetPlayPosition", [](Runtime &, AllegrexContext &ctx) {
        const std::uint32_t id = arg(ctx, 0);
        AtracContext *context = find_context(id);
        if (context == nullptr) {
            finish_traced(ctx, "sceAtracResetPlayPosition", context_error(id));
            return;
        }
        const auto sample = static_cast<std::int32_t>(arg(ctx, 1));
        if (sample < 0 || sample > context->track.end_sample) {
            finish_traced(ctx, "sceAtracResetPlayPosition", atrac_error::kBadSample);
            return;
        }
        context->position = sample;
        finish_traced(ctx, "sceAtracResetPlayPosition", 0u);
    });
}

} // namespace

void register_atrac(HleRegistrar &hle) {
    if (!audio::AtracDecoder::available()) {
        std::cout << "Audio: built without FFmpeg; streamed music is silent\n";
        return;
    }
    register_atrac_functions(hle);
}

} // namespace mhp3rd
