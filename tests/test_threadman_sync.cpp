// ThreadMan primitives through the registered HLE entry points.
// Expected codes: references/pspautotests/tests/threads/{semaphores,events,lwmutex}/*.expected,
// references/uofw/src/kd/usersystemlib/lwmutex.c, references/uofw/src/kd/audio/audio.c.
#include "p3p3ds/hle/hle_modules.hpp"
#include "p3p3ds/kernel_state.hpp"
#include "psprecomp/runtime.hpp"

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

using namespace p3p3ds::hle;

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::cerr << __LINE__ << ": " << #x << "\n"; ++failures; } } while (0)

constexpr std::uint32_t kStub = 0x08A00000u;   // fake import stub PC
constexpr std::uint32_t kRet = 0x08A00100u;    // caller return address
constexpr std::uint32_t kName = 0x08900000u;
constexpr std::uint32_t kScratch = 0x08900100u;

struct Env {
    psprecomp::Runtime rt;
    p3p3ds::KernelState k;
    std::int32_t a{}, b{}, c{};
    Env() {
        register_all_hle_modules(rt, k);
        k.threads().set_import_stubs({kStub});
        rt.memory().store32(kName, 0x74736574u); // "test"
        rt.memory().store32(kName + 4u, 0u);
        a = k.threads().init_root_thread("A", 0x08804000u, 0x09FFFF00u, 0u);
        rt.cpu().pc = 0x08804000u;
    }
    // One PSP syscall from the current thread, mirroring the generated import
    // wrapper: $ra is taken as the resume PC unless the call switched threads.
    std::uint32_t call(const char *lib, std::uint32_t nid, std::initializer_list<std::uint32_t> args) {
        auto &ctx = rt.cpu();
        std::uint32_t reg = 4u;
        for (auto v : args) ctx.gpr[reg++] = v;
        ctx.gpr[31] = kRet;
        ctx.pc = kStub;
        const auto token = psprecomp::capture_runtime_execution_context();
        rt.invoke_import(lib, nid, ctx);
        if (!rt.stopped() && psprecomp::runtime_execution_context_matches(token) && ctx.pc == kStub) ctx.pc = kRet;
        return ctx.gpr[2];
    }
    std::uint32_t tm(std::uint32_t nid, std::initializer_list<std::uint32_t> args) { return call("ThreadManForUser", nid, args); }
    std::uint32_t kl(std::uint32_t nid, std::initializer_list<std::uint32_t> args) { return call("Kernel_Library", nid, args); }
    std::int32_t spawn(std::int32_t priority) {
        const auto uid = static_cast<std::int32_t>(tm(0x446D8DE6u, {kName, 0x08810000u, static_cast<std::uint32_t>(priority), 0x1000u, 0u, 0u}));
        tm(0xF475845Du, {static_cast<std::uint32_t>(uid), 0u, 0u});
        return uid;
    }
    std::int32_t current() const { return k.threads().current_thread_id(); }
};

std::uint32_t e(std::int32_t code) { return static_cast<std::uint32_t>(code); }

void semaphores() {
    Env env;
    CHECK(env.tm(0xD6DA4BA1u, {0u, 0u, 0u, 2u, 0u}) == e(SCE_KERNEL_ERROR_ERROR));          // NULL name
    CHECK(env.tm(0xD6DA4BA1u, {kName, 0x200u, 0u, 2u, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_ATTR));
    const auto sema = env.tm(0xD6DA4BA1u, {kName, 0u, 1u, 1u, 0u});
    CHECK(static_cast<std::int32_t>(sema) > 0);
    // signal.expected: +2 over max fails; negative signals are accepted.
    CHECK(env.tm(0x3F53E640u, {sema, 1u}) == e(SCE_KERNEL_ERROR_SEMA_OVERFLOW));
    // wait.expected: count > max or <= 0 is ILLEGAL_COUNT.
    CHECK(env.tm(0x4E3A1105u, {sema, 2u, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_COUNT));
    CHECK(env.tm(0x4E3A1105u, {sema, 0u, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_COUNT));
    CHECK(env.tm(0x4E3A1105u, {sema, 1u, 0u}) == 0u);
    // poll.expected: empty -> SEMA_ZERO even for 0; signaled + 0 -> ILLEGAL_COUNT.
    CHECK(env.tm(0x58B1F937u, {sema, 0u}) == e(SCE_KERNEL_ERROR_SEMA_ZERO));
    CHECK(env.tm(0x3F53E640u, {sema, 1u}) == 0u);
    CHECK(env.tm(0x58B1F937u, {sema, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_COUNT));
    CHECK(env.tm(0x58B1F937u, {sema, 1u}) == 0u);
    // Zero timeout never blocks.
    env.rt.memory().store32(kScratch, 0u);
    CHECK(env.tm(0x4E3A1105u, {sema, 1u, kScratch}) == e(SCE_KERNEL_ERROR_WAIT_TIMEOUT));
    CHECK(env.k.threads().get_sema(static_cast<std::int32_t>(sema))->waiters.empty());
    CHECK(env.tm(0x4E3A1105u, {0x7FFFu, 1u, 0u}) == e(SCE_KERNEL_ERROR_UNKNOWN_SEMID));

    // Blocking: A (prio 32) waits, B (prio 40) runs and signals, A (higher
    // priority) preempts B immediately and returns 0.
    env.b = env.spawn(40);
    CHECK(env.current() == env.a);
    env.tm(0x4E3A1105u, {sema, 1u, 0u});
    CHECK(env.current() == env.b);
    CHECK(env.k.threads().get_thread(env.a)->status == InternalThreadState::Waiting);
    env.tm(0x3F53E640u, {sema, 1u});
    CHECK(env.current() == env.a);
    CHECK(env.rt.cpu().gpr[2] == 0u && env.rt.cpu().pc == kRet);
    CHECK(env.k.threads().get_sema(static_cast<std::int32_t>(sema))->count == 0);

    // Timeout: A waits 5000us; B then delays 10000us; A wakes first with TIMEOUT.
    env.rt.memory().store32(kScratch, 5000u);
    const auto start = env.k.threads().now();
    env.tm(0x4E3A1105u, {sema, 1u, kScratch});
    CHECK(env.current() == env.b);
    env.tm(0xCEADEB47u, {10000u});
    CHECK(env.current() == env.a);
    CHECK(env.rt.cpu().gpr[2] == e(SCE_KERNEL_ERROR_WAIT_TIMEOUT));
    CHECK(env.rt.memory().load32(kScratch) == 0u);
    CHECK(env.k.threads().now() >= start + 5000u && env.k.threads().now() < start + 10000u);

    // Delete wakes waiters with WAIT_DELETE.
    env.tm(0x4E3A1105u, {sema, 1u, 0u}); // A waits, B still delayed -> idle until B wakes
    CHECK(env.current() == env.b);
    env.tm(0x28B6489Cu, {sema});
    CHECK(env.current() == env.a);
    CHECK(env.rt.cpu().gpr[2] == e(SCE_KERNEL_ERROR_WAIT_DELETE));
    CHECK(env.tm(0x3F53E640u, {sema, 1u}) == e(SCE_KERNEL_ERROR_UNKNOWN_SEMID));
    CHECK(!env.rt.stopped());
}

void event_flags() {
    Env env;
    const auto flag = env.tm(0x55C20A00u, {kName, 0u, 0xFFFFFFFFu, 0u});
    CHECK(static_cast<std::int32_t>(flag) > 0);
    const std::uint32_t out = kScratch;
    // wait.expected: pattern 0 -> EVF_ILPAT, bad mode -> ILLEGAL_MODE.
    CHECK(env.tm(0x402FCF22u, {flag, 0u, 0u, out, 0u}) == e(SCE_KERNEL_ERROR_EVF_ILPAT));
    CHECK(env.tm(0x402FCF22u, {flag, 1u, 0x04u, out, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_MODE));
    CHECK(env.tm(0x402FCF22u, {flag, 1u, 0x30u, out, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_MODE));
    // "Clear 0x00000001: OK (bits=FFFFFFFF) ... cur=FFFFFFFE"
    CHECK(env.tm(0x402FCF22u, {flag, 1u, 0x20u, out, 0u}) == 0u);
    CHECK(env.rt.memory().load32(out) == 0xFFFFFFFFu);
    CHECK(env.k.threads().get_event_flag(static_cast<std::int32_t>(flag))->bits == 0xFFFFFFFEu);
    // ClearEventFlag keeps the given bits.
    CHECK(env.tm(0x812346E4u, {flag, 0x0000FFFFu}) == 0u);
    CHECK(env.k.threads().get_event_flag(static_cast<std::int32_t>(flag))->bits == 0x0000FFFEu);
    // Poll that does not match: EVF_COND and current bits reported.
    CHECK(env.tm(0x30FD7D3Au, {flag, 0xFFFFFFFFu, 0u, out}) == e(SCE_KERNEL_ERROR_EVF_COND));
    CHECK(env.rt.memory().load32(out) == 0x0000FFFEu);
    CHECK(env.tm(0x30FD7D3Au, {flag, 0x10000u, 1u, out}) == e(SCE_KERNEL_ERROR_EVF_COND));

    // A waits for bit 16 (OR|CLEAR); B sets it and A preempts.
    env.b = env.spawn(40);
    env.tm(0x402FCF22u, {flag, 0x10000u, 0x21u, out, 0u});
    CHECK(env.current() == env.b);
    // A second waiter without WAITMULTIPLE fails with EVF_MULTI.
    CHECK(env.tm(0x402FCF22u, {flag, 0x20000u, 0x01u, out, 0u}) == e(SCE_KERNEL_ERROR_EVF_MULTI));
    env.tm(0x1FB15A32u, {flag, 0x10000u});
    CHECK(env.current() == env.a);
    CHECK(env.rt.cpu().gpr[2] == 0u);
    CHECK(env.rt.memory().load32(out) == 0x0001FFFEu);
    CHECK(env.k.threads().get_event_flag(static_cast<std::int32_t>(flag))->bits == 0x0000FFFEu);
    CHECK(env.tm(0xEF9E4C70u, {flag}) == 0u);
    CHECK(env.tm(0x1FB15A32u, {flag, 1u}) == e(SCE_KERNEL_ERROR_UNKNOWN_EVFID));
}

void lw_mutexes() {
    Env env;
    const std::uint32_t wa = 0x08900200u;
    // create.expected: attr > 0x3FF, negative count and count 2 non-recursive fail.
    CHECK(env.tm(0x19CFF145u, {wa, kName, 0x400u, 0u, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_ATTR));
    CHECK(env.tm(0x19CFF145u, {wa, kName, 0u, 2u, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_COUNT));
    CHECK(env.tm(0x19CFF145u, {wa, kName, 0u, 0xFFFFFFFFu, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_COUNT));
    CHECK(env.tm(0x19CFF145u, {wa, 0u, 0u, 0u, 0u}) == e(SCE_KERNEL_ERROR_ERROR));
    CHECK(env.tm(0x19CFF145u, {wa, kName, 0u, 0u, 0u}) == 0u);
    CHECK(static_cast<std::int32_t>(env.rt.memory().load32(wa + 16u)) > 0);
    // lock.expected: count 0/2/-1 -> ILLEGAL_COUNT; relock -> 0x800201CF.
    CHECK(env.kl(0xBEA46419u, {wa, 0u, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_COUNT));
    CHECK(env.kl(0xBEA46419u, {wa, 2u, 0u}) == e(SCE_KERNEL_ERROR_ILLEGAL_COUNT));
    CHECK(env.kl(0xBEA46419u, {wa, 1u, 0u}) == 0u);
    CHECK(env.rt.memory().load32(wa + 0u) == 1u && env.rt.memory().load32(wa + 4u) == static_cast<std::uint32_t>(env.a));
    CHECK(env.kl(0xBEA46419u, {wa, 1u, 0u}) == e(SCE_KERNEL_ERROR_LWMUTEX_RECURSIVE));
    // Interrupts suspended: blocking lock refuses (uOFW lwmutex.c).
    const auto state = env.kl(0x092968F4u, {});
    CHECK(env.kl(0xBEA46419u, {wa, 1u, 0u}) == e(SCE_KERNEL_ERROR_CAN_NOT_WAIT));
    env.kl(0x5F10D406u, {state});

    // Contention: B (prio 20, higher) blocks on the lock A holds.
    env.b = env.spawn(20);
    CHECK(env.current() == env.b); // preempted A at start
    env.kl(0xBEA46419u, {wa, 1u, 0u});
    CHECK(env.current() == env.a);
    CHECK(env.rt.memory().load32(wa + 12u) == 1u); // numWaitThreads
    CHECK(env.kl(0x15B6446Bu, {wa, 2u}) == e(SCE_KERNEL_ERROR_ILLEGAL_COUNT));
    env.kl(0x15B6446Bu, {wa, 1u});
    CHECK(env.current() == env.b);                  // ownership handed over, B preempts
    CHECK(env.rt.cpu().gpr[2] == 0u);
    CHECK(env.rt.memory().load32(wa + 4u) == static_cast<std::uint32_t>(env.b));
    CHECK(env.rt.memory().load32(wa + 12u) == 0u);
    CHECK(env.kl(0x15B6446Bu, {wa, 1u}) == 0u);
    CHECK(env.kl(0x15B6446Bu, {wa, 1u}) == e(SCE_KERNEL_ERROR_LWMUTEX_UNLOCKED));

    // Recursive overflow (lock.expected "Lock 1 => INT_MAX (recursive)").
    const std::uint32_t wr = 0x08900300u;
    CHECK(env.tm(0x19CFF145u, {wr, kName, 0x200u, 1u, 0u}) == 0u);
    CHECK(env.kl(0xBEA46419u, {wr, 0x7FFFFFFFu, 0u}) == e(SCE_KERNEL_ERROR_LWMUTEX_LOCK_OVERFLOW));
    CHECK(env.kl(0xBEA46419u, {wr, 2u, 0u}) == 0u);
    CHECK(env.rt.memory().load32(wr) == 3u);
    CHECK(env.tm(0x60107536u, {wr}) == 0u);
    CHECK(env.kl(0x15B6446Bu, {wr, 1u}) == e(SCE_KERNEL_ERROR_LWMUTEX_NOT_FOUND));
}

void mutexes() {
    Env env;
    const auto m = env.tm(0xB7D098C6u, {kName, 0u, 0u, 0u});
    CHECK(static_cast<std::int32_t>(m) > 0);
    CHECK(env.tm(0xB011B11Fu, {m, 1u, 0u}) == 0u);
    CHECK(env.tm(0xB011B11Fu, {m, 1u, 0u}) == e(SCE_KERNEL_ERROR_MUTEX_RECURSIVE));
    env.b = env.spawn(40);
    env.tm(0xCEADEB47u, {100u});                   // A sleeps, B runs
    CHECK(env.current() == env.b);
    CHECK(env.tm(0x6B30100Fu, {m, 1u}) == e(SCE_KERNEL_ERROR_MUTEX_UNLOCKED));
    env.tm(0xB011B11Fu, {m, 1u, 0u});              // B blocks; idle until A's delay ends
    CHECK(env.current() == env.a);
    env.tm(0x6B30100Fu, {m, 1u});                  // hand over to B (lower priority: no preempt)
    CHECK(env.current() == env.a);
    CHECK(env.k.threads().get_mutex(static_cast<std::int32_t>(m))->owner == env.b);
}

void delays_sleep_and_thread_end() {
    Env env;
    env.b = env.spawn(40);
    const auto t0 = env.k.threads().now();
    env.tm(0xCEADEB47u, {3000u});                  // A delays -> B
    CHECK(env.current() == env.b);
    // Wakeup before sleep is counted.
    CHECK(env.tm(0xD59EAD2Fu, {static_cast<std::uint32_t>(env.b)}) == 0u);
    CHECK(env.tm(0x9ACE131Eu, {}) == 0u && env.current() == env.b);
    const auto t_low = env.tm(0x369ED59Du, {});
    CHECK(t_low + ThreadManager::kSyscallCostUs == static_cast<std::uint32_t>(env.k.threads().now()));
    env.tm(0x9ACE131Eu, {});                        // B sleeps; A's delay expires
    CHECK(env.current() == env.a);
    CHECK(env.k.threads().now() >= t0 + 3000u);
    CHECK(env.k.threads().idle_advances() >= 1u);
    // WaitThreadEnd returns the exit status once B exits.
    env.tm(0x278C0DF5u, {static_cast<std::uint32_t>(env.b), 0u}); // A waits for B
    CHECK(env.current() == env.a || env.current() == env.b);
    CHECK(env.rt.stopped()); // B sleeps forever and A waits: deadlock is reported
    CHECK(env.rt.stop_reason().find("Deadlock") != std::string::npos);
}

void thread_end_status() {
    Env env;
    env.b = env.spawn(40);
    env.tm(0x278C0DF5u, {static_cast<std::uint32_t>(env.b), 0u});
    CHECK(env.current() == env.b);
    env.tm(0xAA73C935u, {0x1234u});                 // B exits
    CHECK(env.current() == env.a);
    CHECK(env.rt.cpu().gpr[2] == 0x1234u);
    CHECK(env.tm(0x278C0DF5u, {static_cast<std::uint32_t>(env.b), 0u}) == 0x1234u);
    CHECK(env.tm(0x9FA03CD3u, {static_cast<std::uint32_t>(env.b)}) == 0u);
}

void callbacks() {
    Env env;
    const auto cb = env.tm(0xE81CAF8Fu, {kName, 0x08820000u, 0x1234u});
    CHECK(env.tm(0x349D6D6Cu, {}) == 0u);           // nothing pending
    CHECK(env.tm(0xC11BA8C4u, {cb, 0x32u}) == 0u);
    env.tm(0x349D6D6Cu, {});                        // runs the callback frame
    auto &ctx = env.rt.cpu();
    CHECK(ctx.pc == 0x08820000u && ctx.gpr[4] == 1u && ctx.gpr[5] == 0x32u && ctx.gpr[6] == 0x1234u);
    CHECK(ctx.gpr[31] == ThreadManager::kCallbackReturnSentinel);
    ctx.gpr[2] = 0u;                                // callback returns 0: keep it
    env.k.threads().callback_returned(env.rt, ctx);
    CHECK(ctx.pc == kRet && ctx.gpr[2] == 1u);      // sceKernelCheckCallback -> 1
    CHECK(env.k.threads().get_callback(static_cast<std::int32_t>(cb)) != nullptr);

    // A CB delay with a notification arriving while waiting runs the callback,
    // then keeps waiting; returning non-zero deletes the callback.
    env.b = env.spawn(40);
    env.tm(0x68DA9E36u, {5000u});                   // A: DelayThreadCB
    CHECK(env.current() == env.b);
    env.tm(0xC11BA8C4u, {cb, 7u});                  // B notifies A's callback -> A preempts
    CHECK(env.current() == env.a && ctx.pc == 0x08820000u && ctx.gpr[5] == 7u);
    ctx.gpr[2] = 1u;
    env.k.threads().callback_returned(env.rt, ctx);
    CHECK(env.k.threads().get_callback(static_cast<std::int32_t>(cb)) == nullptr);
    CHECK(env.current() == env.b);                  // A back in its delay
    env.tm(0xCEADEB47u, {10000u});
    CHECK(env.current() == env.a && ctx.pc == kRet && ctx.gpr[2] == 0u);
}

void audio_blocking() {
    Env env;
    auto ch = env.call("sceAudio", 0x5EC81C55u, {0xFFFFFFFFu, 1024u, 0u});
    CHECK(ch == 7u);
    const auto t0 = env.k.threads().now();
    CHECK(env.call("sceAudio", 0x136CAF51u, {ch, 0x8000u, kScratch}) == 1024u); // plays now
    CHECK(env.call("sceAudio", 0x136CAF51u, {ch, 0x8000u, kScratch}) == 1024u); // queued
    CHECK(env.call("sceAudio", 0x136CAF51u, {ch, 0x10000u, kScratch}) == 0x8026000Bu); // volume
    // Third buffer: slot busy until the second starts -> the only thread idles,
    // then resumes at the stub to retry (uOFW loop).
    env.call("sceAudio", 0x136CAF51u, {ch, 0x8000u, kScratch});
    CHECK(env.rt.cpu().pc == kStub);
    CHECK(env.k.threads().now() >= t0 + 1024u * 1000000u / 44100u);
    CHECK(env.call("sceAudio", 0x136CAF51u, {ch, 0x8000u, kScratch}) == 1024u); // retry succeeds
    CHECK(env.k.audio().channel_state(ch).buffers == 3u);
}

// A thread woken from a retry wait and preempted in the same post-import hook
// (its wake and a higher-priority thread's deadline 1 us apart) must still
// resume at the stub and retry, not at $ra with a stale $v0.
void retry_survives_preemption() {
    int preempted_after_wake = 0;
    for (std::int64_t offset = -4; offset <= 4; ++offset) {
        Env env;
        env.b = env.spawn(40);
        const auto ch = env.call("sceAudio", 0x5EC81C55u, {0xFFFFFFFFu, 1024u, 0u});
        env.call("sceAudio", 0x136CAF51u, {ch, 0x8000u, kScratch}); // A: plays now
        env.call("sceAudio", 0x136CAF51u, {ch, 0x8000u, kScratch}); // A: queued
        // A (prio 32) delays until about when the slot frees; B (prio 40) runs.
        const auto free_at = static_cast<std::int64_t>(env.k.audio().channel_state(ch).slot_free_at);
        const auto now = static_cast<std::int64_t>(env.k.threads().now());
        const auto delay = free_at + static_cast<std::int64_t>(ThreadManager::kSyscallCostUs) + offset - now;
        if (delay <= 0) continue;
        env.tm(0xCEADEB47u, {static_cast<std::uint32_t>(delay)});
        CHECK(env.current() == env.b);
        env.call("sceAudio", 0x136CAF51u, {ch, 0x8000u, kScratch}); // B: slot busy -> retry wait
        CHECK(env.k.audio().channel_state(ch).buffers == 2u);
        if (env.current() == env.a && env.k.threads().get_thread(env.b)->status == InternalThreadState::Ready)
            ++preempted_after_wake;
        if (env.current() == env.a) env.tm(0x9ACE131Eu, {}); // A sleeps -> B continues
        CHECK(env.current() == env.b);
        CHECK(env.rt.cpu().pc == kStub);                     // retry, not $ra
        env.call("sceAudio", 0x136CAF51u, {ch, 0x8000u, kScratch}); // retry succeeds
        // A's delay may expire right after it and preempt B: read B's own result.
        const auto &b_ctx = env.current() == env.b ? env.rt.cpu() : env.k.threads().get_thread(env.b)->context;
        CHECK(b_ctx.gpr[2] == 1024u && b_ctx.pc == kRet);
        CHECK(env.k.audio().channel_state(ch).buffers == 3u);
    }
    CHECK(preempted_after_wake > 0); // the scenario was really exercised
}

// New threads start with identity VFPU source/target prefixes (0xE4).
void new_thread_vfpu_prefixes() {
    Env env;
    env.b = env.spawn(40);
    const auto *t = env.k.threads().get_thread(env.b);
    CHECK(t->context.vfpu_ctrl[0] == 0xE4u && t->context.vfpu_ctrl[1] == 0xE4u && t->context.vfpu_ctrl[2] == 0u);
    CHECK(env.rt.cpu().vfpu_ctrl[0] == 0xE4u && env.rt.cpu().vfpu_ctrl[1] == 0xE4u);
}
} // namespace

namespace { void audio_contracts(); }
int main() {
    semaphores();
    event_flags();
    lw_mutexes();
    mutexes();
    delays_sleep_and_thread_end();
    thread_end_status();
    callbacks();
    audio_blocking();
    retry_survives_preemption();
    new_thread_vfpu_prefixes();
    audio_contracts();
    std::cout << "threadman sync failures=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}

// sceAudio channel contracts (references/uofw/src/kd/audio/audio.c).
namespace {
void audio_contracts() {
    Env env;
    std::vector<std::vector<std::int16_t>> pcm;
    env.k.audio().set_sink([&](unsigned, std::uint64_t, const std::vector<std::int16_t> &s) { pcm.push_back(s); });
    const auto a = [&](std::uint32_t nid, std::initializer_list<std::uint32_t> args) { return env.call("sceAudio", nid, args); };
    const auto ch = a(0x5EC81C55u, {0xFFFFFFFFu, 64u, 0u});
    CHECK(ch == 7u);
    CHECK(a(0x95FD0C2Du, {8u, 0u}) == 0x80260003u);          // ChangeChannelConfig: bad channel
    CHECK(a(0x95FD0C2Du, {ch, 0x20u}) == 0x80260007u);       // invalid format
    CHECK(a(0x95FD0C2Du, {ch, 0x10u}) == 0u);                // mono
    CHECK(a(0x95FD0C2Du, {6u, 0u}) == 0x80260008u);          // not reserved
    CHECK(a(0xB7E1D8E7u, {ch, 0x10000u, 0u}) == 0x8026000Bu); // volume range
    CHECK(a(0xB7E1D8E7u, {ch, 0x8000u, 0x4000u}) == 0u);
    CHECK(a(0xCB2E439Eu, {ch, 65u}) == 0x80260006u);         // SetChannelDataLen alignment
    CHECK(a(0xCB2E439Eu, {6u, 64u}) == 0x80260001u);
    // Mono panned output: sink receives volume-scaled stereo.
    env.rt.memory().store16(kScratch, 0x1000u);
    for (std::uint32_t i = 1; i < 64u; ++i) env.rt.memory().store16(kScratch + i * 2u, 0u);
    CHECK(a(0xE2D56B2Du, {ch, 0x8000u, 0x4000u, kScratch}) == 64u);
    CHECK(pcm.size() == 1u && pcm[0].size() == 128u && pcm[0][0] == 0x1000 && pcm[0][1] == 0x0800);
    CHECK(a(0xB011922Fu, {ch}) > 0u);                        // rest length while playing
    CHECK(a(0xE2D56B2Du, {ch, 0x8000u, 0x8000u, kScratch}) == 64u); // queued behind
    CHECK(a(0xE2D56B2Du, {ch, 0x8000u, 0x8000u, kScratch}) == 0x80260002u); // non-blocking busy
    CHECK(a(0x95FD0C2Du, {ch, 0u}) == 0x80260002u);          // config while queued
    CHECK(a(0x6FC46853u, {ch}) == 0u);                        // release
    CHECK(a(0x6FC46853u, {ch}) == 0x80260008u);
    // Output2 (SRC channel)
    CHECK(a(0x01562BA3u, {16u}) == 0x80000104u);
    CHECK(a(0x01562BA3u, {1024u}) == 0u);
    CHECK(a(0x01562BA3u, {1024u}) == 0x80268002u);
    CHECK(a(0x2D53F36Eu, {0x100000u, kScratch}) == 0x8026000Bu);
    CHECK(a(0x2D53F36Eu, {0x8000u, kScratch}) == 1024u);
    CHECK(a(0x43196845u, {}) == 0x80268002u);                 // still playing
    env.tm(0xCEADEB47u, {100000u});
    CHECK(a(0x43196845u, {}) == 0u);
}
} // namespace
