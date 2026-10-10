#pragma once
// GE back end on another thread. On the PSP the GE draws while the CPU runs:
// the game enqueues a list, keeps working and waits for the GE only at
// sceGeDrawSync / sceGeListSync (first battle: 47 % of the game's time per
// frame lies between the two). AsyncRenderer gives that overlap to any
// GeRenderer: the GE command processor (core/src/hle/ge.cpp) stays on the
// game's thread and only queues what the renderer needs -- a snapshot of the
// GE registers when the render state changed, the CLUT when it changed, and
// each draw / transfer / sync -- and a worker thread replays the queue into
// the real renderer. GeManager calls drain() where the game waits for the GE,
// so the renderer is done with guest memory exactly where the PSP would be.
//
// Without a worker (start hook missing or failing) every call goes straight
// to the inner renderer, as before.
#include "p3p3ds/ge/renderer.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace p3p3ds::ge {

struct WorkerHooks {
    // Runs entry(arg) on another thread; false when no thread could be created.
    std::function<bool(void (*entry)(void *), void *arg)> start;
    std::function<void()> join;          // waits for that thread after it returned
    void (*pause)() = nullptr;           // a short wait while polling (sleep / yield)
    std::uint64_t (*clock)() = nullptr;  // optional: busy / wait accounting units
};

struct AsyncStats {
    std::uint64_t records{}, states{}, cluts{}, draws{}, calls{};
    std::uint64_t drains{};            // drain() calls that had to wait
    std::uint64_t drain_wait{};        // clock units the game's thread waited in drain()
    std::uint64_t full_wait{};         // clock units it waited for room in the queue
    std::atomic<std::uint64_t> busy{}; // clock units the worker spent replaying records
};

class AsyncRenderer final : public GeRenderer {
public:
    // drain_on_sync: also wait at every sync() (end of each list execution):
    // the worker then never runs concurrently with guest code, which makes a
    // run deterministic (used to verify the queue against the direct path).
    AsyncRenderer(std::unique_ptr<GeRenderer> inner, std::size_t queue_bytes, WorkerHooks hooks, bool drain_on_sync = false);
    ~AsyncRenderer() override;
    AsyncRenderer(const AsyncRenderer &) = delete;
    AsyncRenderer &operator=(const AsyncRenderer &) = delete;

    void draw(psprecomp::GuestMemory &memory, const GeRegisters &regs, Prim prim, std::uint32_t count,
              std::uint32_t vertex_address, std::uint32_t index_address) override;
    void transfer(psprecomp::GuestMemory &memory, const GeRegisters &regs) override;
    void sync() override;
    void drain() override;
    [[nodiscard]] const DrawStats &stats() const override { return inner_->stats(); }

    // Runs fn on the worker after everything queued so far (directly without a worker).
    void post(std::function<void(GeRenderer &)> fn);
    // Runs fn on this thread once the worker is idle.
    template <class F>
    decltype(auto) call_idle(F &&fn) {
        drain();
        return fn(*inner_);
    }
    [[nodiscard]] bool threaded() const noexcept { return threaded_; }
    [[nodiscard]] GeRenderer &inner() noexcept { return *inner_; }
    [[nodiscard]] const AsyncStats &async_stats() const noexcept { return stats_; }
    // Records queued so far (to coalesce posts between draws).
    [[nodiscard]] std::uint64_t records() const noexcept { return stats_.records; }

private:
    enum class Type : std::uint32_t { Wrap = 1, State, Clut, Draw, Transfer, Sync, Call, Stop };
    struct Header {
        Type type;
        std::uint32_t bytes; // whole record, multiple of 8
    };

    static void worker_entry(void *self);
    void worker_loop();
    std::uint8_t *reserve(std::uint32_t bytes);
    void publish();
    void push(Type type, const void *payload, std::uint32_t payload_bytes);
    void push_state(const GeRegisters &regs);
    std::uint64_t now() const noexcept { return hooks_.clock != nullptr ? hooks_.clock() : 0u; }
    void pause() const noexcept {
        if (hooks_.pause != nullptr) hooks_.pause();
    }

    std::unique_ptr<GeRenderer> inner_;
    WorkerHooks hooks_;
    bool drain_on_sync_{};
    bool threaded_{};
    std::vector<std::uint8_t> ring_;
    std::atomic<std::uint32_t> head_{0}; // written by the game's thread
    std::atomic<std::uint32_t> tail_{0}; // written by the worker
    std::uint32_t pending_{};            // reserved, not yet published (game's thread)
    // What the worker's register copy holds (game's thread).
    bool state_sent_{}, clut_sent_{};
    std::uint64_t sent_version_{}, sent_clut_hash_{};
    GeRegisters worker_regs_; // the worker's copy, rebuilt from State / Clut records
    AsyncStats stats_;
};

} // namespace p3p3ds::ge
