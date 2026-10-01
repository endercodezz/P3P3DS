#pragma once
// sceMpeg: PSMF (MPEG-2 program stream) ringbuffer, demultiplexing into AVC
// and ATRAC3plus access units, and the decode calls.
//
// Contract sources: references/pspautotests/tests/video/mpeg/**/*.expected
// (hardware), psp/pspsdk/src/mpeg/pspmpeg.h (signatures). uOFW has no MPEG
// sources. tests/test_mpeg.cpp replays basic.c against basic.expected.
//
// Decoding is behind MpegDecoder: no H.264 / ATRAC3plus decoder exists in
// the workspace, so the default decoder outputs black frames and silence
// while every container-level value (AU sizes, timestamps, ringbuffer
// accounting) follows the stream.
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace psprecomp { class Runtime; class GuestMemory; }
namespace p3p3ds { class KernelState; }

namespace p3p3ds::hle {

namespace mpeg {
constexpr std::uint32_t kPacketSize = 2048u;
constexpr std::uint32_t kRingbufferBytesPerPacket = 0x868u; // ringbuffer/memsize.expected
constexpr std::uint32_t kMpegMemSize = 0x10000u;            // basic.expected
constexpr std::uint32_t kAtracEsSize = 0x840u, kAtracOutSize = 0x2000u;
constexpr std::uint32_t kStreamBaseOffset = 0x12C0u, kStreamStride = 0x440u; // basic.expected (two streams)
constexpr std::uint32_t kErrorNoData = 0x80618001u;          // basic.expected
constexpr std::uint32_t kErrorInvalidHandle = 0x80618009u;   // ringbuffer/avail.expected (deleted mpeg)
constexpr std::uint32_t kErrorRingbufferSize = 0x80610022u;  // ringbuffer/construct.expected
constexpr std::uint32_t kErrorAtracNoAu = 0x807F00FDu;       // basic.expected
constexpr std::uint32_t kErrorInvalidValue = 0x806101FEu;    // [INFERRED] bad PSMF magic
// Guest SceMpegRingbuffer layout (pspautotests shared.h SceMpegRingbuffer2).
enum RingbufferField : std::uint32_t {
    RbTotal = 0, RbRead = 4, RbWritten = 8, RbFilled = 12, RbPacketSize = 16, RbData = 20,
    RbCallback = 24, RbCallbackArg = 28, RbDataEnd = 32, RbUnknown = 36, RbMpeg = 40, RbGp = 44,
};
// Guest SceMpegAu layout (shared.h SceMpegAu2).
enum AuField : std::uint32_t { AuPtsHigh = 0, AuPtsLow = 4, AuDtsHigh = 8, AuDtsLow = 12, AuEsBuffer = 16, AuSize = 20 };
} // namespace mpeg

struct MpegAccessUnit {
    std::int64_t pts{-1}, dts{-1};
    std::vector<std::uint8_t> data;
};

// MPEG-2 program stream demultiplexer for PSMF packs (one 2048-byte pack per
// ringbuffer packet). AVC access units are delimited by 4-byte H.264 access
// unit delimiters (00 00 00 01 09); ATRAC3plus AUs are one frame: 8-byte
// header (0F D0 ..) + ((b2&3)<<8|b3)*8+8 bytes. An AU carries its PES PTS/DTS
// only when a PES payload starts exactly at the AU (all 180 AUs of
// pspautotests test.pmf match basic.expected under this rule).
class PsDemuxer {
public:
    void configure(std::uint32_t video_channel, std::uint32_t audio_channel) { video_id_ = 0xE0u + video_channel; audio_id_ = audio_channel; }
    void push_packet(std::span<const std::uint8_t> packet);
    std::optional<MpegAccessUnit> next_video_au(bool end_of_stream);
    std::optional<MpegAccessUnit> next_audio_au(bool end_of_stream);
    // Drops leading packets whose bytes of every tracked stream are consumed
    // (basic.expected: all 180 sceMpegRingbufferAvailableSize values).
    std::uint32_t release_packets(bool track_video, bool track_audio);
    [[nodiscard]] std::uint64_t packets_received() const noexcept { return packets_received_; }
    void reset();

private:
    struct Es {
        std::vector<std::uint8_t> bytes;
        std::uint64_t base{}, consumed{};
        std::map<std::uint64_t, std::pair<std::int64_t, std::int64_t>> timestamps; // ES offset of PES payload -> pts,dts
        [[nodiscard]] std::uint64_t end() const noexcept { return base + bytes.size(); }
        void append(std::span<const std::uint8_t> data, std::int64_t pts, std::int64_t dts);
        MpegAccessUnit take(std::uint64_t size);
    };
    Es video_, audio_;
    std::deque<std::pair<std::uint64_t, std::uint64_t>> packet_ends_; // cumulative video/audio ES end per packet
    std::uint64_t packets_received_{};
    std::uint32_t video_id_{0xE0u}, audio_id_{0u};
};

class MpegDecoder {
public:
    virtual ~MpegDecoder() = default;
    // Decodes one AVC AU; returns true when a picture was written to `dest`
    // (`stride` pixels per row, 272 rows, PSP pixel format `format`).
    virtual bool decode_video(const MpegAccessUnit &au, psprecomp::GuestMemory &memory, std::uint32_t dest,
                              std::uint32_t stride, std::uint32_t format) = 0;
    // Decodes one ATRAC3plus AU to 2048 stereo S16 samples at `dest`.
    virtual void decode_audio(const MpegAccessUnit &au, psprecomp::GuestMemory &memory, std::uint32_t dest) = 0;
};

// Placeholder decoder: black pictures, silent audio. [UNVERIFIED] substitute
// for real H.264 / ATRAC3plus decoding (see docs/NEXT_STEPS.md).
class BlankMpegDecoder final : public MpegDecoder {
public:
    bool decode_video(const MpegAccessUnit &, psprecomp::GuestMemory &memory, std::uint32_t dest,
                      std::uint32_t stride, std::uint32_t format) override;
    void decode_audio(const MpegAccessUnit &, psprecomp::GuestMemory &memory, std::uint32_t dest) override;
};

struct MpegInstance {
    std::uint32_t data{}, ringbuffer{};
    std::uint64_t stream_size{};            // from sceMpegQueryStreamOffset's PSMF header
    std::map<std::uint32_t, std::uint32_t> streams; // guest stream handle -> type (0 AVC, 1 ATRAC)
    std::uint32_t es_buffers{};             // allocated AVC ES buffers (bitmask)
    PsDemuxer demux;
    MpegAccessUnit last_video, last_audio;  // most recent AUs handed to the guest
    bool video_pipeline_primed{};           // first decode returns no picture (basic.expected)
    std::uint64_t video_decoded{}, audio_decoded{};
};

struct MpegState {
    bool initialized{};
    std::map<std::uint32_t, MpegInstance> instances; // keyed by handle (the mpeg data address)
    std::unique_ptr<MpegDecoder> decoder = std::make_unique<BlankMpegDecoder>();
    MpegInstance *find(std::uint32_t handle) {
        auto it = instances.find(handle);
        return it == instances.end() ? nullptr : &it->second;
    }
};

void register_mpeg_module(psprecomp::Runtime &runtime, KernelState &kernel);

} // namespace p3p3ds::hle
