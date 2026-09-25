// sceMpeg and sceJpeg: the game's PSMF movie player (demo_sub.ovl).
//
// The player runs three threads: a feeder that reads the movie into a
// ring buffer through sceMpegRingbufferPut, which calls the game's own read
// callback, and a video and an audio thread that take access units out of
// that ring and decode them. The library's demultiplexer and decoders are
// replaced here: packs are demultiplexed on the host (movie/psmf_demuxer),
// pictures are decoded with FFmpeg's H.264 decoder into the YCbCr buffer the
// game provides, sceJpegCsc turns them into 32-bit pixels, and audio frames
// are decoded with FFmpeg's ATRAC3plus decoder. The game presents the
// pixels itself with sceDisplaySetFrameBuf.
//
// Without FFmpeg, movies are skipped: the stream calls report that no data
// is left, and the game treats the movie as finished.
#include "hle_common.hpp"

#include "audio/atrac_decoder.hpp"
#include "movie/avc_decoder.hpp"
#include "movie/psmf_demuxer.hpp"

#include "psprecomp/common.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <vector>

namespace mhp3rd {
namespace {

namespace mpeg_error {
inline constexpr std::uint32_t kNoData = 0x80618001u;
inline constexpr std::uint32_t kInvalidValue = 0x806101FEu;
inline constexpr std::uint32_t kNoMemory = 0x80610022u;
} // namespace mpeg_error

constexpr std::uint32_t kPsmfMagic = 0x464D5350u;  // "PSMF"
constexpr std::uint32_t kMpegMemorySize = 0x10000u;
constexpr std::uint32_t kRingbufferPacketOverhead = 104u;
constexpr std::uint32_t kAtracEsSize = 2112u;
constexpr std::uint32_t kAtracOutputSize = 8192u;
// The handle sceMpegCreate stores in the SceMpeg variable points this far
// into the work area the game gives it.
constexpr std::uint32_t kHandleOffset = 0x30u;

// SceMpegRingbuffer fields.
namespace ring {
inline constexpr std::uint32_t kPackets = 0x00u;
inline constexpr std::uint32_t kRead = 0x04u;       // next packet to demultiplex
inline constexpr std::uint32_t kWritten = 0x08u;    // next packet to fill
inline constexpr std::uint32_t kFilled = 0x0Cu;     // packets holding data
inline constexpr std::uint32_t kPacketSize = 0x10u;
inline constexpr std::uint32_t kData = 0x14u;
inline constexpr std::uint32_t kCallback = 0x18u;
inline constexpr std::uint32_t kCallbackArgument = 0x1Cu;
inline constexpr std::uint32_t kDataEnd = 0x20u;
inline constexpr std::uint32_t kSemaphore = 0x24u;
inline constexpr std::uint32_t kMpeg = 0x28u;
} // namespace ring

// SceMpegAu: 64-bit PTS and DTS as high and low words, the ES buffer and
// the unit's size.
namespace au {
inline constexpr std::uint32_t kPtsHigh = 0x00u;
inline constexpr std::uint32_t kPtsLow = 0x04u;
inline constexpr std::uint32_t kDtsHigh = 0x08u;
inline constexpr std::uint32_t kDtsLow = 0x0Cu;
inline constexpr std::uint32_t kEsBuffer = 0x10u;
inline constexpr std::uint32_t kSize = 0x14u;
} // namespace au

enum class StreamType : std::uint32_t { Avc = 0u, Atrac = 1u, Pcm = 2u };

bool trace_mpeg() {
    static const bool enabled = std::getenv("MHP3RD_TRACE_MPEG") != nullptr;
    return enabled;
}

struct MpegState {
    std::uint32_t mpeg{};        // guest address of the SceMpeg variable
    std::uint32_t work{};        // the work area sceMpegCreate was given
    std::uint32_t ringbuffer{};
    std::uint32_t stream_packets{};  // packets in the movie, from its header
    std::uint32_t packets_demuxed{};
    std::map<std::uint32_t, StreamType> streams;  // handle -> type
    std::uint32_t next_stream{};
    movie::PsmfDemuxer demuxer;
    movie::AvcDecoder video;
    audio::AtracDecoder audio;
    std::optional<movie::AccessUnit> video_unit;  // taken by GetAvcAu, not yet decoded
    std::optional<movie::AccessUnit> audio_unit;
    movie::Picture picture;
    std::uint64_t pictures{};
};

struct MpegModule {
    std::map<std::uint32_t, std::unique_ptr<MpegState>> instances;  // by SceMpeg address
    std::uint32_t pending_stream_packets{};  // from the last header the game queried
    // YCbCr buffer -> the picture written into it, for sceJpegCsc.
    std::map<std::uint32_t, movie::Picture> ycbcr;
};

MpegModule &module() {
    static MpegModule state;
    return state;
}

MpegState *find_mpeg(std::uint32_t address) {
    const auto found = module().instances.find(address);
    return found != module().instances.end() ? found->second.get() : nullptr;
}

void finish_traced(AllegrexContext &ctx, const char *name, std::uint32_t result, const std::string &details = {},
                   unsigned arguments = 4u) {
    if (trace_mpeg()) {
        std::ostringstream line;
        line << "[mpeg] " << name << "(";
        for (unsigned i = 0; i < arguments; ++i) line << (i != 0u ? ", " : "") << psprecomp::hex32(arg(ctx, i));
        line << ") -> " << psprecomp::hex32(result);
        if (!details.empty()) line << " " << details;
        std::cerr << line.str() << "\n";
    }
    kernel().finish(ctx, result);
}

std::uint32_t load_be32(const psprecomp::GuestMemory &memory, std::uint32_t address) {
    return (static_cast<std::uint32_t>(memory.load8(address)) << 24u) |
           (static_cast<std::uint32_t>(memory.load8(address + 1u)) << 16u) |
           (static_cast<std::uint32_t>(memory.load8(address + 2u)) << 8u) | memory.load8(address + 3u);
}

void store_timestamp(psprecomp::GuestMemory &memory, std::uint32_t high, std::uint32_t low, std::int64_t value) {
    const auto bits = static_cast<std::uint64_t>(value);
    memory.store32(high, static_cast<std::uint32_t>(bits >> 32u));
    memory.store32(low, static_cast<std::uint32_t>(bits));
}

void write_unit(psprecomp::GuestMemory &memory, std::uint32_t address, const movie::AccessUnit &unit) {
    if (address == 0u) return;
    store_timestamp(memory, address + au::kPtsHigh, address + au::kPtsLow, unit.pts);
    store_timestamp(memory, address + au::kDtsHigh, address + au::kDtsLow, unit.dts);
    memory.store32(address + au::kSize, static_cast<std::uint32_t>(unit.data.size()));
}

// Demultiplexes packs out of the ring until `ready` holds or the ring is
// empty. Consumed packets are handed back to the feeder at once: their
// contents now live in the demultiplexer's queues.
template <typename Ready>
bool demux_until(psprecomp::GuestMemory &memory, MpegState &state, Ready ready) {
    const std::uint32_t buffer = state.ringbuffer;
    if (buffer == 0u) return ready();
    const std::uint32_t packets = memory.load32(buffer + ring::kPackets);
    const std::uint32_t data = memory.load32(buffer + ring::kData);
    std::vector<std::uint8_t> pack(movie::kPackSize);
    while (!ready()) {
        std::uint32_t filled = memory.load32(buffer + ring::kFilled);
        if (filled == 0u || packets == 0u) {
            if (state.stream_packets != 0u && state.packets_demuxed >= state.stream_packets) {
                state.demuxer.end_of_stream();
                return ready();
            }
            return false;
        }
        const std::uint32_t read = memory.load32(buffer + ring::kRead) % packets;
        memory.copy_out(data + read * static_cast<std::uint32_t>(movie::kPackSize), pack);
        (void)state.demuxer.push_pack(pack);
        ++state.packets_demuxed;
        memory.store32(buffer + ring::kRead, (read + 1u) % packets);
        memory.store32(buffer + ring::kFilled, filled - 1u);
        if (state.stream_packets != 0u && state.packets_demuxed >= state.stream_packets) state.demuxer.end_of_stream();
    }
    return true;
}

// ---------------------------------------------------------------------------
// Ring buffer

// The feeder asks for `requested` packets; the game's callback reads at most
// the contiguous room up to the end of the ring, so a request that wraps
// calls it a second time for the start of the ring.
void ringbuffer_put(AllegrexContext &ctx, std::uint32_t buffer, std::uint32_t requested, std::uint32_t total) {
    auto &memory = kernel().runtime().memory();
    const std::uint32_t packets = memory.load32(buffer + ring::kPackets);
    const std::uint32_t filled = memory.load32(buffer + ring::kFilled);
    const std::uint32_t room = packets > filled ? packets - filled : 0u;
    requested = std::min(requested, room);
    if (requested == 0u || packets == 0u) {
        finish_traced(ctx, "sceMpegRingbufferPut", total, "packets=" + std::to_string(total), 0u);
        return;
    }
    const std::uint32_t written = memory.load32(buffer + ring::kWritten) % packets;
    const std::uint32_t chunk = std::min(requested, packets - written);
    const std::uint32_t data = memory.load32(buffer + ring::kData);
    const std::uint32_t callback = memory.load32(buffer + ring::kCallback);
    const std::uint32_t argument = memory.load32(buffer + ring::kCallbackArgument);
    const std::uint32_t target = data + written * static_cast<std::uint32_t>(movie::kPackSize);
    if (trace_mpeg())
        std::cerr << "[mpeg] ring callback " << psprecomp::hex32(callback) << "(" << psprecomp::hex32(target) << ", "
                  << chunk << ", " << psprecomp::hex32(argument) << ")\n";
    kernel().call_guest(ctx, callback, {target, chunk, argument, 0u},
                        [buffer, requested, chunk, total](AllegrexContext &ctx, std::uint32_t result) {
                            auto &memory = kernel().runtime().memory();
                            const auto read = static_cast<std::int32_t>(result);
                            if (read < 0) {
                                finish_traced(ctx, "sceMpegRingbufferPut", result, "callback failed", 0u);
                                return;
                            }
                            const auto count = std::min(static_cast<std::uint32_t>(read), chunk);
                            const std::uint32_t packets = memory.load32(buffer + ring::kPackets);
                            const std::uint32_t written = memory.load32(buffer + ring::kWritten);
                            memory.store32(buffer + ring::kWritten, (written + count) % packets);
                            memory.store32(buffer + ring::kFilled, memory.load32(buffer + ring::kFilled) + count);
                            // A short read is the end of the file: do not ask again.
                            if (count == chunk && requested > chunk) {
                                ringbuffer_put(ctx, buffer, requested - chunk, total + count);
                                return;
                            }
                            // The callback has clobbered the argument registers,
                            // so the trace names no arguments.
                            finish_traced(ctx, "sceMpegRingbufferPut", total + count,
                                          "packets=" + std::to_string(total + count), 0u);
                        });
}

void register_ringbuffer(HleRegistrar &hle) {
    hle.add("sceMpeg", "sceMpegRingbufferQueryMemSize", [](Runtime &, AllegrexContext &ctx) {
        finish_traced(ctx, "sceMpegRingbufferQueryMemSize",
                      arg(ctx, 0) * (static_cast<std::uint32_t>(movie::kPackSize) + kRingbufferPacketOverhead), {}, 1u);
    });
    // sceMpegRingbufferConstruct(ring, packets, data, size, callback, argument)
    hle.add("sceMpeg", "sceMpegRingbufferConstruct", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t buffer = arg(ctx, 0);
        const std::uint32_t packets = arg(ctx, 1);
        const std::uint32_t data = arg(ctx, 2);
        memory.store32(buffer + ring::kPackets, packets);
        memory.store32(buffer + ring::kRead, 0u);
        memory.store32(buffer + ring::kWritten, 0u);
        memory.store32(buffer + ring::kFilled, 0u);
        memory.store32(buffer + ring::kPacketSize, static_cast<std::uint32_t>(movie::kPackSize));
        memory.store32(buffer + ring::kData, data);
        memory.store32(buffer + ring::kCallback, arg(ctx, 4));
        memory.store32(buffer + ring::kCallbackArgument, arg(ctx, 5));
        memory.store32(buffer + ring::kDataEnd, data + packets * static_cast<std::uint32_t>(movie::kPackSize));
        memory.store32(buffer + ring::kSemaphore, 0u);
        memory.store32(buffer + ring::kMpeg, 0u);
        finish_traced(ctx, "sceMpegRingbufferConstruct", 0u, {}, 6u);
    });
    hle.add("sceMpeg", "sceMpegRingbufferDestruct", [](Runtime &, AllegrexContext &ctx) {
        finish_traced(ctx, "sceMpegRingbufferDestruct", 0u, {}, 1u);
    });
    // Free packets.
    hle.add("sceMpeg", "sceMpegRingbufferAvailableSize", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t buffer = arg(ctx, 0);
        const std::uint32_t packets = memory.load32(buffer + ring::kPackets);
        const std::uint32_t filled = memory.load32(buffer + ring::kFilled);
        finish_traced(ctx, "sceMpegRingbufferAvailableSize", packets > filled ? packets - filled : 0u, {}, 1u);
    });
    // sceMpegRingbufferPut(ring, packets, available)
    hle.add("sceMpeg", "sceMpegRingbufferPut", [](Runtime &, AllegrexContext &ctx) {
        ringbuffer_put(ctx, arg(ctx, 0), arg(ctx, 1), 0u);
    });
}

// ---------------------------------------------------------------------------
// Library, streams and access units

void register_library(HleRegistrar &hle) {
    for (const char *name : {"sceMpegInit", "sceMpegFinish"})
        hle.add("sceMpeg", name, [name](Runtime &, AllegrexContext &ctx) { finish_traced(ctx, name, 0u, {}, 0u); });
    hle.add("sceMpeg", "sceMpegQueryMemSize", [](Runtime &, AllegrexContext &ctx) {
        finish_traced(ctx, "sceMpegQueryMemSize", kMpegMemorySize, {}, 1u);
    });
    // sceMpegCreate(mpeg, work, size, ring, frameWidth, mode, ddrTop)
    hle.add("sceMpeg", "sceMpegCreate", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t mpeg = arg(ctx, 0);
        const std::uint32_t work = arg(ctx, 1);
        if (arg(ctx, 2) < kMpegMemorySize) {
            finish_traced(ctx, "sceMpegCreate", mpeg_error::kNoMemory, {}, 7u);
            return;
        }
        auto state = std::make_unique<MpegState>();
        state->mpeg = mpeg;
        state->work = work;
        state->ringbuffer = arg(ctx, 3);
        state->stream_packets = module().pending_stream_packets;
        memory.store32(mpeg, work + kHandleOffset);
        if (state->ringbuffer != 0u) memory.store32(state->ringbuffer + ring::kMpeg, mpeg);
        if (!state->video.open())
            log_once("mpeg-avc", "[mpeg] the H.264 decoder could not be opened; movies will be black");
        module().instances[mpeg] = std::move(state);
        finish_traced(ctx, "sceMpegCreate", 0u, {}, 7u);
    });
    hle.add("sceMpeg", "sceMpegDelete", [](Runtime &, AllegrexContext &ctx) {
        module().instances.erase(arg(ctx, 0));
        finish_traced(ctx, "sceMpegDelete", 0u, {}, 1u);
    });
    // sceMpegQueryStreamOffset(mpeg, header, outOffset)
    hle.add("sceMpeg", "sceMpegQueryStreamOffset", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t header = arg(ctx, 1);
        if (memory.load32(header) != kPsmfMagic) {
            memory.store32(arg(ctx, 2), 0u);
            finish_traced(ctx, "sceMpegQueryStreamOffset", mpeg_error::kInvalidValue, {}, 3u);
            return;
        }
        const std::uint32_t offset = load_be32(memory, header + 8u);
        module().pending_stream_packets = load_be32(memory, header + 12u) / static_cast<std::uint32_t>(movie::kPackSize);
        if (MpegState *state = find_mpeg(arg(ctx, 0))) state->stream_packets = module().pending_stream_packets;
        memory.store32(arg(ctx, 2), offset);
        finish_traced(ctx, "sceMpegQueryStreamOffset", 0u, "offset=" + std::to_string(offset), 3u);
    });
    // sceMpegQueryStreamSize(header, outSize)
    hle.add("sceMpeg", "sceMpegQueryStreamSize", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t size = load_be32(memory, arg(ctx, 0) + 12u);
        if ((size & (static_cast<std::uint32_t>(movie::kPackSize) - 1u)) != 0u) {
            finish_traced(ctx, "sceMpegQueryStreamSize", mpeg_error::kInvalidValue, {}, 2u);
            return;
        }
        memory.store32(arg(ctx, 1), size);
        finish_traced(ctx, "sceMpegQueryStreamSize", 0u, "size=" + std::to_string(size), 2u);
    });
    // sceMpegRegistStream(mpeg, type, number) -> stream handle
    hle.add("sceMpeg", "sceMpegRegistStream", [](Runtime &, AllegrexContext &ctx) {
        MpegState *state = find_mpeg(arg(ctx, 0));
        if (state == nullptr) {
            finish_traced(ctx, "sceMpegRegistStream", 0u, {}, 3u);
            return;
        }
        // Handles point into the work area, as the library's do.
        const std::uint32_t handle = state->work + 0x100u + 0x10u * state->next_stream++;
        state->streams[handle] = static_cast<StreamType>(arg(ctx, 1));
        finish_traced(ctx, "sceMpegRegistStream", handle, {}, 3u);
    });
    hle.add("sceMpeg", "sceMpegUnRegistStream", [](Runtime &, AllegrexContext &ctx) {
        if (MpegState *state = find_mpeg(arg(ctx, 0))) state->streams.erase(arg(ctx, 1));
        finish_traced(ctx, "sceMpegUnRegistStream", 0u, {}, 2u);
    });
    hle.add("sceMpeg", "sceMpegMallocAvcEsBuf", [](Runtime &, AllegrexContext &ctx) {
        finish_traced(ctx, "sceMpegMallocAvcEsBuf", 1u, {}, 1u);
    });
    hle.add("sceMpeg", "sceMpegFreeAvcEsBuf", [](Runtime &, AllegrexContext &ctx) {
        finish_traced(ctx, "sceMpegFreeAvcEsBuf", 0u, {}, 2u);
    });
    // sceMpegInitAu(mpeg, esBuffer, au)
    hle.add("sceMpeg", "sceMpegInitAu", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t unit = arg(ctx, 2);
        store_timestamp(memory, unit + au::kPtsHigh, unit + au::kPtsLow, -1);
        store_timestamp(memory, unit + au::kDtsHigh, unit + au::kDtsLow, -1);
        memory.store32(unit + au::kEsBuffer, arg(ctx, 1));
        memory.store32(unit + au::kSize, 0u);
        finish_traced(ctx, "sceMpegInitAu", 0u, {}, 3u);
    });
    // sceMpegQueryAtracEsSize(mpeg, outEsSize, outOutputSize)
    hle.add("sceMpeg", "sceMpegQueryAtracEsSize", [](Runtime &rt, AllegrexContext &ctx) {
        rt.memory().store32(arg(ctx, 1), kAtracEsSize);
        rt.memory().store32(arg(ctx, 2), kAtracOutputSize);
        finish_traced(ctx, "sceMpegQueryAtracEsSize", 0u, {}, 3u);
    });
    // Drops everything demultiplexed so far, for a restart or a stop.
    hle.add("sceMpeg", "sceMpegFlushAllStream", [](Runtime &, AllegrexContext &ctx) {
        if (MpegState *state = find_mpeg(arg(ctx, 0))) {
            state->demuxer.reset();
            state->video.reset();
            state->video_unit.reset();
            state->audio_unit.reset();
        }
        finish_traced(ctx, "sceMpegFlushAllStream", 0u, {}, 1u);
    });

    // sceMpegGetAvcAu(mpeg, stream, au, outAttribute)
    hle.add("sceMpeg", "sceMpegGetAvcAu", [](Runtime &rt, AllegrexContext &ctx) {
        MpegState *state = find_mpeg(arg(ctx, 0));
        if (state == nullptr) {
            finish_traced(ctx, "sceMpegGetAvcAu", mpeg_error::kInvalidValue);
            return;
        }
        auto &memory = rt.memory();
        if (!demux_until(memory, *state, [state] { return state->demuxer.video_ready(); })) {
            finish_traced(ctx, "sceMpegGetAvcAu", mpeg_error::kNoData);
            return;
        }
        state->video_unit = state->demuxer.pop_video();
        write_unit(memory, arg(ctx, 2), *state->video_unit);
        if (arg(ctx, 3) != 0u) memory.store32(arg(ctx, 3), 1u);
        finish_traced(ctx, "sceMpegGetAvcAu", 0u,
                      "pts=" + std::to_string(state->video_unit->pts) + " size=" +
                          std::to_string(state->video_unit->data.size()));
    });
    // sceMpegGetAtracAu(mpeg, stream, au, outAttribute)
    hle.add("sceMpeg", "sceMpegGetAtracAu", [](Runtime &rt, AllegrexContext &ctx) {
        MpegState *state = find_mpeg(arg(ctx, 0));
        if (state == nullptr) {
            finish_traced(ctx, "sceMpegGetAtracAu", mpeg_error::kInvalidValue);
            return;
        }
        auto &memory = rt.memory();
        if (!demux_until(memory, *state, [state] { return state->demuxer.audio_ready(); })) {
            finish_traced(ctx, "sceMpegGetAtracAu", mpeg_error::kNoData);
            return;
        }
        state->audio_unit = state->demuxer.pop_audio();
        write_unit(memory, arg(ctx, 2), *state->audio_unit);
        if (arg(ctx, 3) != 0u) memory.store32(arg(ctx, 3), 0u);
        finish_traced(ctx, "sceMpegGetAtracAu", 0u, "pts=" + std::to_string(state->audio_unit->pts));
    });
}

// ---------------------------------------------------------------------------
// Decoding

// The YCbCr and YUV420 buffers are read only by this library and sceJpegCsc,
// so both hold the planes as the decoder produces them: Y, then Cb, then Cr.
// The host keeps the picture for each buffer, so a conversion does not have
// to read it back.
void store_picture(psprecomp::GuestMemory &memory, std::uint32_t buffer, const movie::Picture &picture) {
    std::uint32_t at = buffer;
    for (const auto *plane : {&picture.y, &picture.cb, &picture.cr}) {
        memory.copy_in(at, *plane);
        at += static_cast<std::uint32_t>(plane->size());
    }
    module().ycbcr[buffer] = picture;
}

void register_decoding(HleRegistrar &hle) {
    // sceMpegAvcQueryYCbCrSize(mpeg, mode, width, height, outSize)
    hle.add("sceMpeg", "sceMpegAvcQueryYCbCrSize", [](Runtime &rt, AllegrexContext &ctx) {
        const std::uint32_t width = arg(ctx, 2);
        const std::uint32_t height = arg(ctx, 3);
        rt.memory().store32(arg(ctx, 4), (width / 2u) * (height / 2u) * 6u + 128u);
        finish_traced(ctx, "sceMpegAvcQueryYCbCrSize", 0u, {}, 5u);
    });
    // sceMpegAvcInitYCbCr(mpeg, mode, width, height, buffer)
    hle.add("sceMpeg", "sceMpegAvcInitYCbCr", [](Runtime &, AllegrexContext &ctx) {
        finish_traced(ctx, "sceMpegAvcInitYCbCr", 0u, {}, 5u);
    });
    // sceMpegAvcDecodeYCbCr(mpeg, au, bufferPointer, outInit): the picture
    // goes to the buffer `bufferPointer` points at.
    hle.add("sceMpeg", "sceMpegAvcDecodeYCbCr", [](Runtime &rt, AllegrexContext &ctx) {
        MpegState *state = find_mpeg(arg(ctx, 0));
        if (state == nullptr) {
            finish_traced(ctx, "sceMpegAvcDecodeYCbCr", mpeg_error::kInvalidValue);
            return;
        }
        auto &memory = rt.memory();
        bool produced = false;
        if (state->video_unit) {
            produced = state->video.decode(state->video_unit->data, state->picture);
            state->video_unit.reset();
        }
        if (produced) {
            ++state->pictures;
            store_picture(memory, memory.load32(arg(ctx, 2)), state->picture);
        }
        if (arg(ctx, 3) != 0u) memory.store32(arg(ctx, 3), produced ? 1u : 0u);
        finish_traced(ctx, "sceMpegAvcDecodeYCbCr", 0u, produced ? "picture " + std::to_string(state->pictures) : "none");
    });
    // sceMpegAvcDecodeStopYCbCr(mpeg, bufferPointer, outStatus): pictures
    // the decoder still holds at the end of the stream.
    hle.add("sceMpeg", "sceMpegAvcDecodeStopYCbCr", [](Runtime &rt, AllegrexContext &ctx) {
        MpegState *state = find_mpeg(arg(ctx, 0));
        bool produced = false;
        if (state != nullptr && state->video.drain(state->picture)) {
            produced = true;
            store_picture(rt.memory(), rt.memory().load32(arg(ctx, 1)), state->picture);
        }
        if (arg(ctx, 2) != 0u) rt.memory().store32(arg(ctx, 2), produced ? 1u : 0u);
        finish_traced(ctx, "sceMpegAvcDecodeStopYCbCr", 0u, {}, 3u);
    });
    // sceMpegAvcDecodeDetail(mpeg, outDetail)
    hle.add("sceMpeg", "sceMpegAvcDecodeDetail", [](Runtime &rt, AllegrexContext &ctx) {
        MpegState *state = find_mpeg(arg(ctx, 0));
        const std::uint32_t detail = arg(ctx, 1);
        if (state != nullptr && detail != 0u) {
            auto &memory = rt.memory();
            memory.store32(detail + 0x00u, 0u);                                          // decode result
            memory.store32(detail + 0x04u, static_cast<std::uint32_t>(state->pictures));  // pictures decoded
            memory.store32(detail + 0x08u, state->picture.width);
            memory.store32(detail + 0x0Cu, state->picture.height);
        }
        finish_traced(ctx, "sceMpegAvcDecodeDetail", 0u, {}, 2u);
    });
    // sceMpegAvcConvertToYuv420(mpeg, output, ycbcr, unknown): the game
    // converts every picture before handing it to sceJpegCsc.
    hle.add("sceMpeg", "sceMpegAvcConvertToYuv420", [](Runtime &rt, AllegrexContext &ctx) {
        const auto found = module().ycbcr.find(arg(ctx, 2));
        if (found == module().ycbcr.end()) {
            finish_traced(ctx, "sceMpegAvcConvertToYuv420", 0u, "no picture");
            return;
        }
        const movie::Picture picture = found->second;
        store_picture(rt.memory(), arg(ctx, 1), picture);
        finish_traced(ctx, "sceMpegAvcConvertToYuv420", 0u);
    });
    // sceMpegAtracDecode(mpeg, au, buffer, init): one ATRAC3plus frame to
    // 2048 stereo samples.
    hle.add("sceMpeg", "sceMpegAtracDecode", [](Runtime &rt, AllegrexContext &ctx) {
        MpegState *state = find_mpeg(arg(ctx, 0));
        if (state == nullptr) {
            finish_traced(ctx, "sceMpegAtracDecode", mpeg_error::kInvalidValue);
            return;
        }
        auto &memory = rt.memory();
        std::vector<std::int16_t> samples(movie::kAtracFrameSamples * 2u, 0);
        bool decoded = false;
        if (state->audio_unit) {
            if (!state->audio.is_open() && state->demuxer.audio_frame_size() > movie::kAtracFrameHeader)
                (void)state->audio.open(audio::AtracCodec::Atrac3Plus, std::max(state->demuxer.audio_channels(), 1u),
                                        static_cast<unsigned>(state->demuxer.audio_frame_size() - movie::kAtracFrameHeader),
                                        {});
            decoded = state->audio.decode(state->audio_unit->data, samples.data()) != 0u;
            state->audio_unit.reset();
        }
        if (std::uint8_t *out = memory.raw_pointer(arg(ctx, 2), samples.size() * 2u)) {
            for (std::size_t i = 0; i < samples.size(); ++i) {
                const auto value = static_cast<std::uint16_t>(samples[i]);
                out[i * 2u] = static_cast<std::uint8_t>(value);
                out[i * 2u + 1u] = static_cast<std::uint8_t>(value >> 8u);
            }
        }
        finish_traced(ctx, "sceMpegAtracDecode", 0u, decoded ? "" : "silent");
    });
}

// sceJpegCsc(image, ycbcr, widthHeight, bufferWidth, colourInfo): converts
// a YCbCr picture to 32-bit pixels (R, G, B, A in memory order).
void register_jpeg(HleRegistrar &hle) {
    hle.add("sceJpeg", "sceJpegCsc", [](Runtime &rt, AllegrexContext &ctx) {
        auto &memory = rt.memory();
        const std::uint32_t image = arg(ctx, 0);
        const std::uint32_t source = arg(ctx, 1);
        const std::uint32_t width = arg(ctx, 2) >> 16u;
        const std::uint32_t height = arg(ctx, 2) & 0xFFFFu;
        const std::uint32_t stride = arg(ctx, 3);
        const auto found = module().ycbcr.find(source);
        if (found == module().ycbcr.end() || stride < width) {
            finish_traced(ctx, "sceJpegCsc", 0u, "no picture", 5u);
            return;
        }
        const movie::Picture &picture = found->second;
        const std::uint32_t rows = std::min(height, picture.height);
        const std::uint32_t columns = std::min(width, picture.width);
        const std::uint32_t chroma_width = (picture.width + 1u) / 2u;
        std::vector<std::uint8_t> line(static_cast<std::size_t>(columns) * 4u);
        for (std::uint32_t row = 0; row < rows; ++row) {
            for (std::uint32_t column = 0; column < columns; ++column) {
                // BT.601; limited-range luma is stretched to full range.
                int y = picture.y[static_cast<std::size_t>(row) * picture.width + column];
                const std::size_t chroma = static_cast<std::size_t>(row / 2u) * chroma_width + column / 2u;
                const int cb = picture.cb[chroma] - 128;
                const int cr = picture.cr[chroma] - 128;
                int scale = 1 << 16;
                if (!picture.full_range) {
                    y -= 16;
                    scale = 76309;  // 255/219
                }
                const int luma = y * scale;
                const int red = (luma + 91881 * cr) >> 16;
                const int green = (luma - 22554 * cb - 46802 * cr) >> 16;
                const int blue = (luma + 116130 * cb) >> 16;
                std::uint8_t *pixel = &line[static_cast<std::size_t>(column) * 4u];
                pixel[0] = static_cast<std::uint8_t>(std::clamp(red, 0, 255));
                pixel[1] = static_cast<std::uint8_t>(std::clamp(green, 0, 255));
                pixel[2] = static_cast<std::uint8_t>(std::clamp(blue, 0, 255));
                pixel[3] = 0xFFu;
            }
            memory.copy_in(image + row * stride * 4u, line);
        }
        finish_traced(ctx, "sceJpegCsc", 0u, {}, 5u);
    });
}

// Movie playback without a decoder: reporting "no data" from the stream
// calls makes the guest treat the movie as finished instead of waiting for
// frames that never arrive, so intros and cut-scenes are skipped.
void register_movie_skip(HleRegistrar &hle) {
    const auto succeed = [](Runtime &, AllegrexContext &ctx) { kernel().finish(ctx, 0u); };
    const auto no_data = [](Runtime &, AllegrexContext &ctx) {
        log_once("mpeg-skip", "[mpeg] built without FFmpeg; reporting the stream as finished");
        kernel().finish(ctx, mpeg_error::kNoData);
    };
    for (const char *name : {"sceMpegInit", "sceMpegFinish", "sceMpegDelete", "sceMpegRegistStream",
                             "sceMpegUnRegistStream", "sceMpegFlushAllStream", "sceMpegInitAu",
                             "sceMpegAvcInitYCbCr", "sceMpegRingbufferDestruct", "sceMpegFreeAvcEsBuf"})
        hle.try_add("sceMpeg", name, succeed);
    for (const char *name : {"sceMpegGetAvcAu", "sceMpegGetAtracAu", "sceMpegAvcDecode", "sceMpegAtracDecode",
                             "sceMpegAvcDecodeYCbCr", "sceMpegAvcDecodeStopYCbCr", "sceMpegAvcDecodeDetail",
                             "sceMpegAvcConvertToYuv420"})
        hle.try_add("sceMpeg", name, no_data);
}

} // namespace

bool mpeg_active() { return !module().instances.empty(); }

void register_mpeg(HleRegistrar &hle) {
    if (!movie::AvcDecoder::available() || !audio::AtracDecoder::available()) {
        register_movie_skip(hle);
        return;
    }
    register_ringbuffer(hle);
    register_library(hle);
    register_decoding(hle);
    register_jpeg(hle);
}

} // namespace mhp3rd
