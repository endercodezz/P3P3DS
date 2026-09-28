// Unit tests for the Refer*Status guest-struct layouts and fill protocols
// (hle/psp_hle_refer_info.h) — the dothack-L6 fix. Oracle: PPSSPP
// Core/HLE/sceKernelThread.cpp:1125 and sceKernelEventFlag.cpp:521.
//
// Standalone executable (test_vfpu convention): no SDL/GL/scheduler deps —
// the header under test is pure layout/encoding logic.

#include "hle/psp_hle_refer_info.h"

#include <cstdio>
#include <cstring>
#include <vector>

static int failures = 0;
static int tests_run = 0;

#define ASSERT_U32_EQ(actual, expected, msg) \
    do { \
        tests_run++; \
        if (static_cast<uint32_t>(actual) != \
            static_cast<uint32_t>(expected)) { \
            std::fprintf(stderr, "FAIL: %s: got 0x%08X, expected 0x%08X\n", \
                msg, static_cast<uint32_t>(actual), \
                static_cast<uint32_t>(expected)); \
            failures++; \
        } \
    } while (0)

#define ASSERT_TRUE(cond, msg) \
    do { \
        tests_run++; \
        if (!(cond)) { \
            std::fprintf(stderr, "FAIL: %s\n", msg); \
            failures++; \
        } \
    } while (0)

// Guest addresses must be >= 0x10000 (recomp.h NULL-page semantics).
static constexpr uint32_t INFO = 0x00020000U;

static uint8_t* g_ram = nullptr;

static void poison(uint32_t addr, uint32_t len) {
    std::memset(g_ram + (addr & 0x07FFFFFFU), 0xCD, len);
}

static uint32_t rd32(uint32_t addr) {
    return psp_mem_read<uint32_t>(g_ram, addr);
}

static PspNativeThreadImage sample_thread_image() {
    PspNativeThreadImage img{};
    std::strncpy(img.name, "RootTh", sizeof(img.name) - 1);
    img.attr = 0x800000FFU;
    img.status = PSP_THREADSTATUS_WAIT;
    img.entry = 0x08865DA4U;
    img.stack = 0x09F00000U;
    img.stack_size = 0x4000U;
    img.gp = 0x08C70000U;
    img.init_priority = 32;
    img.current_priority = 17;
    img.wait_type = PSP_WAITTYPE_SLEEP;
    img.wait_id = 0;
    img.wakeup_count = 3;
    img.exit_status = PSP_ERROR_NOT_DORMANT;
    return img;
}

// ---- Test 1: internal->PSP status map (esp. the DEAD/WAIT==4 trap) ----
static void test_status_encoding_map() {
    struct Row { int internal; uint32_t status; uint32_t wait_type; };
    // Internal values pinned by static_assert in psp_hle_kernel_thread.cpp:
    // DORMANT=0 READY=1 RUNNING=2 WAIT=3 DEAD=4 WAIT_SLEEP=5.
    const Row rows[] = {
        {0, PSP_THREADSTATUS_DORMANT, PSP_WAITTYPE_NONE},   // DORMANT
        {1, PSP_THREADSTATUS_READY,   PSP_WAITTYPE_NONE},   // READY (PSP 2!)
        {2, PSP_THREADSTATUS_RUNNING, PSP_WAITTYPE_NONE},   // RUNNING (PSP 1!)
        {3, PSP_THREADSTATUS_WAIT,    PSP_WAITTYPE_NONE},   // WAIT
        {4, PSP_THREADSTATUS_DORMANT, PSP_WAITTYPE_NONE},   // DEAD -> dormant
        {5, PSP_THREADSTATUS_WAIT,    PSP_WAITTYPE_SLEEP},  // WAIT_SLEEP
    };
    for (const Row& r : rows) {
        PspThreadStatusEncoding e = psp_encode_thread_status(r.internal);
        ASSERT_U32_EQ(e.status, r.status, "status map row");
        ASSERT_U32_EQ(e.wait_type, r.wait_type, "waitType map row");
    }
    // The G1 trap spelled out: internal DEAD(4) must NOT pass through as
    // PSP WAIT(4); a sleeping worker must read as PSP WAIT(4)/SLEEP(1).
    ASSERT_TRUE(psp_encode_thread_status(4).status != 4,
                "internal DEAD must not leak as PSP WAIT");
    ASSERT_U32_EQ(psp_encode_thread_status(5).status, 4,
                  "WAIT_SLEEP reads as PSP WAIT(4)");
    ASSERT_U32_EQ(psp_encode_thread_status(5).wait_type, 1,
                  "WAIT_SLEEP reads as PSP_WAIT_SLEEP(1)");
}

// ---- Test 2: full field layout at wantedSize=108 (post-2.60 SDK) ----
static void test_thread_fill_size_108() {
    poison(INFO, 0x100);
    psp_mem_write<uint32_t>(g_ram, INFO, 108);  // caller's wantedSize
    int32_t rc = psp_write_thread_info(g_ram, INFO, true,
                                       sample_thread_image());
    ASSERT_U32_EQ(rc, 0, "size108: rc");
    ASSERT_U32_EQ(rd32(INFO + 0), 108, "size108: size field reads 108");
    char name[32];
    std::memcpy(name, g_ram + ((INFO + 4) & 0x07FFFFFFU), sizeof(name));
    ASSERT_TRUE(std::strcmp(name, "RootTh") == 0, "size108: name at +4");
    ASSERT_U32_EQ(rd32(INFO + 36), 0x800000FFU, "size108: attr at +36");
    ASSERT_U32_EQ(rd32(INFO + 40), 4, "size108: status at +40");
    ASSERT_U32_EQ(rd32(INFO + 44), 0x08865DA4U, "size108: entry at +44");
    ASSERT_U32_EQ(rd32(INFO + 48), 0x09F00000U, "size108: stack at +48");
    ASSERT_U32_EQ(rd32(INFO + 52), 0x4000U, "size108: stackSize at +52");
    ASSERT_U32_EQ(rd32(INFO + 56), 0x08C70000U, "size108: gp at +56");
    ASSERT_U32_EQ(rd32(INFO + 60), 32, "size108: initPriority at +60");
    ASSERT_U32_EQ(rd32(INFO + 64), 17, "size108: currentPriority at +64");
    ASSERT_U32_EQ(rd32(INFO + 68), 1, "size108: waitType at +68");
    ASSERT_U32_EQ(rd32(INFO + 72), 0, "size108: waitID at +72");
    ASSERT_U32_EQ(rd32(INFO + 76), 3, "size108: wakeupCount at +76");
    ASSERT_U32_EQ(rd32(INFO + 80), 0x800201A4U, "size108: exitStatus at +80");
    ASSERT_U32_EQ(rd32(INFO + 84), 0, "size108: runClocks.lo at +84");
    ASSERT_U32_EQ(rd32(INFO + 88), 0, "size108: runClocks.hi at +88");
    ASSERT_U32_EQ(rd32(INFO + 92), 0, "size108: intrPreempt at +92");
    ASSERT_U32_EQ(rd32(INFO + 96), 0, "size108: threadPreempt at +96");
    ASSERT_U32_EQ(rd32(INFO + 100), 0, "size108: releaseCount at +100");
    ASSERT_U32_EQ(rd32(INFO + 104), 0, "size108: bytes 104..108 zero-filled");
    ASSERT_U32_EQ(rd32(INFO + 108), 0xCDCDCDCDU,
                  "size108: nothing written past 108");
}

// ---- Test 3: wantedSize=104 protocol variants ----
static void test_thread_fill_size_104() {
    // Post-2.60 SDK: size field still reads back 108, copy clipped to 104,
    // no tail fill beyond wantedSize.
    poison(INFO, 0x100);
    psp_mem_write<uint32_t>(g_ram, INFO, 104);
    int32_t rc = psp_write_thread_info(g_ram, INFO, true,
                                       sample_thread_image());
    ASSERT_U32_EQ(rc, 0, "size104/new: rc");
    ASSERT_U32_EQ(rd32(INFO + 0), 108, "size104/new: size reads 108");
    ASSERT_U32_EQ(rd32(INFO + 100), 0, "size104/new: last field copied");
    ASSERT_U32_EQ(rd32(INFO + 104), 0xCDCDCDCDU,
                  "size104/new: nothing past 104");

    // Pre-2.60 SDK (or version never set): size field reads back 104.
    poison(INFO, 0x100);
    psp_mem_write<uint32_t>(g_ram, INFO, 104);
    rc = psp_write_thread_info(g_ram, INFO, false, sample_thread_image());
    ASSERT_U32_EQ(rc, 0, "size104/old: rc");
    ASSERT_U32_EQ(rd32(INFO + 0), 104, "size104/old: size reads 104");
    ASSERT_U32_EQ(rd32(INFO + 40), 4, "size104/old: status at +40");
}

// ---- Test 4: size edge cases ----
static void test_thread_fill_size_edges() {
    // wantedSize=0: nothing written at all.
    poison(INFO, 0x100);
    psp_mem_write<uint32_t>(g_ram, INFO, 0);
    int32_t rc = psp_write_thread_info(g_ram, INFO, true,
                                       sample_thread_image());
    ASSERT_U32_EQ(rc, 0, "size0: rc");
    ASSERT_U32_EQ(rd32(INFO + 4), 0xCDCDCDCDU, "size0: nothing written");

    // Post-2.60: wantedSize>108 -> ILLEGAL_SIZE, nothing written.
    poison(INFO, 0x100);
    psp_mem_write<uint32_t>(g_ram, INFO, 112);
    rc = psp_write_thread_info(g_ram, INFO, true, sample_thread_image());
    ASSERT_U32_EQ(rc, 0x800201BCU, "size112/new: ILLEGAL_SIZE");
    ASSERT_U32_EQ(rd32(INFO + 0), 112, "size112/new: size untouched");
    ASSERT_U32_EQ(rd32(INFO + 40), 0xCDCDCDCDU, "size112/new: no write");

    // Pre-2.60: oversized wantedSize is NOT an error; copy clamps to 104
    // and the tail is left untouched (PPSSPP old branch has no memset).
    poison(INFO, 0x100);
    psp_mem_write<uint32_t>(g_ram, INFO, 200);
    rc = psp_write_thread_info(g_ram, INFO, false, sample_thread_image());
    ASSERT_U32_EQ(rc, 0, "size200/old: rc");
    ASSERT_U32_EQ(rd32(INFO + 0), 104, "size200/old: size reads 104");
    ASSERT_U32_EQ(rd32(INFO + 104), 0xCDCDCDCDU, "size200/old: tail intact");

    // Partial copy honors the byte count exactly: wantedSize=44 copies
    // through status(+40) but not entry(+44).
    poison(INFO, 0x100);
    psp_mem_write<uint32_t>(g_ram, INFO, 44);
    rc = psp_write_thread_info(g_ram, INFO, true, sample_thread_image());
    ASSERT_U32_EQ(rc, 0, "size44: rc");
    ASSERT_U32_EQ(rd32(INFO + 40), 4, "size44: status copied");
    ASSERT_U32_EQ(rd32(INFO + 44), 0xCDCDCDCDU, "size44: entry not copied");

    // info_ptr == 0: no write, success (PPSSPP reads wantedSize 0).
    rc = psp_write_thread_info(g_ram, 0, true, sample_thread_image());
    ASSERT_U32_EQ(rc, 0, "nullptr: rc");
}

// ---- Test 5: eventflag NativeEventFlag layout (52 bytes) ----
static void test_eventflag_fill() {
    PspNativeEventFlagImage img{};
    img.size = sizeof(img);
    std::strncpy(img.name, "SceGuSignal", sizeof(img.name) - 1);
    img.attr = 0x200U;
    img.init_pattern = 0x1U;
    img.current_pattern = 0x4U;
    img.num_wait_threads = 2;

    poison(INFO, 0x100);
    psp_mem_write<uint32_t>(g_ram, INFO, 52);  // caller size != 0 -> copy
    psp_write_eventflag_info(g_ram, INFO, img);
    ASSERT_U32_EQ(rd32(INFO + 0), 52, "ef: size field reads 52");
    char name[32];
    std::memcpy(name, g_ram + ((INFO + 4) & 0x07FFFFFFU), sizeof(name));
    ASSERT_TRUE(std::strcmp(name, "SceGuSignal") == 0, "ef: name at +4");
    ASSERT_U32_EQ(rd32(INFO + 36), 0x200U, "ef: attr at +36");
    ASSERT_U32_EQ(rd32(INFO + 40), 0x1U, "ef: initPattern at +40");
    ASSERT_U32_EQ(rd32(INFO + 44), 0x4U, "ef: currentPattern at +44");
    ASSERT_U32_EQ(rd32(INFO + 48), 2, "ef: numWaitThreads at +48");
    ASSERT_U32_EQ(rd32(INFO + 52), 0xCDCDCDCDU, "ef: nothing past 52");

    // Caller size == 0: PPSSPP skips the copy entirely.
    poison(INFO, 0x100);
    psp_mem_write<uint32_t>(g_ram, INFO, 0);
    psp_write_eventflag_info(g_ram, INFO, img);
    ASSERT_U32_EQ(rd32(INFO + 4), 0xCDCDCDCDU, "ef size0: nothing written");
}

int main() {
    std::printf("Running Refer*Status layout tests...\n\n");

    // 1MB fake guest RAM; all test addresses stay within it after masking.
    std::vector<uint8_t> ram(0x00100000, 0);
    g_ram = ram.data();

    test_status_encoding_map();
    test_thread_fill_size_108();
    test_thread_fill_size_104();
    test_thread_fill_size_edges();
    test_eventflag_fill();

    std::printf("\n%d tests run, %d failures\n", tests_run, failures);
    return failures == 0 ? 0 : 1;
}
