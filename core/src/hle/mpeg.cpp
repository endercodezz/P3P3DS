// sceMpeg (see mpeg.hpp for sources and the verification status).
#include "p3p3ds/hle/mpeg.hpp"

#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/allegrex_context.hpp"
#include "psprecomp/guest_memory.hpp"
#include "psprecomp/runtime.hpp"

#include <algorithm>
#include <cstring>
#include <memory>

namespace p3p3ds::hle {

using namespace mpeg;

// ---- Demultiplexer ------------------------------------------------------------

void PsDemuxer::Es::append(std::span<const std::uint8_t> data, std::int64_t pts, std::int64_t dts) {
    if (pts >= 0) timestamps[end()] = {pts, dts >= 0 ? dts : pts};
    bytes.insert(bytes.end(), data.begin(), data.end());
}

MpegAccessUnit PsDemuxer::Es::take(std::uint64_t size) {
    MpegAccessUnit au;
    const auto at = static_cast<std::size_t>(consumed - base);
    au.data.assign(bytes.begin() + static_cast<std::ptrdiff_t>(at), bytes.begin() + static_cast<std::ptrdiff_t>(at + size));
    if (auto it = timestamps.find(consumed); it != timestamps.end()) { au.pts = it->second.first; au.dts = it->second.second; }
    consumed += size;
    timestamps.erase(timestamps.begin(), timestamps.lower_bound(consumed));
    if (consumed - base > (1u << 20)) { // keep the buffer bounded
        bytes.erase(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(consumed - base));
        base = consumed;
    }
    return au;
}

void PsDemuxer::push_packet(std::span<const std::uint8_t> p) {
    auto be16 = [&](std::size_t i) { return static_cast<std::uint32_t>((p[i] << 8) | p[i + 1]); };
    auto timestamp = [&](std::size_t i) {
        return (static_cast<std::int64_t>((p[i] >> 1) & 7) << 30) | (static_cast<std::int64_t>(p[i + 1]) << 22) |
               (static_cast<std::int64_t>(p[i + 2] >> 1) << 15) | (static_cast<std::int64_t>(p[i + 3]) << 7) | (p[i + 4] >> 1);
    };
    std::size_t pos = 0;
    while (pos + 6 <= p.size()) {
        if (p[pos] != 0 || p[pos + 1] != 0 || p[pos + 2] != 1) break; // end of pack data
        const auto id = p[pos + 3];
        if (id == 0xBAu) { // MPEG-2 pack header
            if (pos + 14 > p.size()) break;
            pos += 14u + (p[pos + 13] & 7u);
            continue;
        }
        if (id == 0xB9u) break; // program end
        const auto length = be16(pos + 4);
        const std::size_t body = pos + 6, next = body + length;
        if (next > p.size()) break;
        const bool video = id == video_id_, audio = id == 0xBDu;
        if ((video || audio) && length >= 3u) {
            const std::uint32_t flags = p[body + 1] >> 6u, header = p[body + 2];
            std::int64_t pts = -1, dts = -1;
            if ((flags & 2u) && length >= 8u) pts = timestamp(body + 3);
            if (flags == 3u && length >= 13u) dts = timestamp(body + 8);
            auto payload = body + 3u + header;
            if (payload <= next) {
                if (video) video_.append(p.subspan(payload, next - payload), pts, dts);
                // Private stream 1: substream id + 3 bytes, then ATRAC3plus frames.
                else if (payload + 4u <= next && p[payload] == audio_id_) audio_.append(p.subspan(payload + 4u, next - payload - 4u), pts, dts);
            }
        }
        pos = next;
    }
    packet_ends_.emplace_back(video_.end(), audio_.end());
    ++packets_received_;
}

std::optional<MpegAccessUnit> PsDemuxer::next_video_au(bool end_of_stream) {
    static constexpr std::uint8_t kAud[] = {0, 0, 0, 1, 9};
    if (video_.end() <= video_.consumed) return std::nullopt;
    const auto from = video_.bytes.begin() + static_cast<std::ptrdiff_t>(video_.consumed - video_.base + 1u);
    const auto hit = std::search(from, video_.bytes.end(), std::begin(kAud), std::end(kAud));
    std::uint64_t size;
    if (hit != video_.bytes.end()) size = video_.base + static_cast<std::uint64_t>(hit - video_.bytes.begin()) - video_.consumed;
    else if (end_of_stream) size = video_.end() - video_.consumed;
    else return std::nullopt;
    return video_.take(size);
}

std::optional<MpegAccessUnit> PsDemuxer::next_audio_au(bool end_of_stream) {
    const auto at = static_cast<std::size_t>(audio_.consumed - audio_.base);
    const auto available = audio_.end() - audio_.consumed;
    if (available < 8u) return std::nullopt;
    const auto *h = audio_.bytes.data() + at;
    std::uint64_t size = available; // malformed header: hand out the rest at stream end
    if (h[0] == 0x0Fu && h[1] == 0xD0u) size = ((((h[2] & 3u) << 8) | h[3]) * 8u + 8u) + 8u;
    else if (!end_of_stream) return std::nullopt;
    if (size > available) return std::nullopt;
    return audio_.take(size);
}

std::uint32_t PsDemuxer::release_packets(bool track_video, bool track_audio) {
    std::uint32_t released = 0;
    while (!packet_ends_.empty()) {
        const auto [v, a] = packet_ends_.front();
        if ((track_video && v > video_.consumed) || (track_audio && a > audio_.consumed)) break;
        packet_ends_.pop_front();
        ++released;
    }
    return released;
}

void PsDemuxer::reset() {
    const auto video = video_id_, audio = audio_id_;
    *this = PsDemuxer{};
    video_id_ = video;
    audio_id_ = audio;
}

// ---- Placeholder decoder -----------------------------------------------------------

bool BlankMpegDecoder::decode_video(const MpegAccessUnit &, psprecomp::GuestMemory &memory, std::uint32_t dest,
                                    std::uint32_t stride, std::uint32_t format) {
    const std::uint32_t bpp = format == 3u ? 4u : 2u;
    const std::uint32_t black = format == 3u ? 0xFF000000u : format == 0u ? 0u : 0x8000u; // opaque where alpha exists
    for (std::uint32_t y = 0; y < 272u; ++y)
        for (std::uint32_t x = 0; x < 480u; ++x) {
            const auto a = dest + (y * stride + x) * bpp;
            if (!memory.contains(a, bpp)) return false;
            if (bpp == 4u) memory.store32(a, black); else memory.store16(a, static_cast<std::uint16_t>(black));
        }
    return true;
}

void BlankMpegDecoder::decode_audio(const MpegAccessUnit &, psprecomp::GuestMemory &memory, std::uint32_t dest) {
    if (memory.contains(dest, kAtracOutSize)) memory.zero(dest, kAtracOutSize);
}

// ---- HLE ------------------------------------------------------------------------------

namespace {

std::int32_t s(std::uint32_t v) { return static_cast<std::int32_t>(v); }

void write_au(psprecomp::GuestMemory &m, std::uint32_t au, const MpegAccessUnit &unit) {
    const auto pts = static_cast<std::uint64_t>(unit.pts), dts = static_cast<std::uint64_t>(unit.dts);
    m.store32(au + AuPtsHigh, static_cast<std::uint32_t>(pts >> 32));
    m.store32(au + AuPtsLow, static_cast<std::uint32_t>(pts));
    m.store32(au + AuDtsHigh, static_cast<std::uint32_t>(dts >> 32));
    m.store32(au + AuDtsLow, static_cast<std::uint32_t>(dts));
    m.store32(au + AuSize, static_cast<std::uint32_t>(unit.data.size()));
}

// Marks consumed packets free in the guest ringbuffer.
void release(psprecomp::GuestMemory &m, MpegInstance &inst) {
    bool video = false, audio = false;
    for (const auto &[handle, type] : inst.streams) { video |= type == 0u; audio |= type == 1u; }
    const auto freed = inst.demux.release_packets(video, audio);
    if (freed == 0u || inst.ringbuffer == 0u) return;
    const auto rb = inst.ringbuffer, total = m.load32(rb + RbTotal);
    m.store32(rb + RbFilled, m.load32(rb + RbFilled) - freed);
    if (s(total) > 0) m.store32(rb + RbRead, (m.load32(rb + RbRead) + freed) % total);
}

// sceMpegGetAvcAu attribute: 1 for a reference picture, 0 otherwise — the
// nal_ref_idc of the AU's first slice NAL (basic.expected: AUs 58/110/169 of
// test.pmf are the only non-reference slices and the only attr=0 results).
std::uint32_t avc_attribute(const std::vector<std::uint8_t> &au) {
    for (std::size_t i = 0; i + 3 < au.size(); ++i) {
        if (au[i] != 0 || au[i + 1] != 0 || au[i + 2] != 1) continue;
        const std::uint32_t header = au[i + 3], type = header & 31u;
        if (type == 1u || type == 5u) return (header >> 5) & 3u ? 1u : 0u;
    }
    return 1u;
}

struct PendingPut {
    std::uint32_t rb{}, handle{};
    std::int32_t remaining{}, added{};
};

bool end_of_stream(const MpegInstance &inst) {
    return inst.stream_size != 0u && inst.demux.packets_received() * kPacketSize >= inst.stream_size;
}

// One read-callback round of sceMpegRingbufferPut; the continuation either
// finishes the put or starts the next round.
void put_step(psprecomp::Runtime &rt, psprecomp::AllegrexContext &resume, MpegState &st, KernelState &kernel,
              std::shared_ptr<PendingPut> put) {
    auto &mem = rt.memory();
    const auto total = mem.load32(put->rb + RbTotal), write = mem.load32(put->rb + RbWritten);
    const auto chunk = std::min<std::int32_t>(put->remaining, s(total - write));
    const auto at = mem.load32(put->rb + RbData) + write * kPacketSize;
    auto then = [&st, &kernel, put, at](psprecomp::Runtime &r, psprecomp::AllegrexContext &ctx, std::uint32_t v0) {
        auto &m = r.memory();
        const auto got = s(v0);
        auto *inst = st.find(put->handle);
        if (got < 0) { ctx.set_gpr(2, v0); return; }
        if (got > 0 && inst != nullptr) {
            std::vector<std::uint8_t> packet(kPacketSize);
            for (std::int32_t i = 0; i < got; ++i) {
                m.copy_out(at + static_cast<std::uint32_t>(i) * kPacketSize, packet);
                inst->demux.push_packet(packet);
            }
            const auto t = m.load32(put->rb + RbTotal);
            m.store32(put->rb + RbWritten, (m.load32(put->rb + RbWritten) + static_cast<std::uint32_t>(got)) % t);
            m.store32(put->rb + RbFilled, m.load32(put->rb + RbFilled) + static_cast<std::uint32_t>(got));
            put->added += got;
            put->remaining -= got;
        }
        if (got == 0 || put->remaining <= 0 || inst == nullptr) {
            r.event("mpeg_ringbuffer_put", {{"added", static_cast<std::uint32_t>(put->added)}});
            ctx.set_gpr(2, static_cast<std::uint32_t>(put->added));
            return;
        }
        put_step(r, ctx, st, kernel, put);
    };
    kernel.threads().call_guest(rt, resume, resume, mem.load32(put->rb + RbCallback),
                                {at, static_cast<std::uint32_t>(chunk), mem.load32(put->rb + RbCallbackArg)},
                                mem.load32(put->rb + RbGp), then);
}

} // namespace

void register_mpeg_module(psprecomp::Runtime &runtime, KernelState &kernel) {
    auto &state = kernel.mpeg();
    auto *k = &kernel;
    const auto reg = [&runtime](std::uint32_t nid, psprecomp::Runtime::HleFunction fn) {
        runtime.register_hle("sceMpeg", nid, std::move(fn));
    };
    auto *st = &state;
    // Resolves an `SceMpeg *` argument to its instance.
    auto instance = [st](psprecomp::Runtime &rt, std::uint32_t mpeg_ptr) -> MpegInstance * {
        if (!rt.memory().contains(mpeg_ptr, 4u)) return nullptr;
        return st->find(rt.memory().load32(mpeg_ptr));
    };

    reg(0x682A619Bu, [st](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceMpegInit
        st->initialized = true;
        ctx.set_gpr(2, 0u);
    });
    reg(0x874624D6u, [st](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceMpegFinish
        st->initialized = false;
        ctx.set_gpr(2, 0u);
    });
    reg(0xD7A29F46u, [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceMpegRingbufferQueryMemSize
        ctx.set_gpr(2, ctx.gpr[4] * kRingbufferBytesPerPacket); // 32-bit wrap matches memsize.expected
    });
    reg(0x37295ED8u, [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegRingbufferConstruct
        const auto rb = ctx.gpr[4], packets = ctx.gpr[5], data = ctx.gpr[6], size = ctx.gpr[7];
        const auto callback = ctx.gpr[8], arg = ctx.gpr[9];
        // construct.expected: fails when size < packets*0x868, both signed.
        if (s(size) < s(packets * kRingbufferBytesPerPacket)) { ctx.set_gpr(2, kErrorRingbufferSize); return; }
        auto &m = rt.memory();
        if (!m.contains(rb, 48u)) { ctx.set_gpr(2, kErrorInvalidValue); return; }
        m.store32(rb + RbTotal, packets);
        m.store32(rb + RbRead, 0u);
        m.store32(rb + RbWritten, 0u);
        m.store32(rb + RbFilled, 0u);
        m.store32(rb + RbPacketSize, kPacketSize);
        m.store32(rb + RbData, data);
        m.store32(rb + RbCallback, callback);
        m.store32(rb + RbCallbackArg, arg);
        m.store32(rb + RbDataEnd, data + packets * kPacketSize);
        m.store32(rb + RbUnknown, 0u);
        m.store32(rb + RbMpeg, 0u);
        m.store32(rb + RbGp, ctx.gpr[28]);
        ctx.set_gpr(2, 0u);
    });
    reg(0x13407F13u, [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceMpegRingbufferDestruct
        ctx.set_gpr(2, 0u); // destruct.expected: always 0, fields untouched
    });
    reg(0xC132E22Fu, [](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceMpegQueryMemSize
        ctx.set_gpr(2, kMpegMemSize);
    });
    reg(0xD8C5F121u, [st](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegCreate
        const auto mpeg_ptr = ctx.gpr[4], data = ctx.gpr[5], size = ctx.gpr[6], rb = ctx.gpr[7];
        auto &m = rt.memory();
        if (size < kMpegMemSize || !m.contains(data, kMpegMemSize) || !m.contains(mpeg_ptr, 4u)) {
            ctx.set_gpr(2, kErrorRingbufferSize); // [INFERRED] code for an undersized work area
            return;
        }
        // Header "LIBMPEG" / "001" (pspautotests shared.h SceMpegBufferHeader).
        static constexpr char kMagic[] = "LIBMPEG\0" "001";
        for (std::uint32_t i = 0; i < 12u; ++i) m.store8(data + i, static_cast<std::uint8_t>(kMagic[i]));
        m.store32(data + 12u, 0xFFFFFFFFu);
        m.store32(data + 16u, rb);
        m.store32(data + 20u, m.contains(rb, 48u) ? m.load32(rb + RbDataEnd) : 0u);
        m.store32(mpeg_ptr, data);
        if (m.contains(rb, 48u)) m.store32(rb + RbMpeg, data);
        auto &created = st->instances[data] = MpegInstance{};
        created.data = data;
        created.ringbuffer = rb;
        rt.event("mpeg_create", {{"handle", data}, {"ringbuffer", rb}});
        ctx.set_gpr(2, 0u);
    });
    reg(0x606A4649u, [st, instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegDelete
        if (auto *inst = instance(rt, ctx.gpr[4])) {
            rt.event("mpeg_delete", {{"handle", inst->data}, {"video_aus", inst->video_decoded}, {"audio_aus", inst->audio_decoded}});
            st->instances.erase(inst->data);
        }
        ctx.set_gpr(2, 0u);
    });
    reg(0x21FF80E4u, [instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegQueryStreamOffset
        auto &m = rt.memory();
        const auto header = ctx.gpr[5], out = ctx.gpr[6];
        if (!m.contains(header, 16u) || m.load32(header) != 0x464D5350u) { ctx.set_gpr(2, kErrorInvalidValue); return; } // "PSMF"
        const auto offset = __builtin_bswap32(m.load32(header + 8u));
        m.store32(out, offset);
        if (auto *inst = instance(rt, ctx.gpr[4])) inst->stream_size = __builtin_bswap32(m.load32(header + 12u));
        ctx.set_gpr(2, 0u);
    });
    reg(0x611E9E11u, [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegQueryStreamSize
        auto &m = rt.memory();
        const auto header = ctx.gpr[4], out = ctx.gpr[5];
        if (!m.contains(header, 16u) || m.load32(header) != 0x464D5350u) { ctx.set_gpr(2, kErrorInvalidValue); return; }
        m.store32(out, __builtin_bswap32(m.load32(header + 12u)));
        ctx.set_gpr(2, 0u);
    });
    reg(0x42560F23u, [instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegRegistStream
        auto *inst = instance(rt, ctx.gpr[4]);
        if (inst == nullptr) { ctx.set_gpr(2, 0u); return; }
        const auto type = ctx.gpr[5], channel = ctx.gpr[6];
        const auto handle = inst->data + kStreamBaseOffset + kStreamStride * static_cast<std::uint32_t>(inst->streams.size());
        inst->streams[handle] = type;
        std::uint32_t video = 0, audio = 0;
        if (type == 0u) video = channel; else if (type == 1u) audio = channel;
        inst->demux.configure(video, audio);
        rt.event("mpeg_regist_stream", {{"type", type}, {"channel", channel}, {"stream", handle}});
        ctx.set_gpr(2, handle);
    });
    reg(0x591A4AA2u, [instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegUnRegistStream
        if (auto *inst = instance(rt, ctx.gpr[4])) inst->streams.erase(ctx.gpr[5]);
        ctx.set_gpr(2, 0u);
    });
    reg(0xA780CF7Eu, [instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegMallocAvcEsBuf
        auto *inst = instance(rt, ctx.gpr[4]);
        std::uint32_t result = 0; // basic.expected: first buffer is 1
        if (inst != nullptr)
            for (std::uint32_t i = 0; i < 2u; ++i)
                if ((inst->es_buffers & (1u << i)) == 0u) { inst->es_buffers |= 1u << i; result = i + 1u; break; }
        ctx.set_gpr(2, result);
    });
    reg(0xCEB870B1u, [instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegFreeAvcEsBuf
        if (auto *inst = instance(rt, ctx.gpr[4]); inst != nullptr && ctx.gpr[5] >= 1u && ctx.gpr[5] <= 2u)
            inst->es_buffers &= ~(1u << (ctx.gpr[5] - 1u));
        ctx.set_gpr(2, 0u);
    });
    reg(0xF8DCB679u, [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegQueryAtracEsSize
        if (rt.memory().contains(ctx.gpr[5], 4u)) rt.memory().store32(ctx.gpr[5], kAtracEsSize);
        if (rt.memory().contains(ctx.gpr[6], 4u)) rt.memory().store32(ctx.gpr[6], kAtracOutSize);
        ctx.set_gpr(2, 0u);
    });
    reg(0x167AFD9Eu, [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegInitAu
        auto &m = rt.memory();
        const auto au = ctx.gpr[6];
        if (!m.contains(au, 24u)) { ctx.set_gpr(2, kErrorInvalidValue); return; }
        m.store32(au + AuEsBuffer, ctx.gpr[5]);
        m.store32(au + AuSize, 0u);
        ctx.set_gpr(2, 0u);
    });
    reg(0xB5F6DC87u, [st](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegRingbufferAvailableSize
        auto &m = rt.memory();
        const auto rb = ctx.gpr[4];
        // avail.expected: free = total - filled (guest fields); a ringbuffer
        // whose mpeg was deleted reports 0x80618009.
        if (!m.contains(rb, 48u) || st->find(m.load32(rb + RbMpeg)) == nullptr) { ctx.set_gpr(2, kErrorInvalidHandle); return; }
        ctx.set_gpr(2, m.load32(rb + RbTotal) - m.load32(rb + RbFilled));
    });
    reg(0xB240A59Eu, [st, k](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegRingbufferPut
        auto &m = rt.memory();
        const auto rb = ctx.gpr[4];
        const auto requested = s(ctx.gpr[5]), available = s(ctx.gpr[6]);
        if (!m.contains(rb, 48u)) { ctx.set_gpr(2, kErrorInvalidValue); return; }
        auto *inst = st->find(m.load32(rb + RbMpeg));
        const auto count = std::min(requested, available);
        if (inst == nullptr || count <= 0 || m.load32(rb + RbCallback) == 0u) { ctx.set_gpr(2, 0u); return; }
        // basic.expected: the read callback is called for the contiguous span
        // at the write position; a short read is followed by another call for
        // the remainder, a zero/negative return ends the put. The result is
        // the number of packets added.
        auto put = std::make_shared<PendingPut>();
        put->rb = rb;
        put->handle = inst->data;
        put->remaining = count;
        auto resume = ctx;
        resume.pc = ctx.gpr[31];
        ctx = resume;
        put_step(rt, ctx, *st, *k, put);
    });
    reg(0xFE246728u, [instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegGetAvcAu
        auto &m = rt.memory();
        auto *inst = instance(rt, ctx.gpr[4]);
        const auto au = ctx.gpr[6], attr = ctx.gpr[7];
        if (inst == nullptr || !m.contains(au, 24u)) { ctx.set_gpr(2, kErrorInvalidHandle); return; }
        auto unit = inst->demux.next_video_au(end_of_stream(*inst));
        if (!unit) { ctx.set_gpr(2, kErrorNoData); return; }
        write_au(m, au, *unit);
        if (attr != 0u && m.contains(attr, 4u)) m.store32(attr, avc_attribute(unit->data));
        inst->last_video = std::move(*unit);
        release(m, *inst);
        ctx.set_gpr(2, 0u);
    });
    reg(0xE1CE83A7u, [instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegGetAtracAu
        auto &m = rt.memory();
        auto *inst = instance(rt, ctx.gpr[4]);
        const auto au = ctx.gpr[6], out = ctx.gpr[7];
        if (inst == nullptr || !m.contains(au, 24u)) { ctx.set_gpr(2, kErrorInvalidHandle); return; }
        auto unit = inst->demux.next_audio_au(end_of_stream(*inst));
        if (!unit) {
            if (out != 0u && m.contains(out, 4u)) m.store32(out, 0u); // basic.expected: "at 00000000"
            ctx.set_gpr(2, kErrorNoData);
            return;
        }
        write_au(m, au, *unit);
        inst->last_audio = std::move(*unit);
        if (out != 0u && m.contains(out, 4u)) m.store32(out, m.load32(au + AuEsBuffer)); // [INFERRED]
        release(m, *inst);
        ctx.set_gpr(2, 0u);
    });
    reg(0x0E3C2E9Du, [st, instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegAvcDecode
        auto &m = rt.memory();
        auto *inst = instance(rt, ctx.gpr[4]);
        const auto au = ctx.gpr[5], width = ctx.gpr[6], buffer = ctx.gpr[7], init = ctx.gpr[8];
        if (inst == nullptr || !m.contains(au, 24u)) { ctx.set_gpr(2, kErrorInvalidHandle); return; }
        // basic.expected: the first decode yields no picture (*init = 0),
        // later decodes one picture each; the AU size is cleared.
        std::uint32_t pictures = 0;
        if (m.load32(au + AuSize) != 0u) {
            if (inst->video_pipeline_primed && m.contains(buffer, 4u)) {
                if (st->decoder->decode_video(inst->last_video, m, m.load32(buffer), width, 3u)) pictures = 1;
            }
            inst->video_pipeline_primed = true;
            ++inst->video_decoded;
        }
        m.store32(au + AuSize, 0u);
        if (init != 0u && m.contains(init, 4u)) m.store32(init, pictures);
        ctx.set_gpr(2, 0u);
    });
    reg(0x740FCCD1u, [](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegAvcDecodeStop
        if (ctx.gpr[7] != 0u && rt.memory().contains(ctx.gpr[7], 4u)) rt.memory().store32(ctx.gpr[7], 0u); // [INFERRED] no pending picture
        ctx.set_gpr(2, 0u);
    });
    reg(0x4571CC64u, [instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegAvcDecodeFlush
        if (auto *inst = instance(rt, ctx.gpr[4])) inst->video_pipeline_primed = false;
        ctx.set_gpr(2, 0u);
    });
    reg(0x800C44DFu, [st, instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegAtracDecode
        auto &m = rt.memory();
        auto *inst = instance(rt, ctx.gpr[4]);
        const auto au = ctx.gpr[5], buffer = ctx.gpr[6];
        if (inst == nullptr || !m.contains(au, 24u) || m.load32(au + AuSize) == 0u) { ctx.set_gpr(2, kErrorAtracNoAu); return; }
        st->decoder->decode_audio(inst->last_audio, m, buffer);
        ++inst->audio_decoded;
        m.store32(au + AuSize, 0u);
        ctx.set_gpr(2, 0u);
    });
    reg(0x707B7629u, [instance](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceMpegFlushAllStream
        auto &m = rt.memory();
        if (auto *inst = instance(rt, ctx.gpr[4])) {
            inst->demux.reset();
            inst->video_pipeline_primed = false;
            if (m.contains(inst->ringbuffer, 48u)) {
                m.store32(inst->ringbuffer + RbRead, 0u);
                m.store32(inst->ringbuffer + RbWritten, 0u);
                m.store32(inst->ringbuffer + RbFilled, 0u);
            }
        }
        ctx.set_gpr(2, 0u);
    });
}

} // namespace p3p3ds::hle
