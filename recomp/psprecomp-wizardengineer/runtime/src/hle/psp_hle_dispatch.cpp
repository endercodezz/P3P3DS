#include "hle/psp_hle.h"
#include "hle/psp_hle_imports.h"
#include "psp_scheduler.h"
#include "recomp.h"

#include <array>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

// ---- HLE Function Table ----
// Maps NID function name -> HLE implementation.
// Populated by per-module registration functions during init.
struct HleEntry {
    HleFunc func;
    const char* name;
};

static std::unordered_map<std::string, HleEntry> g_hle_by_name;

// ---- HLE Trace ----
// Controlled by PSPRECOMP_HLE_TRACE=1 environment variable.
// When enabled, every HLE call logs thread name and function name.
bool g_hle_trace_enabled = false;

// Trace wrapper slot table: each slot stores the original function
// pointer and name. Macro-generated wrapper functions index into
// this table by compile-time slot number.
static constexpr int HLE_TRACE_MAX_SLOTS = 256;

struct HleTraceSlot {
    HleFunc original;
    const char* name;
};

static HleTraceSlot g_trace_slots[HLE_TRACE_MAX_SLOTS];
static int g_trace_slot_count = 0;

// Trace wrapper implementation: logs thread name, function name,
// argument registers (a0-a3), optional a0 string, then calls the
// original function and logs the return value (v0).
static void hle_trace_call(int slot, uint8_t* rdram,
                           recomp_context* ctx) {
    const auto& s = g_trace_slots[slot];
    PspThread* t = psp_get_current_thread();

    // Try to read a0 as a string if it looks like a valid PSP pointer
    const char* a0_str = "";
    char a0_buf[128] = {0};
    uint32_t a0 = ctx->r[4];
    uint32_t a0_masked = a0 & 0x07FFFFFFU;
    if (a0_masked >= 0x00800000U && a0_masked < 0x08000000U) {
        const char* ptr =
            reinterpret_cast<const char*>(rdram + a0_masked);
        // Only include if it looks like printable ASCII
        if (ptr[0] >= 0x20 && ptr[0] < 0x7F) {
            snprintf(a0_buf, sizeof(a0_buf),
                     " a0_str=\"%.64s\"", ptr);
            a0_str = a0_buf;
        }
    }

    std::fprintf(stderr,
        "[HLE-TRACE] %s: %s(a0=0x%08X, a1=0x%08X, "
        "a2=0x%08X, a3=0x%08X%s)\n",
        t ? t->name : "boot", s.name,
        ctx->r[4], ctx->r[5], ctx->r[6], ctx->r[7],
        a0_str);

    // Call original function
    s.original(rdram, ctx);

    // Log return value
    std::fprintf(stderr,
        "[HLE-TRACE] %s: %s -> v0=0x%08X\n",
        t ? t->name : "boot", s.name,
        ctx->r[2]);
}

// Macro-generated wrapper functions (256 slots).
// Each wrapper calls hle_trace_call with its compile-time slot index.
#define TRACE_WRAPPER(N) \
    static void hle_trace_wrapper_##N(uint8_t* rdram, \
                                      recomp_context* ctx) { \
        hle_trace_call(N, rdram, ctx); \
    }

// Generate 256 wrapper functions
TRACE_WRAPPER(0)   TRACE_WRAPPER(1)   TRACE_WRAPPER(2)
TRACE_WRAPPER(3)   TRACE_WRAPPER(4)   TRACE_WRAPPER(5)
TRACE_WRAPPER(6)   TRACE_WRAPPER(7)   TRACE_WRAPPER(8)
TRACE_WRAPPER(9)   TRACE_WRAPPER(10)  TRACE_WRAPPER(11)
TRACE_WRAPPER(12)  TRACE_WRAPPER(13)  TRACE_WRAPPER(14)
TRACE_WRAPPER(15)  TRACE_WRAPPER(16)  TRACE_WRAPPER(17)
TRACE_WRAPPER(18)  TRACE_WRAPPER(19)  TRACE_WRAPPER(20)
TRACE_WRAPPER(21)  TRACE_WRAPPER(22)  TRACE_WRAPPER(23)
TRACE_WRAPPER(24)  TRACE_WRAPPER(25)  TRACE_WRAPPER(26)
TRACE_WRAPPER(27)  TRACE_WRAPPER(28)  TRACE_WRAPPER(29)
TRACE_WRAPPER(30)  TRACE_WRAPPER(31)  TRACE_WRAPPER(32)
TRACE_WRAPPER(33)  TRACE_WRAPPER(34)  TRACE_WRAPPER(35)
TRACE_WRAPPER(36)  TRACE_WRAPPER(37)  TRACE_WRAPPER(38)
TRACE_WRAPPER(39)  TRACE_WRAPPER(40)  TRACE_WRAPPER(41)
TRACE_WRAPPER(42)  TRACE_WRAPPER(43)  TRACE_WRAPPER(44)
TRACE_WRAPPER(45)  TRACE_WRAPPER(46)  TRACE_WRAPPER(47)
TRACE_WRAPPER(48)  TRACE_WRAPPER(49)  TRACE_WRAPPER(50)
TRACE_WRAPPER(51)  TRACE_WRAPPER(52)  TRACE_WRAPPER(53)
TRACE_WRAPPER(54)  TRACE_WRAPPER(55)  TRACE_WRAPPER(56)
TRACE_WRAPPER(57)  TRACE_WRAPPER(58)  TRACE_WRAPPER(59)
TRACE_WRAPPER(60)  TRACE_WRAPPER(61)  TRACE_WRAPPER(62)
TRACE_WRAPPER(63)  TRACE_WRAPPER(64)  TRACE_WRAPPER(65)
TRACE_WRAPPER(66)  TRACE_WRAPPER(67)  TRACE_WRAPPER(68)
TRACE_WRAPPER(69)  TRACE_WRAPPER(70)  TRACE_WRAPPER(71)
TRACE_WRAPPER(72)  TRACE_WRAPPER(73)  TRACE_WRAPPER(74)
TRACE_WRAPPER(75)  TRACE_WRAPPER(76)  TRACE_WRAPPER(77)
TRACE_WRAPPER(78)  TRACE_WRAPPER(79)  TRACE_WRAPPER(80)
TRACE_WRAPPER(81)  TRACE_WRAPPER(82)  TRACE_WRAPPER(83)
TRACE_WRAPPER(84)  TRACE_WRAPPER(85)  TRACE_WRAPPER(86)
TRACE_WRAPPER(87)  TRACE_WRAPPER(88)  TRACE_WRAPPER(89)
TRACE_WRAPPER(90)  TRACE_WRAPPER(91)  TRACE_WRAPPER(92)
TRACE_WRAPPER(93)  TRACE_WRAPPER(94)  TRACE_WRAPPER(95)
TRACE_WRAPPER(96)  TRACE_WRAPPER(97)  TRACE_WRAPPER(98)
TRACE_WRAPPER(99)  TRACE_WRAPPER(100) TRACE_WRAPPER(101)
TRACE_WRAPPER(102) TRACE_WRAPPER(103) TRACE_WRAPPER(104)
TRACE_WRAPPER(105) TRACE_WRAPPER(106) TRACE_WRAPPER(107)
TRACE_WRAPPER(108) TRACE_WRAPPER(109) TRACE_WRAPPER(110)
TRACE_WRAPPER(111) TRACE_WRAPPER(112) TRACE_WRAPPER(113)
TRACE_WRAPPER(114) TRACE_WRAPPER(115) TRACE_WRAPPER(116)
TRACE_WRAPPER(117) TRACE_WRAPPER(118) TRACE_WRAPPER(119)
TRACE_WRAPPER(120) TRACE_WRAPPER(121) TRACE_WRAPPER(122)
TRACE_WRAPPER(123) TRACE_WRAPPER(124) TRACE_WRAPPER(125)
TRACE_WRAPPER(126) TRACE_WRAPPER(127) TRACE_WRAPPER(128)
TRACE_WRAPPER(129) TRACE_WRAPPER(130) TRACE_WRAPPER(131)
TRACE_WRAPPER(132) TRACE_WRAPPER(133) TRACE_WRAPPER(134)
TRACE_WRAPPER(135) TRACE_WRAPPER(136) TRACE_WRAPPER(137)
TRACE_WRAPPER(138) TRACE_WRAPPER(139) TRACE_WRAPPER(140)
TRACE_WRAPPER(141) TRACE_WRAPPER(142) TRACE_WRAPPER(143)
TRACE_WRAPPER(144) TRACE_WRAPPER(145) TRACE_WRAPPER(146)
TRACE_WRAPPER(147) TRACE_WRAPPER(148) TRACE_WRAPPER(149)
TRACE_WRAPPER(150) TRACE_WRAPPER(151) TRACE_WRAPPER(152)
TRACE_WRAPPER(153) TRACE_WRAPPER(154) TRACE_WRAPPER(155)
TRACE_WRAPPER(156) TRACE_WRAPPER(157) TRACE_WRAPPER(158)
TRACE_WRAPPER(159) TRACE_WRAPPER(160) TRACE_WRAPPER(161)
TRACE_WRAPPER(162) TRACE_WRAPPER(163) TRACE_WRAPPER(164)
TRACE_WRAPPER(165) TRACE_WRAPPER(166) TRACE_WRAPPER(167)
TRACE_WRAPPER(168) TRACE_WRAPPER(169) TRACE_WRAPPER(170)
TRACE_WRAPPER(171) TRACE_WRAPPER(172) TRACE_WRAPPER(173)
TRACE_WRAPPER(174) TRACE_WRAPPER(175) TRACE_WRAPPER(176)
TRACE_WRAPPER(177) TRACE_WRAPPER(178) TRACE_WRAPPER(179)
TRACE_WRAPPER(180) TRACE_WRAPPER(181) TRACE_WRAPPER(182)
TRACE_WRAPPER(183) TRACE_WRAPPER(184) TRACE_WRAPPER(185)
TRACE_WRAPPER(186) TRACE_WRAPPER(187) TRACE_WRAPPER(188)
TRACE_WRAPPER(189) TRACE_WRAPPER(190) TRACE_WRAPPER(191)
TRACE_WRAPPER(192) TRACE_WRAPPER(193) TRACE_WRAPPER(194)
TRACE_WRAPPER(195) TRACE_WRAPPER(196) TRACE_WRAPPER(197)
TRACE_WRAPPER(198) TRACE_WRAPPER(199) TRACE_WRAPPER(200)
TRACE_WRAPPER(201) TRACE_WRAPPER(202) TRACE_WRAPPER(203)
TRACE_WRAPPER(204) TRACE_WRAPPER(205) TRACE_WRAPPER(206)
TRACE_WRAPPER(207) TRACE_WRAPPER(208) TRACE_WRAPPER(209)
TRACE_WRAPPER(210) TRACE_WRAPPER(211) TRACE_WRAPPER(212)
TRACE_WRAPPER(213) TRACE_WRAPPER(214) TRACE_WRAPPER(215)
TRACE_WRAPPER(216) TRACE_WRAPPER(217) TRACE_WRAPPER(218)
TRACE_WRAPPER(219) TRACE_WRAPPER(220) TRACE_WRAPPER(221)
TRACE_WRAPPER(222) TRACE_WRAPPER(223) TRACE_WRAPPER(224)
TRACE_WRAPPER(225) TRACE_WRAPPER(226) TRACE_WRAPPER(227)
TRACE_WRAPPER(228) TRACE_WRAPPER(229) TRACE_WRAPPER(230)
TRACE_WRAPPER(231) TRACE_WRAPPER(232) TRACE_WRAPPER(233)
TRACE_WRAPPER(234) TRACE_WRAPPER(235) TRACE_WRAPPER(236)
TRACE_WRAPPER(237) TRACE_WRAPPER(238) TRACE_WRAPPER(239)
TRACE_WRAPPER(240) TRACE_WRAPPER(241) TRACE_WRAPPER(242)
TRACE_WRAPPER(243) TRACE_WRAPPER(244) TRACE_WRAPPER(245)
TRACE_WRAPPER(246) TRACE_WRAPPER(247) TRACE_WRAPPER(248)
TRACE_WRAPPER(249) TRACE_WRAPPER(250) TRACE_WRAPPER(251)
TRACE_WRAPPER(252) TRACE_WRAPPER(253) TRACE_WRAPPER(254)
TRACE_WRAPPER(255)

// Lookup table of wrapper function pointers, indexed by slot number.
static HleFunc g_trace_wrappers[HLE_TRACE_MAX_SLOTS] = {
    hle_trace_wrapper_0,   hle_trace_wrapper_1,
    hle_trace_wrapper_2,   hle_trace_wrapper_3,
    hle_trace_wrapper_4,   hle_trace_wrapper_5,
    hle_trace_wrapper_6,   hle_trace_wrapper_7,
    hle_trace_wrapper_8,   hle_trace_wrapper_9,
    hle_trace_wrapper_10,  hle_trace_wrapper_11,
    hle_trace_wrapper_12,  hle_trace_wrapper_13,
    hle_trace_wrapper_14,  hle_trace_wrapper_15,
    hle_trace_wrapper_16,  hle_trace_wrapper_17,
    hle_trace_wrapper_18,  hle_trace_wrapper_19,
    hle_trace_wrapper_20,  hle_trace_wrapper_21,
    hle_trace_wrapper_22,  hle_trace_wrapper_23,
    hle_trace_wrapper_24,  hle_trace_wrapper_25,
    hle_trace_wrapper_26,  hle_trace_wrapper_27,
    hle_trace_wrapper_28,  hle_trace_wrapper_29,
    hle_trace_wrapper_30,  hle_trace_wrapper_31,
    hle_trace_wrapper_32,  hle_trace_wrapper_33,
    hle_trace_wrapper_34,  hle_trace_wrapper_35,
    hle_trace_wrapper_36,  hle_trace_wrapper_37,
    hle_trace_wrapper_38,  hle_trace_wrapper_39,
    hle_trace_wrapper_40,  hle_trace_wrapper_41,
    hle_trace_wrapper_42,  hle_trace_wrapper_43,
    hle_trace_wrapper_44,  hle_trace_wrapper_45,
    hle_trace_wrapper_46,  hle_trace_wrapper_47,
    hle_trace_wrapper_48,  hle_trace_wrapper_49,
    hle_trace_wrapper_50,  hle_trace_wrapper_51,
    hle_trace_wrapper_52,  hle_trace_wrapper_53,
    hle_trace_wrapper_54,  hle_trace_wrapper_55,
    hle_trace_wrapper_56,  hle_trace_wrapper_57,
    hle_trace_wrapper_58,  hle_trace_wrapper_59,
    hle_trace_wrapper_60,  hle_trace_wrapper_61,
    hle_trace_wrapper_62,  hle_trace_wrapper_63,
    hle_trace_wrapper_64,  hle_trace_wrapper_65,
    hle_trace_wrapper_66,  hle_trace_wrapper_67,
    hle_trace_wrapper_68,  hle_trace_wrapper_69,
    hle_trace_wrapper_70,  hle_trace_wrapper_71,
    hle_trace_wrapper_72,  hle_trace_wrapper_73,
    hle_trace_wrapper_74,  hle_trace_wrapper_75,
    hle_trace_wrapper_76,  hle_trace_wrapper_77,
    hle_trace_wrapper_78,  hle_trace_wrapper_79,
    hle_trace_wrapper_80,  hle_trace_wrapper_81,
    hle_trace_wrapper_82,  hle_trace_wrapper_83,
    hle_trace_wrapper_84,  hle_trace_wrapper_85,
    hle_trace_wrapper_86,  hle_trace_wrapper_87,
    hle_trace_wrapper_88,  hle_trace_wrapper_89,
    hle_trace_wrapper_90,  hle_trace_wrapper_91,
    hle_trace_wrapper_92,  hle_trace_wrapper_93,
    hle_trace_wrapper_94,  hle_trace_wrapper_95,
    hle_trace_wrapper_96,  hle_trace_wrapper_97,
    hle_trace_wrapper_98,  hle_trace_wrapper_99,
    hle_trace_wrapper_100, hle_trace_wrapper_101,
    hle_trace_wrapper_102, hle_trace_wrapper_103,
    hle_trace_wrapper_104, hle_trace_wrapper_105,
    hle_trace_wrapper_106, hle_trace_wrapper_107,
    hle_trace_wrapper_108, hle_trace_wrapper_109,
    hle_trace_wrapper_110, hle_trace_wrapper_111,
    hle_trace_wrapper_112, hle_trace_wrapper_113,
    hle_trace_wrapper_114, hle_trace_wrapper_115,
    hle_trace_wrapper_116, hle_trace_wrapper_117,
    hle_trace_wrapper_118, hle_trace_wrapper_119,
    hle_trace_wrapper_120, hle_trace_wrapper_121,
    hle_trace_wrapper_122, hle_trace_wrapper_123,
    hle_trace_wrapper_124, hle_trace_wrapper_125,
    hle_trace_wrapper_126, hle_trace_wrapper_127,
    hle_trace_wrapper_128, hle_trace_wrapper_129,
    hle_trace_wrapper_130, hle_trace_wrapper_131,
    hle_trace_wrapper_132, hle_trace_wrapper_133,
    hle_trace_wrapper_134, hle_trace_wrapper_135,
    hle_trace_wrapper_136, hle_trace_wrapper_137,
    hle_trace_wrapper_138, hle_trace_wrapper_139,
    hle_trace_wrapper_140, hle_trace_wrapper_141,
    hle_trace_wrapper_142, hle_trace_wrapper_143,
    hle_trace_wrapper_144, hle_trace_wrapper_145,
    hle_trace_wrapper_146, hle_trace_wrapper_147,
    hle_trace_wrapper_148, hle_trace_wrapper_149,
    hle_trace_wrapper_150, hle_trace_wrapper_151,
    hle_trace_wrapper_152, hle_trace_wrapper_153,
    hle_trace_wrapper_154, hle_trace_wrapper_155,
    hle_trace_wrapper_156, hle_trace_wrapper_157,
    hle_trace_wrapper_158, hle_trace_wrapper_159,
    hle_trace_wrapper_160, hle_trace_wrapper_161,
    hle_trace_wrapper_162, hle_trace_wrapper_163,
    hle_trace_wrapper_164, hle_trace_wrapper_165,
    hle_trace_wrapper_166, hle_trace_wrapper_167,
    hle_trace_wrapper_168, hle_trace_wrapper_169,
    hle_trace_wrapper_170, hle_trace_wrapper_171,
    hle_trace_wrapper_172, hle_trace_wrapper_173,
    hle_trace_wrapper_174, hle_trace_wrapper_175,
    hle_trace_wrapper_176, hle_trace_wrapper_177,
    hle_trace_wrapper_178, hle_trace_wrapper_179,
    hle_trace_wrapper_180, hle_trace_wrapper_181,
    hle_trace_wrapper_182, hle_trace_wrapper_183,
    hle_trace_wrapper_184, hle_trace_wrapper_185,
    hle_trace_wrapper_186, hle_trace_wrapper_187,
    hle_trace_wrapper_188, hle_trace_wrapper_189,
    hle_trace_wrapper_190, hle_trace_wrapper_191,
    hle_trace_wrapper_192, hle_trace_wrapper_193,
    hle_trace_wrapper_194, hle_trace_wrapper_195,
    hle_trace_wrapper_196, hle_trace_wrapper_197,
    hle_trace_wrapper_198, hle_trace_wrapper_199,
    hle_trace_wrapper_200, hle_trace_wrapper_201,
    hle_trace_wrapper_202, hle_trace_wrapper_203,
    hle_trace_wrapper_204, hle_trace_wrapper_205,
    hle_trace_wrapper_206, hle_trace_wrapper_207,
    hle_trace_wrapper_208, hle_trace_wrapper_209,
    hle_trace_wrapper_210, hle_trace_wrapper_211,
    hle_trace_wrapper_212, hle_trace_wrapper_213,
    hle_trace_wrapper_214, hle_trace_wrapper_215,
    hle_trace_wrapper_216, hle_trace_wrapper_217,
    hle_trace_wrapper_218, hle_trace_wrapper_219,
    hle_trace_wrapper_220, hle_trace_wrapper_221,
    hle_trace_wrapper_222, hle_trace_wrapper_223,
    hle_trace_wrapper_224, hle_trace_wrapper_225,
    hle_trace_wrapper_226, hle_trace_wrapper_227,
    hle_trace_wrapper_228, hle_trace_wrapper_229,
    hle_trace_wrapper_230, hle_trace_wrapper_231,
    hle_trace_wrapper_232, hle_trace_wrapper_233,
    hle_trace_wrapper_234, hle_trace_wrapper_235,
    hle_trace_wrapper_236, hle_trace_wrapper_237,
    hle_trace_wrapper_238, hle_trace_wrapper_239,
    hle_trace_wrapper_240, hle_trace_wrapper_241,
    hle_trace_wrapper_242, hle_trace_wrapper_243,
    hle_trace_wrapper_244, hle_trace_wrapper_245,
    hle_trace_wrapper_246, hle_trace_wrapper_247,
    hle_trace_wrapper_248, hle_trace_wrapper_249,
    hle_trace_wrapper_250, hle_trace_wrapper_251,
    hle_trace_wrapper_252, hle_trace_wrapper_253,
    hle_trace_wrapper_254, hle_trace_wrapper_255,
};

/// Allocate a trace wrapper slot for the given function.
/// Returns the wrapper function pointer that logs + dispatches.
/// If slots exhausted, returns the original function (no tracing).
static HleFunc alloc_trace_wrapper(HleFunc original,
                                   const char* name) {
    if (g_trace_slot_count >= HLE_TRACE_MAX_SLOTS) {
        std::fprintf(stderr,
            "[HLE-TRACE] WARNING: trace slot overflow, "
            "'%s' will not be traced\n", name);
        return original;
    }
    int slot = g_trace_slot_count++;
    g_trace_slots[slot] = { original, name };
    return g_trace_wrappers[slot];
}

// ---- Unimplemented import stubs (issue #40) ----
// Every generated-table stub whose name has no registered HLE handler is
// bound to a loud one-shot logger instead of being left as the emitted
// silent no-op. Identity comes from a compile-time slot (same trick as the
// trace wrappers above, via template instantiation instead of macros).
static constexpr int HLE_UNIMPL_MAX_SLOTS = 512;

struct HleUnimplSlot {
    const RecompNidStub* stub;
    bool logged;
};

static HleUnimplSlot g_unimpl_slots[HLE_UNIMPL_MAX_SLOTS];
static int g_unimpl_slot_count = 0;

static void hle_unimpl_call(int slot, uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    HleUnimplSlot& s = g_unimpl_slots[slot];
    if (!s.logged) {
        s.logged = true;
        std::fprintf(stderr,
            "[HLE] UNIMPLEMENTED import %s called "
            "(NID 0x%08X, module %s, stub 0x%08X) — no-op returning 0\n",
            s.stub->func_name, s.stub->nid, s.stub->module_name,
            s.stub->stub_addr);
    }
    // Deterministic return: v0 = 0 (SCE_OK). The raw stub body
    // ("jr $ra; nop") leaves whatever was in r2 — every caller that
    // consumes the result then branches on garbage (e.g. .hack//Link's
    // main wait loop branching on sceKernelGetThreadCurrentPriority,
    // L5 investigation). 0 is the least-surprise PSP success value.
    ctx->r[2] = SCE_OK;
}

template <int N>
static void hle_unimpl_wrapper(uint8_t* rdram, recomp_context* ctx) {
    hle_unimpl_call(N, rdram, ctx);
}

template <int... Ns>
static constexpr std::array<HleFunc, sizeof...(Ns)>
make_unimpl_wrappers(std::integer_sequence<int, Ns...>) {
    return { &hle_unimpl_wrapper<Ns>... };
}

static constexpr auto g_unimpl_wrappers =
    make_unimpl_wrappers(std::make_integer_sequence<int, HLE_UNIMPL_MAX_SLOTS>{});

// Shared overflow fallback: still loud, just not per-stub-identified.
static void hle_unimpl_overflow(uint8_t* rdram, recomp_context* ctx) {
    (void)rdram;
    static bool logged = false;
    if (!logged) {
        logged = true;
        std::fprintf(stderr,
            "[HLE] UNIMPLEMENTED import called (slot table overflowed at %d; "
            "raise HLE_UNIMPL_MAX_SLOTS to identify it) — no-op returning 0\n",
            HLE_UNIMPL_MAX_SLOTS);
    }
    ctx->r[2] = SCE_OK;  // deterministic v0, same as hle_unimpl_call
}

/// Allocate a loud unimplemented-stub wrapper bound to `stub`.
static HleFunc alloc_unimpl_wrapper(const RecompNidStub* stub) {
    if (g_unimpl_slot_count >= HLE_UNIMPL_MAX_SLOTS) {
        return &hle_unimpl_overflow;
    }
    int slot = g_unimpl_slot_count++;
    g_unimpl_slots[slot] = { stub, false };
    return g_unimpl_wrappers[static_cast<size_t>(slot)];
}

// ---- Unimplemented Syscall Logging ----
static std::unordered_set<uint32_t> g_unimpl_syscall_logged;
static bool g_strict_mode_hle = false;
static bool g_strict_checked_hle = false;

static void check_strict_mode() {
    if (!g_strict_checked_hle) {
        const char* env = std::getenv("PSPRECOMP_STRICT");
        g_strict_mode_hle = (env && env[0] == '1');
        g_strict_checked_hle = true;
    }
}

// ---- Registration ----

void psp_hle_register(const char* nid_name, HleFunc fn) {
    g_hle_by_name[nid_name] = { fn, nid_name };
}

// ---- Init ----

void psp_hle_init() {
    check_strict_mode();

    // Check trace environment variable
    const char* trace_env = std::getenv("PSPRECOMP_HLE_TRACE");
    g_hle_trace_enabled = (trace_env && trace_env[0] == '1');

    // 1. Call per-module registration functions
    psp_hle_register_all_modules();

    std::fprintf(stderr, "[HLE] Registered %zu HLE functions\n",
                 g_hle_by_name.size());

    if (g_hle_trace_enabled) {
        std::fprintf(stderr,
            "[HLE] Trace mode ENABLED (PSPRECOMP_HLE_TRACE=1)\n");
    }

    // 2. Wire the generated import-stub table (issue #40: recomp_nid_stubs
    //    comes from <output>/syscall_table.cpp, emitted per-game from
    //    analysis.json imports[]) into the dispatch table. For each stub
    //    address, if an HLE function was registered by name, override the
    //    dispatch entry; otherwise bind a LOUD per-stub unimplemented
    //    handler that logs its NID name on first call (never silent).
    //    When tracing is enabled, wrap each implementation in a decorator.
    int implemented = 0;
    int unimplemented = 0;

    for (int i = 0; i < recomp_nid_stub_count; i++) {
        const RecompNidStub& stub = recomp_nid_stubs[i];
        auto it = g_hle_by_name.find(stub.func_name);

        if (it != g_hle_by_name.end()) {
            HleFunc fn = it->second.func;
            // Wrap in trace decorator if tracing is enabled
            if (g_hle_trace_enabled) {
                fn = alloc_trace_wrapper(fn, it->second.name);
            }
            // Register the (possibly wrapped) HLE function
            psp_dispatch_register(stub.stub_addr, fn);
            implemented++;
        } else {
            psp_dispatch_register(stub.stub_addr, alloc_unimpl_wrapper(&stub));
            unimplemented++;
        }
    }

    std::fprintf(stderr,
        "[HLE] Import stubs: %d/%d implemented, %d unimplemented\n",
        implemented, recomp_nid_stub_count, unimplemented);

    if (g_strict_mode_hle && unimplemented > 0) {
        std::fprintf(stderr,
            "[HLE] STRICT mode: %d unimplemented NIDs "
            "(loud no-op on first call)\n",
            unimplemented);
    }
}

// ---- Generated-table NID lookup (issue #40) ----
// Linear scan over recomp_nid_stubs, cached on first use. NIDs are
// PSP-API-universal constants, so runtime code may key on a NID (never on a
// per-game stub address) and resolve the current game's stub through here.
uint32_t psp_hle_stub_addr_for_nid(uint32_t nid) {
    static std::unordered_map<uint32_t, uint32_t> cache = [] {
        std::unordered_map<uint32_t, uint32_t> m;
        m.reserve(static_cast<size_t>(recomp_nid_stub_count));
        for (int i = 0; i < recomp_nid_stub_count; i++) {
            m.emplace(recomp_nid_stubs[i].nid, recomp_nid_stubs[i].stub_addr);
        }
        return m;
    }();
    auto it = cache.find(nid);
    return it != cache.end() ? it->second : 0;
}

// ---- Syscall Dispatch ----
// For actual SYSCALL instructions in the binary (not import stubs).
// Import stubs use the dispatch table override mechanism above.
// Actual syscall instructions are rare in Patapon (mostly debug traps with code=0).

void psp_hle_syscall(uint8_t* rdram, recomp_context* ctx, uint32_t code) {
    (void)rdram;

    // SYSCALL 0 = debug breakpoint, silently ignore
    if (code == 0) {
        return;
    }

    // Log unimplemented syscall codes (deduplicated)
    if (g_unimpl_syscall_logged.insert(code).second) {
        std::fprintf(stderr,
            "[HLE] UNIMPLEMENTED syscall code 0x%05X\n", code);
    }

    check_strict_mode();
    if (g_strict_mode_hle) {
        std::fprintf(stderr,
            "STRICT: aborting on unimplemented syscall 0x%05X\n", code);
        std::abort();
    }

    // Return SCE_OK by default
    ctx->r[2] = SCE_OK;
}

// ---- Module Registration ----
// Later plans (04-02 through 04-05) add real registration calls here.
// Each module provides a psp_hle_register_xxx() that calls psp_hle_register()
// for each NID it implements.

void psp_hle_register_all_modules() {
    // Kernel modules (04-02)
    psp_hle_register_kernel_thread();
    psp_hle_register_kernel_memory();

    // Kernel sync primitives (04-03)
    psp_hle_register_kernel_sema();
    psp_hle_register_kernel_mutex();
    psp_hle_register_kernel_lwmutex();
    psp_hle_register_kernel_eventflag();

    // File I/O (04-04)
    psp_hle_register_io();

    // Display, GE, power, ctrl, utility (04-05)
    psp_hle_register_display();
    psp_hle_register_ge();
    psp_hle_register_power();
    psp_hle_register_ctrl();
    psp_hle_register_utility();

    // SAS voice state machine (issue #29)
    psp_hle_register_sas();
}
