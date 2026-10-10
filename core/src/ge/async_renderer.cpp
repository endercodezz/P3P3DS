// GE back end on a worker thread: see async_renderer.hpp.
//
// The queue is a single-producer / single-consumer byte ring. Records are
// [Header][payload] rounded to 8 bytes; a Wrap record sends the reader back
// to the start when a record does not fit before the end. head_ (producer)
// and tail_ (consumer) are published with release / read with acquire, so
// everything written before a record -- its payload, and guest memory the
// game wrote before the GE command -- is visible to the worker.
#include "p3p3ds/ge/async_renderer.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace p3p3ds::ge {
namespace {

// Everything of GeRegisters but the CLUT (sent on its own, only when it changed).
struct StatePayload {
    std::array<std::uint32_t, 256> reg;
    std::array<float, 12 * 8> bone;
    std::array<float, 12> world, view, tgen;
    std::array<float, 16> proj;
    std::uint64_t state_version;
};
struct ClutPayload {
    std::array<std::uint32_t, 256> clut;
    std::uint32_t clut_words;
    std::uint32_t pad;
    std::uint64_t clut_hash;
};
struct DrawPayload {
    psprecomp::GuestMemory *memory;
    std::uint32_t prim, count, vertex, index;
};
struct TransferPayload {
    psprecomp::GuestMemory *memory;
};
struct CallPayload {
    std::function<void(GeRenderer &)> *fn;
};

constexpr std::uint32_t round8(std::size_t n) { return static_cast<std::uint32_t>((n + 7u) & ~std::size_t{7}); }
constexpr std::size_t kMinQueue = 64u << 10;

} // namespace

AsyncRenderer::AsyncRenderer(std::unique_ptr<GeRenderer> inner, std::size_t queue_bytes, WorkerHooks hooks, bool drain_on_sync)
    : inner_(std::move(inner)), hooks_(std::move(hooks)), drain_on_sync_(drain_on_sync) {
    if (!hooks_.start) return;
    ring_.resize(round8(std::max(queue_bytes, kMinQueue)));
    threaded_ = hooks_.start(&AsyncRenderer::worker_entry, this);
    if (!threaded_) {
        ring_.clear();
        ring_.shrink_to_fit();
    }
}

AsyncRenderer::~AsyncRenderer() {
    if (!threaded_) return;
    push(Type::Stop, nullptr, 0);
    drain();
    if (hooks_.join) hooks_.join();
}

void AsyncRenderer::worker_entry(void *self) { static_cast<AsyncRenderer *>(self)->worker_loop(); }

void AsyncRenderer::worker_loop() {
    const auto cap = static_cast<std::uint32_t>(ring_.size());
    bool busy = false;
    std::uint64_t busy_start = 0;
    for (;;) {
        std::uint32_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire)) {
            if (busy) { // busy time is taken per stretch of work, not per record
                stats_.busy.fetch_add(now() - busy_start, std::memory_order_relaxed);
                busy = false;
            }
            pause();
            continue;
        }
        if (!busy) {
            busy = true;
            busy_start = now();
        }
        Header header;
        std::memcpy(&header, &ring_[t], sizeof header);
        const std::uint8_t *payload = &ring_[t] + sizeof(Header);
        bool stop = false;
        switch (header.type) {
        case Type::Wrap:
            tail_.store(0u, std::memory_order_release);
            continue;
        case Type::State: {
            StatePayload s;
            std::memcpy(&s, payload, sizeof s);
            worker_regs_.reg = s.reg;
            worker_regs_.bone = s.bone;
            worker_regs_.world = s.world;
            worker_regs_.view = s.view;
            worker_regs_.tgen = s.tgen;
            worker_regs_.proj = s.proj;
            worker_regs_.state_version = s.state_version;
            break;
        }
        case Type::Clut: {
            ClutPayload c;
            std::memcpy(&c, payload, sizeof c);
            worker_regs_.clut = c.clut;
            worker_regs_.clut_words = c.clut_words;
            worker_regs_.clut_hash = c.clut_hash;
            break;
        }
        case Type::Draw: {
            DrawPayload d;
            std::memcpy(&d, payload, sizeof d);
            inner_->draw(*d.memory, worker_regs_, static_cast<Prim>(d.prim), d.count, d.vertex, d.index);
            break;
        }
        case Type::Transfer: {
            TransferPayload d;
            std::memcpy(&d, payload, sizeof d);
            inner_->transfer(*d.memory, worker_regs_);
            break;
        }
        case Type::Sync:
            inner_->sync();
            break;
        case Type::Call: {
            CallPayload c;
            std::memcpy(&c, payload, sizeof c);
            (*c.fn)(*inner_);
            delete c.fn;
            break;
        }
        case Type::Stop:
            stop = true;
            break;
        }
        t += header.bytes;
        if (t == cap) t = 0;
        if (stop) stats_.busy.fetch_add(now() - busy_start, std::memory_order_relaxed);
        tail_.store(t, std::memory_order_release);
        if (stop) return;
    }
}

// Room for one record of `bytes` (a multiple of 8, far below the queue size);
// waits while the worker has not freed enough. One byte always stays free, so
// head == tail means empty.
std::uint8_t *AsyncRenderer::reserve(std::uint32_t bytes) {
    const auto cap = static_cast<std::uint32_t>(ring_.size());
    const std::uint32_t h = head_.load(std::memory_order_relaxed);
    std::uint64_t wait_start = 0;
    bool waited = false;
    for (;;) {
        const std::uint32_t t = tail_.load(std::memory_order_acquire);
        const std::uint32_t free = (t + cap - h - 1u) % cap;
        if (h + bytes <= cap) {
            if (free >= bytes) {
                pending_ = (h + bytes) % cap;
                break;
            }
        } else if (free >= (cap - h) + bytes) {
            const Header wrap{Type::Wrap, cap - h};
            std::memcpy(&ring_[h], &wrap, sizeof wrap);
            pending_ = bytes;
            if (waited) stats_.full_wait += now() - wait_start;
            return ring_.data();
        }
        if (!waited) {
            waited = true;
            wait_start = now();
        }
        pause();
    }
    if (waited) stats_.full_wait += now() - wait_start;
    return &ring_[h];
}

void AsyncRenderer::publish() { head_.store(pending_, std::memory_order_release); }

void AsyncRenderer::push(Type type, const void *payload, std::uint32_t payload_bytes) {
    const std::uint32_t bytes = round8(sizeof(Header) + payload_bytes);
    std::uint8_t *at = reserve(bytes);
    const Header header{type, bytes};
    std::memcpy(at, &header, sizeof header);
    if (payload_bytes != 0u) std::memcpy(at + sizeof header, payload, payload_bytes);
    publish();
    ++stats_.records;
}

// The worker's register copy follows the game's only when the render state
// changed (state_version; CLUT loads change it too), the CLUT only when its
// hash changed.
void AsyncRenderer::push_state(const GeRegisters &regs) {
    if (state_sent_ && regs.state_version == sent_version_) return;
    StatePayload s;
    s.reg = regs.reg;
    s.bone = regs.bone;
    s.world = regs.world;
    s.view = regs.view;
    s.tgen = regs.tgen;
    s.proj = regs.proj;
    s.state_version = regs.state_version;
    push(Type::State, &s, sizeof s);
    state_sent_ = true;
    sent_version_ = regs.state_version;
    ++stats_.states;
    if (clut_sent_ && regs.clut_hash == sent_clut_hash_) return;
    ClutPayload c;
    c.clut = regs.clut;
    c.clut_words = regs.clut_words;
    c.pad = 0;
    c.clut_hash = regs.clut_hash;
    push(Type::Clut, &c, sizeof c);
    clut_sent_ = true;
    sent_clut_hash_ = regs.clut_hash;
    ++stats_.cluts;
}

void AsyncRenderer::draw(psprecomp::GuestMemory &memory, const GeRegisters &regs, Prim prim, std::uint32_t count,
                         std::uint32_t vertex_address, std::uint32_t index_address) {
    if (!threaded_) {
        inner_->draw(memory, regs, prim, count, vertex_address, index_address);
        return;
    }
    push_state(regs);
    const DrawPayload d{&memory, static_cast<std::uint32_t>(prim), count, vertex_address, index_address};
    push(Type::Draw, &d, sizeof d);
    ++stats_.draws;
}

void AsyncRenderer::transfer(psprecomp::GuestMemory &memory, const GeRegisters &regs) {
    if (!threaded_) {
        inner_->transfer(memory, regs);
        return;
    }
    push_state(regs);
    const TransferPayload d{&memory};
    push(Type::Transfer, &d, sizeof d);
}

void AsyncRenderer::sync() {
    if (!threaded_) {
        inner_->sync();
        return;
    }
    push(Type::Sync, nullptr, 0);
    if (drain_on_sync_) drain();
}

void AsyncRenderer::drain() {
    if (!threaded_) return;
    if (tail_.load(std::memory_order_acquire) == head_.load(std::memory_order_relaxed)) return;
    ++stats_.drains;
    const std::uint64_t start = now();
    while (tail_.load(std::memory_order_acquire) != head_.load(std::memory_order_relaxed)) pause();
    stats_.drain_wait += now() - start;
}

void AsyncRenderer::post(std::function<void(GeRenderer &)> fn) {
    if (!threaded_) {
        fn(*inner_);
        return;
    }
    const CallPayload c{new std::function<void(GeRenderer &)>(std::move(fn))};
    push(Type::Call, &c, sizeof c);
    ++stats_.calls;
}

} // namespace p3p3ds::ge
