#include "hle/psp_hle.h"
#include "recomp.h"

// ---- HLE Functions ----
// Patapon only imports one scePower NID: 0xEBD177D6.
// sceKernelPowerTick is under sceSuspendForUser, registered
// by psp_hle_register_utility().

// NID 0xEBD177D6 is missing from data/niddb, so the generated import table
// (issue #40) carries the walker's canonical fallback name "NID_0xEBD177D6"
// — handlers for unresolved NIDs register under exactly that name.
// Identification: PPSSPP Core/HLE/scePower.cpp maps 0xEBD177D6 to
// scePowerSetClockFrequency (exported as "scePower_EBD177D6", the 3.71+
// alias). Returning SCE_OK without touching clocks is the correct no-op.
static void hle_NID_0xEBD177D6(
    uint8_t* rdram, recomp_context* ctx
) {
    ctx->r[2] = SCE_OK;
    (void)rdram;
}

// ---- Registration ----

void psp_hle_register_power() {
    psp_hle_register("NID_0xEBD177D6",
                      hle_NID_0xEBD177D6);
}
