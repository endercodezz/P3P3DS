// ThreadManForUser / Kernel_Library HLE entry points.
//
// Convention: services that may block return std::optional and leave $v0 to
// the waker when they block (ctx then belongs to another thread). Services
// that may wake a higher-priority thread set $v0 = 0 themselves before
// preempting, so the wrapper only writes $v0 for errors.
// NIDs: psp/pspsdk/src/user/ThreadManForUser.S, references/uofw/src/kd/*/exports.exp.
#include "p3p3ds/hle/threadman.hpp"
#include "p3p3ds/kernel_state.hpp"

#include <optional>
#include <string>

namespace p3p3ds::hle {
namespace {

std::uint32_t u(std::int32_t value) { return static_cast<std::uint32_t>(value); }
std::int32_t s(std::uint32_t value) { return static_cast<std::int32_t>(value); }

std::optional<std::string> guest_name(const psprecomp::GuestMemory &memory, std::uint32_t address) {
    if (address == 0u || !memory.contains(address, 1u)) return std::nullopt;
    std::string name;
    for (std::uint32_t i = 0; i < 32u && memory.contains(address + i, 1u); ++i) {
        const char c = static_cast<char>(memory.load8(address + i));
        if (c == '\0') break;
        name += c;
    }
    return name;
}

void set_result(psprecomp::AllegrexContext &ctx, std::optional<std::int32_t> result) {
    if (result) ctx.set_gpr(2, u(*result));
}
void set_error(psprecomp::AllegrexContext &ctx, std::int32_t result) {
    if (result != 0) ctx.set_gpr(2, u(result));
}

} // namespace

void register_threadman_for_user(psprecomp::Runtime &runtime, KernelState &kernel) {
    register_thread_trampolines(runtime, kernel);
    auto &tm = kernel.threads();
    const auto reg = [&runtime](std::uint32_t nid, psprecomp::Runtime::HleFunction fn) {
        runtime.register_hle("ThreadManForUser", nid, std::move(fn));
    };

    // ---- Callbacks ----------------------------------------------------------
    reg(0xE81CAF8Fu, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelCreateCallback
        const auto name = guest_name(rt.memory(), ctx.gpr[4]);
        if (!name) { ctx.set_gpr(2, u(SCE_KERNEL_ERROR_ERROR)); return; }
        const auto uid = kernel.threads().create_callback(*name, ctx.gpr[5], ctx.gpr[6]);
        ctx.set_gpr(2, u(uid));
        rt.event("callback_create", {{"uid", u(uid)}, {"function", ctx.gpr[5]}, {"common", ctx.gpr[6]},
            {"owner", u(kernel.threads().current_thread_id())}, {"caller", ctx.gpr[31] - 8u}}, *name);
    });
    reg(0xEDBA5844u, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelDeleteCallback
        ctx.set_gpr(2, u(tm.delete_callback(s(ctx.gpr[4]))));
    });
    reg(0xC11BA8C4u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelNotifyCallback
        ctx.set_gpr(2, u(tm.notify_callback(rt, s(ctx.gpr[4]), ctx.gpr[5])));
    });
    reg(0x349D6D6Cu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelCheckCallback
        if (!tm.check_callbacks(rt, ctx)) ctx.set_gpr(2, 0u);
    });

    // ---- Threads ------------------------------------------------------------
    reg(0x446D8DE6u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelCreateThread
        const auto name = guest_name(rt.memory(), ctx.gpr[4]);
        if (!name) { ctx.set_gpr(2, u(SCE_KERNEL_ERROR_ERROR)); return; }
        const auto result = tm.create_thread(*name, ctx.gpr[5], s(ctx.gpr[6]), ctx.gpr[7], ctx.gpr[8], ctx.gpr[9], rt.memory());
        ctx.set_gpr(2, u(result));
        rt.event("thread_create", {{"uid", u(result)}, {"entry", ctx.gpr[5]}}, *name);
    });
    reg(0xF475845Du, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelStartThread
        const auto thid = s(ctx.gpr[4]);
        const auto result = tm.start_thread(thid, ctx.gpr[5], ctx.gpr[6], rt.memory(), ctx);
        rt.event("thread_start", {{"uid", u(thid)}, {"result", u(result)}});
        rt.event("thread_switch", {{"uid", u(tm.current_thread_id())}, {"entry", ctx.pc}});
        if (auto proof = tm.verified_thread_entry(ctx, psprecomp::runtime_thread_uid()))
            rt.event("thread_entry_transfer", {{"uid", u(proof->uid)}, {"entry", proof->entry_pc},
                {"from_uid", u(proof->from_uid)}, {"started", 1u}}, tm.get_thread(proof->uid)->name);
        if (result < 0) ctx.set_gpr(2, u(result));
    });
    reg(0xAA73C935u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelExitThread
        tm.exit_current_thread(s(ctx.gpr[4]), ctx, rt);
    });
    reg(0x9FA03CD3u, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelDeleteThread
        ctx.set_gpr(2, u(tm.delete_thread(s(ctx.gpr[4]))));
    });
    reg(0x278C0DF5u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelWaitThreadEnd
        set_result(ctx, tm.wait_thread_end(rt, ctx, s(ctx.gpr[4]), ctx.gpr[5], false));
    });
    reg(0x840E8133u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelWaitThreadEndCB
        set_result(ctx, tm.wait_thread_end(rt, ctx, s(ctx.gpr[4]), ctx.gpr[5], true));
    });
    reg(0x9ACE131Eu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelSleepThread
        set_result(ctx, tm.sleep_current(rt, ctx, false));
    });
    reg(0x82826F70u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelSleepThreadCB
        set_result(ctx, tm.sleep_current(rt, ctx, true));
    });
    reg(0xD59EAD2Fu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelWakeupThread
        set_error(ctx, tm.wakeup_thread(rt, ctx, s(ctx.gpr[4])));
    });
    reg(0x9944F31Fu, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelSuspendThread
        ctx.set_gpr(2, u(tm.suspend_thread(s(ctx.gpr[4]))));
    });
    reg(0x75156E8Fu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelResumeThread
        set_error(ctx, tm.resume_thread(rt, ctx, s(ctx.gpr[4])));
    });
    reg(0x71BC9871u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelChangeThreadPriority
        set_error(ctx, tm.change_priority(rt, ctx, s(ctx.gpr[4]), s(ctx.gpr[5])));
    });
    reg(0x17C1684Eu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelReferThreadStatus
        ctx.set_gpr(2, u(tm.refer_thread_status(rt.memory(), s(ctx.gpr[4]), ctx.gpr[5])));
    });
    reg(0x94AA61EEu, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelGetThreadCurrentPriority
        const auto *t = tm.current_thread();
        ctx.set_gpr(2, t != nullptr ? u(t->current_priority) : u(SCE_KERNEL_ERROR_ILLEGAL_THID));
    });
    reg(0x293B45B8u, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelGetThreadId
        ctx.set_gpr(2, u(tm.current_thread_id()));
    });
    reg(0xEA748E31u, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelChangeCurrentThreadAttr
        ctx.set_gpr(2, u(tm.change_current_thread_attr(ctx.gpr[4], ctx.gpr[5])));
    });

    // ---- Time -----------------------------------------------------------------
    reg(0xCEADEB47u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelDelayThread
        set_result(ctx, tm.delay_current(rt, ctx, ctx.gpr[4], false));
    });
    reg(0x68DA9E36u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelDelayThreadCB
        set_result(ctx, tm.delay_current(rt, ctx, ctx.gpr[4], true));
    });
    reg(0x369ED59Du, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelGetSystemTimeLow
        ctx.set_gpr(2, static_cast<std::uint32_t>(tm.now()));
    });
    reg(0x82BC5777u, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelGetSystemTimeWide
        ctx.set_gpr(2, static_cast<std::uint32_t>(tm.now()));
        ctx.set_gpr(3, static_cast<std::uint32_t>(tm.now() >> 32u));
    });

    // ---- Semaphores -------------------------------------------------------------
    reg(0xD6DA4BA1u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelCreateSema
        const auto name = guest_name(rt.memory(), ctx.gpr[4]);
        if (!name) { ctx.set_gpr(2, u(SCE_KERNEL_ERROR_ERROR)); return; }
        ctx.set_gpr(2, u(tm.create_sema(*name, ctx.gpr[5], s(ctx.gpr[6]), s(ctx.gpr[7]))));
    });
    reg(0x28B6489Cu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelDeleteSema
        set_error(ctx, tm.delete_sema(rt, ctx, s(ctx.gpr[4])));
    });
    reg(0x3F53E640u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelSignalSema
        set_error(ctx, tm.signal_sema(rt, ctx, s(ctx.gpr[4]), s(ctx.gpr[5])));
    });
    reg(0x4E3A1105u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelWaitSema
        set_result(ctx, tm.wait_sema(rt, ctx, s(ctx.gpr[4]), s(ctx.gpr[5]), ctx.gpr[6], false));
    });
    reg(0x6D212BACu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelWaitSemaCB
        set_result(ctx, tm.wait_sema(rt, ctx, s(ctx.gpr[4]), s(ctx.gpr[5]), ctx.gpr[6], true));
    });
    reg(0x58B1F937u, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelPollSema
        ctx.set_gpr(2, u(tm.poll_sema(s(ctx.gpr[4]), s(ctx.gpr[5]))));
    });

    // ---- Event flags -------------------------------------------------------------
    reg(0x55C20A00u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelCreateEventFlag
        const auto name = guest_name(rt.memory(), ctx.gpr[4]);
        if (!name) { ctx.set_gpr(2, u(SCE_KERNEL_ERROR_ERROR)); return; }
        ctx.set_gpr(2, u(tm.create_event_flag(*name, ctx.gpr[5], ctx.gpr[6])));
    });
    reg(0xEF9E4C70u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelDeleteEventFlag
        set_error(ctx, tm.delete_event_flag(rt, ctx, s(ctx.gpr[4])));
    });
    reg(0x1FB15A32u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelSetEventFlag
        set_error(ctx, tm.set_event_flag(rt, ctx, s(ctx.gpr[4]), ctx.gpr[5]));
    });
    reg(0x812346E4u, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelClearEventFlag
        ctx.set_gpr(2, u(tm.clear_event_flag(s(ctx.gpr[4]), ctx.gpr[5])));
    });
    reg(0x402FCF22u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelWaitEventFlag
        set_result(ctx, tm.wait_event_flag(rt, ctx, s(ctx.gpr[4]), ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], false));
    });
    reg(0x328C546Au, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelWaitEventFlagCB
        set_result(ctx, tm.wait_event_flag(rt, ctx, s(ctx.gpr[4]), ctx.gpr[5], ctx.gpr[6], ctx.gpr[7], ctx.gpr[8], true));
    });
    reg(0x30FD7D3Au, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelPollEventFlag
        ctx.set_gpr(2, u(tm.poll_event_flag(rt.memory(), s(ctx.gpr[4]), ctx.gpr[5], ctx.gpr[6], ctx.gpr[7])));
    });

    // ---- Mutexes -------------------------------------------------------------------
    reg(0xB7D098C6u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelCreateMutex
        const auto name = guest_name(rt.memory(), ctx.gpr[4]);
        if (!name) { ctx.set_gpr(2, u(SCE_KERNEL_ERROR_ERROR)); return; }
        ctx.set_gpr(2, u(tm.create_mutex(*name, ctx.gpr[5], s(ctx.gpr[6]))));
    });
    reg(0xF8170FBEu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelDeleteMutex
        set_error(ctx, tm.delete_mutex(rt, ctx, s(ctx.gpr[4])));
    });
    reg(0xB011B11Fu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelLockMutex
        set_result(ctx, tm.lock_mutex(rt, ctx, s(ctx.gpr[4]), s(ctx.gpr[5]), ctx.gpr[6], false));
    });
    reg(0x5BF4DD27u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelLockMutexCB
        set_result(ctx, tm.lock_mutex(rt, ctx, s(ctx.gpr[4]), s(ctx.gpr[5]), ctx.gpr[6], true));
    });
    reg(0x6B30100Fu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelUnlockMutex
        set_error(ctx, tm.unlock_mutex(rt, ctx, s(ctx.gpr[4]), s(ctx.gpr[5])));
    });

    // ---- LwMutex -------------------------------------------------------------------
    reg(0x19CFF145u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelCreateLwMutex
        const auto name = guest_name(rt.memory(), ctx.gpr[5]);
        if (!name) { ctx.set_gpr(2, u(SCE_KERNEL_ERROR_ERROR)); return; }
        ctx.set_gpr(2, u(tm.create_lw_mutex(rt.memory(), ctx.gpr[4], *name, ctx.gpr[6], s(ctx.gpr[7]))));
    });
    reg(0x60107536u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelDeleteLwMutex
        set_error(ctx, tm.delete_lw_mutex(rt, ctx, ctx.gpr[4]));
    });
    const auto lib = [&runtime](std::uint32_t nid, psprecomp::Runtime::HleFunction fn) {
        runtime.register_hle("Kernel_Library", nid, std::move(fn));
    };
    lib(0xBEA46419u, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelLockLwMutex
        // uOFW usersystemlib/lwmutex.c: a blocking lock with interrupts suspended fails.
        if (!kernel.interrupts_enabled()) { ctx.set_gpr(2, u(SCE_KERNEL_ERROR_CAN_NOT_WAIT)); return; }
        set_result(ctx, kernel.threads().lock_lw_mutex(rt, ctx, ctx.gpr[4], s(ctx.gpr[5]), ctx.gpr[6], false));
    });
    lib(0x1FC64E09u, [&kernel](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelLockLwMutexCB
        if (!kernel.interrupts_enabled()) { ctx.set_gpr(2, u(SCE_KERNEL_ERROR_CAN_NOT_WAIT)); return; }
        set_result(ctx, kernel.threads().lock_lw_mutex(rt, ctx, ctx.gpr[4], s(ctx.gpr[5]), ctx.gpr[6], true));
    });
    lib(0xDC692EE3u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelTryLockLwMutex
        const auto result = tm.try_lock_lw_mutex(rt.memory(), ctx.gpr[4], s(ctx.gpr[5]));
        // uOFW: the pre-6.00 entry collapses every failure to MUTEX_LOCKED.
        ctx.set_gpr(2, result == 0 ? 0u : u(SCE_KERNEL_ERROR_MUTEX_LOCKED));
    });
    lib(0x37431849u, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelTryLockLwMutex_600
        ctx.set_gpr(2, u(tm.try_lock_lw_mutex(rt.memory(), ctx.gpr[4], s(ctx.gpr[5]))));
    });
    lib(0x15B6446Bu, [&tm](psprecomp::Runtime &rt, psprecomp::AllegrexContext &ctx) { // sceKernelUnlockLwMutex
        set_error(ctx, tm.unlock_lw_mutex(rt, ctx, ctx.gpr[4], s(ctx.gpr[5])));
    });
    lib(0x293B45B8u, [&tm](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelGetThreadId
        ctx.set_gpr(2, u(tm.current_thread_id()));
    });
    // Interrupt state: uOFW interruptman returns the previous state and
    // sceKernelCpuResumeIntr restores it. Only the enabled bit is modelled.
    lib(0x092968F4u, [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelCpuSuspendIntr
        ctx.set_gpr(2, kernel.interrupts_enabled() ? 1u : 0u);
        kernel.set_interrupts_enabled(false);
    });
    lib(0x5F10D406u, [&kernel](psprecomp::Runtime &, psprecomp::AllegrexContext &ctx) { // sceKernelCpuResumeIntr
        kernel.set_interrupts_enabled(ctx.gpr[4] != 0u);
        ctx.set_gpr(2, 0u);
    });
}

} // namespace p3p3ds::hle
