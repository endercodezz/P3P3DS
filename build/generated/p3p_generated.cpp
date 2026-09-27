#include "psprecomp/runtime.hpp"
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

namespace psprecomp {
static const std::uint16_t kEntryIds_sub_08804000[7] = {
    1, 0, 0, 0, 2, 0, 3,
};
void sub_08804000_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08804000u;
        entry_id = (entry_delta < 28u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08804000[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08804000;
    case 2u: goto L_08804010;
    case 3u: goto L_08804018;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08804000:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08804010u);
    // nop
    ctx.pc = 0x088040A8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804010u) goto L_08804010;
    return;
L_08804010:
    ctx.set_gpr(31, 0x08804018u);
    // nop
    ctx.pc = 0x08B4F714u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804018u) goto L_08804018;
    return;
L_08804018:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08804000(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08804000_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08804024[56] = {
    1, 2, 3, 4, 5, 6, 0, 7, 8, 9, 10, 0, 11, 12, 0, 13, 14, 15, 0, 16, 17, 18, 0, 19, 20, 0, 21, 22, 23, 24, 25, 26,
    0, 27, 28, 29, 30, 31, 32, 33, 34, 0, 35, 0, 36, 37, 38, 39, 0, 40, 41, 0, 42, 0, 43, 44,
};
void sub_08804024_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08804024u;
        entry_id = (entry_delta < 224u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08804024[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08804024;
    case 2u: goto L_08804028;
    case 3u: goto L_0880402C;
    case 4u: goto L_08804030;
    case 5u: goto L_08804034;
    case 6u: goto L_08804038;
    case 7u: goto L_08804040;
    case 8u: goto L_08804044;
    case 9u: goto L_08804048;
    case 10u: goto L_0880404C;
    case 11u: goto L_08804054;
    case 12u: goto L_08804058;
    case 13u: goto L_08804060;
    case 14u: goto L_08804064;
    case 15u: goto L_08804068;
    case 16u: goto L_08804070;
    case 17u: goto L_08804074;
    case 18u: goto L_08804078;
    case 19u: goto L_08804080;
    case 20u: goto L_08804084;
    case 21u: goto L_0880408C;
    case 22u: goto L_08804090;
    case 23u: goto L_08804094;
    case 24u: goto L_08804098;
    case 25u: goto L_0880409C;
    case 26u: goto L_088040A0;
    case 27u: goto L_088040A8;
    case 28u: goto L_088040AC;
    case 29u: goto L_088040B0;
    case 30u: goto L_088040B4;
    case 31u: goto L_088040B8;
    case 32u: goto L_088040BC;
    case 33u: goto L_088040C0;
    case 34u: goto L_088040C4;
    case 35u: goto L_088040CC;
    case 36u: goto L_088040D4;
    case 37u: goto L_088040D8;
    case 38u: goto L_088040DC;
    case 39u: goto L_088040E0;
    case 40u: goto L_088040E8;
    case 41u: goto L_088040EC;
    case 42u: goto L_088040F4;
    case 43u: goto L_088040FC;
    case 44u: goto L_08804100;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08804024:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    goto L_08804028;
L_08804028:
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    goto L_0880402C;
L_0880402C:
    ctx.set_gpr(17, 2244u << 16u);
    goto L_08804030;
L_08804030:
    ctx.set_gpr(2, rt.memory().aot_load8(ctx.gpr[17] + static_cast<std::uint32_t>(-21888)));
    goto L_08804034;
L_08804034:
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    goto L_08804038;
L_08804038:
    { const bool branch_taken = ctx.gpr[2] != 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08804094;
      }
      goto L_08804040;
    }
L_08804040:
    ctx.set_gpr(16, 2235u << 16u);
    goto L_08804044;
L_08804044:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(22788)));
    goto L_08804048;
L_08804048:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(0)));
    goto L_0880404C;
L_0880404C:
    if (ctx.gpr[3] == 0u) {
    ctx.set_gpr(2, 0u << 16u);
        goto L_08804074;
    }
    goto L_08804054;
L_08804054:
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(4));
    goto L_08804058;
L_08804058:
    jump_target = ctx.gpr[3];
    ctx.set_gpr(31, 0x08804060u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(22788), ctx.gpr[2]);
    ctx.pc = jump_target;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804060u) goto L_08804060;
    return;
L_08804060:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(22788)));
    goto L_08804064;
L_08804064:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(0)));
    goto L_08804068;
L_08804068:
    { const bool branch_taken = ctx.gpr[3] != 0u;
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08804058;
      }
      goto L_08804070;
    }
L_08804070:
    ctx.set_gpr(2, 0u << 16u);
    goto L_08804074;
L_08804074:
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(0));
    goto L_08804078;
L_08804078:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08804090;
      }
      goto L_08804080;
    }
L_08804080:
    ctx.set_gpr(4, 2232u << 16u);
    goto L_08804084;
L_08804084:
    ctx.set_gpr(31, 0x0880408Cu);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(2324));
    ctx.pc = 0x00000000u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880408Cu) goto L_0880408C;
    return;
L_0880408C:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    goto L_08804090;
L_08804090:
    rt.memory().aot_store8(ctx.gpr[17] + static_cast<std::uint32_t>(-21888), static_cast<std::uint8_t>(ctx.gpr[2]));
    goto L_08804094;
L_08804094:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    goto L_08804098;
L_08804098:
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    goto L_0880409C;
L_0880409C:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    goto L_088040A0;
L_088040A0:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_088040A8:
    ctx.set_gpr(2, 0u << 16u);
    goto L_088040AC;
L_088040AC:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    goto L_088040B0;
L_088040B0:
    ctx.set_gpr(4, 2232u << 16u);
    goto L_088040B4;
L_088040B4:
    ctx.set_gpr(5, 2244u << 16u);
    goto L_088040B8;
L_088040B8:
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(0));
    goto L_088040BC;
L_088040BC:
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    goto L_088040C0;
L_088040C0:
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(2324));
    goto L_088040C4;
L_088040C4:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(-21884));
      if (branch_taken) {
          goto L_088040D4;
      }
      goto L_088040CC;
    }
L_088040CC:
    ctx.set_gpr(31, 0x088040D4u);
    // nop
    ctx.pc = 0x00000000u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088040D4u) goto L_088040D4;
    return;
L_088040D4:
    ctx.set_gpr(4, 2232u << 16u);
    goto L_088040D8;
L_088040D8:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(2320)));
    goto L_088040DC;
L_088040DC:
    ctx.set_gpr(2, 0u << 16u);
    goto L_088040E0;
L_088040E0:
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(25, ctx.gpr[2] + static_cast<std::uint32_t>(0));
      if (branch_taken) {
          goto L_088040FC;
      }
      goto L_088040E8;
    }
L_088040E8:
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(2320));
    goto L_088040EC;
L_088040EC:
    { const bool branch_taken = ctx.gpr[25] == 0u;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
      if (branch_taken) {
          goto L_088040FC;
      }
      goto L_088040F4;
    }
L_088040F4:
    jump_target = ctx.gpr[25];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_088040FC:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    goto L_08804100;
L_08804100:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08804024(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08804024_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_088040A8[22] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 0, 4, 0, 0, 5, 0, 6,
};
void sub_088040A8_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x088040A8u;
        entry_id = (entry_delta < 88u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_088040A8[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_088040A8;
    case 2u: goto L_088040CC;
    case 3u: goto L_088040D4;
    case 4u: goto L_088040E8;
    case 5u: goto L_088040F4;
    case 6u: goto L_088040FC;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_088040A8:
    ctx.set_gpr(2, 0u << 16u);
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(4, 2232u << 16u);
    ctx.set_gpr(5, 2244u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(0));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(2324));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(-21884));
      if (branch_taken) {
          goto L_088040D4;
      }
      goto L_088040CC;
    }
L_088040CC:
    ctx.set_gpr(31, 0x088040D4u);
    // nop
    ctx.pc = 0x00000000u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088040D4u) goto L_088040D4;
    return;
L_088040D4:
    ctx.set_gpr(4, 2232u << 16u);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(2320)));
    ctx.set_gpr(2, 0u << 16u);
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(25, ctx.gpr[2] + static_cast<std::uint32_t>(0));
      if (branch_taken) {
          goto L_088040FC;
      }
      goto L_088040E8;
    }
L_088040E8:
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(2320));
    { const bool branch_taken = ctx.gpr[25] == 0u;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
      if (branch_taken) {
          goto L_088040FC;
      }
      goto L_088040F4;
    }
L_088040F4:
    jump_target = ctx.gpr[25];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_088040FC:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_088040A8(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_088040A8_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_module_start[67] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0, 0, 7, 0, 0,
    8, 0, 0, 0, 9, 10, 0, 0, 0, 11, 0, 12, 0, 0, 0, 13, 0, 0, 14, 0, 0, 0, 15, 0, 0, 0, 16, 0, 0, 0, 0, 0,
    0, 0, 17,
};
void module_start_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08804108u;
        entry_id = (entry_delta < 268u && (entry_delta & 3u) == 0u) ? kEntryIds_module_start[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08804108;
    case 2u: goto L_0880413C;
    case 3u: goto L_08804148;
    case 4u: goto L_08804150;
    case 5u: goto L_08804158;
    case 6u: goto L_08804168;
    case 7u: goto L_0880417C;
    case 8u: goto L_08804188;
    case 9u: goto L_08804198;
    case 10u: goto L_0880419C;
    case 11u: goto L_088041AC;
    case 12u: goto L_088041B4;
    case 13u: goto L_088041C4;
    case 14u: goto L_088041D0;
    case 15u: goto L_088041E0;
    case 16u: goto L_088041F0;
    case 17u: goto L_08804210;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08804108:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    ctx.set_gpr(2, 1538u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(19, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, ctx.gpr[2] | 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(18, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, 0u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[31]);
    ctx.set_gpr(31, 0x0880413Cu);
    ctx.set_gpr(17, ctx.gpr[16] + static_cast<std::uint32_t>(0));
    ctx.pc = 0x08B7FC0Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880413Cu) goto L_0880413C;
    return;
L_0880413C:
    ctx.set_gpr(2, 3u << 16u);
    ctx.set_gpr(31, 0x08804148u);
    ctx.set_gpr(4, ctx.gpr[2] | 774u);
    ctx.pc = 0x08B7FC04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804148u) goto L_08804148;
    return;
L_08804148:
    { const bool branch_taken = ctx.gpr[17] == 0u;
    ctx.set_gpr(9, 0u << 16u);
      if (branch_taken) {
          goto L_0880417C;
      }
      goto L_08804150;
    }
L_08804150:
    ctx.set_gpr(31, 0x08804158u);
    // nop
    ctx.pc = 0x08B7FC9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804158u) goto L_08804158;
    return;
L_08804158:
    ctx.set_gpr(8, ctx.gpr[2] << 11u);
    ctx.set_gpr(7, ctx.gpr[2] ^ ctx.gpr[8]);
    ctx.set_gpr(31, 0x08804168u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(0), ctx.gpr[7]);
    ctx.pc = 0x08B7FB94u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804168u) goto L_08804168;
    return;
L_08804168:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(5, ctx.gpr[6] ^ ctx.gpr[2]);
    ctx.set_gpr(4, ctx.gpr[5] ^ ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(0), ctx.gpr[4]);
    ctx.set_gpr(9, 0u << 16u);
    goto L_0880417C;
L_0880417C:
    ctx.set_gpr(3, ctx.gpr[9] + static_cast<std::uint32_t>(0));
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(4, ctx.gpr[3] + 0u);
      if (branch_taken) {
          goto L_08804210;
      }
      goto L_08804188;
    }
L_08804188:
    ctx.set_gpr(3, 0u << 16u);
    ctx.set_gpr(11, ctx.gpr[3] + static_cast<std::uint32_t>(0));
    { const bool branch_taken = ctx.gpr[11] == 0u;
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(32));
      if (branch_taken) {
          goto L_0880419C;
      }
      goto L_08804198;
    }
L_08804198:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(0)));
    goto L_0880419C;
L_0880419C:
    ctx.set_gpr(3, 2243u << 16u);
    ctx.set_gpr(12, ctx.gpr[3] + static_cast<std::uint32_t>(22608));
    { const bool branch_taken = ctx.gpr[12] == 0u;
    ctx.set_gpr(7, 4u << 16u);
      if (branch_taken) {
          goto L_088041B4;
      }
      goto L_088041AC;
    }
L_088041AC:
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(22608)));
    ctx.set_gpr(7, ctx.gpr[13] << 10u);
    goto L_088041B4;
L_088041B4:
    ctx.set_gpr(5, 0u << 16u);
    ctx.set_gpr(14, ctx.gpr[5] + static_cast<std::uint32_t>(0));
    { const bool branch_taken = ctx.gpr[14] == 0u;
    ctx.set_gpr(8, 32768u << 16u);
      if (branch_taken) {
          goto L_088041D0;
      }
      goto L_088041C4;
    }
L_088041C4:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(15, 32768u << 16u);
    ctx.set_gpr(8, ctx.gpr[16] | ctx.gpr[15]);
    goto L_088041D0;
L_088041D0:
    ctx.set_gpr(17, 2176u << 16u);
    ctx.set_gpr(5, ctx.gpr[17] + static_cast<std::uint32_t>(16924));
    ctx.set_gpr(31, 0x088041E0u);
    ctx.set_gpr(9, 0u + 0u);
    ctx.pc = 0x08B7FCB4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088041E0u) goto L_088041E0;
    return;
L_088041E0:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(5, ctx.gpr[19] + 0u);
    ctx.set_gpr(31, 0x088041F0u);
    ctx.set_gpr(6, ctx.gpr[18] + 0u);
    ctx.pc = 0x08B7FC74u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088041F0u) goto L_088041F0;
    return;
L_088041F0:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08804210:
    ctx.set_gpr(10, 2232u << 16u);
    ctx.set_gpr(4, ctx.gpr[10] + static_cast<std::uint32_t>(2504));
    goto L_08804188;
}

void module_start(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    module_start_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_user_main[176] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 4, 5, 0, 6, 0, 7, 0, 0, 8, 0, 0, 0, 0,
    0, 0, 9, 0, 10, 0, 0, 0, 0, 0, 11, 0, 12, 0, 0, 13, 0, 0, 14, 0, 15, 0, 0, 0, 16, 0, 17, 0, 0, 0, 18, 0,
    0, 19, 20, 0, 21, 0, 22, 0, 23, 0, 24, 0, 25, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 28, 0, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30, 0, 0, 0, 0, 31, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32,
};
void user_main_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x0880421Cu;
        entry_id = (entry_delta < 704u && (entry_delta & 3u) == 0u) ? kEntryIds_user_main[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_0880421C;
    case 2u: goto L_08804254;
    case 3u: goto L_08804260;
    case 4u: goto L_08804268;
    case 5u: goto L_0880426C;
    case 6u: goto L_08804274;
    case 7u: goto L_0880427C;
    case 8u: goto L_08804288;
    case 9u: goto L_088042A4;
    case 10u: goto L_088042AC;
    case 11u: goto L_088042C4;
    case 12u: goto L_088042CC;
    case 13u: goto L_088042D8;
    case 14u: goto L_088042E4;
    case 15u: goto L_088042EC;
    case 16u: goto L_088042FC;
    case 17u: goto L_08804304;
    case 18u: goto L_08804314;
    case 19u: goto L_08804320;
    case 20u: goto L_08804324;
    case 21u: goto L_0880432C;
    case 22u: goto L_08804334;
    case 23u: goto L_0880433C;
    case 24u: goto L_08804344;
    case 25u: goto L_0880434C;
    case 26u: goto L_08804354;
    case 27u: goto L_0880437C;
    case 28u: goto L_088043A4;
    case 29u: goto L_088043B4;
    case 30u: goto L_088043F8;
    case 31u: goto L_0880440C;
    case 32u: goto L_088044D8;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_0880421C:
    ctx.set_gpr(3, 0u << 16u);
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-1024));
    ctx.set_gpr(6, ctx.gpr[3] + static_cast<std::uint32_t>(0));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(1012), ctx.gpr[21]);
    ctx.set_gpr(21, ctx.gpr[29] + static_cast<std::uint32_t>(96));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(1004), ctx.gpr[19]);
    ctx.set_gpr(19, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(1000), ctx.gpr[18]);
    ctx.set_gpr(18, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(1016), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(1008), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(996), ctx.gpr[17]);
    { const bool branch_taken = ctx.gpr[6] == 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(992), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08804260;
      }
      goto L_08804254;
    }
L_08804254:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(0)));
    { const bool branch_taken = ctx.gpr[4] != 0u;
    ctx.set_gpr(20, 0u + 0u);
      if (branch_taken) {
          goto L_0880426C;
      }
      goto L_08804260;
    }
L_08804260:
    ctx.set_gpr(31, 0x08804268u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x08B4E6A0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804268u) goto L_08804268;
    return;
L_08804268:
    ctx.set_gpr(20, 0u + 0u);
    goto L_0880426C;
L_0880426C:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[19]) <= 0;
    ctx.set_gpr(16, ctx.gpr[18] + 0u);
      if (branch_taken) {
          goto L_088042AC;
      }
      goto L_08804274;
    }
L_08804274:
    ctx.set_gpr(17, ctx.gpr[29] + 0u);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    goto L_0880427C;
L_0880427C:
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.set_gpr(31, 0x08804288u);
    ctx.set_gpr(20, ctx.gpr[20] + static_cast<std::uint32_t>(1));
    ctx.pc = 0x08B4EA88u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804288u) goto L_08804288;
    return;
L_08804288:
    ctx.set_gpr(6, ctx.gpr[16] + ctx.gpr[2]);
    ctx.set_gpr(16, ctx.gpr[6] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(3, ctx.gpr[16] - ctx.gpr[18]);
    ctx.set_gpr(5, static_cast<std::int32_t>(ctx.gpr[20]) < 20 ? 1u : 0u);
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[3]) < static_cast<std::int32_t>(ctx.gpr[19]) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[5] == 0u;
    ctx.set_gpr(17, ctx.gpr[17] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_088042AC;
      }
      goto L_088042A4;
    }
L_088042A4:
    if (ctx.gpr[2] != 0u) {
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
        goto L_0880427C;
    }
    goto L_088042AC;
L_088042AC:
    ctx.set_gpr(9, ctx.gpr[20] << 2u);
    ctx.set_gpr(7, 2231u << 16u);
    ctx.set_gpr(8, ctx.gpr[9] + ctx.gpr[29]);
    ctx.set_gpr(2, ctx.gpr[7] + static_cast<std::uint32_t>(16640));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(0), 0u);
      if (branch_taken) {
          goto L_0880437C;
      }
      goto L_088042C4;
    }
L_088042C4:
    ctx.set_gpr(31, 0x088042CCu);
    ctx.set_gpr(4, ctx.gpr[21] + 0u);
    ctx.pc = 0x08B74100u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088042CCu) goto L_088042CC;
    return;
L_088042CC:
    ctx.set_gpr(10, 2176u << 16u);
    ctx.set_gpr(31, 0x088042D8u);
    ctx.set_gpr(4, ctx.gpr[10] + static_cast<std::uint32_t>(16648));
    ctx.pc = 0x08B7FBC4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088042D8u) goto L_088042D8;
    return;
L_088042D8:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(31, 0x088042E4u);
    ctx.set_gpr(5, ctx.gpr[21] + 0u);
    ctx.pc = 0x08B73F1Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088042E4u) goto L_088042E4;
    return;
L_088042E4:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    // nop
      if (branch_taken) {
          goto L_0880434C;
      }
      goto L_088042EC;
    }
L_088042EC:
    rt.memory().aot_store32(ctx.gpr[26] + static_cast<std::uint32_t>(4), ctx.gpr[21]);
    ctx.set_gpr(5, 2232u << 16u);
    ctx.set_gpr(31, 0x088042FCu);
    ctx.set_gpr(4, ctx.gpr[5] + static_cast<std::uint32_t>(-1360));
    ctx.pc = 0x08B72DF8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088042FCu) goto L_088042FC;
    return;
L_088042FC:
    ctx.set_gpr(31, 0x08804304u);
    // nop
    ctx.pc = 0x08804000u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804304u) goto L_08804304;
    return;
L_08804304:
    ctx.set_gpr(4, 0u << 16u);
    ctx.set_gpr(21, ctx.gpr[4] + static_cast<std::uint32_t>(0));
    { const bool branch_taken = ctx.gpr[21] == 0u;
    ctx.set_gpr(6, 0u << 16u);
      if (branch_taken) {
          goto L_08804320;
      }
      goto L_08804314;
    }
L_08804314:
    ctx.set_gpr(4, ctx.gpr[6] + static_cast<std::uint32_t>(0));
    { const bool branch_taken = ctx.gpr[4] != 0u;
    // nop
      if (branch_taken) {
          goto L_08804334;
      }
      goto L_08804320;
    }
L_08804320:
    ctx.set_gpr(4, ctx.gpr[20] + 0u);
    goto L_08804324;
L_08804324:
    ctx.set_gpr(31, 0x0880432Cu);
    ctx.set_gpr(5, ctx.gpr[29] + 0u);
    ctx.pc = 0x08804538u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880432Cu) goto L_0880432C;
    return;
L_0880432C:
    std::fprintf(stderr, "[jal0-probe] entered 0880432C v0=%08X sp=%08X ra=%08X\n", ctx.gpr[2], ctx.gpr[29], ctx.gpr[31]);
    ctx.set_gpr(31, 0x08804334u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B72F04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804334u) goto L_08804334;
    return;
L_08804334:
    std::fprintf(stderr, "[jal0-probe] entered 08804334 v0=%08X sp=%08X ra=%08X\n", ctx.gpr[2], ctx.gpr[29], ctx.gpr[31]);
    ctx.set_gpr(31, 0x0880433Cu);
    // nop
    ctx.pc = 0x08B72DF8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880433Cu) goto L_0880433C;
    return;
L_0880433C:
    std::fprintf(stderr, "[jal0-probe] entered 0880433C v0=%08X sp=%08X ra=%08X\n", ctx.gpr[2], ctx.gpr[29], ctx.gpr[31]);
    ctx.set_gpr(31, 0x08804344u);
    // nop
    ctx.pc = 0x00000000u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804344u) goto L_08804344;
    return;
L_08804344:
    ctx.set_gpr(4, ctx.gpr[20] + 0u);
    goto L_08804324;
L_0880434C:
    ctx.set_gpr(31, 0x08804354u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.pc = 0x08B7FD34u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804354u) goto L_08804354;
    return;
L_08804354:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(1016)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(1012)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(1008)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(1004)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(1000)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(996)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(992)));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(1024));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_0880437C:
    ctx.set_gpr(4, ctx.gpr[21] + static_cast<std::uint32_t>(616));
    ctx.set_gpr(12, ctx.gpr[21] + static_cast<std::uint32_t>(708));
    ctx.set_gpr(11, ctx.gpr[21] + static_cast<std::uint32_t>(800));
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(8), ctx.gpr[12]);
    ctx.set_gpr(2, ctx.gpr[21] + static_cast<std::uint32_t>(20));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(24));
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(12), ctx.gpr[11]);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(0), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(16), 0u);
    goto L_088043A4;
L_088043A4:
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(-1));
    rt.memory().aot_store8(ctx.gpr[2] + static_cast<std::uint32_t>(0), static_cast<std::uint8_t>(0u));
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[5]) >= 0;
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_088043A4;
      }
      goto L_088043B4;
    }
L_088043B4:
    ctx.set_gpr(14, 2232u << 16u);
    ctx.set_gpr(13, ctx.gpr[14] + static_cast<std::uint32_t>(2516));
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(52), ctx.gpr[13]);
    ctx.set_gpr(6, ctx.gpr[21] + static_cast<std::uint32_t>(124));
    ctx.set_gpr(5, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(48), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(56), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(60), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(64), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(68), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(72), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(76), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(80), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(84), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(88), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(92), 0u);
    rt.memory().aot_store8(ctx.gpr[21] + static_cast<std::uint32_t>(96), static_cast<std::uint8_t>(0u));
    goto L_088043F8;
L_088043F8:
    ctx.set_gpr(16, ctx.gpr[6] + ctx.gpr[5]);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(15, ctx.gpr[5] < static_cast<std::uint32_t>(36) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[15] != 0u;
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(0), static_cast<std::uint8_t>(0u));
      if (branch_taken) {
          goto L_088043F8;
      }
      goto L_0880440C;
    }
L_0880440C:
    ctx.set_gpr(9, 0u + static_cast<std::uint32_t>(0));
    ctx.set_gpr(8, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(13070));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(-21555));
    ctx.set_gpr(25, 0u + static_cast<std::uint32_t>(4660));
    ctx.set_gpr(24, 0u + static_cast<std::uint32_t>(-6547));
    ctx.set_gpr(19, 0u + static_cast<std::uint32_t>(-8468));
    ctx.set_gpr(18, 0u + static_cast<std::uint32_t>(5));
    ctx.set_gpr(17, 0u + static_cast<std::uint32_t>(11));
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(168), ctx.gpr[8]);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(172), ctx.gpr[9]);
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(276));
    rt.memory().aot_store16(ctx.gpr[21] + static_cast<std::uint32_t>(176), static_cast<std::uint16_t>(ctx.gpr[2]));
    rt.memory().aot_store16(ctx.gpr[21] + static_cast<std::uint32_t>(178), static_cast<std::uint16_t>(ctx.gpr[3]));
    rt.memory().aot_store16(ctx.gpr[21] + static_cast<std::uint32_t>(180), static_cast<std::uint16_t>(ctx.gpr[25]));
    rt.memory().aot_store16(ctx.gpr[21] + static_cast<std::uint32_t>(182), static_cast<std::uint16_t>(ctx.gpr[24]));
    rt.memory().aot_store16(ctx.gpr[21] + static_cast<std::uint32_t>(184), static_cast<std::uint16_t>(ctx.gpr[19]));
    rt.memory().aot_store16(ctx.gpr[21] + static_cast<std::uint32_t>(186), static_cast<std::uint16_t>(ctx.gpr[18]));
    rt.memory().aot_store16(ctx.gpr[21] + static_cast<std::uint32_t>(188), static_cast<std::uint16_t>(ctx.gpr[17]));
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(160), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(192), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(196), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(200), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(204), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(208), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(212), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(252), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(256), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(260), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(264), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(268), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(272), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(276), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(280), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(284), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(288), 0u);
    rt.memory().aot_store8(ctx.gpr[21] + static_cast<std::uint32_t>(216), static_cast<std::uint8_t>(0u));
    rt.memory().aot_store8(ctx.gpr[21] + static_cast<std::uint32_t>(224), static_cast<std::uint8_t>(0u));
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(248), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(328), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(332), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(336), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(340), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(596), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(468), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(600), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(604), 0u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(608), 0u);
    ctx.set_gpr(31, 0x088044D8u);
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(612), 0u);
    ctx.pc = 0x08B73CF0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088044D8u) goto L_088044D8;
    return;
L_088044D8:
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(892), 0u);
    goto L_088042EC;
}

void user_main(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    user_main_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08804538[376] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 3, 0, 4, 0, 0, 0, 5, 0, 6, 0, 7, 0,
    8, 0, 9, 0, 10, 0, 11, 0, 0, 0, 12, 0, 0, 13, 0, 0, 14, 0, 15, 0, 16, 0, 0, 17, 0, 18, 0, 19, 0, 20, 0, 21,
    0, 22, 0, 23, 0, 24, 0, 0, 0, 0, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27, 0, 0, 0, 28, 0, 0, 29, 0,
    0, 0, 0, 0, 30, 0, 31, 0, 32, 0, 0, 33, 0, 0, 0, 0, 34, 0, 0, 0, 0, 0, 0, 35, 0, 0, 36, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 37, 0, 38, 0, 39, 40, 0, 41, 0, 0, 42, 0, 0, 0, 0, 43, 0, 0, 44, 0, 45, 0, 0, 0, 0, 0, 46, 0,
    0, 0, 0, 0, 47, 0, 0, 0, 0, 48, 0, 0, 0, 0, 0, 0, 0, 0, 49, 0, 50, 0, 51, 52, 0, 53, 0, 0, 54, 0, 0, 0,
    55, 0, 56, 0, 57, 0, 58, 0, 59, 0, 60, 0, 61, 0, 62, 0, 63, 0, 64, 0, 65, 0, 66, 0, 67, 0, 68, 0, 69, 0, 70, 0,
    71, 0, 0, 0, 0, 0, 72, 73, 0, 74, 0, 75, 0, 76, 0, 77, 78, 0, 0, 0, 0, 79, 0, 0, 0, 0, 80, 0, 81, 0, 82, 0,
    83, 0, 84, 0, 85, 0, 86, 0, 87, 0, 88, 0, 89, 0, 0, 90, 0, 0, 91, 0, 92, 0, 93, 0, 94, 0, 95, 0, 96, 0, 97, 0,
    98, 0, 99, 0, 100, 0, 101, 0, 102, 0, 103, 0, 104, 0, 105, 0, 106, 0, 107, 0, 108, 0, 109, 0, 110, 0, 111, 0, 112, 0, 113, 0,
    114, 0, 115, 0, 116, 0, 117, 0, 118, 0, 119, 0, 120, 0, 121, 0, 0, 0, 0, 122, 0, 123, 0, 124,
};
void sub_08804538_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08804538u;
        entry_id = (entry_delta < 1504u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08804538[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08804538;
    case 2u: goto L_08804570;
    case 3u: goto L_08804588;
    case 4u: goto L_08804590;
    case 5u: goto L_088045A0;
    case 6u: goto L_088045A8;
    case 7u: goto L_088045B0;
    case 8u: goto L_088045B8;
    case 9u: goto L_088045C0;
    case 10u: goto L_088045C8;
    case 11u: goto L_088045D0;
    case 12u: goto L_088045E0;
    case 13u: goto L_088045EC;
    case 14u: goto L_088045F8;
    case 15u: goto L_08804600;
    case 16u: goto L_08804608;
    case 17u: goto L_08804614;
    case 18u: goto L_0880461C;
    case 19u: goto L_08804624;
    case 20u: goto L_0880462C;
    case 21u: goto L_08804634;
    case 22u: goto L_0880463C;
    case 23u: goto L_08804644;
    case 24u: goto L_0880464C;
    case 25u: goto L_08804660;
    case 26u: goto L_088046D8;
    case 27u: goto L_08804714;
    case 28u: goto L_08804724;
    case 29u: goto L_08804730;
    case 30u: goto L_08804748;
    case 31u: goto L_08804750;
    case 32u: goto L_08804758;
    case 33u: goto L_08804764;
    case 34u: goto L_08804778;
    case 35u: goto L_08804794;
    case 36u: goto L_088047A0;
    case 37u: goto L_088047C8;
    case 38u: goto L_088047D0;
    case 39u: goto L_088047D8;
    case 40u: goto L_088047DC;
    case 41u: goto L_088047E4;
    case 42u: goto L_088047F0;
    case 43u: goto L_08804804;
    case 44u: goto L_08804810;
    case 45u: goto L_08804818;
    case 46u: goto L_08804830;
    case 47u: goto L_08804848;
    case 48u: goto L_0880485C;
    case 49u: goto L_08804880;
    case 50u: goto L_08804888;
    case 51u: goto L_08804890;
    case 52u: goto L_08804894;
    case 53u: goto L_0880489C;
    case 54u: goto L_088048A8;
    case 55u: goto L_088048B8;
    case 56u: goto L_088048C0;
    case 57u: goto L_088048C8;
    case 58u: goto L_088048D0;
    case 59u: goto L_088048D8;
    case 60u: goto L_088048E0;
    case 61u: goto L_088048E8;
    case 62u: goto L_088048F0;
    case 63u: goto L_088048F8;
    case 64u: goto L_08804900;
    case 65u: goto L_08804908;
    case 66u: goto L_08804910;
    case 67u: goto L_08804918;
    case 68u: goto L_08804920;
    case 69u: goto L_08804928;
    case 70u: goto L_08804930;
    case 71u: goto L_08804938;
    case 72u: goto L_08804950;
    case 73u: goto L_08804954;
    case 74u: goto L_0880495C;
    case 75u: goto L_08804964;
    case 76u: goto L_0880496C;
    case 77u: goto L_08804974;
    case 78u: goto L_08804978;
    case 79u: goto L_0880498C;
    case 80u: goto L_088049A0;
    case 81u: goto L_088049A8;
    case 82u: goto L_088049B0;
    case 83u: goto L_088049B8;
    case 84u: goto L_088049C0;
    case 85u: goto L_088049C8;
    case 86u: goto L_088049D0;
    case 87u: goto L_088049D8;
    case 88u: goto L_088049E0;
    case 89u: goto L_088049E8;
    case 90u: goto L_088049F4;
    case 91u: goto L_08804A00;
    case 92u: goto L_08804A08;
    case 93u: goto L_08804A10;
    case 94u: goto L_08804A18;
    case 95u: goto L_08804A20;
    case 96u: goto L_08804A28;
    case 97u: goto L_08804A30;
    case 98u: goto L_08804A38;
    case 99u: goto L_08804A40;
    case 100u: goto L_08804A48;
    case 101u: goto L_08804A50;
    case 102u: goto L_08804A58;
    case 103u: goto L_08804A60;
    case 104u: goto L_08804A68;
    case 105u: goto L_08804A70;
    case 106u: goto L_08804A78;
    case 107u: goto L_08804A80;
    case 108u: goto L_08804A88;
    case 109u: goto L_08804A90;
    case 110u: goto L_08804A98;
    case 111u: goto L_08804AA0;
    case 112u: goto L_08804AA8;
    case 113u: goto L_08804AB0;
    case 114u: goto L_08804AB8;
    case 115u: goto L_08804AC0;
    case 116u: goto L_08804AC8;
    case 117u: goto L_08804AD0;
    case 118u: goto L_08804AD8;
    case 119u: goto L_08804AE0;
    case 120u: goto L_08804AE8;
    case 121u: goto L_08804AF0;
    case 122u: goto L_08804B04;
    case 123u: goto L_08804B0C;
    case 124u: goto L_08804B14;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08804538:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-368));
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(356), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(352), ctx.gpr[30]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(348), ctx.gpr[23]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(344), ctx.gpr[22]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(340), ctx.gpr[21]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(336), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(332), ctx.gpr[19]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(328), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(324), ctx.gpr[17]);
    ctx.set_gpr(31, 0x08804570u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(320), ctx.gpr[16]);
    ctx.pc = 0x08B7FEBCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804570u) goto L_08804570;
    return;
L_08804570:
    ctx.set_gpr(3, 2235u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(22796)));
    ctx.set_gpr(5, 2176u << 16u);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(17688));
    ctx.set_gpr(31, 0x08804588u);
    ctx.set_gpr(6, 0u + 0u);
    ctx.pc = 0x08B7FC54u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804588u) goto L_08804588;
    return;
L_08804588:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(3, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08804974;
      }
      goto L_08804590;
    }
L_08804590:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, 2235u << 16u);
    ctx.set_gpr(31, 0x088045A0u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(22792), ctx.gpr[3]);
    ctx.pc = 0x08B7FBACu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088045A0u) goto L_088045A0;
    return;
L_088045A0:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(2, 2233u << 16u);
      if (branch_taken) {
          goto L_08804978;
      }
      goto L_088045A8;
    }
L_088045A8:
    ctx.set_gpr(31, 0x088045B0u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(768));
    ctx.pc = 0x08B8012Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088045B0u) goto L_088045B0;
    return;
L_088045B0:
    if (static_cast<std::int32_t>(ctx.gpr[2]) < 0) {
    ctx.set_gpr(19, 2273u << 16u);
        goto L_08804B14;
    }
    goto L_088045B8;
L_088045B8:
    ctx.set_gpr(31, 0x088045C0u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(770));
    ctx.pc = 0x08B8012Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088045C0u) goto L_088045C0;
    return;
L_088045C0:
    ctx.set_gpr(19, 2273u << 16u);
    ctx.set_gpr(20, 2273u << 16u);
    goto L_088045C8;
L_088045C8:
    ctx.set_gpr(31, 0x088045D0u);
    ctx.set_gpr(17, 2273u << 16u);
    ctx.pc = 0x08AB304Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088045D0u) goto L_088045D0;
    return;
L_088045D0:
    ctx.set_gpr(3, 2233u << 16u);
    ctx.set_gpr(4, ctx.gpr[3] + static_cast<std::uint32_t>(23880));
    ctx.set_gpr(31, 0x088045E0u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(246));
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088045E0u) goto L_088045E0;
    return;
L_088045E0:
    ctx.set_gpr(5, 4u << 16u);
    ctx.set_gpr(31, 0x088045ECu);
    ctx.set_gpr(4, 10u << 16u);
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088045ECu) goto L_088045EC;
    return;
L_088045EC:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(31, 0x088045F8u);
    ctx.set_gpr(5, 10u << 16u);
    ctx.pc = 0x08A9F8F8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088045F8u) goto L_088045F8;
    return;
L_088045F8:
    ctx.set_gpr(31, 0x08804600u);
    ctx.set_gpr(22, 2273u << 16u);
    ctx.pc = 0x08AB2A6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804600u) goto L_08804600;
    return;
L_08804600:
    ctx.set_gpr(31, 0x08804608u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(400));
    ctx.pc = 0x08A99F60u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804608u) goto L_08804608;
    return;
L_08804608:
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(100));
    ctx.set_gpr(31, 0x08804614u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(2000));
    ctx.pc = 0x08AB1190u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804614u) goto L_08804614;
    return;
L_08804614:
    ctx.set_gpr(31, 0x0880461Cu);
    ctx.set_gpr(30, 2282u << 16u);
    ctx.pc = 0x08AA155Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880461Cu) goto L_0880461C;
    return;
L_0880461C:
    ctx.set_gpr(31, 0x08804624u);
    ctx.set_gpr(21, 0u + static_cast<std::uint32_t>(2));
    ctx.pc = 0x08814E88u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804624u) goto L_08804624;
    return;
L_08804624:
    ctx.set_gpr(31, 0x0880462Cu);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(4));
    ctx.pc = 0x08AB2984u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880462Cu) goto L_0880462C;
    return;
L_0880462C:
    ctx.set_gpr(31, 0x08804634u);
    // nop
    ctx.pc = 0x08AB3020u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804634u) goto L_08804634;
    return;
L_08804634:
    ctx.set_gpr(31, 0x0880463Cu);
    // nop
    ctx.pc = 0x08B641E4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880463Cu) goto L_0880463C;
    return;
L_0880463C:
    ctx.set_gpr(31, 0x08804644u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(16));
    ctx.pc = 0x08B64314u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804644u) goto L_08804644;
    return;
L_08804644:
    ctx.set_gpr(31, 0x0880464Cu);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x08B48B48u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880464Cu) goto L_0880464C;
    return;
L_0880464C:
    ctx.set_gpr(4, 2176u << 16u);
    ctx.set_gpr(9, 2233u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(17680));
    ctx.set_gpr(31, 0x08804660u);
    ctx.set_gpr(16, ctx.gpr[9] + static_cast<std::uint32_t>(23908));
    ctx.pc = 0x08B48B0Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804660u) goto L_08804660;
    return;
L_08804660:
    ctx.set_gpr(3, 2233u << 16u);
    ctx.set_gpr(2, rt.memory().aot_load_word_left(ctx.gpr[16] + static_cast<std::uint32_t>(3), ctx.gpr[2]));
    ctx.set_gpr(3, rt.memory().aot_load_word_left(ctx.gpr[16] + static_cast<std::uint32_t>(7), ctx.gpr[3]));
    ctx.set_gpr(5, rt.memory().aot_load_word_left(ctx.gpr[16] + static_cast<std::uint32_t>(11), ctx.gpr[5]));
    ctx.set_gpr(6, rt.memory().aot_load_word_left(ctx.gpr[16] + static_cast<std::uint32_t>(15), ctx.gpr[6]));
    ctx.set_gpr(7, rt.memory().aot_load_word_left(ctx.gpr[16] + static_cast<std::uint32_t>(19), ctx.gpr[7]));
    ctx.set_gpr(8, rt.memory().aot_load_word_left(ctx.gpr[16] + static_cast<std::uint32_t>(23), ctx.gpr[8]));
    ctx.set_gpr(4, 2233u << 16u);
    ctx.set_gpr(2, rt.memory().aot_load_word_right(ctx.gpr[4] + static_cast<std::uint32_t>(23908), ctx.gpr[2]));
    ctx.set_gpr(3, rt.memory().aot_load_word_right(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[3]));
    ctx.set_gpr(7, rt.memory().aot_load_word_right(ctx.gpr[16] + static_cast<std::uint32_t>(16), ctx.gpr[7]));
    ctx.set_gpr(8, rt.memory().aot_load_word_right(ctx.gpr[16] + static_cast<std::uint32_t>(20), ctx.gpr[8]));
    ctx.set_gpr(6, rt.memory().aot_load_word_right(ctx.gpr[16] + static_cast<std::uint32_t>(12), ctx.gpr[6]));
    ctx.set_gpr(5, rt.memory().aot_load_word_right(ctx.gpr[16] + static_cast<std::uint32_t>(8), ctx.gpr[5]));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(52), ctx.gpr[2]);
    ctx.set_gpr(2, 2233u << 16u);
    ctx.set_gpr(4, ctx.gpr[29] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(56), ctx.gpr[3]);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(23932));
    ctx.set_gpr(3, ctx.gpr[29] + static_cast<std::uint32_t>(58));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(68), ctx.gpr[7]);
    ctx.set_gpr(18, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(72), ctx.gpr[8]);
    ctx.set_gpr(23, ctx.gpr[29] + static_cast<std::uint32_t>(52));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(64), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(60), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    ctx.set_gpr(31, 0x088046D8u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), 0u);
    ctx.pc = 0x08A7556Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088046D8u) goto L_088046D8;
    return;
L_088046D8:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(2));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(16));
    ctx.set_gpr(4, ctx.gpr[18] + 0u);
    ctx.set_gpr(5, ctx.gpr[17] + static_cast<std::uint32_t>(4240));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(28), ctx.gpr[2]);
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(256));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(44), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(32), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(40), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(48), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), 0u);
    ctx.set_gpr(31, 0x08804714u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(36), 0u);
    ctx.pc = 0x08B353B0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804714u) goto L_08804714;
    return;
L_08804714:
    ctx.set_gpr(9, 2233u << 16u);
    ctx.set_gpr(4, ctx.gpr[9] + static_cast<std::uint32_t>(23880));
    ctx.set_gpr(31, 0x08804724u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(309));
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804724u) goto L_08804724;
    return;
L_08804724:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(4240)));
    ctx.set_gpr(31, 0x08804730u);
    ctx.set_gpr(5, 4u << 16u);
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804730u) goto L_08804730;
    return;
L_08804730:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(4240)));
    ctx.set_gpr(3, 2273u << 16u);
    ctx.set_gpr(5, ctx.gpr[2] + 0u);
    ctx.set_gpr(4, ctx.gpr[18] + 0u);
    ctx.set_gpr(31, 0x08804748u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4228), ctx.gpr[2]);
    ctx.pc = 0x08B35554u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804748u) goto L_08804748;
    return;
L_08804748:
    ctx.set_gpr(31, 0x08804750u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x08B3342Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804750u) goto L_08804750;
    return;
L_08804750:
    ctx.set_gpr(31, 0x08804758u);
    // nop
    ctx.pc = 0x08807BF4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804758u) goto L_08804758;
    return;
L_08804758:
    ctx.set_gpr(2, 2282u << 16u);
    ctx.set_gpr(31, 0x08804764u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(-2768));
    ctx.pc = 0x08B36EFCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804764u) goto L_08804764;
    return;
L_08804764:
    ctx.set_gpr(5, 2233u << 16u);
    ctx.set_gpr(6, ctx.gpr[16] + 0u);
    ctx.set_gpr(4, ctx.gpr[23] + 0u);
    ctx.set_gpr(31, 0x08804778u);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(23940));
    ctx.pc = 0x08B751F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804778u) goto L_08804778;
    return;
L_08804778:
    ctx.set_gpr(3, 2233u << 16u);
    ctx.set_gpr(2, 3u << 16u);
    ctx.set_gpr(4, ctx.gpr[3] + static_cast<std::uint32_t>(23880));
    ctx.set_gpr(2, ctx.gpr[2] | 39137u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(345));
    ctx.set_gpr(31, 0x08804794u);
    rt.memory().aot_store32(ctx.gpr[22] + static_cast<std::uint32_t>(4236), ctx.gpr[2]);
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804794u) goto L_08804794;
    return;
L_08804794:
    ctx.set_gpr(5, 4u << 16u);
    ctx.set_gpr(31, 0x088047A0u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[22] + static_cast<std::uint32_t>(4236)));
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088047A0u) goto L_088047A0;
    return;
L_088047A0:
    ctx.set_gpr(9, 2282u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(-2768)));
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[22] + static_cast<std::uint32_t>(4236)));
    ctx.set_gpr(3, 2273u << 16u);
    ctx.set_gpr(7, ctx.gpr[2] + 0u);
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(6, ctx.gpr[23] + 0u);
    ctx.set_gpr(9, ctx.gpr[30] + static_cast<std::uint32_t>(-3076));
    ctx.set_gpr(31, 0x088047C8u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4224), ctx.gpr[2]);
    ctx.pc = 0x08B37C20u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088047C8u) goto L_088047C8;
    return;
L_088047C8:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[30] + static_cast<std::uint32_t>(-3076)));
    goto L_088047DC;
L_088047D0:
    ctx.set_gpr(31, 0x088047D8u);
    // nop
    ctx.pc = 0x08B7FCDCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088047D8u) goto L_088047D8;
    return;
L_088047D8:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[30] + static_cast<std::uint32_t>(-3076)));
    goto L_088047DC;
L_088047DC:
    ctx.set_gpr(31, 0x088047E4u);
    ctx.set_gpr(5, ctx.gpr[29] + 0u);
    ctx.pc = 0x08B38FB8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088047E4u) goto L_088047E4;
    return;
L_088047E4:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    { const bool branch_taken = ctx.gpr[2] != ctx.gpr[21];
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(500));
      if (branch_taken) {
          goto L_088047D0;
      }
      goto L_088047F0;
    }
L_088047F0:
    ctx.set_gpr(4, 2282u << 16u);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(-2768)));
    ctx.set_gpr(16, 2233u << 16u);
    ctx.set_gpr(31, 0x08804804u);
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(23952));
    ctx.pc = 0x08B33500u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804804u) goto L_08804804;
    return;
L_08804804:
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(23952));
    ctx.set_gpr(31, 0x08804810u);
    ctx.set_gpr(16, 2282u << 16u);
    ctx.pc = 0x08B3359Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804810u) goto L_08804810;
    return;
L_08804810:
    ctx.set_gpr(31, 0x08804818u);
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(-3072));
    ctx.pc = 0x08B36EFCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804818u) goto L_08804818;
    return;
L_08804818:
    ctx.set_gpr(9, 2233u << 16u);
    ctx.set_gpr(5, 2233u << 16u);
    ctx.set_gpr(6, ctx.gpr[9] + static_cast<std::uint32_t>(23908));
    ctx.set_gpr(4, ctx.gpr[23] + 0u);
    ctx.set_gpr(31, 0x08804830u);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(23960));
    ctx.pc = 0x08B751F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804830u) goto L_08804830;
    return;
L_08804830:
    ctx.set_gpr(2, 2233u << 16u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(23880));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(412));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(13665));
    ctx.set_gpr(31, 0x08804848u);
    rt.memory().aot_store32(ctx.gpr[22] + static_cast<std::uint32_t>(4236), ctx.gpr[2]);
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804848u) goto L_08804848;
    return;
L_08804848:
    ctx.set_gpr(5, 4u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[22] + static_cast<std::uint32_t>(4236)));
    ctx.set_gpr(18, 2282u << 16u);
    ctx.set_gpr(31, 0x0880485Cu);
    ctx.set_gpr(17, 0u + static_cast<std::uint32_t>(2));
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880485Cu) goto L_0880485C;
    return;
L_0880485C:
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[22] + static_cast<std::uint32_t>(4236)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(-3072)));
    ctx.set_gpr(3, 2273u << 16u);
    ctx.set_gpr(6, ctx.gpr[23] + 0u);
    ctx.set_gpr(7, ctx.gpr[2] + 0u);
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(9, ctx.gpr[18] + static_cast<std::uint32_t>(-3080));
    ctx.set_gpr(31, 0x08804880u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4224), ctx.gpr[2]);
    ctx.pc = 0x08B37C20u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804880u) goto L_08804880;
    return;
L_08804880:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[18] + static_cast<std::uint32_t>(-3080)));
    goto L_08804894;
L_08804888:
    ctx.set_gpr(31, 0x08804890u);
    // nop
    ctx.pc = 0x08B7FCDCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804890u) goto L_08804890;
    return;
L_08804890:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[18] + static_cast<std::uint32_t>(-3080)));
    goto L_08804894;
L_08804894:
    ctx.set_gpr(31, 0x0880489Cu);
    ctx.set_gpr(5, ctx.gpr[29] + 0u);
    ctx.pc = 0x08B38FB8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880489Cu) goto L_0880489C;
    return;
L_0880489C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    { const bool branch_taken = ctx.gpr[2] != ctx.gpr[17];
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(500));
      if (branch_taken) {
          goto L_08804888;
      }
      goto L_088048A8;
    }
L_088048A8:
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(-3072)));
    ctx.set_gpr(4, 2233u << 16u);
    ctx.set_gpr(31, 0x088048B8u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(23972));
    ctx.pc = 0x08B33500u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088048B8u) goto L_088048B8;
    return;
L_088048B8:
    ctx.set_gpr(31, 0x088048C0u);
    // nop
    ctx.pc = 0x08AA165Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088048C0u) goto L_088048C0;
    return;
L_088048C0:
    ctx.set_gpr(31, 0x088048C8u);
    // nop
    ctx.pc = 0x08807E9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088048C8u) goto L_088048C8;
    return;
L_088048C8:
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(4, 2235u << 16u);
      if (branch_taken) {
          goto L_088049A0;
      }
      goto L_088048D0;
    }
L_088048D0:
    ctx.set_gpr(31, 0x088048D8u);
    // nop
    ctx.pc = 0x08B7FC9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088048D8u) goto L_088048D8;
    return;
L_088048D8:
    ctx.set_gpr(31, 0x088048E0u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B15678u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088048E0u) goto L_088048E0;
    return;
L_088048E0:
    ctx.set_gpr(31, 0x088048E8u);
    // nop
    ctx.pc = 0x08AB904Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088048E8u) goto L_088048E8;
    return;
L_088048E8:
    { const bool branch_taken = ctx.gpr[2] != 0u;
    // nop
      if (branch_taken) {
          goto L_088048E0;
      }
      goto L_088048F0;
    }
L_088048F0:
    ctx.set_gpr(31, 0x088048F8u);
    // nop
    ctx.pc = 0x08A9AA64u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088048F8u) goto L_088048F8;
    return;
L_088048F8:
    ctx.set_gpr(31, 0x08804900u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.pc = 0x08AA2744u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804900u) goto L_08804900;
    return;
L_08804900:
    ctx.set_gpr(31, 0x08804908u);
    // nop
    ctx.pc = 0x08AA3EC8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804908u) goto L_08804908;
    return;
L_08804908:
    ctx.set_gpr(31, 0x08804910u);
    // nop
    ctx.pc = 0x08AB6AB8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804910u) goto L_08804910;
    return;
L_08804910:
    ctx.set_gpr(31, 0x08804918u);
    // nop
    ctx.pc = 0x08AB5704u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804918u) goto L_08804918;
    return;
L_08804918:
    ctx.set_gpr(31, 0x08804920u);
    // nop
    ctx.pc = 0x08B143D0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804920u) goto L_08804920;
    return;
L_08804920:
    ctx.set_gpr(31, 0x08804928u);
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    ctx.pc = 0x08A99FC0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804928u) goto L_08804928;
    return;
L_08804928:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
      if (branch_taken) {
          goto L_088048E0;
      }
      goto L_08804930;
    }
L_08804930:
    ctx.set_gpr(31, 0x08804938u);
    // nop
    ctx.pc = 0x08B14570u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804938u) goto L_08804938;
    return;
L_08804938:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[20] + static_cast<std::uint32_t>(4244)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[19] + static_cast<std::uint32_t>(4232)));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(1));
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[6];
    ctx.set_gpr(5, ctx.gpr[3] + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08804954;
      }
      goto L_08804950;
    }
L_08804950:
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(4244), ctx.gpr[4]);
    goto L_08804954;
L_08804954:
    { const bool branch_taken = ctx.gpr[3] == ctx.gpr[6];
    // nop
      if (branch_taken) {
          goto L_088048E0;
      }
      goto L_0880495C;
    }
L_0880495C:
    ctx.set_gpr(31, 0x08804964u);
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(4232), ctx.gpr[5]);
    ctx.pc = 0x08AB904Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804964u) goto L_08804964;
    return;
L_08804964:
    { const bool branch_taken = ctx.gpr[2] != 0u;
    // nop
      if (branch_taken) {
          goto L_088048E0;
      }
      goto L_0880496C;
    }
L_0880496C:
    // nop
    goto L_088048F0;
L_08804974:
    ctx.set_gpr(2, 2233u << 16u);
    goto L_08804978;
L_08804978:
    ctx.set_gpr(4, 2233u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(23868));
    ctx.set_gpr(5, ctx.gpr[2] + static_cast<std::uint32_t>(23880));
    ctx.set_gpr(31, 0x0880498Cu);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(214));
    ctx.pc = 0x08A9C724u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x0880498Cu) goto L_0880498C;
    return;
L_0880498C:
    ctx.set_gpr(19, 2273u << 16u);
    ctx.set_gpr(20, 2273u << 16u);
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(4232), 0u);
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(4244), 0u);
    goto L_088045C8;
L_088049A0:
    ctx.set_gpr(31, 0x088049A8u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(24568));
    ctx.pc = 0x08ABA2BCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049A8u) goto L_088049A8;
    return;
L_088049A8:
    ctx.set_gpr(31, 0x088049B0u);
    // nop
    ctx.pc = 0x08A61F04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049B0u) goto L_088049B0;
    return;
L_088049B0:
    ctx.set_gpr(31, 0x088049B8u);
    // nop
    ctx.pc = 0x08AB69D0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049B8u) goto L_088049B8;
    return;
L_088049B8:
    ctx.set_gpr(31, 0x088049C0u);
    // nop
    ctx.pc = 0x08AB4B68u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049C0u) goto L_088049C0;
    return;
L_088049C0:
    ctx.set_gpr(31, 0x088049C8u);
    // nop
    ctx.pc = 0x08A9A998u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049C8u) goto L_088049C8;
    return;
L_088049C8:
    ctx.set_gpr(31, 0x088049D0u);
    // nop
    ctx.pc = 0x08AB4720u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049D0u) goto L_088049D0;
    return;
L_088049D0:
    ctx.set_gpr(31, 0x088049D8u);
    // nop
    ctx.pc = 0x0881D2E8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049D8u) goto L_088049D8;
    return;
L_088049D8:
    ctx.set_gpr(31, 0x088049E0u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.pc = 0x0881C744u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049E0u) goto L_088049E0;
    return;
L_088049E0:
    ctx.set_gpr(31, 0x088049E8u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(199));
    ctx.pc = 0x08AC4D50u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049E8u) goto L_088049E8;
    return;
L_088049E8:
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(31, 0x088049F4u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(24400));
    ctx.pc = 0x08AC47C4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x088049F4u) goto L_088049F4;
    return;
L_088049F4:
    ctx.set_gpr(4, 2273u << 16u);
    ctx.set_gpr(31, 0x08804A00u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(8592));
    ctx.pc = 0x08B0DE1Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A00u) goto L_08804A00;
    return;
L_08804A00:
    ctx.set_gpr(31, 0x08804A08u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x0880ED98u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A08u) goto L_08804A08;
    return;
L_08804A08:
    ctx.set_gpr(31, 0x08804A10u);
    // nop
    ctx.pc = 0x089BA440u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A10u) goto L_08804A10;
    return;
L_08804A10:
    ctx.set_gpr(31, 0x08804A18u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x0899870Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A18u) goto L_08804A18;
    return;
L_08804A18:
    ctx.set_gpr(31, 0x08804A20u);
    // nop
    ctx.pc = 0x0897D6B0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A20u) goto L_08804A20;
    return;
L_08804A20:
    ctx.set_gpr(31, 0x08804A28u);
    // nop
    ctx.pc = 0x089835ECu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A28u) goto L_08804A28;
    return;
L_08804A28:
    ctx.set_gpr(31, 0x08804A30u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x089878C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A30u) goto L_08804A30;
    return;
L_08804A30:
    ctx.set_gpr(31, 0x08804A38u);
    // nop
    ctx.pc = 0x08977CD8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A38u) goto L_08804A38;
    return;
L_08804A38:
    ctx.set_gpr(31, 0x08804A40u);
    // nop
    ctx.pc = 0x089B87A0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A40u) goto L_08804A40;
    return;
L_08804A40:
    ctx.set_gpr(31, 0x08804A48u);
    // nop
    ctx.pc = 0x08997A94u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A48u) goto L_08804A48;
    return;
L_08804A48:
    ctx.set_gpr(31, 0x08804A50u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x08998234u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A50u) goto L_08804A50;
    return;
L_08804A50:
    ctx.set_gpr(31, 0x08804A58u);
    // nop
    ctx.pc = 0x0899CAA4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A58u) goto L_08804A58;
    return;
L_08804A58:
    ctx.set_gpr(31, 0x08804A60u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x0899CAACu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A60u) goto L_08804A60;
    return;
L_08804A60:
    ctx.set_gpr(31, 0x08804A68u);
    // nop
    ctx.pc = 0x089B33DCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A68u) goto L_08804A68;
    return;
L_08804A68:
    ctx.set_gpr(31, 0x08804A70u);
    // nop
    ctx.pc = 0x0881E65Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A70u) goto L_08804A70;
    return;
L_08804A70:
    ctx.set_gpr(31, 0x08804A78u);
    // nop
    ctx.pc = 0x089B6FA0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A78u) goto L_08804A78;
    return;
L_08804A78:
    ctx.set_gpr(31, 0x08804A80u);
    // nop
    ctx.pc = 0x08994A74u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A80u) goto L_08804A80;
    return;
L_08804A80:
    ctx.set_gpr(31, 0x08804A88u);
    // nop
    ctx.pc = 0x08999F64u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A88u) goto L_08804A88;
    return;
L_08804A88:
    ctx.set_gpr(31, 0x08804A90u);
    // nop
    ctx.pc = 0x08806348u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A90u) goto L_08804A90;
    return;
L_08804A90:
    ctx.set_gpr(31, 0x08804A98u);
    // nop
    ctx.pc = 0x08806490u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804A98u) goto L_08804A98;
    return;
L_08804A98:
    ctx.set_gpr(31, 0x08804AA0u);
    // nop
    ctx.pc = 0x0889DDE8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AA0u) goto L_08804AA0;
    return;
L_08804AA0:
    ctx.set_gpr(31, 0x08804AA8u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x088908CCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AA8u) goto L_08804AA8;
    return;
L_08804AA8:
    ctx.set_gpr(31, 0x08804AB0u);
    // nop
    ctx.pc = 0x088A6E24u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AB0u) goto L_08804AB0;
    return;
L_08804AB0:
    ctx.set_gpr(31, 0x08804AB8u);
    // nop
    ctx.pc = 0x0889716Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AB8u) goto L_08804AB8;
    return;
L_08804AB8:
    ctx.set_gpr(31, 0x08804AC0u);
    // nop
    ctx.pc = 0x088A11A0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AC0u) goto L_08804AC0;
    return;
L_08804AC0:
    ctx.set_gpr(31, 0x08804AC8u);
    // nop
    ctx.pc = 0x0896CE18u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AC8u) goto L_08804AC8;
    return;
L_08804AC8:
    ctx.set_gpr(31, 0x08804AD0u);
    // nop
    ctx.pc = 0x088A12D4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AD0u) goto L_08804AD0;
    return;
L_08804AD0:
    ctx.set_gpr(31, 0x08804AD8u);
    // nop
    ctx.pc = 0x088A15B8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AD8u) goto L_08804AD8;
    return;
L_08804AD8:
    ctx.set_gpr(31, 0x08804AE0u);
    // nop
    ctx.pc = 0x088A1698u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AE0u) goto L_08804AE0;
    return;
L_08804AE0:
    ctx.set_gpr(31, 0x08804AE8u);
    // nop
    ctx.pc = 0x088A90A8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AE8u) goto L_08804AE8;
    return;
L_08804AE8:
    ctx.set_gpr(31, 0x08804AF0u);
    // nop
    ctx.pc = 0x08804C00u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804AF0u) goto L_08804AF0;
    return;
L_08804AF0:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(2));
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(6, 0u + 0u);
    ctx.set_gpr(31, 0x08804B04u);
    ctx.set_gpr(7, 0u + 0u);
    ctx.pc = 0x08804DE0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804B04u) goto L_08804B04;
    return;
L_08804B04:
    ctx.set_gpr(31, 0x08804B0Cu);
    // nop
    ctx.pc = 0x0892C55Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08804B0Cu) goto L_08804B0C;
    return;
L_08804B0C:
    // nop
    goto L_088048D0;
L_08804B14:
    ctx.set_gpr(20, 2273u << 16u);
    goto L_088045C8;
}

void sub_08804538(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08804538_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B4E6A0[56] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 4, 0, 0, 5, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 7, 0,
    8, 0, 0, 0, 0, 0, 9, 10, 0, 11, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 13, 0, 14, 15,
};
void sub_08B4E6A0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B4E6A0u;
        entry_id = (entry_delta < 224u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B4E6A0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B4E6A0;
    case 2u: goto L_08B4E6C4;
    case 3u: goto L_08B4E6D4;
    case 4u: goto L_08B4E6E0;
    case 5u: goto L_08B4E6EC;
    case 6u: goto L_08B4E708;
    case 7u: goto L_08B4E718;
    case 8u: goto L_08B4E720;
    case 9u: goto L_08B4E738;
    case 10u: goto L_08B4E73C;
    case 11u: goto L_08B4E744;
    case 12u: goto L_08B4E75C;
    case 13u: goto L_08B4E770;
    case 14u: goto L_08B4E778;
    case 15u: goto L_08B4E77C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B4E6A0:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(28752)));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(18, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[31]);
    { const bool branch_taken = ctx.gpr[3] != 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B4E738;
      }
      goto L_08B4E6C4;
    }
L_08B4E6C4:
    ctx.set_gpr(3, 2243u << 16u);
    ctx.set_gpr(4, ctx.gpr[3] + static_cast<std::uint32_t>(22612));
    { const bool branch_taken = ctx.gpr[4] == 0u;
    ctx.set_gpr(17, 1u << 16u);
      if (branch_taken) {
          goto L_08B4E6EC;
      }
      goto L_08B4E6D4;
    }
L_08B4E6D4:
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(22612)));
    { const bool branch_taken = ctx.gpr[7] == 0u;
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
      if (branch_taken) {
          goto L_08B4E778;
      }
      goto L_08B4E6E0;
    }
L_08B4E6E0:
    ctx.set_gpr(17, ctx.gpr[7] << 10u);
    { const bool branch_taken = ctx.gpr[17] == 0u;
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(28752)));
      if (branch_taken) {
          goto L_08B4E73C;
      }
      goto L_08B4E6EC;
    }
L_08B4E6EC:
    ctx.set_gpr(2, 2233u << 16u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(2));
    ctx.set_gpr(5, ctx.gpr[2] + static_cast<std::uint32_t>(23844));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(3));
    ctx.set_gpr(7, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, 0x08B4E708u);
    ctx.set_gpr(8, 0u + static_cast<std::uint32_t>(4096));
    ctx.pc = 0x08B7FC1Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B4E708u) goto L_08B4E708;
    return;
L_08B4E708:
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) <= 0;
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(28760), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B4E738;
      }
      goto L_08B4E718;
    }
L_08B4E718:
    ctx.set_gpr(31, 0x08B4E720u);
    // nop
    ctx.pc = 0x08B7FC24u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B4E720u) goto L_08B4E720;
    return;
L_08B4E720:
    ctx.set_gpr(7, ctx.gpr[2] + ctx.gpr[17]);
    ctx.set_gpr(6, 2272u << 16u);
    ctx.set_gpr(5, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(28756), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(28748), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(28752), ctx.gpr[2]);
    goto L_08B4E738;
L_08B4E738:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(28752)));
    goto L_08B4E73C;
L_08B4E73C:
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
        goto L_08B4E778;
    }
    goto L_08B4E744;
L_08B4E744:
    ctx.set_gpr(6, 2272u << 16u);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(28748)));
    ctx.set_gpr(4, ctx.gpr[5] + ctx.gpr[18]);
    ctx.set_gpr(8, ctx.gpr[4] < ctx.gpr[2] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[8] != 0u;
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
      if (branch_taken) {
          goto L_08B4E778;
      }
      goto L_08B4E75C;
    }
L_08B4E75C:
    ctx.set_gpr(11, 2272u << 16u);
    ctx.set_gpr(10, rt.memory().aot_load32(ctx.gpr[11] + static_cast<std::uint32_t>(28756)));
    ctx.set_gpr(9, ctx.gpr[10] < ctx.gpr[4] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[9] != 0u;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
      if (branch_taken) {
          goto L_08B4E77C;
      }
      goto L_08B4E770;
    }
L_08B4E770:
    ctx.set_gpr(2, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(28748), ctx.gpr[4]);
    goto L_08B4E778;
L_08B4E778:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    goto L_08B4E77C;
L_08B4E77C:
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B4E6A0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B4E6A0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B4F714[17] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 4, 0, 0, 5,
};
void sub_08B4F714_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B4F714u;
        entry_id = (entry_delta < 68u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B4F714[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B4F714;
    case 2u: goto L_08B4F73C;
    case 3u: goto L_08B4F740;
    case 4u: goto L_08B4F748;
    case 5u: goto L_08B4F754;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B4F714:
    ctx.set_gpr(2, 2232u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(2308));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(-4)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(-1));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[2] + static_cast<std::uint32_t>(-4));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    { const bool branch_taken = ctx.gpr[4] == ctx.gpr[3];
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B4F754;
      }
      goto L_08B4F73C;
    }
L_08B4F73C:
    ctx.set_gpr(17, 0u + static_cast<std::uint32_t>(-1));
    goto L_08B4F740;
L_08B4F740:
    jump_target = ctx.gpr[4];
    ctx.set_gpr(31, 0x08B4F748u);
    ctx.set_gpr(16, ctx.gpr[16] + static_cast<std::uint32_t>(-4));
    ctx.pc = jump_target;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B4F748u) goto L_08B4F748;
    return;
L_08B4F748:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(0)));
    { const bool branch_taken = ctx.gpr[4] != ctx.gpr[17];
    // nop
      if (branch_taken) {
          goto L_08B4F740;
      }
      goto L_08B4F754;
    }
L_08B4F754:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B4F714(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B4F714_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B72DF8[47] = {
    1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 4, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 6, 7, 0, 0, 0, 0, 0,
    0, 8, 0, 0, 9, 0, 10, 0, 0, 11, 0, 0, 12, 0, 13,
};
void sub_08B72DF8_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B72DF8u;
        entry_id = (entry_delta < 188u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B72DF8[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B72DF8;
    case 2u: goto L_08B72E18;
    case 3u: goto L_08B72E20;
    case 4u: goto L_08B72E30;
    case 5u: goto L_08B72E40;
    case 6u: goto L_08B72E5C;
    case 7u: goto L_08B72E60;
    case 8u: goto L_08B72E7C;
    case 9u: goto L_08B72E88;
    case 10u: goto L_08B72E90;
    case 11u: goto L_08B72E9C;
    case 12u: goto L_08B72EA8;
    case 13u: goto L_08B72EB0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B72DF8:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(19, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(31, 0x08B72E18u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72E18u) goto L_08B72E18;
    return;
L_08B72E18:
    ctx.set_gpr(31, 0x08B72E20u);
    ctx.set_gpr(17, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B73E2Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72E20u) goto L_08B72E20;
    return;
L_08B72E20:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(328)));
    ctx.set_gpr(18, ctx.gpr[2] + 0u);
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
      if (branch_taken) {
          goto L_08B72E7C;
      }
      goto L_08B72E30;
    }
L_08B72E30:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[3]) < 32 ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    // nop
      if (branch_taken) {
          goto L_08B72E88;
      }
      goto L_08B72E40;
    }
L_08B72E40:
    ctx.set_gpr(2, ctx.gpr[3] << 2u);
    ctx.set_gpr(2, ctx.gpr[2] + ctx.gpr[16]);
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(8), ctx.gpr[19]);
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, 0x08B72E5Cu);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72E5Cu) goto L_08B72E5C;
    return;
L_08B72E5C:
    ctx.set_gpr(2, 0u + 0u);
    goto L_08B72E60;
L_08B72E60:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B72E7C:
    ctx.set_gpr(16, ctx.gpr[2] + static_cast<std::uint32_t>(332));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(328), ctx.gpr[16]);
    goto L_08B72E30;
L_08B72E88:
    ctx.set_gpr(31, 0x08B72E90u);
    // nop
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72E90u) goto L_08B72E90;
    return;
L_08B72E90:
    ctx.set_gpr(4, ctx.gpr[18] + 0u);
    ctx.set_gpr(31, 0x08B72E9Cu);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(268));
    ctx.pc = 0x08B73088u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72E9Cu) goto L_08B72E9C;
    return;
L_08B72E9C:
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
      if (branch_taken) {
          goto L_08B72E60;
      }
      goto L_08B72EA8;
    }
L_08B72EA8:
    ctx.set_gpr(31, 0x08B72EB0u);
    // nop
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72EB0u) goto L_08B72EB0;
    return;
L_08B72EB0:
    ctx.set_gpr(17, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[18] + static_cast<std::uint32_t>(328)));
    ctx.set_gpr(3, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(264), 0u);
    rt.memory().aot_store32(ctx.gpr[18] + static_cast<std::uint32_t>(328), ctx.gpr[16]);
    goto L_08B72E40;
}

void sub_08B72DF8(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B72DF8_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B72F04[64] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 4, 0, 0, 5, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0,
    7, 0, 0, 0, 8, 0, 0, 0, 9, 0, 0, 10, 0, 0, 11, 0, 0, 12, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14,
};
void sub_08B72F04_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B72F04u;
        entry_id = (entry_delta < 256u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B72F04[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B72F04;
    case 2u: goto L_08B72F38;
    case 3u: goto L_08B72F44;
    case 4u: goto L_08B72F4C;
    case 5u: goto L_08B72F58;
    case 6u: goto L_08B72F74;
    case 7u: goto L_08B72F84;
    case 8u: goto L_08B72F94;
    case 9u: goto L_08B72FA4;
    case 10u: goto L_08B72FB0;
    case 11u: goto L_08B72FBC;
    case 12u: goto L_08B72FC8;
    case 13u: goto L_08B72FD0;
    case 14u: goto L_08B73000;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B72F04:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-48));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(28), ctx.gpr[23]);
    ctx.set_gpr(23, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(36), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(32), ctx.gpr[30]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[22]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[21]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(31, 0x08B72F38u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.pc = 0x08B73E2Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72F38u) goto L_08B72F38;
    return;
L_08B72F38:
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(328)));
    { const bool branch_taken = ctx.gpr[20] == 0u;
    // nop
      if (branch_taken) {
          goto L_08B72FC8;
      }
      goto L_08B72F44;
    }
L_08B72F44:
    ctx.set_gpr(30, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[20] + static_cast<std::uint32_t>(4)));
    goto L_08B72F4C;
L_08B72F4C:
    ctx.set_gpr(4, ctx.gpr[22] + static_cast<std::uint32_t>(-1));
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[4]) < 0;
    ctx.set_gpr(21, ctx.gpr[20] + static_cast<std::uint32_t>(136));
      if (branch_taken) {
          goto L_08B72FBC;
      }
      goto L_08B72F58;
    }
L_08B72F58:
    ctx.set_gpr(3, ctx.gpr[4] << 2u);
    ctx.set_gpr(2, ctx.gpr[20] + ctx.gpr[3]);
    ctx.set_gpr(17, ctx.gpr[2] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(18, ctx.gpr[21] + ctx.gpr[3]);
    ctx.set_gpr(16, ctx.gpr[30] << (ctx.gpr[4] & 31u));
    ctx.set_gpr(19, 0u + 0u);
    goto L_08B72F94;
L_08B72F74:
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[18] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(4, ctx.gpr[23] + 0u);
    jump_target = ctx.gpr[2];
    ctx.set_gpr(31, 0x08B72F84u);
    ctx.set_gpr(19, ctx.gpr[19] + static_cast<std::uint32_t>(1));
    ctx.pc = jump_target;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72F84u) goto L_08B72F84;
    return;
L_08B72F84:
    ctx.set_gpr(16, static_cast<std::uint32_t>(static_cast<std::int32_t>(ctx.gpr[16]) >> 1u));
    ctx.set_gpr(17, ctx.gpr[17] + static_cast<std::uint32_t>(-4));
    { const bool branch_taken = ctx.gpr[19] == ctx.gpr[22];
    ctx.set_gpr(18, ctx.gpr[18] + static_cast<std::uint32_t>(-4));
      if (branch_taken) {
          goto L_08B72FBC;
      }
      goto L_08B72F94;
    }
L_08B72F94:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[21] + static_cast<std::uint32_t>(128)));
    ctx.set_gpr(2, ctx.gpr[2] & ctx.gpr[16]);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(0)));
      if (branch_taken) {
          goto L_08B72F74;
      }
      goto L_08B72FA4;
    }
L_08B72FA4:
    ctx.set_gpr(19, ctx.gpr[19] + static_cast<std::uint32_t>(1));
    jump_target = ctx.gpr[2];
    ctx.set_gpr(31, 0x08B72FB0u);
    ctx.set_gpr(16, static_cast<std::uint32_t>(static_cast<std::int32_t>(ctx.gpr[16]) >> 1u));
    ctx.pc = jump_target;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72FB0u) goto L_08B72FB0;
    return;
L_08B72FB0:
    ctx.set_gpr(17, ctx.gpr[17] + static_cast<std::uint32_t>(-4));
    { const bool branch_taken = ctx.gpr[19] != ctx.gpr[22];
    ctx.set_gpr(18, ctx.gpr[18] + static_cast<std::uint32_t>(-4));
      if (branch_taken) {
          goto L_08B72F94;
      }
      goto L_08B72FBC;
    }
L_08B72FBC:
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[20] + static_cast<std::uint32_t>(0)));
    if (ctx.gpr[20] != 0u) {
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[20] + static_cast<std::uint32_t>(4)));
        goto L_08B72F4C;
    }
    goto L_08B72FC8;
L_08B72FC8:
    ctx.set_gpr(31, 0x08B72FD0u);
    ctx.set_gpr(4, ctx.gpr[23] + 0u);
    ctx.pc = 0x08B4E5B8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B72FD0u) goto L_08B72FD0;
    return;
L_08B72FD0:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-64));
    ctx.set_gpr(2, ctx.gpr[29] + static_cast<std::uint32_t>(40));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(40), ctx.gpr[6]);
    ctx.set_gpr(6, ctx.gpr[2] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(44), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(48), ctx.gpr[8]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(52), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(56), ctx.gpr[10]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(60), ctx.gpr[11]);
    ctx.set_gpr(31, 0x08B73000u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
    ctx.pc = 0x08B77DACu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73000u) goto L_08B73000;
    return;
L_08B73000:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(64));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B72F04(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B72F04_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B7300C[8] = {
    1, 0, 2, 0, 0, 3, 0, 4,
};
void sub_08B7300C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B7300Cu;
        entry_id = (entry_delta < 32u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B7300C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B7300C;
    case 2u: goto L_08B73014;
    case 3u: goto L_08B73020;
    case 4u: goto L_08B73028;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B7300C:
    { const bool branch_taken = ctx.gpr[26] == 0u;
    ctx.set_gpr(2, 2244u << 16u);
      if (branch_taken) {
          goto L_08B73028;
      }
      goto L_08B73014;
    }
L_08B73014:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[26] + static_cast<std::uint32_t>(4)));
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(2, 2244u << 16u);
        goto L_08B73028;
    }
    goto L_08B73020;
L_08B73020:
    jump_target = ctx.gpr[31];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B73028:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(-23896)));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B7300C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B7300C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B73030[528] = {
    1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 5, 0, 0, 6, 0, 7, 0, 0, 8, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 12, 0, 0, 0, 0, 0, 13, 0, 0, 14, 0, 0, 0, 0, 0,
    0, 0, 0, 15, 0, 16, 0, 0, 0, 17, 0, 0, 18, 0, 0, 19, 0, 20, 21, 0, 0, 0, 0, 22, 0, 0, 23, 0, 24, 25, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 26, 0, 0, 27, 0, 0, 28, 0, 0, 29, 0, 0, 0, 0, 0, 30, 0, 0, 0, 0, 0, 31, 0,
    32, 33, 0, 0, 0, 0, 0, 0, 0, 0, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 35, 0, 36, 0, 0, 0, 0, 0, 37,
    0, 38, 0, 39, 40, 0, 0, 0, 41, 0, 0, 0, 0, 0, 42, 0, 0, 43, 0, 0, 0, 0, 0, 44, 0, 45, 0, 46, 0, 0, 47, 0,
    0, 48, 0, 0, 0, 49, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 51, 0, 0, 52, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 53, 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 55, 0, 0, 0, 0, 0, 56, 0, 0, 0, 57, 58, 0, 0, 0,
    59, 0, 60, 0, 61, 0, 62, 63, 0, 0, 0, 0, 64, 0, 65, 0, 66, 0, 0, 0, 0, 0, 0, 67, 0, 68, 69, 0, 0, 70, 71, 0,
    0, 72, 0, 73, 0, 0, 0, 74, 0, 0, 75, 0, 0, 0, 0, 0, 0, 0, 0, 76, 0, 0, 0, 77, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 78, 0, 0, 79, 0, 0, 80, 0, 81, 0, 0, 82, 0, 0, 83, 0, 0, 84, 0, 0, 0, 85, 86, 0, 0, 0, 87, 0, 0, 88, 89,
    0, 0, 0, 0, 0, 0, 90, 0, 91, 0, 92, 0, 0, 0, 93, 0, 94, 0, 0, 0, 95, 0, 0, 96, 0, 0, 0, 97, 0, 98, 0, 99,
    0, 0, 0, 100, 0, 101, 0, 102, 0, 0, 0, 103, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 104, 0, 105, 0, 0,
    106, 0, 0, 0, 107, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 108, 0, 109, 0, 0, 110, 0, 0, 0, 111, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 112, 0, 0, 0, 113, 0, 0, 114, 0, 0, 115, 0, 0, 116, 0, 0, 0, 117, 0, 0, 0, 0, 0, 0, 118, 0, 119,
    0, 0, 120, 0, 121, 0, 0, 0, 122, 0, 0, 0, 123, 0, 0, 124,
};
void sub_08B73030_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B73030u;
        entry_id = (entry_delta < 2112u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B73030[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B73030;
    case 2u: goto L_08B73044;
    case 3u: goto L_08B73088;
    case 4u: goto L_08B730C8;
    case 5u: goto L_08B730D8;
    case 6u: goto L_08B730E4;
    case 7u: goto L_08B730EC;
    case 8u: goto L_08B730F8;
    case 9u: goto L_08B73110;
    case 10u: goto L_08B7313C;
    case 11u: goto L_08B73170;
    case 12u: goto L_08B73174;
    case 13u: goto L_08B7318C;
    case 14u: goto L_08B73198;
    case 15u: goto L_08B731BC;
    case 16u: goto L_08B731C4;
    case 17u: goto L_08B731D4;
    case 18u: goto L_08B731E0;
    case 19u: goto L_08B731EC;
    case 20u: goto L_08B731F4;
    case 21u: goto L_08B731F8;
    case 22u: goto L_08B7320C;
    case 23u: goto L_08B73218;
    case 24u: goto L_08B73220;
    case 25u: goto L_08B73224;
    case 26u: goto L_08B73254;
    case 27u: goto L_08B73260;
    case 28u: goto L_08B7326C;
    case 29u: goto L_08B73278;
    case 30u: goto L_08B73290;
    case 31u: goto L_08B732A8;
    case 32u: goto L_08B732B0;
    case 33u: goto L_08B732B4;
    case 34u: goto L_08B732D8;
    case 35u: goto L_08B7330C;
    case 36u: goto L_08B73314;
    case 37u: goto L_08B7332C;
    case 38u: goto L_08B73334;
    case 39u: goto L_08B7333C;
    case 40u: goto L_08B73340;
    case 41u: goto L_08B73350;
    case 42u: goto L_08B73368;
    case 43u: goto L_08B73374;
    case 44u: goto L_08B7338C;
    case 45u: goto L_08B73394;
    case 46u: goto L_08B7339C;
    case 47u: goto L_08B733A8;
    case 48u: goto L_08B733B4;
    case 49u: goto L_08B733C4;
    case 50u: goto L_08B733F4;
    case 51u: goto L_08B733FC;
    case 52u: goto L_08B73408;
    case 53u: goto L_08B73444;
    case 54u: goto L_08B73468;
    case 55u: goto L_08B73474;
    case 56u: goto L_08B7348C;
    case 57u: goto L_08B7349C;
    case 58u: goto L_08B734A0;
    case 59u: goto L_08B734B0;
    case 60u: goto L_08B734B8;
    case 61u: goto L_08B734C0;
    case 62u: goto L_08B734C8;
    case 63u: goto L_08B734CC;
    case 64u: goto L_08B734E0;
    case 65u: goto L_08B734E8;
    case 66u: goto L_08B734F0;
    case 67u: goto L_08B7350C;
    case 68u: goto L_08B73514;
    case 69u: goto L_08B73518;
    case 70u: goto L_08B73524;
    case 71u: goto L_08B73528;
    case 72u: goto L_08B73534;
    case 73u: goto L_08B7353C;
    case 74u: goto L_08B7354C;
    case 75u: goto L_08B73558;
    case 76u: goto L_08B7357C;
    case 77u: goto L_08B7358C;
    case 78u: goto L_08B735B4;
    case 79u: goto L_08B735C0;
    case 80u: goto L_08B735CC;
    case 81u: goto L_08B735D4;
    case 82u: goto L_08B735E0;
    case 83u: goto L_08B735EC;
    case 84u: goto L_08B735F8;
    case 85u: goto L_08B73608;
    case 86u: goto L_08B7360C;
    case 87u: goto L_08B7361C;
    case 88u: goto L_08B73628;
    case 89u: goto L_08B7362C;
    case 90u: goto L_08B73648;
    case 91u: goto L_08B73650;
    case 92u: goto L_08B73658;
    case 93u: goto L_08B73668;
    case 94u: goto L_08B73670;
    case 95u: goto L_08B73680;
    case 96u: goto L_08B7368C;
    case 97u: goto L_08B7369C;
    case 98u: goto L_08B736A4;
    case 99u: goto L_08B736AC;
    case 100u: goto L_08B736BC;
    case 101u: goto L_08B736C4;
    case 102u: goto L_08B736CC;
    case 103u: goto L_08B736DC;
    case 104u: goto L_08B7371C;
    case 105u: goto L_08B73724;
    case 106u: goto L_08B73730;
    case 107u: goto L_08B73740;
    case 108u: goto L_08B7376C;
    case 109u: goto L_08B73774;
    case 110u: goto L_08B73780;
    case 111u: goto L_08B73790;
    case 112u: goto L_08B737C4;
    case 113u: goto L_08B737D4;
    case 114u: goto L_08B737E0;
    case 115u: goto L_08B737EC;
    case 116u: goto L_08B737F8;
    case 117u: goto L_08B73808;
    case 118u: goto L_08B73824;
    case 119u: goto L_08B7382C;
    case 120u: goto L_08B73838;
    case 121u: goto L_08B73840;
    case 122u: goto L_08B73850;
    case 123u: goto L_08B73860;
    case 124u: goto L_08B7386C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B73030:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(31, 0x08B73044u);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    ctx.pc = 0x08B7300Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73044u) goto L_08B73044;
    return;
L_08B73044:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(5, ctx.gpr[16] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    goto L_08B73088;
L_08B73088:
    ctx.set_gpr(3, ctx.gpr[5] + static_cast<std::uint32_t>(19));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-80));
    ctx.set_gpr(2, ctx.gpr[3] < static_cast<std::uint32_t>(31) ? 1u : 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(44), ctx.gpr[19]);
    ctx.set_gpr(19, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(36), ctx.gpr[17]);
    ctx.set_gpr(17, 0u + static_cast<std::uint32_t>(16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(68), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(64), ctx.gpr[30]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(60), ctx.gpr[23]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(56), ctx.gpr[22]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(52), ctx.gpr[21]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(48), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(40), ctx.gpr[18]);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(32), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08B730D8;
      }
      goto L_08B730C8;
    }
L_08B730C8:
    ctx.set_gpr(17, ctx.gpr[3] + 0u);
    ctx.set_gpr(17, (ctx.gpr[17] & ~0x0000000Fu) | ((0u & 0x0000000Fu) << 0u));
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[17]) < 0;
    ctx.set_gpr(2, 0u + 0u);
      if (branch_taken) {
          goto L_08B73224;
      }
      goto L_08B730D8;
    }
L_08B730D8:
    ctx.set_gpr(2, ctx.gpr[17] < ctx.gpr[5] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(2, 0u + 0u);
      if (branch_taken) {
          goto L_08B73224;
      }
      goto L_08B730E4;
    }
L_08B730E4:
    ctx.set_gpr(31, 0x08B730ECu);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.pc = 0x08B73DC8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B730ECu) goto L_08B730EC;
    return;
L_08B730EC:
    ctx.set_gpr(2, ctx.gpr[17] < static_cast<std::uint32_t>(504) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(3, ctx.gpr[17] >> 3u);
      if (branch_taken) {
          goto L_08B73254;
      }
      goto L_08B730F8;
    }
L_08B730F8:
    ctx.set_gpr(8, 2244u << 16u);
    ctx.set_gpr(12, ctx.gpr[8] + static_cast<std::uint32_t>(-22984));
    ctx.set_gpr(2, ctx.gpr[17] + ctx.gpr[12]);
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(12)));
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[16];
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
      if (branch_taken) {
          goto L_08B7333C;
      }
      goto L_08B73110;
    }
L_08B73110:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(3, (ctx.gpr[3] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[3]);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(4)));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(8), ctx.gpr[5]);
    ctx.set_gpr(2, ctx.gpr[2] | 1u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    ctx.set_gpr(31, 0x08B7313Cu);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(12), ctx.gpr[6]);
    ctx.pc = 0x08B73E04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7313Cu) goto L_08B7313C;
    return;
L_08B7313C:
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(68)));
    ctx.set_gpr(30, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(64)));
    ctx.set_gpr(23, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(60)));
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(56)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(52)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(48)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(44)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(40)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(80));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B73170:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    goto L_08B73174;
L_08B73174:
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(21, (ctx.gpr[21] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(2, ctx.gpr[21] < ctx.gpr[17] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(5, ctx.gpr[21] - ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B73198;
      }
      goto L_08B7318C;
    }
L_08B7318C:
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[5]) < 16 ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B734F0;
      }
      goto L_08B73198;
    }
L_08B73198:
    ctx.set_gpr(2, 2273u << 16u);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(3852)));
    ctx.set_gpr(30, 2244u << 16u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[30] + static_cast<std::uint32_t>(-22992)));
    ctx.set_gpr(3, ctx.gpr[17] + ctx.gpr[3]);
    ctx.set_gpr(23, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(22, ctx.gpr[16] + ctx.gpr[21]);
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[23];
    ctx.set_gpr(18, ctx.gpr[3] + static_cast<std::uint32_t>(16));
      if (branch_taken) {
          goto L_08B731C4;
      }
      goto L_08B731BC;
    }
L_08B731BC:
    ctx.set_gpr(18, ctx.gpr[3] + static_cast<std::uint32_t>(4111));
    ctx.set_gpr(18, (ctx.gpr[18] & ~0x00000FFFu) | ((0u & 0x00000FFFu) << 0u));
    goto L_08B731C4;
L_08B731C4:
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.set_gpr(5, ctx.gpr[18] + 0u);
    ctx.set_gpr(31, 0x08B731D4u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[13]);
    ctx.pc = 0x08B7500Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B731D4u) goto L_08B731D4;
    return;
L_08B731D4:
    ctx.set_gpr(20, ctx.gpr[2] + 0u);
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[23];
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
      if (branch_taken) {
          goto L_08B731F4;
      }
      goto L_08B731E0;
    }
L_08B731E0:
    ctx.set_gpr(2, ctx.gpr[2] < ctx.gpr[22] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(23, 2273u << 16u);
      if (branch_taken) {
          goto L_08B73514;
      }
      goto L_08B731EC;
    }
L_08B731EC:
    { const bool branch_taken = ctx.gpr[16] == ctx.gpr[13];
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[23] + static_cast<std::uint32_t>(3804)));
      if (branch_taken) {
          goto L_08B73518;
      }
      goto L_08B731F4;
    }
L_08B731F4:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    goto L_08B731F8;
L_08B731F8:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, (ctx.gpr[2] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(3, ctx.gpr[2] < ctx.gpr[17] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[3] != 0u;
    ctx.set_gpr(5, ctx.gpr[2] - ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B73218;
      }
      goto L_08B7320C;
    }
L_08B7320C:
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[5]) < 16 ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B734F0;
      }
      goto L_08B73218;
    }
L_08B73218:
    ctx.set_gpr(31, 0x08B73220u);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.pc = 0x08B73E04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73220u) goto L_08B73220;
    return;
L_08B73220:
    ctx.set_gpr(2, 0u + 0u);
    goto L_08B73224;
L_08B73224:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(68)));
    ctx.set_gpr(30, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(64)));
    ctx.set_gpr(23, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(60)));
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(56)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(52)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(48)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(44)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(40)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(80));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B73254:
    ctx.set_gpr(4, ctx.gpr[17] >> 9u);
    { const bool branch_taken = ctx.gpr[4] == 0u;
    ctx.set_gpr(5, ctx.gpr[3] << 3u);
      if (branch_taken) {
          goto L_08B73278;
      }
      goto L_08B73260;
    }
L_08B73260:
    ctx.set_gpr(2, ctx.gpr[4] < static_cast<std::uint32_t>(5) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(3, ctx.gpr[4] + static_cast<std::uint32_t>(91));
      if (branch_taken) {
          goto L_08B7339C;
      }
      goto L_08B7326C;
    }
L_08B7326C:
    ctx.set_gpr(2, ctx.gpr[17] >> 6u);
    ctx.set_gpr(3, ctx.gpr[2] + static_cast<std::uint32_t>(56));
    ctx.set_gpr(5, ctx.gpr[3] << 3u);
    goto L_08B73278;
L_08B73278:
    ctx.set_gpr(8, 2244u << 16u);
    ctx.set_gpr(12, ctx.gpr[8] + static_cast<std::uint32_t>(-22984));
    ctx.set_gpr(6, ctx.gpr[5] + ctx.gpr[12]);
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(12)));
    { const bool branch_taken = ctx.gpr[6] == ctx.gpr[16];
    ctx.set_gpr(10, ctx.gpr[3] + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08B73340;
      }
      goto L_08B73290;
    }
L_08B73290:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(4, (ctx.gpr[4] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(5, ctx.gpr[4] - ctx.gpr[17]);
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[5]) < 16 ? 1u : 0u);
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-1));
        goto L_08B73394;
    }
    goto L_08B732A8;
L_08B732A8:
    if (static_cast<std::int32_t>(ctx.gpr[5]) < 0) {
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
        goto L_08B7330C;
    }
    goto L_08B732B0;
L_08B732B0:
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[4]);
    goto L_08B732B4;
L_08B732B4:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, ctx.gpr[2] | 1u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(8), ctx.gpr[5]);
    ctx.set_gpr(31, 0x08B732D8u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(12), ctx.gpr[6]);
    ctx.pc = 0x08B73E04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B732D8u) goto L_08B732D8;
    return;
L_08B732D8:
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(68)));
    ctx.set_gpr(30, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(64)));
    ctx.set_gpr(23, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(60)));
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(56)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(52)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(48)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(44)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(40)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(80));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B7330C:
    { const bool branch_taken = ctx.gpr[6] == ctx.gpr[16];
    ctx.set_gpr(10, ctx.gpr[3] + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08B73340;
      }
      goto L_08B73314;
    }
L_08B73314:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(4, (ctx.gpr[4] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(5, ctx.gpr[4] - ctx.gpr[17]);
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[5]) < 16 ? 1u : 0u);
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-1));
        goto L_08B73394;
    }
    goto L_08B7332C;
L_08B7332C:
    if (static_cast<std::int32_t>(ctx.gpr[5]) < 0) {
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
        goto L_08B7330C;
    }
    goto L_08B73334;
L_08B73334:
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[4]);
    goto L_08B732B4;
L_08B7333C:
    ctx.set_gpr(10, ctx.gpr[3] + static_cast<std::uint32_t>(2));
    goto L_08B73340;
L_08B73340:
    ctx.set_gpr(11, ctx.gpr[12] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[11] + static_cast<std::uint32_t>(8)));
    { const bool branch_taken = ctx.gpr[16] == ctx.gpr[11];
    ctx.set_gpr(13, ctx.gpr[8] + static_cast<std::uint32_t>(-22984));
      if (branch_taken) {
          goto L_08B735CC;
      }
      goto L_08B73350;
    }
L_08B73350:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(3, (ctx.gpr[3] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(4, ctx.gpr[3] - ctx.gpr[17]);
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[4]) < 16 ? 1u : 0u);
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(2, ctx.gpr[16] + ctx.gpr[17]);
        goto L_08B733C4;
    }
    goto L_08B73368;
L_08B73368:
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(12), ctx.gpr[11]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[4]) < 0;
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(8), ctx.gpr[11]);
      if (branch_taken) {
          goto L_08B733FC;
      }
      goto L_08B73374;
    }
L_08B73374:
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[3]);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.set_gpr(2, ctx.gpr[2] | 1u);
    ctx.set_gpr(31, 0x08B7338Cu);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    ctx.pc = 0x08B73E04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7338Cu) goto L_08B7338C;
    return;
L_08B7338C:
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(8));
    goto L_08B73224;
L_08B73394:
    ctx.set_gpr(10, ctx.gpr[3] + static_cast<std::uint32_t>(1));
    goto L_08B73340;
L_08B7339C:
    ctx.set_gpr(2, ctx.gpr[4] < static_cast<std::uint32_t>(21) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(5, ctx.gpr[3] << 3u);
      if (branch_taken) {
          goto L_08B73278;
      }
      goto L_08B733A8;
    }
L_08B733A8:
    ctx.set_gpr(2, ctx.gpr[4] < static_cast<std::uint32_t>(85) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(2, ctx.gpr[4] < static_cast<std::uint32_t>(341) ? 1u : 0u);
      if (branch_taken) {
          goto L_08B736C4;
      }
      goto L_08B733B4;
    }
L_08B733B4:
    ctx.set_gpr(2, ctx.gpr[17] >> 12u);
    ctx.set_gpr(3, ctx.gpr[2] + static_cast<std::uint32_t>(110));
    ctx.set_gpr(5, ctx.gpr[3] << 3u);
    goto L_08B73278;
L_08B733C4:
    ctx.set_gpr(3, ctx.gpr[2] + ctx.gpr[4]);
    ctx.set_gpr(6, ctx.gpr[4] | 1u);
    ctx.set_gpr(5, ctx.gpr[17] | 1u);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(12), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(0), ctx.gpr[4]);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(8), ctx.gpr[11]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(4), ctx.gpr[6]);
    ctx.set_gpr(31, 0x08B733F4u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(12), ctx.gpr[11]);
    ctx.pc = 0x08B73E04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B733F4u) goto L_08B733F4;
    return;
L_08B733F4:
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(8));
    goto L_08B73224;
L_08B733FC:
    ctx.set_gpr(2, ctx.gpr[3] < static_cast<std::uint32_t>(512) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, ctx.gpr[3] >> 3u);
      if (branch_taken) {
          goto L_08B735D4;
      }
      goto L_08B73408;
    }
L_08B73408:
    ctx.set_gpr(3, ctx.gpr[3] >> 3u);
    ctx.set_gpr(13, ctx.gpr[8] + static_cast<std::uint32_t>(-22984));
    ctx.set_gpr(4, ctx.gpr[3] << 3u);
    ctx.set_gpr(4, ctx.gpr[12] + ctx.gpr[4]);
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(3, ctx.gpr[3] >> 2u);
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, ctx.gpr[2] << (ctx.gpr[3] & 31u));
    ctx.set_gpr(8, ctx.gpr[8] | ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(12), ctx.gpr[4]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(8), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(12), ctx.gpr[16]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(8), ctx.gpr[16]);
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(4), ctx.gpr[8]);
    goto L_08B73444;
L_08B73444:
    ctx.set_gpr(3, static_cast<std::int32_t>(ctx.gpr[10]) < 0 ? 1u : 0u);
    ctx.set_gpr(2, ctx.gpr[10] + static_cast<std::uint32_t>(3));
    if (ctx.gpr[3] == 0u) ctx.set_gpr(2, ctx.gpr[10]);
    ctx.set_gpr(2, static_cast<std::uint32_t>(static_cast<std::int32_t>(ctx.gpr[2]) >> 2u));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(5, ctx.gpr[3] << (ctx.gpr[2] & 31u));
    ctx.set_gpr(4, ctx.gpr[8] < ctx.gpr[5] ? 1u : 0u);
    if (ctx.gpr[4] != 0u) {
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
        goto L_08B73174;
    }
    goto L_08B73468;
L_08B73468:
    ctx.set_gpr(2, ctx.gpr[5] & ctx.gpr[8]);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(2, ctx.gpr[10] << 3u);
      if (branch_taken) {
          goto L_08B734A0;
      }
      goto L_08B73474;
    }
L_08B73474:
    ctx.set_gpr(2, ctx.gpr[10] + 0u);
    ctx.set_gpr(5, ctx.gpr[5] << 1u);
    ctx.set_gpr(2, (ctx.gpr[2] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(3, ctx.gpr[8] & ctx.gpr[5]);
    { const bool branch_taken = ctx.gpr[3] != 0u;
    ctx.set_gpr(10, ctx.gpr[2] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B7349C;
      }
      goto L_08B7348C;
    }
L_08B7348C:
    ctx.set_gpr(5, ctx.gpr[5] << 1u);
    ctx.set_gpr(2, ctx.gpr[8] & ctx.gpr[5]);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(10, ctx.gpr[10] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B7348C;
      }
      goto L_08B7349C;
    }
L_08B7349C:
    ctx.set_gpr(2, ctx.gpr[10] << 3u);
    goto L_08B734A0;
L_08B734A0:
    ctx.set_gpr(7, ctx.gpr[12] + ctx.gpr[2]);
    ctx.set_gpr(6, ctx.gpr[10] + 0u);
    ctx.set_gpr(4, ctx.gpr[7] + 0u);
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(12)));
    goto L_08B734B0;
L_08B734B0:
    if (ctx.gpr[4] != ctx.gpr[16]) {
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
        goto L_08B734CC;
    }
    goto L_08B734B8;
L_08B734B8:
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[6]) < 63 ? 1u : 0u);
    goto L_08B73648;
L_08B734C0:
    { const bool branch_taken = ctx.gpr[4] == ctx.gpr[16];
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[6]) < 63 ? 1u : 0u);
      if (branch_taken) {
          goto L_08B73648;
      }
      goto L_08B734C8;
    }
L_08B734C8:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    goto L_08B734CC;
L_08B734CC:
    ctx.set_gpr(3, (ctx.gpr[3] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(9, ctx.gpr[3] - ctx.gpr[17]);
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[9]) < 16 ? 1u : 0u);
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
        goto L_08B736DC;
    }
    goto L_08B734E0;
L_08B734E0:
    if (static_cast<std::int32_t>(ctx.gpr[9]) < 0) {
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
        goto L_08B734C0;
    }
    goto L_08B734E8;
L_08B734E8:
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[3]);
    goto L_08B732B4;
L_08B734F0:
    ctx.set_gpr(2, ctx.gpr[17] | 1u);
    ctx.set_gpr(5, ctx.gpr[5] | 1u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[5]);
    ctx.set_gpr(31, 0x08B7350Cu);
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(8), ctx.gpr[3]);
    ctx.pc = 0x08B73E04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7350Cu) goto L_08B7350C;
    return;
L_08B7350C:
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(8));
    goto L_08B73224;
L_08B73514:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[23] + static_cast<std::uint32_t>(3804)));
    goto L_08B73518;
L_08B73518:
    ctx.set_gpr(4, ctx.gpr[18] + ctx.gpr[2]);
    { const bool branch_taken = ctx.gpr[22] == ctx.gpr[20];
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(3804), ctx.gpr[4]);
      if (branch_taken) {
          goto L_08B73724;
      }
      goto L_08B73524;
    }
L_08B73524:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[30] + static_cast<std::uint32_t>(-22992)));
    goto L_08B73528;
L_08B73528:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
    { const bool branch_taken = ctx.gpr[3] == ctx.gpr[2];
    ctx.set_gpr(2, ctx.gpr[20] - ctx.gpr[22]);
      if (branch_taken) {
          goto L_08B73824;
      }
      goto L_08B73534;
    }
L_08B73534:
    ctx.set_gpr(2, ctx.gpr[4] + ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(3804), ctx.gpr[2]);
    goto L_08B7353C;
L_08B7353C:
    ctx.set_gpr(2, ctx.gpr[20] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(3, ctx.gpr[2] & 15u);
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(5, 0u + 0u);
      if (branch_taken) {
          goto L_08B73558;
      }
      goto L_08B7354C;
    }
L_08B7354C:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(16));
    ctx.set_gpr(5, ctx.gpr[2] - ctx.gpr[3]);
    ctx.set_gpr(20, ctx.gpr[20] + ctx.gpr[5]);
    goto L_08B73558;
L_08B73558:
    ctx.set_gpr(3, ctx.gpr[20] + ctx.gpr[18]);
    ctx.set_gpr(3, ctx.gpr[3] & 4095u);
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(4096));
    ctx.set_gpr(2, ctx.gpr[2] - ctx.gpr[3]);
    ctx.set_gpr(18, ctx.gpr[5] + ctx.gpr[2]);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.set_gpr(5, ctx.gpr[18] + 0u);
    ctx.set_gpr(31, 0x08B7357Cu);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[13]);
    ctx.pc = 0x08B7500Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7357Cu) goto L_08B7357C;
    return;
L_08B7357C:
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(4, ctx.gpr[2] - ctx.gpr[20]);
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[3];
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
      if (branch_taken) {
          goto L_08B7382C;
      }
      goto L_08B7358C;
    }
L_08B7358C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[23] + static_cast<std::uint32_t>(3804)));
    ctx.set_gpr(3, ctx.gpr[18] + ctx.gpr[4]);
    ctx.set_gpr(3, ctx.gpr[3] | 1u);
    ctx.set_gpr(2, ctx.gpr[18] + ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(3804), ctx.gpr[2]);
    ctx.set_gpr(16, ctx.gpr[20] + 0u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[13];
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(8), ctx.gpr[20]);
      if (branch_taken) {
          goto L_08B73740;
      }
      goto L_08B735B4;
    }
L_08B735B4:
    ctx.set_gpr(2, ctx.gpr[21] < static_cast<std::uint32_t>(16) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
      if (branch_taken) {
          goto L_08B73790;
      }
      goto L_08B735C0;
    }
L_08B735C0:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    goto L_08B731F8;
L_08B735CC:
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(4)));
    goto L_08B73444;
L_08B735D4:
    ctx.set_gpr(5, ctx.gpr[3] >> 9u);
    { const bool branch_taken = ctx.gpr[5] == 0u;
    ctx.set_gpr(6, ctx.gpr[4] << 3u);
      if (branch_taken) {
          goto L_08B735F8;
      }
      goto L_08B735E0;
    }
L_08B735E0:
    ctx.set_gpr(2, ctx.gpr[5] < static_cast<std::uint32_t>(5) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, ctx.gpr[5] + static_cast<std::uint32_t>(91));
      if (branch_taken) {
          goto L_08B737E0;
      }
      goto L_08B735EC;
    }
L_08B735EC:
    ctx.set_gpr(2, ctx.gpr[3] >> 6u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(56));
    ctx.set_gpr(6, ctx.gpr[4] << 3u);
    goto L_08B735F8;
L_08B735F8:
    ctx.set_gpr(7, ctx.gpr[12] + ctx.gpr[6]);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(8)));
    if (ctx.gpr[7] == ctx.gpr[6]) {
    ctx.set_gpr(5, ctx.gpr[8] + static_cast<std::uint32_t>(-22984));
        goto L_08B73808;
    }
    goto L_08B73608;
L_08B73608:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(4)));
    goto L_08B7360C;
L_08B7360C:
    ctx.set_gpr(2, (ctx.gpr[2] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(2, ctx.gpr[3] < ctx.gpr[2] ? 1u : 0u);
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(12)));
        goto L_08B7362C;
    }
    goto L_08B7361C;
L_08B7361C:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(8)));
    if (ctx.gpr[7] != ctx.gpr[6]) {
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(4)));
        goto L_08B7360C;
    }
    goto L_08B73628;
L_08B73628:
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(12)));
    goto L_08B7362C;
L_08B7362C:
    ctx.set_gpr(13, ctx.gpr[8] + static_cast<std::uint32_t>(-22984));
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(4)));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(12), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(8), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(12), ctx.gpr[16]);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(8), ctx.gpr[16]);
    goto L_08B73444;
L_08B73648:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(8));
      if (branch_taken) {
          goto L_08B73658;
      }
      goto L_08B73650;
    }
L_08B73650:
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(16));
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(1));
    goto L_08B73658;
L_08B73658:
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, ctx.gpr[6] & 3u);
    if (ctx.gpr[2] != 0u) {
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(12)));
        goto L_08B734B0;
    }
    goto L_08B73668;
L_08B73668:
    ctx.set_gpr(3, ctx.gpr[10] + 0u);
    ctx.set_gpr(4, ctx.gpr[7] + 0u);
    goto L_08B73670;
L_08B73670:
    ctx.set_gpr(2, ctx.gpr[3] & 3u);
    ctx.set_gpr(7, ctx.gpr[7] + static_cast<std::uint32_t>(-8));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-1));
      if (branch_taken) {
          goto L_08B73850;
      }
      goto L_08B73680;
    }
L_08B73680:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(0)));
    { const bool branch_taken = ctx.gpr[7] == ctx.gpr[2];
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-8));
      if (branch_taken) {
          goto L_08B73670;
      }
      goto L_08B7368C;
    }
L_08B7368C:
    ctx.set_gpr(5, ctx.gpr[5] << 1u);
    ctx.set_gpr(2, ctx.gpr[8] < ctx.gpr[5] ? 1u : 0u);
    if (ctx.gpr[2] != 0u) {
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
        goto L_08B73174;
    }
    goto L_08B7369C;
L_08B7369C:
    { const bool branch_taken = ctx.gpr[5] == 0u;
    ctx.set_gpr(2, ctx.gpr[5] & ctx.gpr[8]);
      if (branch_taken) {
          goto L_08B73170;
      }
      goto L_08B736A4;
    }
L_08B736A4:
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(10, ctx.gpr[6] + 0u);
      if (branch_taken) {
          goto L_08B7349C;
      }
      goto L_08B736AC;
    }
L_08B736AC:
    ctx.set_gpr(5, ctx.gpr[5] << 1u);
    ctx.set_gpr(2, ctx.gpr[8] & ctx.gpr[5]);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B736AC;
      }
      goto L_08B736BC;
    }
L_08B736BC:
    ctx.set_gpr(10, ctx.gpr[6] + 0u);
    goto L_08B7349C;
L_08B736C4:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(2, ctx.gpr[4] < static_cast<std::uint32_t>(1365) ? 1u : 0u);
      if (branch_taken) {
          goto L_08B73774;
      }
      goto L_08B736CC;
    }
L_08B736CC:
    ctx.set_gpr(2, ctx.gpr[17] >> 15u);
    ctx.set_gpr(3, ctx.gpr[2] + static_cast<std::uint32_t>(119));
    ctx.set_gpr(5, ctx.gpr[3] << 3u);
    goto L_08B73278;
L_08B736DC:
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, ctx.gpr[16] + ctx.gpr[17]);
    ctx.set_gpr(3, ctx.gpr[2] + ctx.gpr[9]);
    ctx.set_gpr(5, ctx.gpr[17] | 1u);
    ctx.set_gpr(6, ctx.gpr[9] | 1u);
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(8), ctx.gpr[7]);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(12), ctx.gpr[8]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(12), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(8), ctx.gpr[11]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(4), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(12), ctx.gpr[11]);
    ctx.set_gpr(31, 0x08B7371Cu);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(0), ctx.gpr[9]);
    ctx.pc = 0x08B73E04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7371Cu) goto L_08B7371C;
    return;
L_08B7371C:
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(8));
    goto L_08B73224;
L_08B73724:
    ctx.set_gpr(2, ctx.gpr[20] & 4095u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[30] + static_cast<std::uint32_t>(-22992)));
      if (branch_taken) {
          goto L_08B73528;
      }
      goto L_08B73730;
    }
L_08B73730:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, ctx.gpr[18] + ctx.gpr[21]);
    ctx.set_gpr(2, ctx.gpr[2] | 1u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    goto L_08B73740;
L_08B73740:
    ctx.set_gpr(5, 2273u << 16u);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[23] + static_cast<std::uint32_t>(3804)));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(3848)));
    ctx.set_gpr(7, 2273u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(3844)));
    ctx.set_gpr(3, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, ctx.gpr[2] < ctx.gpr[6] ? 1u : 0u);
    if (ctx.gpr[2] != 0u) ctx.set_gpr(3, ctx.gpr[6]);
    ctx.set_gpr(4, ctx.gpr[4] < ctx.gpr[6] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[4] == 0u;
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(3848), ctx.gpr[3]);
      if (branch_taken) {
          goto L_08B731F8;
      }
      goto L_08B7376C;
    }
L_08B7376C:
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(3844), ctx.gpr[6]);
    goto L_08B731F8;
L_08B73774:
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(126));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1008));
      if (branch_taken) {
          goto L_08B73278;
      }
      goto L_08B73780;
    }
L_08B73780:
    ctx.set_gpr(2, ctx.gpr[17] >> 18u);
    ctx.set_gpr(3, ctx.gpr[2] + static_cast<std::uint32_t>(124));
    ctx.set_gpr(5, ctx.gpr[3] << 3u);
    goto L_08B73278;
L_08B73790:
    ctx.set_gpr(2, ctx.gpr[21] + static_cast<std::uint32_t>(-12));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, (ctx.gpr[2] & ~0x0000000Fu) | ((0u & 0x0000000Fu) << 0u));
    ctx.set_gpr(5, ctx.gpr[2] < static_cast<std::uint32_t>(16) ? 1u : 0u);
    ctx.set_gpr(3, ctx.gpr[3] & 1u);
    ctx.set_gpr(3, ctx.gpr[2] | ctx.gpr[3]);
    ctx.set_gpr(2, ctx.gpr[4] + ctx.gpr[2]);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(5));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(8), ctx.gpr[4]);
    { const bool branch_taken = ctx.gpr[5] != 0u;
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
      if (branch_taken) {
          goto L_08B73740;
      }
      goto L_08B737C4;
    }
L_08B737C4:
    ctx.set_gpr(5, ctx.gpr[6] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.set_gpr(31, 0x08B737D4u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[13]);
    ctx.pc = 0x08B7B5D0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B737D4u) goto L_08B737D4;
    return;
L_08B737D4:
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    goto L_08B73740;
L_08B737E0:
    ctx.set_gpr(2, ctx.gpr[5] < static_cast<std::uint32_t>(21) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(6, ctx.gpr[4] << 3u);
      if (branch_taken) {
          goto L_08B735F8;
      }
      goto L_08B737EC;
    }
L_08B737EC:
    ctx.set_gpr(2, ctx.gpr[5] < static_cast<std::uint32_t>(85) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(2, ctx.gpr[5] < static_cast<std::uint32_t>(341) ? 1u : 0u);
      if (branch_taken) {
          goto L_08B73838;
      }
      goto L_08B737F8;
    }
L_08B737F8:
    ctx.set_gpr(2, ctx.gpr[3] >> 12u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(110));
    ctx.set_gpr(6, ctx.gpr[4] << 3u);
    goto L_08B735F8;
L_08B73808:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(4, ctx.gpr[4] >> 2u);
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, ctx.gpr[2] << (ctx.gpr[4] & 31u));
    ctx.set_gpr(3, ctx.gpr[3] | ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    goto L_08B7362C;
L_08B73824:
    rt.memory().aot_store32(ctx.gpr[30] + static_cast<std::uint32_t>(-22992), ctx.gpr[20]);
    goto L_08B7353C;
L_08B7382C:
    ctx.set_gpr(18, 0u + 0u);
    ctx.set_gpr(4, 0u + 0u);
    goto L_08B7358C;
L_08B73838:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(2, ctx.gpr[5] < static_cast<std::uint32_t>(1365) ? 1u : 0u);
      if (branch_taken) {
          goto L_08B73860;
      }
      goto L_08B73840;
    }
L_08B73840:
    ctx.set_gpr(2, ctx.gpr[3] >> 15u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(119));
    ctx.set_gpr(6, ctx.gpr[4] << 3u);
    goto L_08B735F8;
L_08B73850:
    ctx.set_gpr(2, ~(0u | ctx.gpr[5]));
    ctx.set_gpr(8, ctx.gpr[8] & ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(4), ctx.gpr[8]);
    goto L_08B7368C;
L_08B73860:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(126));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(1008));
      if (branch_taken) {
          goto L_08B735F8;
      }
      goto L_08B7386C;
    }
L_08B7386C:
    ctx.set_gpr(2, ctx.gpr[3] >> 18u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(124));
    ctx.set_gpr(6, ctx.gpr[4] << 3u);
    goto L_08B735F8;
}

void sub_08B73030(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B73030_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B73170[432] = {
    1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 7, 0, 0, 8,
    0, 9, 10, 0, 0, 0, 0, 11, 0, 0, 12, 0, 13, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    15, 0, 0, 0, 0, 0, 0, 16, 0, 17, 18, 0, 0, 19, 20, 0, 0, 21, 0, 22, 0, 0, 0, 23, 0, 0, 24, 0, 0, 0, 0, 0,
    0, 0, 0, 25, 0, 0, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27, 0, 0, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 29, 0, 0, 30, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32,
    0, 0, 0, 0, 0, 0, 0, 0, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 34, 0, 0, 0, 35, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 36, 0, 37,
};
void sub_08B73170_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B73170u;
        entry_id = (entry_delta < 1728u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B73170[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B73170;
    case 2u: goto L_08B7318C;
    case 3u: goto L_08B73198;
    case 4u: goto L_08B731BC;
    case 5u: goto L_08B731C4;
    case 6u: goto L_08B731D4;
    case 7u: goto L_08B731E0;
    case 8u: goto L_08B731EC;
    case 9u: goto L_08B731F4;
    case 10u: goto L_08B731F8;
    case 11u: goto L_08B7320C;
    case 12u: goto L_08B73218;
    case 13u: goto L_08B73220;
    case 14u: goto L_08B73224;
    case 15u: goto L_08B734F0;
    case 16u: goto L_08B7350C;
    case 17u: goto L_08B73514;
    case 18u: goto L_08B73518;
    case 19u: goto L_08B73524;
    case 20u: goto L_08B73528;
    case 21u: goto L_08B73534;
    case 22u: goto L_08B7353C;
    case 23u: goto L_08B7354C;
    case 24u: goto L_08B73558;
    case 25u: goto L_08B7357C;
    case 26u: goto L_08B7358C;
    case 27u: goto L_08B735B4;
    case 28u: goto L_08B735C0;
    case 29u: goto L_08B73724;
    case 30u: goto L_08B73730;
    case 31u: goto L_08B73740;
    case 32u: goto L_08B7376C;
    case 33u: goto L_08B73790;
    case 34u: goto L_08B737C4;
    case 35u: goto L_08B737D4;
    case 36u: goto L_08B73824;
    case 37u: goto L_08B7382C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B73170:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(21, (ctx.gpr[21] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(2, ctx.gpr[21] < ctx.gpr[17] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(5, ctx.gpr[21] - ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B73198;
      }
      goto L_08B7318C;
    }
L_08B7318C:
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[5]) < 16 ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B734F0;
      }
      goto L_08B73198;
    }
L_08B73198:
    ctx.set_gpr(2, 2273u << 16u);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(3852)));
    ctx.set_gpr(30, 2244u << 16u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[30] + static_cast<std::uint32_t>(-22992)));
    ctx.set_gpr(3, ctx.gpr[17] + ctx.gpr[3]);
    ctx.set_gpr(23, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(22, ctx.gpr[16] + ctx.gpr[21]);
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[23];
    ctx.set_gpr(18, ctx.gpr[3] + static_cast<std::uint32_t>(16));
      if (branch_taken) {
          goto L_08B731C4;
      }
      goto L_08B731BC;
    }
L_08B731BC:
    ctx.set_gpr(18, ctx.gpr[3] + static_cast<std::uint32_t>(4111));
    ctx.set_gpr(18, (ctx.gpr[18] & ~0x00000FFFu) | ((0u & 0x00000FFFu) << 0u));
    goto L_08B731C4;
L_08B731C4:
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.set_gpr(5, ctx.gpr[18] + 0u);
    ctx.set_gpr(31, 0x08B731D4u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[13]);
    ctx.pc = 0x08B7500Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B731D4u) goto L_08B731D4;
    return;
L_08B731D4:
    ctx.set_gpr(20, ctx.gpr[2] + 0u);
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[23];
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
      if (branch_taken) {
          goto L_08B731F4;
      }
      goto L_08B731E0;
    }
L_08B731E0:
    ctx.set_gpr(2, ctx.gpr[2] < ctx.gpr[22] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(23, 2273u << 16u);
      if (branch_taken) {
          goto L_08B73514;
      }
      goto L_08B731EC;
    }
L_08B731EC:
    { const bool branch_taken = ctx.gpr[16] == ctx.gpr[13];
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[23] + static_cast<std::uint32_t>(3804)));
      if (branch_taken) {
          goto L_08B73518;
      }
      goto L_08B731F4;
    }
L_08B731F4:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    goto L_08B731F8;
L_08B731F8:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, (ctx.gpr[2] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(3, ctx.gpr[2] < ctx.gpr[17] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[3] != 0u;
    ctx.set_gpr(5, ctx.gpr[2] - ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B73218;
      }
      goto L_08B7320C;
    }
L_08B7320C:
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[5]) < 16 ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B734F0;
      }
      goto L_08B73218;
    }
L_08B73218:
    ctx.set_gpr(31, 0x08B73220u);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.pc = 0x08B73E04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73220u) goto L_08B73220;
    return;
L_08B73220:
    ctx.set_gpr(2, 0u + 0u);
    goto L_08B73224;
L_08B73224:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(68)));
    ctx.set_gpr(30, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(64)));
    ctx.set_gpr(23, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(60)));
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(56)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(52)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(48)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(44)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(40)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(80));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B734F0:
    ctx.set_gpr(2, ctx.gpr[17] | 1u);
    ctx.set_gpr(5, ctx.gpr[5] | 1u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[5]);
    ctx.set_gpr(31, 0x08B7350Cu);
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(8), ctx.gpr[3]);
    ctx.pc = 0x08B73E04u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7350Cu) goto L_08B7350C;
    return;
L_08B7350C:
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(8));
    goto L_08B73224;
L_08B73514:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[23] + static_cast<std::uint32_t>(3804)));
    goto L_08B73518;
L_08B73518:
    ctx.set_gpr(4, ctx.gpr[18] + ctx.gpr[2]);
    { const bool branch_taken = ctx.gpr[22] == ctx.gpr[20];
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(3804), ctx.gpr[4]);
      if (branch_taken) {
          goto L_08B73724;
      }
      goto L_08B73524;
    }
L_08B73524:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[30] + static_cast<std::uint32_t>(-22992)));
    goto L_08B73528;
L_08B73528:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
    { const bool branch_taken = ctx.gpr[3] == ctx.gpr[2];
    ctx.set_gpr(2, ctx.gpr[20] - ctx.gpr[22]);
      if (branch_taken) {
          goto L_08B73824;
      }
      goto L_08B73534;
    }
L_08B73534:
    ctx.set_gpr(2, ctx.gpr[4] + ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(3804), ctx.gpr[2]);
    goto L_08B7353C;
L_08B7353C:
    ctx.set_gpr(2, ctx.gpr[20] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(3, ctx.gpr[2] & 15u);
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(5, 0u + 0u);
      if (branch_taken) {
          goto L_08B73558;
      }
      goto L_08B7354C;
    }
L_08B7354C:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(16));
    ctx.set_gpr(5, ctx.gpr[2] - ctx.gpr[3]);
    ctx.set_gpr(20, ctx.gpr[20] + ctx.gpr[5]);
    goto L_08B73558;
L_08B73558:
    ctx.set_gpr(3, ctx.gpr[20] + ctx.gpr[18]);
    ctx.set_gpr(3, ctx.gpr[3] & 4095u);
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(4096));
    ctx.set_gpr(2, ctx.gpr[2] - ctx.gpr[3]);
    ctx.set_gpr(18, ctx.gpr[5] + ctx.gpr[2]);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.set_gpr(5, ctx.gpr[18] + 0u);
    ctx.set_gpr(31, 0x08B7357Cu);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[13]);
    ctx.pc = 0x08B7500Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7357Cu) goto L_08B7357C;
    return;
L_08B7357C:
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(4, ctx.gpr[2] - ctx.gpr[20]);
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[3];
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
      if (branch_taken) {
          goto L_08B7382C;
      }
      goto L_08B7358C;
    }
L_08B7358C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[23] + static_cast<std::uint32_t>(3804)));
    ctx.set_gpr(3, ctx.gpr[18] + ctx.gpr[4]);
    ctx.set_gpr(3, ctx.gpr[3] | 1u);
    ctx.set_gpr(2, ctx.gpr[18] + ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(3804), ctx.gpr[2]);
    ctx.set_gpr(16, ctx.gpr[20] + 0u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[13];
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(8), ctx.gpr[20]);
      if (branch_taken) {
          goto L_08B73740;
      }
      goto L_08B735B4;
    }
L_08B735B4:
    ctx.set_gpr(2, ctx.gpr[21] < static_cast<std::uint32_t>(16) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
      if (branch_taken) {
          goto L_08B73790;
      }
      goto L_08B735C0;
    }
L_08B735C0:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    goto L_08B731F8;
L_08B73724:
    ctx.set_gpr(2, ctx.gpr[20] & 4095u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[30] + static_cast<std::uint32_t>(-22992)));
      if (branch_taken) {
          goto L_08B73528;
      }
      goto L_08B73730;
    }
L_08B73730:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, ctx.gpr[18] + ctx.gpr[21]);
    ctx.set_gpr(2, ctx.gpr[2] | 1u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    goto L_08B73740;
L_08B73740:
    ctx.set_gpr(5, 2273u << 16u);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[23] + static_cast<std::uint32_t>(3804)));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(3848)));
    ctx.set_gpr(7, 2273u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(3844)));
    ctx.set_gpr(3, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, ctx.gpr[2] < ctx.gpr[6] ? 1u : 0u);
    if (ctx.gpr[2] != 0u) ctx.set_gpr(3, ctx.gpr[6]);
    ctx.set_gpr(4, ctx.gpr[4] < ctx.gpr[6] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[4] == 0u;
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(3848), ctx.gpr[3]);
      if (branch_taken) {
          goto L_08B731F8;
      }
      goto L_08B7376C;
    }
L_08B7376C:
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(3844), ctx.gpr[6]);
    goto L_08B731F8;
L_08B73790:
    ctx.set_gpr(2, ctx.gpr[21] + static_cast<std::uint32_t>(-12));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, (ctx.gpr[2] & ~0x0000000Fu) | ((0u & 0x0000000Fu) << 0u));
    ctx.set_gpr(5, ctx.gpr[2] < static_cast<std::uint32_t>(16) ? 1u : 0u);
    ctx.set_gpr(3, ctx.gpr[3] & 1u);
    ctx.set_gpr(3, ctx.gpr[2] | ctx.gpr[3]);
    ctx.set_gpr(2, ctx.gpr[4] + ctx.gpr[2]);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(5));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(8), ctx.gpr[4]);
    { const bool branch_taken = ctx.gpr[5] != 0u;
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
      if (branch_taken) {
          goto L_08B73740;
      }
      goto L_08B737C4;
    }
L_08B737C4:
    ctx.set_gpr(5, ctx.gpr[6] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.set_gpr(31, 0x08B737D4u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[13]);
    ctx.pc = 0x08B7B5D0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B737D4u) goto L_08B737D4;
    return;
L_08B737D4:
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    goto L_08B73740;
L_08B73824:
    rt.memory().aot_store32(ctx.gpr[30] + static_cast<std::uint32_t>(-22992), ctx.gpr[20]);
    goto L_08B7353C;
L_08B7382C:
    ctx.set_gpr(18, 0u + 0u);
    ctx.set_gpr(4, 0u + 0u);
    goto L_08B7358C;
}

void sub_08B73170(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B73170_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B7500C[22] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 4, 0, 0, 0, 0, 5, 0, 6,
};
void sub_08B7500C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B7500Cu;
        entry_id = (entry_delta < 88u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B7500C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B7500C;
    case 2u: goto L_08B75030;
    case 3u: goto L_08B75040;
    case 4u: goto L_08B75044;
    case 5u: goto L_08B75058;
    case 6u: goto L_08B75060;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B7500C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, 2284u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B75030u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(3960), 0u);
    ctx.pc = 0x08B4E6A0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B75030u) goto L_08B75030;
    return;
L_08B75030:
    ctx.set_gpr(3, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
    { const bool branch_taken = ctx.gpr[3] == ctx.gpr[2];
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(3960)));
      if (branch_taken) {
          goto L_08B75058;
      }
      goto L_08B75040;
    }
L_08B75040:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    goto L_08B75044;
L_08B75044:
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[3] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B75058:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
      if (branch_taken) {
          goto L_08B75044;
      }
      goto L_08B75060;
    }
L_08B75060:
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
    ctx.set_gpr(2, ctx.gpr[3] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B7500C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B7500C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B73DC8[12] = {
    1, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 4,
};
void sub_08B73DC8_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B73DC8u;
        entry_id = (entry_delta < 48u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B73DC8[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B73DC8;
    case 2u: goto L_08B73DD8;
    case 3u: goto L_08B73DEC;
    case 4u: goto L_08B73DF4;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B73DC8:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B73DD8u);
    // nop
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73DD8u) goto L_08B73DD8;
    return;
L_08B73DD8:
    ctx.set_gpr(5, 2273u << 16u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(3860)));
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(3, ctx.gpr[2] + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08B73DF4;
      }
      goto L_08B73DEC;
    }
L_08B73DEC:
    ctx.set_gpr(2, 2273u << 16u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(3856), ctx.gpr[4]);
    goto L_08B73DF4;
L_08B73DF4:
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(3860), ctx.gpr[3]);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B73DC8(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B73DC8_entry(rt, ctx, 0u, aot_mem);
}

void sub_08B73E04_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    (void)direct_entry_id;
    std::uint32_t jump_target = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t local_transfers = 0u;
    std::uint32_t entry_id = 0u;
LOCAL_DISPATCH:
    switch (local_pc) {
    case 0x08B73E04u: goto L_08B73E04;
    case 0x08B73E18u: goto L_08B73E18;
    case 0x08B73E20u: goto L_08B73E20;
    case 0x08B7FB9Cu: goto L_08B7FB9C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
L_08B73E04:
    ctx.set_gpr(3, 2273u << 16u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(3860)));
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(-1));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(3860), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B73E20;
      }
      goto L_08B73E18;
    }
L_08B73E18:
    jump_target = ctx.gpr[31];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B73E20:
    ctx.set_gpr(2, 2273u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(3856)));
    goto L_08B7FB9C;
L_08B7FB9C:
    jump_target = ctx.gpr[31];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B73E04(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B73E04_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B73CF0[53] = {
    1, 0, 0, 2, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 5, 6, 0, 7, 0, 0, 8, 9, 0, 0, 0, 0, 10, 0, 0,
    11, 0, 12, 0, 0, 13, 0, 0, 14, 0, 15, 0, 0, 16, 0, 0, 0, 0, 0, 0, 17,
};
void sub_08B73CF0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B73CF0u;
        entry_id = (entry_delta < 212u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B73CF0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B73CF0;
    case 2u: goto L_08B73CFC;
    case 3u: goto L_08B73D08;
    case 4u: goto L_08B73D18;
    case 5u: goto L_08B73D34;
    case 6u: goto L_08B73D38;
    case 7u: goto L_08B73D40;
    case 8u: goto L_08B73D4C;
    case 9u: goto L_08B73D50;
    case 10u: goto L_08B73D64;
    case 11u: goto L_08B73D70;
    case 12u: goto L_08B73D78;
    case 13u: goto L_08B73D84;
    case 14u: goto L_08B73D90;
    case 15u: goto L_08B73D98;
    case 16u: goto L_08B73DA4;
    case 17u: goto L_08B73DC0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B73CF0:
    ctx.set_gpr(2, ctx.gpr[6] < static_cast<std::uint32_t>(16) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(7, ctx.gpr[4] + 0u);
      if (branch_taken) {
          goto L_08B73D70;
      }
      goto L_08B73CFC;
    }
L_08B73CFC:
    ctx.set_gpr(3, ctx.gpr[4] & 3u);
    if (ctx.gpr[3] == 0u) {
    ctx.set_gpr(5, ctx.gpr[5] & 255u);
        goto L_08B73D38;
    }
    goto L_08B73D08;
L_08B73D08:
    ctx.set_gpr(2, ctx.gpr[3] < static_cast<std::uint32_t>(4) ? 1u : 0u);
    ctx.set_gpr(8, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(ctx.gpr[5]))));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08B73D34;
      }
      goto L_08B73D18;
    }
L_08B73D18:
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, ctx.gpr[3] + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(2, ctx.gpr[2] < static_cast<std::uint32_t>(4) ? 1u : 0u);
    rt.memory().aot_store8(ctx.gpr[7] + static_cast<std::uint32_t>(0), static_cast<std::uint8_t>(ctx.gpr[8]));
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(-1));
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(7, ctx.gpr[7] + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08B73D18;
      }
      goto L_08B73D34;
    }
L_08B73D34:
    ctx.set_gpr(5, ctx.gpr[5] & 255u);
    goto L_08B73D38;
L_08B73D38:
    { const bool branch_taken = ctx.gpr[5] != 0u;
    ctx.set_gpr(3, ctx.gpr[5] + 0u);
      if (branch_taken) {
          goto L_08B73DA4;
      }
      goto L_08B73D40;
    }
L_08B73D40:
    ctx.set_gpr(8, ctx.gpr[6] >> 3u);
    { const bool branch_taken = ctx.gpr[8] == 0u;
    ctx.set_gpr(6, ctx.gpr[6] & 7u);
      if (branch_taken) {
          goto L_08B73D64;
      }
      goto L_08B73D4C;
    }
L_08B73D4C:
    ctx.set_gpr(2, 0u + 0u);
    goto L_08B73D50;
L_08B73D50:
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(0), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    { const bool branch_taken = ctx.gpr[8] != ctx.gpr[2];
    ctx.set_gpr(7, ctx.gpr[7] + static_cast<std::uint32_t>(8));
      if (branch_taken) {
          goto L_08B73D50;
      }
      goto L_08B73D64;
    }
L_08B73D64:
    ctx.set_gpr(2, ctx.gpr[6] < static_cast<std::uint32_t>(4) ? 1u : 0u);
    if (ctx.gpr[2] == 0u) {
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(0), ctx.gpr[3]);
        goto L_08B73D98;
    }
    goto L_08B73D70;
L_08B73D70:
    { const bool branch_taken = ctx.gpr[6] == 0u;
    ctx.set_gpr(3, ctx.gpr[7] + 0u);
      if (branch_taken) {
          goto L_08B73D90;
      }
      goto L_08B73D78;
    }
L_08B73D78:
    ctx.set_gpr(5, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(ctx.gpr[5]))));
    ctx.set_gpr(2, ctx.gpr[6] + ctx.gpr[7]);
    rt.memory().aot_store8(ctx.gpr[3] + static_cast<std::uint32_t>(0), static_cast<std::uint8_t>(ctx.gpr[5]));
    goto L_08B73D84;
L_08B73D84:
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(1));
    if (ctx.gpr[3] != ctx.gpr[2]) {
    rt.memory().aot_store8(ctx.gpr[3] + static_cast<std::uint32_t>(0), static_cast<std::uint8_t>(ctx.gpr[5]));
        goto L_08B73D84;
    }
    goto L_08B73D90;
L_08B73D90:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(2, ctx.gpr[4] + 0u);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B73D98:
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(-4));
    ctx.set_gpr(7, ctx.gpr[7] + static_cast<std::uint32_t>(4));
    goto L_08B73D70;
L_08B73DA4:
    ctx.set_gpr(2, ctx.gpr[5] << 8u);
    ctx.set_gpr(2, ctx.gpr[5] | ctx.gpr[2]);
    ctx.set_gpr(3, ctx.gpr[2] << 16u);
    ctx.set_gpr(8, ctx.gpr[6] >> 3u);
    ctx.set_gpr(3, ctx.gpr[2] | ctx.gpr[3]);
    { const bool branch_taken = ctx.gpr[8] == 0u;
    ctx.set_gpr(6, ctx.gpr[6] & 7u);
      if (branch_taken) {
          goto L_08B73D64;
      }
      goto L_08B73DC0;
    }
L_08B73DC0:
    ctx.set_gpr(2, 0u + 0u);
    goto L_08B73D50;
}

void sub_08B73CF0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B73CF0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B73E2C[56] = {
    1, 0, 0, 0, 2, 0, 0, 3, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 6, 0, 0, 7, 0, 8, 0, 0, 0,
    9, 0, 0, 0, 10, 11, 0, 0, 0, 0, 0, 12, 13, 0, 0, 14, 0, 0, 0, 15, 0, 16, 0, 17,
};
void sub_08B73E2C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B73E2Cu;
        entry_id = (entry_delta < 224u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B73E2C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B73E2C;
    case 2u: goto L_08B73E3C;
    case 3u: goto L_08B73E48;
    case 4u: goto L_08B73E50;
    case 5u: goto L_08B73E7C;
    case 6u: goto L_08B73E88;
    case 7u: goto L_08B73E94;
    case 8u: goto L_08B73E9C;
    case 9u: goto L_08B73EAC;
    case 10u: goto L_08B73EBC;
    case 11u: goto L_08B73EC0;
    case 12u: goto L_08B73ED8;
    case 13u: goto L_08B73EDC;
    case 14u: goto L_08B73EE8;
    case 15u: goto L_08B73EF8;
    case 16u: goto L_08B73F00;
    case 17u: goto L_08B73F08;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B73E2C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    { const bool branch_taken = ctx.gpr[26] == 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08B73ED8;
      }
      goto L_08B73E3C;
    }
L_08B73E3C:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[26] + static_cast<std::uint32_t>(4)));
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(2, 2232u << 16u);
      if (branch_taken) {
          goto L_08B73EDC;
      }
      goto L_08B73E48;
    }
L_08B73E48:
    ctx.set_gpr(31, 0x08B73E50u);
    // nop
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73E50u) goto L_08B73E50;
    return;
L_08B73E50:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(892)));
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, 2273u << 16u);
    ctx.set_gpr(3, static_cast<std::uint32_t>(static_cast<std::int32_t>(ctx.gpr[6]) >> 31u));
    ctx.set_gpr(3, ctx.gpr[3] >> 27u);
    ctx.set_gpr(5, ctx.gpr[2] + static_cast<std::uint32_t>(3864));
    ctx.set_gpr(2, ctx.gpr[6] + ctx.gpr[3]);
    ctx.set_gpr(2, ctx.gpr[2] & 31u);
    ctx.set_gpr(2, ctx.gpr[2] - ctx.gpr[3]);
    ctx.set_gpr(7, ctx.gpr[2] << 2u);
    ctx.set_gpr(3, ctx.gpr[7] + ctx.gpr[5]);
    goto L_08B73E7C;
L_08B73E7C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(132)));
    { const bool branch_taken = ctx.gpr[6] == ctx.gpr[2];
    // nop
      if (branch_taken) {
          goto L_08B73F00;
      }
      goto L_08B73E88;
    }
L_08B73E88:
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(0)));
    { const bool branch_taken = ctx.gpr[5] != 0u;
    ctx.set_gpr(3, ctx.gpr[7] + ctx.gpr[5]);
      if (branch_taken) {
          goto L_08B73E7C;
      }
      goto L_08B73E94;
    }
L_08B73E94:
    ctx.set_gpr(31, 0x08B73E9Cu);
    // nop
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73E9Cu) goto L_08B73E9C;
    return;
L_08B73E9C:
    ctx.set_gpr(2, 2232u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(-1004));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, 2235u << 16u);
      if (branch_taken) {
          goto L_08B73EBC;
      }
      goto L_08B73EAC;
    }
L_08B73EAC:
    ctx.set_gpr(5, 2235u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-26624));
    ctx.set_gpr(31, 0x08B73EBCu);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(20720));
    ctx.pc = 0x08B7FC14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73EBCu) goto L_08B73EBC;
    return;
L_08B73EBC:
    ctx.set_gpr(2, 2244u << 16u);
    goto L_08B73EC0;
L_08B73EC0:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(-23896)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B73ED8:
    ctx.set_gpr(2, 2232u << 16u);
    goto L_08B73EDC;
L_08B73EDC:
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(-1004));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, 2235u << 16u);
      if (branch_taken) {
          goto L_08B73EBC;
      }
      goto L_08B73EE8;
    }
L_08B73EE8:
    ctx.set_gpr(5, 2235u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-26656));
    ctx.set_gpr(31, 0x08B73EF8u);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(20720));
    ctx.pc = 0x08B7FC14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73EF8u) goto L_08B73EF8;
    return;
L_08B73EF8:
    ctx.set_gpr(2, 2244u << 16u);
    goto L_08B73EC0;
L_08B73F00:
    ctx.set_gpr(31, 0x08B73F08u);
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(4)));
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73F08u) goto L_08B73F08;
    return;
L_08B73F08:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B73E2C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B73E2C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B73F1C[76] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 0, 0, 0, 5, 0, 6, 0, 0, 7,
    0, 8, 0, 9, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 11, 0, 12, 0, 0, 0, 13, 0, 0, 0, 0, 14, 0, 0, 0, 15, 0, 0,
    16, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 18,
};
void sub_08B73F1C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B73F1Cu;
        entry_id = (entry_delta < 304u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B73F1C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B73F1C;
    case 2u: goto L_08B73F44;
    case 3u: goto L_08B73F70;
    case 4u: goto L_08B73F74;
    case 5u: goto L_08B73F84;
    case 6u: goto L_08B73F8C;
    case 7u: goto L_08B73F98;
    case 8u: goto L_08B73FA0;
    case 9u: goto L_08B73FA8;
    case 10u: goto L_08B73FC8;
    case 11u: goto L_08B73FD4;
    case 12u: goto L_08B73FDC;
    case 13u: goto L_08B73FEC;
    case 14u: goto L_08B74000;
    case 15u: goto L_08B74010;
    case 16u: goto L_08B7401C;
    case 17u: goto L_08B74040;
    case 18u: goto L_08B74048;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B73F1C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    ctx.set_gpr(20, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(31, 0x08B73F44u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73F44u) goto L_08B73F44;
    return;
L_08B73F44:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, static_cast<std::uint32_t>(static_cast<std::int32_t>(ctx.gpr[17]) >> 31u));
    ctx.set_gpr(2, ctx.gpr[2] >> 27u);
    ctx.set_gpr(3, ctx.gpr[17] + ctx.gpr[2]);
    ctx.set_gpr(3, ctx.gpr[3] & 31u);
    ctx.set_gpr(3, ctx.gpr[3] - ctx.gpr[2]);
    ctx.set_gpr(2, 2273u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(3864));
    ctx.set_gpr(18, ctx.gpr[3] << 2u);
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(892), ctx.gpr[17]);
    goto L_08B73F74;
L_08B73F70:
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    goto L_08B73F74;
L_08B73F74:
    ctx.set_gpr(5, ctx.gpr[18] + ctx.gpr[2]);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(132)));
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B73FC8;
      }
      goto L_08B73F84;
    }
L_08B73F84:
    { const bool branch_taken = ctx.gpr[17] == ctx.gpr[3];
    // nop
      if (branch_taken) {
          goto L_08B74040;
      }
      goto L_08B73F8C;
    }
L_08B73F8C:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(0)));
    { const bool branch_taken = ctx.gpr[16] != 0u;
    ctx.set_gpr(19, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B73F70;
      }
      goto L_08B73F98;
    }
L_08B73F98:
    ctx.set_gpr(31, 0x08B73FA0u);
    ctx.set_gpr(19, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73FA0u) goto L_08B73FA0;
    return;
L_08B73FA0:
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(2, 0u + 0u);
      if (branch_taken) {
          goto L_08B73FDC;
      }
      goto L_08B73FA8;
    }
L_08B73FA8:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B73FC8:
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(4), ctx.gpr[20]);
    ctx.set_gpr(31, 0x08B73FD4u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(132), ctx.gpr[17]);
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73FD4u) goto L_08B73FD4;
    return;
L_08B73FD4:
    { const bool branch_taken = ctx.gpr[16] != 0u;
    ctx.set_gpr(2, 0u + 0u);
      if (branch_taken) {
          goto L_08B73FA8;
      }
      goto L_08B73FDC;
    }
L_08B73FDC:
    ctx.set_gpr(2, 2244u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(-23896)));
    ctx.set_gpr(31, 0x08B73FECu);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(260));
    ctx.pc = 0x08B73088u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73FECu) goto L_08B73FEC;
    return;
L_08B73FEC:
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(31, 0x08B74000u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(260));
    ctx.pc = 0x08B73CF0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B74000u) goto L_08B74000;
    return;
L_08B74000:
    ctx.set_gpr(3, ctx.gpr[18] + ctx.gpr[16]);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[20]);
    ctx.set_gpr(31, 0x08B74010u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(132), ctx.gpr[17]);
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B74010u) goto L_08B74010;
    return;
L_08B74010:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(31, 0x08B7401Cu);
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7401Cu) goto L_08B7401C;
    return;
L_08B7401C:
    ctx.set_gpr(2, 0u + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B74040:
    ctx.set_gpr(31, 0x08B74048u);
    // nop
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B74048u) goto L_08B74048;
    return;
L_08B74048:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B73F1C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B73F1C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B74100[95] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
};
void sub_08B74100_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B74100u;
        entry_id = (entry_delta < 380u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B74100[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B74100;
    case 2u: goto L_08B7413C;
    case 3u: goto L_08B7414C;
    case 4u: goto L_08B74198;
    case 5u: goto L_08B741AC;
    case 6u: goto L_08B74278;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B74100:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(708));
    ctx.set_gpr(3, ctx.gpr[16] + static_cast<std::uint32_t>(800));
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(616));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(5, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(12), ctx.gpr[3]);
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(25));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(0), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(16), 0u);
    goto L_08B7413C;
L_08B7413C:
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(1));
    rt.memory().aot_store8(ctx.gpr[2] + static_cast<std::uint32_t>(20), static_cast<std::uint8_t>(0u));
    { const bool branch_taken = ctx.gpr[5] != ctx.gpr[3];
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08B7413C;
      }
      goto L_08B7414C;
    }
L_08B7414C:
    ctx.set_gpr(2, 2235u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(-26588));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(52), ctx.gpr[2]);
    ctx.set_gpr(5, ctx.gpr[16] + static_cast<std::uint32_t>(124));
    ctx.set_gpr(3, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(48), 0u);
    ctx.set_gpr(2, 0u + 0u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(36));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(56), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(60), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(64), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(68), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(72), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(76), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(80), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(84), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(88), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(92), 0u);
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(96), static_cast<std::uint8_t>(0u));
    goto L_08B74198;
L_08B74198:
    ctx.set_gpr(2, ctx.gpr[5] + ctx.gpr[2]);
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(1));
    rt.memory().aot_store8(ctx.gpr[2] + static_cast<std::uint32_t>(0), static_cast<std::uint8_t>(0u));
    { const bool branch_taken = ctx.gpr[3] != ctx.gpr[6];
    ctx.set_gpr(2, ctx.gpr[3] + 0u);
      if (branch_taken) {
          goto L_08B74198;
      }
      goto L_08B741AC;
    }
L_08B741AC:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(168), ctx.gpr[2]);
    ctx.set_gpr(3, 0u + 0u);
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(13070));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(172), ctx.gpr[3]);
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(4660));
    ctx.set_gpr(5, 0u + 0u);
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(176), static_cast<std::uint16_t>(ctx.gpr[2]));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-21555));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(276));
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(178), static_cast<std::uint16_t>(ctx.gpr[2]));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-6547));
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(180), static_cast<std::uint16_t>(ctx.gpr[3]));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(-8468));
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(182), static_cast<std::uint16_t>(ctx.gpr[2]));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(5));
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(184), static_cast<std::uint16_t>(ctx.gpr[3]));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(11));
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(186), static_cast<std::uint16_t>(ctx.gpr[2]));
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(188), static_cast<std::uint16_t>(ctx.gpr[3]));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(160), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(192), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(196), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(200), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(204), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(208), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(212), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(252), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(256), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(260), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(264), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(268), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(272), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(276), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(280), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(284), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(288), 0u);
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(216), static_cast<std::uint8_t>(0u));
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(224), static_cast<std::uint8_t>(0u));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(248), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(328), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(332), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(336), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(340), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(596), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(468), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(600), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(604), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(608), 0u);
    ctx.set_gpr(31, 0x08B74278u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(612), 0u);
    ctx.pc = 0x08B73CF0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B74278u) goto L_08B74278;
    return;
L_08B74278:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(892), 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B74100(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B74100_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08AB304C[136] = {
    1, 0, 0, 0, 0, 0, 2, 0, 3, 0, 4, 0, 0, 0, 0, 5, 0, 0, 0, 6, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11, 0, 0,
    12, 0, 0, 0, 13, 14, 0, 0, 15, 0, 16, 0, 17, 0, 18, 0, 19, 0, 20, 0, 0, 21, 0, 0, 0, 22, 23, 0, 24, 0, 25, 0,
    0, 0, 26, 0, 0, 0, 27, 0, 0, 0, 0, 0, 28, 0, 0, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30,
    0, 31, 0, 0, 0, 0, 32, 0, 0, 0, 33, 0, 34, 0, 0, 0, 0, 35, 0, 36, 0, 37, 0, 0, 0, 38, 0, 39, 0, 0, 0, 0,
    0, 40, 0, 41, 0, 42, 0, 43,
};
void sub_08AB304C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08AB304Cu;
        entry_id = (entry_delta < 544u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08AB304C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08AB304C;
    case 2u: goto L_08AB3064;
    case 3u: goto L_08AB306C;
    case 4u: goto L_08AB3074;
    case 5u: goto L_08AB3088;
    case 6u: goto L_08AB3098;
    case 7u: goto L_08AB30A0;
    case 8u: goto L_08AB30A8;
    case 9u: goto L_08AB30B0;
    case 10u: goto L_08AB30B8;
    case 11u: goto L_08AB30C0;
    case 12u: goto L_08AB30CC;
    case 13u: goto L_08AB30DC;
    case 14u: goto L_08AB30E0;
    case 15u: goto L_08AB30EC;
    case 16u: goto L_08AB30F4;
    case 17u: goto L_08AB30FC;
    case 18u: goto L_08AB3104;
    case 19u: goto L_08AB310C;
    case 20u: goto L_08AB3114;
    case 21u: goto L_08AB3120;
    case 22u: goto L_08AB3130;
    case 23u: goto L_08AB3134;
    case 24u: goto L_08AB313C;
    case 25u: goto L_08AB3144;
    case 26u: goto L_08AB3154;
    case 27u: goto L_08AB3164;
    case 28u: goto L_08AB317C;
    case 29u: goto L_08AB3188;
    case 30u: goto L_08AB31C8;
    case 31u: goto L_08AB31D0;
    case 32u: goto L_08AB31E4;
    case 33u: goto L_08AB31F4;
    case 34u: goto L_08AB31FC;
    case 35u: goto L_08AB3210;
    case 36u: goto L_08AB3218;
    case 37u: goto L_08AB3220;
    case 38u: goto L_08AB3230;
    case 39u: goto L_08AB3238;
    case 40u: goto L_08AB3250;
    case 41u: goto L_08AB3258;
    case 42u: goto L_08AB3260;
    case 43u: goto L_08AB3268;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08AB304C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-64));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(40), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(36), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(32), ctx.gpr[16]);
    ctx.set_gpr(31, 0x08AB3064u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(48), std::bit_cast<std::uint32_t>(ctx.fpr[20]));
    ctx.pc = 0x08B1477Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3064u) goto L_08AB3064;
    return;
L_08AB3064:
    ctx.set_gpr(31, 0x08AB306Cu);
    // nop
    ctx.pc = 0x08B14350u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB306Cu) goto L_08AB306C;
    return;
L_08AB306C:
    ctx.set_gpr(31, 0x08AB3074u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(2));
    ctx.pc = 0x08B1D100u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3074u) goto L_08AB3074;
    return;
L_08AB3074:
    ctx.set_gpr(5, 2243u << 16u);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(4364));
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(31, 0x08AB3088u);
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B1CE3Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3088u) goto L_08AB3088;
    return;
L_08AB3088:
    ctx.set_gpr(4, 0u + 0u);
    ctx.set_gpr(2, 2262u << 16u);
    ctx.set_gpr(31, 0x08AB3098u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(11220), ctx.gpr[16]);
    ctx.pc = 0x08B1D100u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3098u) goto L_08AB3098;
    return;
L_08AB3098:
    ctx.set_gpr(31, 0x08AB30A0u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x08B1D100u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB30A0u) goto L_08AB30A0;
    return;
L_08AB30A0:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(17, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08AB30DC;
      }
      goto L_08AB30A8;
    }
L_08AB30A8:
    ctx.set_gpr(31, 0x08AB30B0u);
    // nop
    ctx.pc = 0x08B1BB84u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB30B0u) goto L_08AB30B0;
    return;
L_08AB30B0:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08AB3250;
      }
      goto L_08AB30B8;
    }
L_08AB30B8:
    ctx.set_gpr(31, 0x08AB30C0u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B1C018u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB30C0u) goto L_08AB30C0;
    return;
L_08AB30C0:
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, 0x08AB30CCu);
    ctx.set_gpr(5, ctx.gpr[16] + 0u);
    ctx.pc = 0x08B1CEFCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB30CCu) goto L_08AB30CC;
    return;
L_08AB30CC:
    ctx.set_gpr(5, 2243u << 16u);
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, 0x08AB30DCu);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(4380));
    ctx.pc = 0x08B1CE3Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB30DCu) goto L_08AB30DC;
    return;
L_08AB30DC:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    goto L_08AB30E0;
L_08AB30E0:
    ctx.set_gpr(2, 2262u << 16u);
    ctx.set_gpr(31, 0x08AB30ECu);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(11216), ctx.gpr[17]);
    ctx.pc = 0x08B1D100u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB30ECu) goto L_08AB30EC;
    return;
L_08AB30EC:
    ctx.set_gpr(31, 0x08AB30F4u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.pc = 0x08B1D100u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB30F4u) goto L_08AB30F4;
    return;
L_08AB30F4:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(17, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08AB3130;
      }
      goto L_08AB30FC;
    }
L_08AB30FC:
    ctx.set_gpr(31, 0x08AB3104u);
    // nop
    ctx.pc = 0x08B1BB84u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3104u) goto L_08AB3104;
    return;
L_08AB3104:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08AB3260;
      }
      goto L_08AB310C;
    }
L_08AB310C:
    ctx.set_gpr(31, 0x08AB3114u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B1C018u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3114u) goto L_08AB3114;
    return;
L_08AB3114:
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, 0x08AB3120u);
    ctx.set_gpr(5, ctx.gpr[16] + 0u);
    ctx.pc = 0x08B1CEFCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3120u) goto L_08AB3120;
    return;
L_08AB3120:
    ctx.set_gpr(5, 2243u << 16u);
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, 0x08AB3130u);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(4396));
    ctx.pc = 0x08B1CE3Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3130u) goto L_08AB3130;
    return;
L_08AB3130:
    ctx.set_gpr(2, 2262u << 16u);
    goto L_08AB3134;
L_08AB3134:
    ctx.set_gpr(31, 0x08AB313Cu);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(11212), ctx.gpr[17]);
    ctx.pc = 0x08AB33B0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB313Cu) goto L_08AB313C;
    return;
L_08AB313C:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08AB3188;
      }
      goto L_08AB3144;
    }
L_08AB3144:
    ctx.set_gpr(2, 2235u << 16u);
    ctx.fpr[12] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(8988)));
    ctx.set_gpr(31, 0x08AB3154u);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.pc = 0x08B1C5F4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3154u) goto L_08AB3154;
    return;
L_08AB3154:
    ctx.set_gpr(2, 2235u << 16u);
    ctx.fpr[12] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(8992)));
    ctx.set_gpr(31, 0x08AB3164u);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.pc = 0x08B1C594u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3164u) goto L_08AB3164;
    return;
L_08AB3164:
    ctx.set_gpr(2, 2235u << 16u);
    ctx.set_gpr(3, 2235u << 16u);
    ctx.fpr[12] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(8996)));
    ctx.fpr[13] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(9000)));
    ctx.set_gpr(31, 0x08AB317Cu);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.pc = 0x08AB3318u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB317Cu) goto L_08AB317C;
    return;
L_08AB317C:
    ctx.set_gpr(2, 2235u << 16u);
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(9004)));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(136), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
    goto L_08AB3188;
L_08AB3188:
    ctx.set_gpr(3, 2233u << 16u);
    ctx.set_gpr(4, ctx.gpr[3] + static_cast<std::uint32_t>(1912));
    ctx.set_gpr(2, 2235u << 16u);
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(1912)));
    ctx.fpr[20] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(9008)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, 2262u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), std::bit_cast<std::uint32_t>(ctx.fpr[20]));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(11228), ctx.gpr[16]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), 0u);
    ctx.set_gpr(31, 0x08AB31C8u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), 0u);
    ctx.pc = 0x08AB3040u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB31C8u) goto L_08AB31C8;
    return;
L_08AB31C8:
    ctx.set_gpr(31, 0x08AB31D0u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B1C3DCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB31D0u) goto L_08AB31D0;
    return;
L_08AB31D0:
    ctx.fpr[12] = std::bit_cast<float>(0u);
    ctx.set_gpr(5, ctx.gpr[29] + 0u);
    ctx.set_gpr(6, 0u + 0u);
    ctx.set_gpr(31, 0x08AB31E4u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B1BF40u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB31E4u) goto L_08AB31E4;
    return;
L_08AB31E4:
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[20]));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), 0u);
    ctx.set_gpr(31, 0x08AB31F4u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), 0u);
    ctx.pc = 0x08AB3040u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB31F4u) goto L_08AB31F4;
    return;
L_08AB31F4:
    ctx.set_gpr(31, 0x08AB31FCu);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B1C3DCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB31FCu) goto L_08AB31FC;
    return;
L_08AB31FC:
    ctx.fpr[12] = std::bit_cast<float>(0u);
    ctx.set_gpr(5, ctx.gpr[29] + 0u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(31, 0x08AB3210u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B1BF40u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3210u) goto L_08AB3210;
    return;
L_08AB3210:
    ctx.set_gpr(31, 0x08AB3218u);
    // nop
    ctx.pc = 0x08AB3040u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3218u) goto L_08AB3218;
    return;
L_08AB3218:
    ctx.set_gpr(31, 0x08AB3220u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B1C3DCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3220u) goto L_08AB3220;
    return;
L_08AB3220:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(5, ctx.gpr[29] + static_cast<std::uint32_t>(12));
    ctx.set_gpr(31, 0x08AB3230u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(1));
    ctx.pc = 0x08B1BDC8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3230u) goto L_08AB3230;
    return;
L_08AB3230:
    ctx.set_gpr(31, 0x08AB3238u);
    // nop
    ctx.pc = 0x08A9B198u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3238u) goto L_08AB3238;
    return;
L_08AB3238:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(40)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    ctx.fpr[20] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(48)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(64));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08AB3250:
    ctx.set_gpr(31, 0x08AB3258u);
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.pc = 0x08B1D1B0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3258u) goto L_08AB3258;
    return;
L_08AB3258:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    goto L_08AB30E0;
L_08AB3260:
    ctx.set_gpr(31, 0x08AB3268u);
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.pc = 0x08B1D1B0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB3268u) goto L_08AB3268;
    return;
L_08AB3268:
    ctx.set_gpr(2, 2262u << 16u);
    goto L_08AB3134;
}

void sub_08AB304C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08AB304C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B1477C[72] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 4, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 6,
    0, 0, 0, 0, 0, 7, 0, 0, 8, 0, 0, 9, 0, 0, 0, 0, 10, 0, 0, 11, 0, 12, 0, 0, 13, 0, 0, 0, 0, 0, 0, 14,
    0, 15, 0, 0, 0, 16, 0, 17,
};
void sub_08B1477C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B1477Cu;
        entry_id = (entry_delta < 288u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B1477C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B1477C;
    case 2u: goto L_08B147AC;
    case 3u: goto L_08B147C0;
    case 4u: goto L_08B147C8;
    case 5u: goto L_08B147E8;
    case 6u: goto L_08B147F8;
    case 7u: goto L_08B14810;
    case 8u: goto L_08B1481C;
    case 9u: goto L_08B14828;
    case 10u: goto L_08B1483C;
    case 11u: goto L_08B14848;
    case 12u: goto L_08B14850;
    case 13u: goto L_08B1485C;
    case 14u: goto L_08B14878;
    case 15u: goto L_08B14880;
    case 16u: goto L_08B14890;
    case 17u: goto L_08B14898;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B1477C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, 2262u << 16u);
    ctx.set_gpr(17, ctx.gpr[17] + static_cast<std::uint32_t>(17920));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, 10u << 16u);
    ctx.set_gpr(6, ctx.gpr[16] | 3200u);
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(5, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B147ACu);
    ctx.set_gpr(16, ctx.gpr[17] + ctx.gpr[16]);
    ctx.pc = 0x08B73CF0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B147ACu) goto L_08B147AC;
    return;
L_08B147AC:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(4, 0u + 0u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(16384));
    ctx.set_gpr(31, 0x08B147C0u);
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(0), static_cast<std::uint8_t>(ctx.gpr[2]));
    ctx.pc = 0x08B7FC5Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B147C0u) goto L_08B147C0;
    return;
L_08B147C0:
    ctx.set_gpr(31, 0x08B147C8u);
    // nop
    ctx.pc = 0x08B6029Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B147C8u) goto L_08B147C8;
    return;
L_08B147C8:
    ctx.set_gpr(5, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(rt.memory().aot_load8(ctx.gpr[16] + static_cast<std::uint32_t>(0))))));
    ctx.set_gpr(4, 0u + 0u);
    ctx.set_gpr(6, 5u << 16u);
    ctx.set_gpr(2, ctx.gpr[5] << 18u);
    ctx.set_gpr(5, ctx.gpr[5] << 16u);
    ctx.set_gpr(5, ctx.gpr[5] + ctx.gpr[2]);
    ctx.set_gpr(31, 0x08B147E8u);
    ctx.set_gpr(5, ctx.gpr[5] + ctx.gpr[17]);
    ctx.pc = 0x08B603D8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B147E8u) goto L_08B147E8;
    return;
L_08B147E8:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(3));
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(31, 0x08B147F8u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(512));
    ctx.pc = 0x08B60B94u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B147F8u) goto L_08B147F8;
    return;
L_08B147F8:
    ctx.set_gpr(6, 8u << 16u);
    ctx.set_gpr(6, ctx.gpr[6] | 32768u);
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(512));
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(480));
    ctx.set_gpr(31, 0x08B14810u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(272));
    ctx.pc = 0x08B614E8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14810u) goto L_08B14810;
    return;
L_08B14810:
    ctx.set_gpr(4, 17u << 16u);
    ctx.set_gpr(31, 0x08B1481Cu);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(512));
    ctx.pc = 0x08B61664u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1481Cu) goto L_08B1481C;
    return;
L_08B1481C:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1808));
    ctx.set_gpr(31, 0x08B14828u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1912));
    ctx.pc = 0x08B61FF4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14828u) goto L_08B14828;
    return;
L_08B14828:
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(480));
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(272));
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(2048));
    ctx.set_gpr(31, 0x08B1483Cu);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(2048));
    ctx.pc = 0x08B61EFCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1483Cu) goto L_08B1483C;
    return;
L_08B1483C:
    ctx.set_gpr(4, 0u | 50000u);
    ctx.set_gpr(31, 0x08B14848u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(10000));
    ctx.pc = 0x08B61FA0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14848u) goto L_08B14848;
    return;
L_08B14848:
    ctx.set_gpr(31, 0x08B14850u);
    // nop
    ctx.pc = 0x08B610C8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14850u) goto L_08B14850;
    return;
L_08B14850:
    ctx.set_gpr(4, 0u + 0u);
    ctx.set_gpr(31, 0x08B1485Cu);
    ctx.set_gpr(5, 0u + 0u);
    ctx.pc = 0x08B613F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1485Cu) goto L_08B1485C;
    return;
L_08B1485C:
    ctx.set_gpr(4, 2272u << 16u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(8));
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(32));
    ctx.set_gpr(8, 0u + 0u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(17936));
    ctx.set_gpr(31, 0x08B14878u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(8));
    ctx.pc = 0x08B5F300u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14878u) goto L_08B14878;
    return;
L_08B14878:
    ctx.set_gpr(31, 0x08B14880u);
    // nop
    ctx.pc = 0x08B14550u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14880u) goto L_08B14880;
    return;
L_08B14880:
    ctx.set_gpr(5, 2225u << 16u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(31, 0x08B14890u);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(17152));
    ctx.pc = 0x08B61228u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14890u) goto L_08B14890;
    return;
L_08B14890:
    ctx.set_gpr(31, 0x08B14898u);
    // nop
    ctx.pc = 0x08B15DCCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14898u) goto L_08B14898;
    return;
L_08B14898:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    ctx.pc = 0x08B61478u; (void)rt.invoke_chained_call(ctx, &aot_mem); return;
}

void sub_08B1477C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B1477C_entry(rt, ctx, 0u, aot_mem);
}

void sub_08B14350_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    (void)direct_entry_id;
    std::uint32_t jump_target = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t local_transfers = 0u;
    std::uint32_t entry_id = 0u;
LOCAL_DISPATCH:
    switch (local_pc) {
    case 0x08B14350u: goto L_08B14350;
    case 0x08B14360u: goto L_08B14360;
    case 0x08B14368u: goto L_08B14368;
    case 0x08B14370u: goto L_08B14370;
    case 0x08B14378u: goto L_08B14378;
    case 0x08B14380u: goto L_08B14380;
    case 0x08B14388u: goto L_08B14388;
    case 0x08B14390u: goto L_08B14390;
    case 0x08B14398u: goto L_08B14398;
    case 0x08B1D07Cu: goto L_08B1D07C;
    case 0x08B1D0B0u: goto L_08B1D0B0;
    case 0x08B1D0C8u: goto L_08B1D0C8;
    case 0x08B1D0E0u: goto L_08B1D0E0;
    case 0x08B1D0E8u: goto L_08B1D0E8;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
L_08B14350:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B14360u);
    // nop
    ctx.pc = 0x08B156F8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14360u) goto L_08B14360;
    return;
L_08B14360:
    ctx.set_gpr(31, 0x08B14368u);
    // nop
    ctx.pc = 0x08B177F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14368u) goto L_08B14368;
    return;
L_08B14368:
    ctx.set_gpr(31, 0x08B14370u);
    // nop
    ctx.pc = 0x08B15E58u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14370u) goto L_08B14370;
    return;
L_08B14370:
    ctx.set_gpr(31, 0x08B14378u);
    // nop
    ctx.pc = 0x08B15AB0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14378u) goto L_08B14378;
    return;
L_08B14378:
    ctx.set_gpr(31, 0x08B14380u);
    // nop
    ctx.pc = 0x08B1B8ACu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14380u) goto L_08B14380;
    return;
L_08B14380:
    ctx.set_gpr(31, 0x08B14388u);
    // nop
    ctx.pc = 0x08B1C214u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14388u) goto L_08B14388;
    return;
L_08B14388:
    ctx.set_gpr(31, 0x08B14390u);
    // nop
    ctx.pc = 0x08B17AC0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14390u) goto L_08B14390;
    return;
L_08B14390:
    ctx.set_gpr(31, 0x08B14398u);
    // nop
    ctx.pc = 0x08B19928u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14398u) goto L_08B14398;
    return;
L_08B14398:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    goto L_08B1D07C;
L_08B1D07C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    std::fprintf(stderr, "[jal0-probe] 08B1D07C save-ra=%08X sp=%08X old-slot4=%08X\n", ctx.gpr[31], ctx.gpr[29], rt.memory().aot_load32(ctx.gpr[29] + 4u));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(3, 2243u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(8, 2272u << 16u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(48));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(22680)));
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(22676)));
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(21744));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(31, 0x08B1D0B0u);
    ctx.set_gpr(9, 0u + 0u);
    ctx.pc = 0x08B175C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1D0B0u) goto L_08B1D0B0;
    return;
L_08B1D0B0:
    std::fprintf(stderr, "[jal0-probe] 08B1D0B0 restore-ra-slot=%08X sp=%08X current-ra=%08X\n", rt.memory().aot_load32(ctx.gpr[29] + 4u), ctx.gpr[29], ctx.gpr[31]);
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27300));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(128));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(21740), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B1D0E0;
      }
      goto L_08B1D0C8;
    }
L_08B1D0C8:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21740)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u < ctx.gpr[2] ? 1u : 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B1D0E0:
    ctx.set_gpr(31, 0x08B1D0E8u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1D0E8u) goto L_08B1D0E8;
    return;
L_08B1D0E8:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21740)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u < ctx.gpr[2] ? 1u : 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B14350(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B14350_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B1D100[43] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
    0, 0, 0, 0, 6, 0, 7, 0, 8, 0, 9,
};
void sub_08B1D100_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B1D100u;
        entry_id = (entry_delta < 172u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B1D100[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B1D100;
    case 2u: goto L_08B1D12C;
    case 3u: goto L_08B1D138;
    case 4u: goto L_08B1D150;
    case 5u: goto L_08B1D17C;
    case 6u: goto L_08B1D190;
    case 7u: goto L_08B1D198;
    case 8u: goto L_08B1D1A0;
    case 9u: goto L_08B1D1A8;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B1D100:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int8_t>(ctx.gpr[4]))));
    ctx.set_gpr(4, 2235u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(157));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21740)));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27300));
      if (branch_taken) {
          goto L_08B1D190;
      }
      goto L_08B1D12C;
    }
L_08B1D12C:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(31, 0x08B1D138u);
    ctx.set_gpr(5, 0u + 0u);
    ctx.pc = 0x08B17054u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1D138u) goto L_08B1D138;
    return;
L_08B1D138:
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27300));
    ctx.set_gpr(3, ctx.gpr[17] & 255u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(160));
      if (branch_taken) {
          goto L_08B1D1A0;
      }
      goto L_08B1D150;
    }
L_08B1D150:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(0), static_cast<std::uint8_t>(ctx.gpr[2]));
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(44), static_cast<std::uint8_t>(ctx.gpr[3]));
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(1), static_cast<std::uint8_t>(ctx.gpr[3]));
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(2), static_cast<std::uint8_t>(0u));
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(3), static_cast<std::uint8_t>(0u));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(16), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(40), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(36), 0u);
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    goto L_08B1D17C;
L_08B1D17C:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B1D190:
    ctx.set_gpr(31, 0x08B1D198u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1D198u) goto L_08B1D198;
    return;
L_08B1D198:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21740)));
    goto L_08B1D12C;
L_08B1D1A0:
    ctx.set_gpr(31, 0x08B1D1A8u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1D1A8u) goto L_08B1D1A8;
    return;
L_08B1D1A8:
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    goto L_08B1D17C;
}

void sub_08B1D100(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B1D100_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B1CE3C[47] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 6, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 8,
};
void sub_08B1CE3C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B1CE3Cu;
        entry_id = (entry_delta < 188u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B1CE3C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B1CE3C;
    case 2u: goto L_08B1CE64;
    case 3u: goto L_08B1CE68;
    case 4u: goto L_08B1CE74;
    case 5u: goto L_08B1CEAC;
    case 6u: goto L_08B1CEB4;
    case 7u: goto L_08B1CEEC;
    case 8u: goto L_08B1CEF4;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B1CE3C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, 2235u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27300));
    ctx.set_gpr(16, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    { const bool branch_taken = ctx.gpr[17] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(60));
      if (branch_taken) {
          goto L_08B1CEEC;
      }
      goto L_08B1CE64;
    }
L_08B1CE64:
    ctx.set_gpr(4, 2235u << 16u);
    goto L_08B1CE68;
L_08B1CE68:
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27300));
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(61));
      if (branch_taken) {
          goto L_08B1CEAC;
      }
      goto L_08B1CE74;
    }
L_08B1CE74:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(8)));
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(20), ctx.gpr[3]);
    ctx.set_gpr(2, ctx.gpr[17] + 0u);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(24), ctx.gpr[4]);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(28), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(32), ctx.gpr[6]);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B1CEAC:
    ctx.set_gpr(31, 0x08B1CEB4u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1CEB4u) goto L_08B1CEB4;
    return;
L_08B1CEB4:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(8)));
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(20), ctx.gpr[3]);
    ctx.set_gpr(2, ctx.gpr[17] + 0u);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(24), ctx.gpr[4]);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(28), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(32), ctx.gpr[6]);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B1CEEC:
    ctx.set_gpr(31, 0x08B1CEF4u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1CEF4u) goto L_08B1CEF4;
    return;
L_08B1CEF4:
    ctx.set_gpr(4, 2235u << 16u);
    goto L_08B1CE68;
}

void sub_08B1CE3C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B1CE3C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B1BB84[56] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 5, 0, 6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 8, 0, 0, 0, 9, 0, 10, 0, 11, 0, 0, 12,
};
void sub_08B1BB84_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B1BB84u;
        entry_id = (entry_delta < 224u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B1BB84[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B1BB84;
    case 2u: goto L_08B1BBA8;
    case 3u: goto L_08B1BBB4;
    case 4u: goto L_08B1BBC8;
    case 5u: goto L_08B1BBE8;
    case 6u: goto L_08B1BBF0;
    case 7u: goto L_08B1BC30;
    case 8u: goto L_08B1BC34;
    case 9u: goto L_08B1BC44;
    case 10u: goto L_08B1BC4C;
    case 11u: goto L_08B1BC54;
    case 12u: goto L_08B1BC60;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B1BB84:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[16]);
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(4, 2235u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[31]);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27348));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21648)));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(367));
      if (branch_taken) {
          goto L_08B1BC44;
      }
      goto L_08B1BBA8;
    }
L_08B1BBA8:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(31, 0x08B1BBB4u);
    ctx.set_gpr(5, 0u + 0u);
    ctx.pc = 0x08B17054u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1BBB4u) goto L_08B1BBB4;
    return;
L_08B1BBB4:
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(16));
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(144));
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(370));
      if (branch_taken) {
          goto L_08B1BC54;
      }
      goto L_08B1BBC8;
    }
L_08B1BBC8:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(148), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(144), ctx.gpr[2]);
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(0), static_cast<std::uint8_t>(0u));
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(1), static_cast<std::uint8_t>(0u));
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(2), static_cast<std::uint8_t>(0u));
    rt.memory().aot_store8(ctx.gpr[16] + static_cast<std::uint32_t>(3), static_cast<std::uint8_t>(0u));
    ctx.set_gpr(31, 0x08B1BBE8u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), 0u);
    ctx.pc = 0x08B16CCCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1BBE8u) goto L_08B1BBE8;
    return;
L_08B1BBE8:
    ctx.set_gpr(31, 0x08B1BBF0u);
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(80));
    ctx.pc = 0x08B16CCCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1BBF0u) goto L_08B1BBF0;
    return;
L_08B1BBF0:
    ctx.set_gpr(2, 2233u << 16u);
    ctx.set_gpr(3, ctx.gpr[2] + static_cast<std::uint32_t>(2984));
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(2984)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(4)));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[7]);
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(164));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(200), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(152), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(156), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(160), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(204), 0u);
    ctx.set_gpr(31, 0x08B1BC30u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(208), ctx.gpr[16]);
    ctx.pc = 0x08B66824u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1BC30u) goto L_08B1BC30;
    return;
L_08B1BC30:
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    goto L_08B1BC34;
L_08B1BC34:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B1BC44:
    ctx.set_gpr(31, 0x08B1BC4Cu);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1BC4Cu) goto L_08B1BC4C;
    return;
L_08B1BC4C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21648)));
    goto L_08B1BBA8;
L_08B1BC54:
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(31, 0x08B1BC60u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27348));
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1BC60u) goto L_08B1BC60;
    return;
L_08B1BC60:
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    goto L_08B1BC34;
}

void sub_08B1BB84(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B1BB84_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B1C018[24] = {
    1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 4, 0, 0, 0, 0, 5, 0, 6, 0, 7, 0, 8,
};
void sub_08B1C018_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B1C018u;
        entry_id = (entry_delta < 96u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B1C018[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B1C018;
    case 2u: goto L_08B1C038;
    case 3u: goto L_08B1C040;
    case 4u: goto L_08B1C048;
    case 5u: goto L_08B1C05C;
    case 6u: goto L_08B1C064;
    case 7u: goto L_08B1C06C;
    case 8u: goto L_08B1C074;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B1C018:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, 2235u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27348));
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(506));
      if (branch_taken) {
          goto L_08B1C05C;
      }
      goto L_08B1C038;
    }
L_08B1C038:
    ctx.set_gpr(31, 0x08B1C040u);
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(16));
    ctx.pc = 0x08B16CCCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1C040u) goto L_08B1C040;
    return;
L_08B1C040:
    ctx.set_gpr(31, 0x08B1C048u);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.pc = 0x08B1B70Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1C048u) goto L_08B1C048;
    return;
L_08B1C048:
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B1C05C:
    ctx.set_gpr(31, 0x08B1C064u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1C064u) goto L_08B1C064;
    return;
L_08B1C064:
    ctx.set_gpr(31, 0x08B1C06Cu);
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(16));
    ctx.pc = 0x08B16CCCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1C06Cu) goto L_08B1C06C;
    return;
L_08B1C06C:
    ctx.set_gpr(31, 0x08B1C074u);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.pc = 0x08B1B70Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1C074u) goto L_08B1C074;
    return;
L_08B1C074:
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B1C018(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B1C018_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B1CEFC[25] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 5, 0, 0, 6,
};
void sub_08B1CEFC_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B1CEFCu;
        entry_id = (entry_delta < 100u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B1CEFC[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B1CEFC;
    case 2u: goto L_08B1CF24;
    case 3u: goto L_08B1CF30;
    case 4u: goto L_08B1CF48;
    case 5u: goto L_08B1CF50;
    case 6u: goto L_08B1CF5C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B1CEFC:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, 2235u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27300));
    ctx.set_gpr(17, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(85));
      if (branch_taken) {
          goto L_08B1CF48;
      }
      goto L_08B1CF24;
    }
L_08B1CF24:
    ctx.set_gpr(5, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, 0x08B1CF30u);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.pc = 0x08B1B798u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1CF30u) goto L_08B1CF30;
    return;
L_08B1CF30:
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B1CF48:
    ctx.set_gpr(31, 0x08B1CF50u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1CF50u) goto L_08B1CF50;
    return;
L_08B1CF50:
    ctx.set_gpr(5, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, 0x08B1CF5Cu);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.pc = 0x08B1B798u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1CF5Cu) goto L_08B1CF5C;
    return;
L_08B1CF5C:
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B1CEFC(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B1CEFC_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B6029C[77] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 4, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 8, 0, 9, 0, 0, 0, 10, 0, 11, 0, 12, 0, 0, 13, 0, 0, 0, 0, 0, 0,
    0, 14, 0, 15, 0, 16, 0, 0, 17, 0, 18, 0, 19,
};
void sub_08B6029C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B6029Cu;
        entry_id = (entry_delta < 308u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B6029C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B6029C;
    case 2u: goto L_08B602CC;
    case 3u: goto L_08B602D4;
    case 4u: goto L_08B602DC;
    case 5u: goto L_08B602F4;
    case 6u: goto L_08B60308;
    case 7u: goto L_08B60330;
    case 8u: goto L_08B6034C;
    case 9u: goto L_08B60354;
    case 10u: goto L_08B60364;
    case 11u: goto L_08B6036C;
    case 12u: goto L_08B60374;
    case 13u: goto L_08B60380;
    case 14u: goto L_08B603A0;
    case 15u: goto L_08B603A8;
    case 16u: goto L_08B603B0;
    case 17u: goto L_08B603BC;
    case 18u: goto L_08B603C4;
    case 19u: goto L_08B603CC;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B6029C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-48));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[18]);
    ctx.set_gpr(18, 2272u << 16u);
    ctx.set_gpr(2, 32768u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[18] + static_cast<std::uint32_t>(29352)));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(28), ctx.gpr[19]);
    ctx.set_gpr(3, ctx.gpr[2] | 32u);
    ctx.set_gpr(19, ctx.gpr[18] + static_cast<std::uint32_t>(29352));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(32), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[17]);
    { const bool branch_taken = ctx.gpr[4] != 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08B60380;
      }
      goto L_08B602CC;
    }
L_08B602CC:
    ctx.set_gpr(31, 0x08B602D4u);
    // nop
    ctx.pc = 0x08B7FEB4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B602D4u) goto L_08B602D4;
    return;
L_08B602D4:
    ctx.set_gpr(31, 0x08B602DCu);
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(20), ctx.gpr[2]);
    ctx.pc = 0x08B63B84u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B602DCu) goto L_08B602DC;
    return;
L_08B602DC:
    ctx.set_gpr(5, 2233u << 16u);
    ctx.set_gpr(4, ctx.gpr[5] + static_cast<std::uint32_t>(-19580));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(512));
    ctx.set_gpr(31, 0x08B602F4u);
    ctx.set_gpr(7, 0u + 0u);
    ctx.pc = 0x08B7FCC4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B602F4u) goto L_08B602F4;
    return;
L_08B602F4:
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(17, ctx.gpr[3] + static_cast<std::uint32_t>(29304));
    ctx.set_gpr(4, ctx.gpr[29] + 0u);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(3, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B60380;
      }
      goto L_08B60308;
    }
L_08B60308:
    ctx.set_gpr(10, 2230u << 16u);
    ctx.set_gpr(8, 2230u << 16u);
    ctx.set_gpr(7, ctx.gpr[8] + static_cast<std::uint32_t>(15184));
    ctx.set_gpr(9, ctx.gpr[10] + static_cast<std::uint32_t>(15056));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(44), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(31, 0x08B60330u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[17]);
    ctx.pc = 0x08B7FE84u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60330u) goto L_08B60330;
    return;
L_08B60330:
    ctx.set_gpr(6, 2235u << 16u);
    ctx.set_gpr(4, ctx.gpr[6] + static_cast<std::uint32_t>(16596));
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(7, 0u + 0u);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(5, 0u + 0u);
      if (branch_taken) {
          goto L_08B603C4;
      }
      goto L_08B6034C;
    }
L_08B6034C:
    ctx.set_gpr(31, 0x08B60354u);
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(32), ctx.gpr[2]);
    ctx.pc = 0x08B7FE8Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60354u) goto L_08B60354;
    return;
L_08B60354:
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(5, 0u + 0u);
      if (branch_taken) {
          goto L_08B603A0;
      }
      goto L_08B60364;
    }
L_08B60364:
    ctx.set_gpr(31, 0x08B6036Cu);
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(24), ctx.gpr[2]);
    ctx.pc = 0x08B7FE5Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B6036Cu) goto L_08B6036C;
    return;
L_08B6036C:
    ctx.set_gpr(31, 0x08B60374u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x08B7FE94u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60374u) goto L_08B60374;
    return;
L_08B60374:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[18] + static_cast<std::uint32_t>(29352), ctx.gpr[4]);
    ctx.set_gpr(3, 0u + 0u);
    goto L_08B60380;
L_08B60380:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(28)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(2, ctx.gpr[3] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(48));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B603A0:
    ctx.set_gpr(31, 0x08B603A8u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(44)));
    ctx.pc = 0x08B7FC6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B603A8u) goto L_08B603A8;
    return;
L_08B603A8:
    ctx.set_gpr(31, 0x08B603B0u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[19] + static_cast<std::uint32_t>(32)));
    ctx.pc = 0x08B7FE64u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B603B0u) goto L_08B603B0;
    return;
L_08B603B0:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(32), ctx.gpr[2]);
    ctx.set_gpr(3, ctx.gpr[16] + 0u);
    goto L_08B603BC;
L_08B603BC:
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(44), ctx.gpr[2]);
    goto L_08B60380;
L_08B603C4:
    ctx.set_gpr(31, 0x08B603CCu);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(44)));
    ctx.pc = 0x08B7FC6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B603CCu) goto L_08B603CC;
    return;
L_08B603CC:
    ctx.set_gpr(3, ctx.gpr[16] + 0u);
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-1));
    goto L_08B603BC;
}

void sub_08B6029C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B6029C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B63B84[63] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
};
void sub_08B63B84_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B63B84u;
        entry_id = (entry_delta < 252u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B63B84[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B63B84;
    case 2u: goto L_08B63BFC;
    case 3u: goto L_08B63C7C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B63B84:
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(14, ctx.gpr[3] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(15, 0u + static_cast<std::uint32_t>(480));
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(-1));
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(60), ctx.gpr[15]);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(272));
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(12), ctx.gpr[4]);
    ctx.set_gpr(13, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(12, 0u + static_cast<std::uint32_t>(100));
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(4), 0u);
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(11, 0u + static_cast<std::uint32_t>(480));
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(40), ctx.gpr[15]);
    ctx.set_gpr(10, 0u + static_cast<std::uint32_t>(272));
    ctx.set_gpr(9, 0u | 65535u);
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(44), 0u);
    ctx.set_gpr(8, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(48), 0u);
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(52), 0u);
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(56), 0u);
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(8), 0u);
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(16), 0u);
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(72), 0u);
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(68), 0u);
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(36), ctx.gpr[5]);
    ctx.set_gpr(5, ctx.gpr[14] + static_cast<std::uint32_t>(76));
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(64), ctx.gpr[6]);
    ctx.set_gpr(6, ctx.gpr[14] + static_cast<std::uint32_t>(324));
    goto L_08B63BFC;
L_08B63BFC:
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-1));
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(0), ctx.gpr[13]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(16), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(148), ctx.gpr[12]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(152), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(156), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(160), ctx.gpr[11]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(164), ctx.gpr[10]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(168), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(172), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(176), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(180), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(184), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(216), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(220), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(224), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(188), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(192), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(196), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(200), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(204), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(208), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(212), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(228), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(236), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(232), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(240), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(244), 0u);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(252));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(0), ctx.gpr[8]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[3]) >= 0;
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(252));
      if (branch_taken) {
          goto L_08B63BFC;
      }
      goto L_08B63C7C;
    }
L_08B63C7C:
    ctx.set_gpr(8, 2272u << 16u);
    ctx.set_gpr(7, ctx.gpr[8] + static_cast<std::uint32_t>(29304));
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(29304), 0u);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(4), 0u);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B63B84(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B63B84_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B603D8[152] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 5,
    0, 0, 0, 6, 0, 0, 0, 0, 7, 0, 8, 0, 9, 0, 0, 0, 10, 0, 0, 11, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 14, 0, 0, 0, 0, 0, 0, 0, 0, 15, 0, 16, 0,
    0, 17, 0, 18, 0, 0, 0, 19, 0, 0, 20, 0, 0, 21, 0, 0, 0, 0, 0, 0, 0, 22, 0, 23, 0, 24, 0, 0, 0, 0, 0, 0,
    0, 25, 0, 26, 0, 0, 27, 0, 0, 0, 28, 0, 0, 29, 0, 30, 0, 31, 0, 32, 0, 33, 0, 34,
};
void sub_08B603D8_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B603D8u;
        entry_id = (entry_delta < 608u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B603D8[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B603D8;
    case 2u: goto L_08B60414;
    case 3u: goto L_08B6041C;
    case 4u: goto L_08B60438;
    case 5u: goto L_08B60454;
    case 6u: goto L_08B60464;
    case 7u: goto L_08B60478;
    case 8u: goto L_08B60480;
    case 9u: goto L_08B60488;
    case 10u: goto L_08B60498;
    case 11u: goto L_08B604A4;
    case 12u: goto L_08B604B0;
    case 13u: goto L_08B60520;
    case 14u: goto L_08B60524;
    case 15u: goto L_08B60548;
    case 16u: goto L_08B60550;
    case 17u: goto L_08B6055C;
    case 18u: goto L_08B60564;
    case 19u: goto L_08B60574;
    case 20u: goto L_08B60580;
    case 21u: goto L_08B6058C;
    case 22u: goto L_08B605AC;
    case 23u: goto L_08B605B4;
    case 24u: goto L_08B605BC;
    case 25u: goto L_08B605DC;
    case 26u: goto L_08B605E4;
    case 27u: goto L_08B605F0;
    case 28u: goto L_08B60600;
    case 29u: goto L_08B6060C;
    case 30u: goto L_08B60614;
    case 31u: goto L_08B6061C;
    case 32u: goto L_08B60624;
    case 33u: goto L_08B6062C;
    case 34u: goto L_08B60634;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B603D8:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-48));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    ctx.set_gpr(20, ctx.gpr[6] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(19, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(18, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[19] + static_cast<std::uint32_t>(29352));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(32), std::bit_cast<std::uint32_t>(ctx.fpr[20]));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[19] + static_cast<std::uint32_t>(29352)));
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
      if (branch_taken) {
          goto L_08B60634;
      }
      goto L_08B60414;
    }
L_08B60414:
    ctx.set_gpr(31, 0x08B6041Cu);
    // nop
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B6041Cu) goto L_08B6041C;
    return;
L_08B6041C:
    ctx.set_gpr(5, ctx.gpr[16] << 6u);
    ctx.set_gpr(6, ctx.gpr[5] - ctx.gpr[16]);
    ctx.set_gpr(5, ctx.gpr[6] << 2u);
    ctx.set_gpr(6, ctx.gpr[5] + ctx.gpr[17]);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(324)));
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[3]) >= 0;
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B6061C;
      }
      goto L_08B60438;
    }
L_08B60438:
    ctx.set_gpr(10, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(11, ctx.gpr[17] + static_cast<std::uint32_t>(76));
    ctx.set_gpr(9, ctx.gpr[5] + ctx.gpr[11]);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(324), ctx.gpr[10]);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(72), ctx.gpr[9]);
    ctx.set_gpr(31, 0x08B60454u);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(12), ctx.gpr[16]);
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60454u) goto L_08B60454;
    return;
L_08B60454:
    ctx.set_gpr(8, ctx.gpr[16] + static_cast<std::uint32_t>(-3));
    ctx.set_gpr(7, ctx.gpr[8] < static_cast<std::uint32_t>(2) ? 1u : 0u);
    if (ctx.gpr[7] == 0u) {
    ctx.set_gpr(15, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(72)));
        goto L_08B605BC;
    }
    goto L_08B60464;
L_08B60464:
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(13, (ctx.gpr[18] >> 0u) & 0x1FFFFFFFu);
    rt.memory().aot_store32(ctx.gpr[12] + static_cast<std::uint32_t>(12), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[12] + static_cast<std::uint32_t>(8), ctx.gpr[13]);
    rt.memory().aot_store32(ctx.gpr[12] + static_cast<std::uint32_t>(4), ctx.gpr[13]);
    goto L_08B60478;
L_08B60478:
    ctx.set_gpr(16, ctx.gpr[19] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    goto L_08B60480;
L_08B60480:
    if (ctx.gpr[18] == 0u) {
    ctx.set_gpr(25, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(72)));
        goto L_08B6058C;
    }
    goto L_08B60488;
L_08B60488:
    ctx.set_gpr(16, ctx.gpr[19] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    { const bool branch_taken = ctx.gpr[19] == 0u;
    ctx.set_gpr(2, 2235u << 16u);
      if (branch_taken) {
          goto L_08B60548;
      }
      goto L_08B60498;
    }
L_08B60498:
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    { const bool branch_taken = ctx.gpr[5] != 0u;
    ctx.set_gpr(3, 0u + 0u);
      if (branch_taken) {
          goto L_08B60520;
      }
      goto L_08B604A4;
    }
L_08B604A4:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(40)));
    { const bool branch_taken = ctx.gpr[4] == 0u;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
      if (branch_taken) {
          goto L_08B60524;
      }
      goto L_08B604B0;
    }
L_08B604B0:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(60)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(64)));
    ctx.set_gpr(11, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(18, 40192u << 16u);
    ctx.set_gpr(19, 53760u << 16u);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(156), ctx.gpr[3]);
    ctx.set_gpr(13, 39936u << 16u);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(160), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(164), ctx.gpr[6]);
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(44)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[11] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(25, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(15, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(40)));
    ctx.set_gpr(24, (ctx.gpr[12] >> 24u) & 0x0000000Fu);
    ctx.set_gpr(16, ctx.gpr[24] << 16u);
    ctx.set_gpr(9, ctx.gpr[17] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(14, ctx.gpr[16] | ctx.gpr[18]);
    ctx.set_gpr(12, (ctx.gpr[12] & ~0xFF000000u) | ((0u & 0x000000FFu) << 24u));
    ctx.set_gpr(20, ctx.gpr[25] | ctx.gpr[19]);
    ctx.set_gpr(10, ctx.gpr[14] | ctx.gpr[15]);
    ctx.set_gpr(8, ctx.gpr[12] | ctx.gpr[13]);
    ctx.set_gpr(7, ctx.gpr[9] + static_cast<std::uint32_t>(8));
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(0), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(8), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(0), ctx.gpr[10]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(4), ctx.gpr[8]);
    ctx.set_gpr(3, 0u + 0u);
    goto L_08B60520;
L_08B60520:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    goto L_08B60524;
L_08B60524:
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.fpr[20] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    ctx.set_gpr(2, ctx.gpr[3] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(48));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B60548:
    ctx.set_gpr(31, 0x08B60550u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(17472));
    ctx.pc = 0x08B60D58u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60550u) goto L_08B60550;
    return;
L_08B60550:
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(16));
    ctx.set_gpr(31, 0x08B6055Cu);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(16));
    ctx.pc = 0x08B60F20u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B6055Cu) goto L_08B6055C;
    return;
L_08B6055C:
    ctx.set_gpr(31, 0x08B60564u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(7));
    ctx.pc = 0x08B60F4Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60564u) goto L_08B60564;
    return;
L_08B60564:
    ctx.set_gpr(6, 2235u << 16u);
    ctx.fpr[20] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(8320)));
    ctx.set_gpr(31, 0x08B60574u);
    ctx.set_fpr_bits(12, ctx.fpr_bits(20));
    ctx.pc = 0x08B60F74u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60574u) goto L_08B60574;
    return;
L_08B60574:
    ctx.set_fpr_bits(12, ctx.fpr_bits(20));
    ctx.set_gpr(31, 0x08B60580u);
    ctx.set_fpr_bits(13, ctx.fpr_bits(20));
    ctx.pc = 0x08B60FA0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60580u) goto L_08B60580;
    return;
L_08B60580:
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    goto L_08B60498;
L_08B6058C:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(32)));
    ctx.set_gpr(24, 2235u << 16u);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[25] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[25] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(7, ctx.gpr[24] + static_cast<std::uint32_t>(17444));
    ctx.set_gpr(20, 2272u << 16u);
    ctx.set_gpr(31, 0x08B605ACu);
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(29344), 0u);
    ctx.pc = 0x08B7FE8Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B605ACu) goto L_08B605AC;
    return;
L_08B605AC:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(3, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B60520;
      }
      goto L_08B605B4;
    }
L_08B605B4:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(24), ctx.gpr[2]);
    goto L_08B60488;
L_08B605BC:
    ctx.set_gpr(16, 16384u << 16u);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.set_gpr(4, (ctx.gpr[4] & ~0x1FFFFFFFu) | ((ctx.gpr[18] & 0x1FFFFFFFu) << 0u));
    ctx.set_gpr(14, (ctx.gpr[18] >> 30u) & 0x00000001u);
    rt.memory().aot_store32(ctx.gpr[15] + static_cast<std::uint32_t>(12), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[15] + static_cast<std::uint32_t>(8), ctx.gpr[4]);
    { const bool branch_taken = ctx.gpr[14] != 0u;
    rt.memory().aot_store32(ctx.gpr[15] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
      if (branch_taken) {
          goto L_08B60478;
      }
      goto L_08B605DC;
    }
L_08B605DC:
    ctx.set_gpr(31, 0x08B605E4u);
    // nop
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B605E4u) goto L_08B605E4;
    return;
L_08B605E4:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    // PSP CACHE is a no-op in coherent host memory.
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(72)));
    goto L_08B605F0;
L_08B605F0:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, ctx.gpr[3] & 63u);
    { const bool branch_taken = ctx.gpr[17] == 0u;
    ctx.set_gpr(5, ctx.gpr[3] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B6060C;
      }
      goto L_08B60600;
    }
L_08B60600:
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(8), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(0), 0u);
    goto L_08B605F0;
L_08B6060C:
    ctx.set_gpr(31, 0x08B60614u);
    ctx.set_gpr(16, ctx.gpr[19] + static_cast<std::uint32_t>(29352));
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60614u) goto L_08B60614;
    return;
L_08B60614:
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    goto L_08B60480;
L_08B6061C:
    ctx.set_gpr(31, 0x08B60624u);
    // nop
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60624u) goto L_08B60624;
    return;
L_08B60624:
    ctx.set_gpr(4, 32768u << 16u);
    ctx.set_gpr(3, ctx.gpr[4] | 33u);
    goto L_08B6062C;
L_08B6062C:
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(72), 0u);
    goto L_08B60520;
L_08B60634:
    ctx.set_gpr(2, 32768u << 16u);
    ctx.set_gpr(3, ctx.gpr[2] | 1u);
    goto L_08B6062C;
}

void sub_08B603D8(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B603D8_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B60D58[1] = {
    1,
};
void sub_08B60D58_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B60D58u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B60D58[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B60D58;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B60D58:
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(14, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(11, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(29424)));
    ctx.set_gpr(10, ctx.gpr[5] & 15u);
    ctx.set_gpr(8, ctx.gpr[6] & 15u);
    ctx.set_gpr(7, ctx.gpr[10] << 12u);
    ctx.set_gpr(3, ctx.gpr[8] << 8u);
    ctx.set_gpr(25, ctx.gpr[9] & 15u);
    ctx.set_gpr(15, ctx.gpr[7] | ctx.gpr[3]);
    ctx.set_gpr(24, ctx.gpr[25] << 4u);
    ctx.set_gpr(12, ctx.gpr[15] | ctx.gpr[24]);
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[11] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(13, ctx.gpr[14] & 15u);
    ctx.set_gpr(10, ctx.gpr[12] | ctx.gpr[13]);
    ctx.set_gpr(6, 57856u << 16u);
    ctx.set_gpr(8, ctx.gpr[10] | ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(0), ctx.gpr[8]);
    ctx.set_gpr(10, ctx.gpr[9] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(28)));
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(24, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(25, ctx.gpr[3] & 15u);
    ctx.set_gpr(5, ctx.gpr[7] & 15u);
    ctx.set_gpr(14, ctx.gpr[25] << 12u);
    ctx.set_gpr(15, ctx.gpr[5] << 8u);
    ctx.set_gpr(13, ctx.gpr[24] & 15u);
    ctx.set_gpr(6, ctx.gpr[14] | ctx.gpr[15]);
    ctx.set_gpr(12, ctx.gpr[13] << 4u);
    ctx.set_gpr(3, ctx.gpr[6] | ctx.gpr[12]);
    ctx.set_gpr(7, ctx.gpr[9] & 15u);
    ctx.set_gpr(24, ctx.gpr[3] | ctx.gpr[7]);
    ctx.set_gpr(25, 58112u << 16u);
    ctx.set_gpr(15, ctx.gpr[24] | ctx.gpr[25]);
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(0), ctx.gpr[15]);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(44)));
    ctx.set_gpr(14, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(40)));
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(15, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(32)));
    ctx.set_gpr(13, ctx.gpr[2] & 15u);
    ctx.set_gpr(5, ctx.gpr[14] & 15u);
    ctx.set_gpr(7, ctx.gpr[13] << 12u);
    ctx.set_gpr(9, ctx.gpr[5] << 8u);
    ctx.set_gpr(6, ctx.gpr[12] & 15u);
    ctx.set_gpr(24, ctx.gpr[7] | ctx.gpr[9]);
    ctx.set_gpr(25, ctx.gpr[6] << 4u);
    ctx.set_gpr(2, ctx.gpr[24] | ctx.gpr[25]);
    ctx.set_gpr(14, ctx.gpr[15] & 15u);
    ctx.set_gpr(12, ctx.gpr[2] | ctx.gpr[14]);
    ctx.set_gpr(13, 58368u << 16u);
    ctx.set_gpr(9, ctx.gpr[12] | ctx.gpr[13]);
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(4), ctx.gpr[9]);
    ctx.set_gpr(9, ctx.gpr[10] + static_cast<std::uint32_t>(12));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(60)));
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(56)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(52)));
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(48)));
    ctx.set_gpr(5, ctx.gpr[3] & 15u);
    ctx.set_gpr(4, ctx.gpr[7] & 15u);
    ctx.set_gpr(24, ctx.gpr[5] << 12u);
    ctx.set_gpr(25, ctx.gpr[4] << 8u);
    ctx.set_gpr(15, ctx.gpr[6] & 15u);
    ctx.set_gpr(13, ctx.gpr[24] | ctx.gpr[25]);
    ctx.set_gpr(14, ctx.gpr[15] << 4u);
    ctx.set_gpr(3, ctx.gpr[13] | ctx.gpr[14]);
    ctx.set_gpr(7, ctx.gpr[12] & 15u);
    ctx.set_gpr(5, ctx.gpr[3] | ctx.gpr[7]);
    ctx.set_gpr(6, 58624u << 16u);
    ctx.set_gpr(4, ctx.gpr[5] | ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(8), ctx.gpr[4]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B60D58(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B60D58_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B60F20[1] = {
    1,
};
void sub_08B60F20_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B60F20u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B60F20[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B60F20;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B60F20:
    ctx.set_gpr(10, 2272u << 16u);
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[10] + static_cast<std::uint32_t>(29424)));
    ctx.set_gpr(9, ctx.gpr[5] << 8u);
    ctx.set_gpr(8, ctx.gpr[9] | ctx.gpr[4]);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(4, 13824u << 16u);
    ctx.set_gpr(3, ctx.gpr[8] | ctx.gpr[4]);
    ctx.set_gpr(2, ctx.gpr[6] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(0), ctx.gpr[3]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B60F20(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B60F20_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B60F4C[1] = {
    1,
};
void sub_08B60F4C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B60F4Cu;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B60F4C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B60F4C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B60F4C:
    ctx.set_gpr(9, 2272u << 16u);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(29424)));
    ctx.set_gpr(7, ctx.gpr[4] & 7u);
    ctx.set_gpr(8, 21248u << 16u);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, ctx.gpr[7] | ctx.gpr[8]);
    ctx.set_gpr(3, ctx.gpr[5] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(8), ctx.gpr[3]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B60F4C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B60F4C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B60F74[1] = {
    1,
};
void sub_08B60F74_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B60F74u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B60F74[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B60F74;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B60F74:
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(29424)));
    ctx.set_gpr(9, std::bit_cast<std::uint32_t>(ctx.fpr[12]));
    ctx.set_gpr(8, 23296u << 16u);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(7, ctx.gpr[9] >> 8u);
    ctx.set_gpr(4, ctx.gpr[7] | ctx.gpr[8]);
    ctx.set_gpr(2, ctx.gpr[5] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(0), ctx.gpr[4]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B60F74(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B60F74_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B60FA0[1] = {
    1,
};
void sub_08B60FA0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B60FA0u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B60FA0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B60FA0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B60FA0:
    ctx.set_gpr(13, 2272u << 16u);
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(29424)));
    ctx.set_gpr(12, std::bit_cast<std::uint32_t>(ctx.fpr[12]));
    ctx.set_gpr(5, std::bit_cast<std::uint32_t>(ctx.fpr[13]));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(10, ctx.gpr[12] >> 8u);
    ctx.set_gpr(11, 18432u << 16u);
    ctx.set_gpr(7, ctx.gpr[10] | ctx.gpr[11]);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(0), ctx.gpr[7]);
    ctx.set_gpr(3, ctx.gpr[5] >> 8u);
    ctx.set_gpr(7, ctx.gpr[6] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(4, 18688u << 16u);
    ctx.set_gpr(8, ctx.gpr[3] | ctx.gpr[4]);
    ctx.set_gpr(2, ctx.gpr[7] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(0), ctx.gpr[8]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B60FA0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B60FA0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B63C90[21] = {
    1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 4, 5, 0, 0, 6, 0, 7, 0, 8,
};
void sub_08B63C90_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B63C90u;
        entry_id = (entry_delta < 84u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B63C90[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B63C90;
    case 2u: goto L_08B63CAC;
    case 3u: goto L_08B63CBC;
    case 4u: goto L_08B63CC0;
    case 5u: goto L_08B63CC4;
    case 6u: goto L_08B63CD0;
    case 7u: goto L_08B63CD8;
    case 8u: goto L_08B63CE0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B63C90:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(4, ctx.gpr[3] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(12)));
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(3, 0u + 0u);
      if (branch_taken) {
          goto L_08B63CC0;
      }
      goto L_08B63CAC;
    }
L_08B63CAC:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(16)));
    if (ctx.gpr[5] == 0u) {
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(8)));
        goto L_08B63CD0;
    }
    goto L_08B63CBC;
L_08B63CBC:
    ctx.set_gpr(3, 0u + 0u);
    goto L_08B63CC0;
L_08B63CC0:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    goto L_08B63CC4;
L_08B63CC4:
    ctx.set_gpr(2, ctx.gpr[3] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B63CD0:
    ctx.set_gpr(31, 0x08B63CD8u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(24)));
    ctx.pc = 0x08B7FEACu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B63CD8u) goto L_08B63CD8;
    return;
L_08B63CD8:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) >= 0;
    ctx.set_gpr(3, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B63CBC;
      }
      goto L_08B63CE0;
    }
L_08B63CE0:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    goto L_08B63CC4;
}

void sub_08B63C90(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B63C90_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B60B94[69] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 6, 0, 0, 7, 0, 0, 0, 0, 0, 8, 9, 0, 10, 0, 0, 0,
    0, 0, 11, 0, 12,
};
void sub_08B60B94_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B60B94u;
        entry_id = (entry_delta < 276u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B60B94[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B60B94;
    case 2u: goto L_08B60BC0;
    case 3u: goto L_08B60BC4;
    case 4u: goto L_08B60C1C;
    case 5u: goto L_08B60C3C;
    case 6u: goto L_08B60C54;
    case 7u: goto L_08B60C60;
    case 8u: goto L_08B60C78;
    case 9u: goto L_08B60C7C;
    case 10u: goto L_08B60C84;
    case 11u: goto L_08B60C9C;
    case 12u: goto L_08B60CA4;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B60B94:
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(30700)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(18, ctx.gpr[6] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[31]);
      if (branch_taken) {
          goto L_08B60C9C;
      }
      goto L_08B60BC0;
    }
L_08B60BC0:
    ctx.set_gpr(8, 2272u << 16u);
    goto L_08B60BC4;
L_08B60BC4:
    ctx.set_gpr(6, ctx.gpr[8] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(25, (ctx.gpr[17] >> 24u) & 0x0000000Fu);
    ctx.set_gpr(5, ctx.gpr[25] << 16u);
    ctx.set_gpr(15, rt.memory().aot_load32(ctx.gpr[12] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, 40192u << 16u);
    ctx.set_gpr(14, ctx.gpr[17] + 0u);
    ctx.set_gpr(4, 53760u << 16u);
    ctx.set_gpr(13, ctx.gpr[5] | ctx.gpr[2]);
    ctx.set_gpr(14, (ctx.gpr[14] & ~0xFF000000u) | ((0u & 0x000000FFu) << 24u));
    ctx.set_gpr(3, 39936u << 16u);
    ctx.set_gpr(9, ctx.gpr[15] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(24, ctx.gpr[16] | ctx.gpr[4]);
    ctx.set_gpr(10, ctx.gpr[14] | ctx.gpr[3]);
    ctx.set_gpr(11, ctx.gpr[13] | ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[15] + static_cast<std::uint32_t>(0), ctx.gpr[24]);
    ctx.set_gpr(7, ctx.gpr[9] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(0), ctx.gpr[11]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(4), ctx.gpr[10]);
    rt.memory().aot_store32(ctx.gpr[12] + static_cast<std::uint32_t>(8), ctx.gpr[7]);
    ctx.set_gpr(7, ctx.gpr[6] + static_cast<std::uint32_t>(76));
    goto L_08B60C1C;
L_08B60C1C:
    ctx.set_gpr(10, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(60)));
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(64)));
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-1));
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(160), ctx.gpr[10]);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(164), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(156), ctx.gpr[16]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[4]) >= 0;
    ctx.set_gpr(7, ctx.gpr[7] + static_cast<std::uint32_t>(252));
      if (branch_taken) {
          goto L_08B60C1C;
      }
      goto L_08B60C3C;
    }
L_08B60C3C:
    ctx.set_gpr(6, ctx.gpr[8] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(11, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(52)));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(36), ctx.gpr[16]);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(44), ctx.gpr[17]);
    { const bool branch_taken = ctx.gpr[11] != 0u;
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(40), ctx.gpr[18]);
      if (branch_taken) {
          goto L_08B60C78;
      }
      goto L_08B60C54;
    }
L_08B60C54:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(64)));
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(56)));
        goto L_08B60C7C;
    }
    goto L_08B60C60;
L_08B60C60:
    { const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[2])) * static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[18])); ctx.lo = static_cast<std::uint32_t>(product); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(product) >> 32u); }
    ctx.set_gpr(7, ctx.lo);
    ctx.set_gpr(12, ctx.gpr[7] << 2u);
    ctx.set_gpr(16, ctx.gpr[17] + ctx.gpr[12]);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(52), ctx.gpr[16]);
    ctx.set_gpr(6, ctx.gpr[8] + static_cast<std::uint32_t>(29352));
    goto L_08B60C78;
L_08B60C78:
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(56)));
    goto L_08B60C7C;
L_08B60C7C:
    if (ctx.gpr[8] == 0u) {
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(56), ctx.gpr[18]);
        goto L_08B60C84;
    }
    goto L_08B60C84;
L_08B60C84:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B60C9C:
    jump_target = ctx.gpr[2];
    ctx.set_gpr(31, 0x08B60CA4u);
    // nop
    ctx.pc = jump_target;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60CA4u) goto L_08B60CA4;
    return;
L_08B60CA4:
    ctx.set_gpr(8, 2272u << 16u);
    goto L_08B60BC4;
}

void sub_08B60B94(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B60B94_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B609F0[54] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 4, 5, 0, 0, 0, 6, 0,
    0, 0, 0, 0, 7, 8, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 10, 0, 11, 0, 12,
};
void sub_08B609F0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B609F0u;
        entry_id = (entry_delta < 216u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B609F0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B609F0;
    case 2u: goto L_08B60A30;
    case 3u: goto L_08B60A34;
    case 4u: goto L_08B60A54;
    case 5u: goto L_08B60A58;
    case 6u: goto L_08B60A68;
    case 7u: goto L_08B60A80;
    case 8u: goto L_08B60A84;
    case 9u: goto L_08B60AA4;
    case 10u: goto L_08B60AB4;
    case 11u: goto L_08B60ABC;
    case 12u: goto L_08B60AC4;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B609F0:
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(30696)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[21]);
    ctx.set_gpr(21, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    ctx.set_gpr(20, ctx.gpr[7] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(19, ctx.gpr[6] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(18, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[8] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[31]);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08B60ABC;
      }
      goto L_08B60A30;
    }
L_08B60A30:
    ctx.set_gpr(4, 2272u << 16u);
    goto L_08B60A34;
L_08B60A34:
    ctx.set_gpr(16, ctx.gpr[4] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(40)));
    ctx.set_gpr(4, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(60), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(64), ctx.gpr[19]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(48), ctx.gpr[20]);
    { const bool branch_taken = ctx.gpr[3] != 0u;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(36), ctx.gpr[21]);
      if (branch_taken) {
          goto L_08B60A58;
      }
      goto L_08B60A54;
    }
L_08B60A54:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(40), ctx.gpr[17]);
    goto L_08B60A58;
L_08B60A58:
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(60)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(64)));
    ctx.set_gpr(31, 0x08B60A68u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(40), ctx.gpr[17]);
    ctx.pc = 0x08B7FE14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60A68u) goto L_08B60A68;
    return;
L_08B60A68:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(8, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(5, ctx.gpr[17] + 0u);
    ctx.set_gpr(6, ctx.gpr[21] + 0u);
    { const bool branch_taken = ctx.gpr[2] == ctx.gpr[8];
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08B60AA4;
      }
      goto L_08B60A80;
    }
L_08B60A80:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(24)));
    goto L_08B60A84;
L_08B60A84:
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B60AA4:
    ctx.set_gpr(10, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(48)));
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(31, 0x08B60AB4u);
    ctx.set_gpr(4, ctx.gpr[9] + ctx.gpr[10]);
    ctx.pc = 0x08B7FE1Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60AB4u) goto L_08B60AB4;
    return;
L_08B60AB4:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(24)));
    goto L_08B60A84;
L_08B60ABC:
    jump_target = ctx.gpr[2];
    ctx.set_gpr(31, 0x08B60AC4u);
    // nop
    ctx.pc = jump_target;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B60AC4u) goto L_08B60AC4;
    return;
L_08B60AC4:
    ctx.set_gpr(4, 2272u << 16u);
    goto L_08B60A34;
}

void sub_08B609F0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B609F0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B61478[22] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 4, 0, 0, 0, 0, 0, 0, 0, 5,
};
void sub_08B61478_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B61478u;
        entry_id = (entry_delta < 88u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B61478[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B61478;
    case 2u: goto L_08B614A0;
    case 3u: goto L_08B614A4;
    case 4u: goto L_08B614AC;
    case 5u: goto L_08B614CC;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B61478:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, 2272u << 16u);
    ctx.set_gpr(3, ctx.gpr[17] + static_cast<std::uint32_t>(29352));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    { const bool branch_taken = ctx.gpr[4] != 0u;
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
      if (branch_taken) {
          goto L_08B614CC;
      }
      goto L_08B614A0;
    }
L_08B614A0:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(36)));
    goto L_08B614A4;
L_08B614A4:
    ctx.set_gpr(31, 0x08B614ACu);
    // nop
    ctx.pc = 0x08B7FE1Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B614ACu) goto L_08B614AC;
    return;
L_08B614AC:
    ctx.set_gpr(4, ctx.gpr[17] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(8), ctx.gpr[16]);
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B614CC:
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(48)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(40)));
    ctx.set_gpr(4, ctx.gpr[7] + ctx.gpr[8]);
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
    goto L_08B614A4;
}

void sub_08B61478(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B61478_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B614E8[26] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 4,
};
void sub_08B614E8_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B614E8u;
        entry_id = (entry_delta < 104u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B614E8[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B614E8;
    case 2u: goto L_08B61520;
    case 3u: goto L_08B61528;
    case 4u: goto L_08B6154C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B614E8:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(8, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[8] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(2, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(3, ctx.gpr[5] + 0u);
    ctx.set_gpr(10, ctx.gpr[6] + 0u);
    ctx.set_gpr(8, ctx.gpr[7] + 0u);
    ctx.set_gpr(5, ctx.gpr[2] + 0u);
    ctx.set_gpr(6, ctx.gpr[3] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B61520u);
    ctx.set_gpr(7, ctx.gpr[10] + 0u);
    ctx.pc = 0x08B609F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B61520u) goto L_08B61520;
    return;
L_08B61520:
    ctx.set_gpr(5, ctx.gpr[16] + static_cast<std::uint32_t>(76));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    goto L_08B61528;
L_08B61528:
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(36)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(60)));
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(64)));
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(-1));
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(156), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(160), ctx.gpr[4]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(164), ctx.gpr[7]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[6]) >= 0;
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(252));
      if (branch_taken) {
          goto L_08B61528;
      }
      goto L_08B6154C;
    }
L_08B6154C:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B614E8(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B614E8_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B61664[1] = {
    1,
};
void sub_08B61664_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B61664u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B61664[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B61664;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B61664:
    ctx.set_gpr(11, 2272u << 16u);
    ctx.set_gpr(10, ctx.gpr[11] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[10] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(25, (ctx.gpr[4] >> 24u) & 0x0000000Fu);
    ctx.set_gpr(15, ctx.gpr[25] << 16u);
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(24, 40704u << 16u);
    ctx.set_gpr(3, ctx.gpr[4] + 0u);
    ctx.set_gpr(11, ctx.gpr[9] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(14, ctx.gpr[15] | ctx.gpr[24]);
    ctx.set_gpr(3, (ctx.gpr[3] & ~0xFF000000u) | ((0u & 0x000000FFu) << 24u));
    ctx.set_gpr(6, 40448u << 16u);
    ctx.set_gpr(7, ctx.gpr[14] | ctx.gpr[5]);
    ctx.set_gpr(12, ctx.gpr[3] | ctx.gpr[6]);
    ctx.set_gpr(2, ctx.gpr[11] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(0), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(0), ctx.gpr[12]);
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(56), ctx.gpr[5]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(52), ctx.gpr[4]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B61664(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B61664_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B61FF4[1] = {
    1,
};
void sub_08B61FF4_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B61FF4u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B61FF4[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B61FF4;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B61FF4:
    ctx.set_gpr(13, 2272u << 16u);
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(29424)));
    ctx.set_gpr(11, ctx.gpr[4] << 4u);
    ctx.set_gpr(12, 19456u << 16u);
    ctx.set_gpr(10, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(3, ctx.gpr[5] << 4u);
    ctx.set_gpr(7, 19712u << 16u);
    ctx.set_gpr(4, ctx.gpr[10] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(6, ctx.gpr[11] | ctx.gpr[12]);
    ctx.set_gpr(8, ctx.gpr[3] | ctx.gpr[7]);
    ctx.set_gpr(2, ctx.gpr[4] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(0), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(0), ctx.gpr[8]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B61FF4(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B61FF4_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B61EFC[1] = {
    1,
};
void sub_08B61EFC_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B61EFCu;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B61EFC[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B61EFC;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B61EFC:
    ctx.fpr[11] = std::bit_cast<float>(ctx.gpr[6]);
    ctx.fpr[10] = std::bit_cast<float>(ctx.gpr[7]);
    ctx.set_gpr(13, 2235u << 16u);
    ctx.set_gpr(8, 2235u << 16u);
    ctx.fpr[9] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(11)));
    ctx.fpr[7] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(10)));
    ctx.fpr[8] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8324)));
    ctx.fpr[2] = std::bit_cast<float>(ctx.gpr[4]);
    ctx.fpr[5] = std::bit_cast<float>(ctx.gpr[5]);
    ctx.fpr[1] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(8328)));
    ctx.set_gpr(3, 2272u << 16u);
    { const float fs = ctx.fpr[9]; const float ft = ctx.fpr[8]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(3, 0x7FC00000u); else ctx.fpr[3] = fs * ft; }
    { const float fs = ctx.fpr[7]; const float ft = ctx.fpr[1]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(6, 0x7FC00000u); else ctx.fpr[6] = fs * ft; }
    ctx.fpr[4] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(2)));
    ctx.fpr[0] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(5)));
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(29424)));
    ctx.set_gpr(12, std::bit_cast<std::uint32_t>(ctx.fpr[3]));
    ctx.set_gpr(9, std::bit_cast<std::uint32_t>(ctx.fpr[4]));
    ctx.set_gpr(15, rt.memory().aot_load32(ctx.gpr[13] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(24, std::bit_cast<std::uint32_t>(ctx.fpr[6]));
    ctx.set_gpr(4, std::bit_cast<std::uint32_t>(ctx.fpr[0]));
    ctx.set_gpr(10, ctx.gpr[12] >> 8u);
    ctx.set_gpr(11, 16896u << 16u);
    ctx.set_gpr(7, ctx.gpr[9] >> 8u);
    ctx.set_gpr(14, ctx.gpr[24] >> 8u);
    ctx.set_gpr(9, ctx.gpr[15] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(8, ctx.gpr[4] >> 8u);
    ctx.set_gpr(5, 17152u << 16u);
    ctx.set_gpr(6, 17664u << 16u);
    ctx.set_gpr(3, 17920u << 16u);
    ctx.set_gpr(25, ctx.gpr[10] | ctx.gpr[11]);
    ctx.set_gpr(12, ctx.gpr[14] | ctx.gpr[5]);
    ctx.set_gpr(11, ctx.gpr[7] | ctx.gpr[6]);
    ctx.set_gpr(10, ctx.gpr[8] | ctx.gpr[3]);
    ctx.set_gpr(2, ctx.gpr[9] + static_cast<std::uint32_t>(12));
    rt.memory().aot_store32(ctx.gpr[15] + static_cast<std::uint32_t>(0), ctx.gpr[25]);
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(0), ctx.gpr[12]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(4), ctx.gpr[11]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(8), ctx.gpr[10]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B61EFC(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B61EFC_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B61FA0[9] = {
    1, 0, 0, 0, 0, 0, 0, 0, 2,
};
void sub_08B61FA0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B61FA0u;
        entry_id = (entry_delta < 36u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B61FA0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B61FA0;
    case 2u: goto L_08B61FC0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B61FA0:
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(7, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(29424)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(6, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B61FC0u);
    ctx.set_gpr(5, ctx.gpr[7] + 0u);
    ctx.pc = 0x08B63564u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B61FC0u) goto L_08B61FC0;
    return;
L_08B61FC0:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B61FA0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B61FA0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B610C8[5] = {
    1, 0, 0, 0, 2,
};
void sub_08B610C8_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B610C8u;
        entry_id = (entry_delta < 20u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B610C8[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B610C8;
    case 2u: goto L_08B610D8;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B610C8:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B610D8u);
    ctx.set_gpr(4, 0u + 0u);
    ctx.pc = 0x08B60640u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B610D8u) goto L_08B610D8;
    return;
L_08B610D8:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B610C8(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B610C8_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B613F0[17] = {
    1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 4,
};
void sub_08B613F0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B613F0u;
        entry_id = (entry_delta < 68u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B613F0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B613F0;
    case 2u: goto L_08B61400;
    case 3u: goto L_08B61424;
    case 4u: goto L_08B61430;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B613F0:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(3, ctx.gpr[4] < static_cast<std::uint32_t>(5) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[3] == 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
      if (branch_taken) {
          goto L_08B61430;
      }
      goto L_08B61400;
    }
L_08B61400:
    ctx.set_gpr(7, 2235u << 16u);
    ctx.set_gpr(6, ctx.gpr[4] << 2u);
    ctx.set_gpr(2, ctx.gpr[7] + static_cast<std::uint32_t>(17556));
    ctx.set_gpr(4, ctx.gpr[6] + ctx.gpr[2]);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[3];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B61424:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B61430:
    ctx.set_gpr(2, 0u + 0u);
    goto L_08B61424;
}

void sub_08B613F0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B613F0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B6141C[3] = {
    1, 0, 2,
};
void sub_08B6141C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B6141Cu;
        entry_id = (entry_delta < 12u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B6141C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B6141C;
    case 2u: goto L_08B61424;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B6141C:
    ctx.set_gpr(31, 0x08B61424u);
    ctx.set_gpr(4, ctx.gpr[5] + 0u);
    ctx.pc = 0x08B7FE94u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B61424u) goto L_08B61424;
    return;
L_08B61424:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B6141C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B6141C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B61430[4] = {
    1, 0, 0, 2,
};
void sub_08B61430_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B61424u;
        entry_id = (entry_delta < 16u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B61430[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B61424;
    case 2u: goto L_08B61430;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B61424:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B61430:
    ctx.set_gpr(2, 0u + 0u);
    goto L_08B61424;
}

void sub_08B61430(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B61430_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B61438[9] = {
    1, 0, 0, 0, 2, 0, 0, 0, 3,
};
void sub_08B61438_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B61428u;
        entry_id = (entry_delta < 36u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B61438[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B61428;
    case 2u: goto L_08B61438;
    case 3u: goto L_08B61448;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B61428:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B61438:
    ctx.set_gpr(8, 2272u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(29376)));
    ctx.set_gpr(31, 0x08B61448u);
    // nop
    ctx.pc = 0x08B7FE5Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B61448u) goto L_08B61448;
    return;
L_08B61448:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    goto L_08B61428;
}

void sub_08B61438(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B61438_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B61450[11] = {
    1, 0, 0, 0, 0, 0, 2, 0, 3, 0, 4,
};
void sub_08B61450_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B61428u;
        entry_id = (entry_delta < 44u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B61450[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B61428;
    case 2u: goto L_08B61440;
    case 3u: goto L_08B61448;
    case 4u: goto L_08B61450;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B61428:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B61440:
    ctx.set_gpr(31, 0x08B61448u);
    // nop
    ctx.pc = 0x08B7FE5Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B61448u) goto L_08B61448;
    return;
L_08B61448:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    goto L_08B61428;
L_08B61450:
    ctx.set_gpr(9, 2272u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(29380)));
    goto L_08B61440;
}

void sub_08B61450(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B61450_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B5F300[124] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 4, 0, 0, 0,
    0, 5, 0, 6, 0, 0, 0, 0, 7, 0, 8, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 11, 0, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 15, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17,
};
void sub_08B5F300_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B5F300u;
        entry_id = (entry_delta < 496u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B5F300[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B5F300;
    case 2u: goto L_08B5F344;
    case 3u: goto L_08B5F368;
    case 4u: goto L_08B5F370;
    case 5u: goto L_08B5F384;
    case 6u: goto L_08B5F38C;
    case 7u: goto L_08B5F3A0;
    case 8u: goto L_08B5F3A8;
    case 9u: goto L_08B5F3BC;
    case 10u: goto L_08B5F3EC;
    case 11u: goto L_08B5F414;
    case 12u: goto L_08B5F41C;
    case 13u: goto L_08B5F454;
    case 14u: goto L_08B5F45C;
    case 15u: goto L_08B5F494;
    case 16u: goto L_08B5F4A4;
    case 17u: goto L_08B5F4EC;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B5F300:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-48));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[22]);
    ctx.set_gpr(22, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(19, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(18, ctx.gpr[6] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[7] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(32), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(28), ctx.gpr[23]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[21]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    { const bool branch_taken = ctx.gpr[8] != 0u;
    rt.memory().aot_store32(ctx.gpr[22] + static_cast<std::uint32_t>(29220), 0u);
      if (branch_taken) {
          goto L_08B5F4A4;
      }
      goto L_08B5F344;
    }
L_08B5F344:
    ctx.set_gpr(20, 2272u << 16u);
    ctx.set_gpr(21, 2272u << 16u);
    ctx.set_gpr(13, ctx.gpr[20] + static_cast<std::uint32_t>(29224));
    ctx.set_gpr(2, ctx.gpr[21] + static_cast<std::uint32_t>(29272));
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(40), 0u);
    ctx.set_gpr(23, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(12), 0u);
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(36), 0u);
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(44), 0u);
    goto L_08B5F368;
L_08B5F368:
    { const bool branch_taken = ctx.gpr[16] != 0u;
    ctx.set_gpr(9, ctx.gpr[20] + static_cast<std::uint32_t>(29224));
      if (branch_taken) {
          goto L_08B5F45C;
      }
      goto L_08B5F370;
    }
L_08B5F370:
    ctx.set_gpr(10, ctx.gpr[21] + static_cast<std::uint32_t>(29272));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(28), 0u);
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(8), 0u);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(24), 0u);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(32), 0u);
    goto L_08B5F384;
L_08B5F384:
    { const bool branch_taken = ctx.gpr[18] != 0u;
    ctx.set_gpr(16, ctx.gpr[20] + static_cast<std::uint32_t>(29224));
      if (branch_taken) {
          goto L_08B5F41C;
      }
      goto L_08B5F38C;
    }
L_08B5F38C:
    ctx.set_gpr(2, ctx.gpr[21] + static_cast<std::uint32_t>(29272));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(16), 0u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(4), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(12), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(20), 0u);
    goto L_08B5F3A0;
L_08B5F3A0:
    { const bool branch_taken = ctx.gpr[19] != 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1));
      if (branch_taken) {
          goto L_08B5F3EC;
      }
      goto L_08B5F3A8;
    }
L_08B5F3A8:
    ctx.set_gpr(22, ctx.gpr[20] + static_cast<std::uint32_t>(29224));
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(29272), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(29224), 0u);
    rt.memory().aot_store32(ctx.gpr[22] + static_cast<std::uint32_t>(4), 0u);
    rt.memory().aot_store32(ctx.gpr[22] + static_cast<std::uint32_t>(8), 0u);
    goto L_08B5F3BC;
L_08B5F3BC:
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(29216), 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    ctx.set_gpr(23, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(28)));
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(48));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B5F3EC:
    ctx.set_gpr(25, ctx.gpr[19] << 6u);
    ctx.set_gpr(24, ctx.gpr[17] + ctx.gpr[25]);
    ctx.set_gpr(19, ctx.gpr[20] + static_cast<std::uint32_t>(29224));
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(4), ctx.gpr[24]);
    rt.memory().aot_store32(ctx.gpr[22] + static_cast<std::uint32_t>(29220), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(8), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(29224), ctx.gpr[17]);
    ctx.set_gpr(17, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(31, 0x08B5F414u);
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(29216), 0u);
    ctx.pc = 0x08B5F4FCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B5F414u) goto L_08B5F414;
    return;
L_08B5F414:
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(29272), ctx.gpr[17]);
    goto L_08B5F3BC;
L_08B5F41C:
    ctx.set_gpr(15, ctx.gpr[19] + ctx.gpr[18]);
    ctx.set_gpr(13, ctx.gpr[15] << 6u);
    ctx.set_gpr(14, ctx.gpr[19] << 6u);
    ctx.set_gpr(3, ctx.gpr[17] + ctx.gpr[14]);
    ctx.set_gpr(11, ctx.gpr[20] + static_cast<std::uint32_t>(29224));
    ctx.set_gpr(12, ctx.gpr[17] + ctx.gpr[13]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(16), ctx.gpr[12]);
    ctx.set_gpr(16, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(18, ctx.gpr[21] + static_cast<std::uint32_t>(29272));
    rt.memory().aot_store32(ctx.gpr[22] + static_cast<std::uint32_t>(29220), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(12), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(20), ctx.gpr[3]);
    ctx.set_gpr(31, 0x08B5F454u);
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(29216), ctx.gpr[16]);
    ctx.pc = 0x08B5F4FCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B5F454u) goto L_08B5F454;
    return;
L_08B5F454:
    rt.memory().aot_store32(ctx.gpr[18] + static_cast<std::uint32_t>(4), ctx.gpr[16]);
    goto L_08B5F3A0;
L_08B5F45C:
    ctx.set_gpr(7, ctx.gpr[19] + ctx.gpr[18]);
    ctx.set_gpr(8, ctx.gpr[7] + ctx.gpr[16]);
    ctx.set_gpr(4, ctx.gpr[8] << 6u);
    ctx.set_gpr(6, ctx.gpr[7] << 6u);
    ctx.set_gpr(24, ctx.gpr[17] + ctx.gpr[6]);
    ctx.set_gpr(16, ctx.gpr[20] + static_cast<std::uint32_t>(29224));
    ctx.set_gpr(5, ctx.gpr[17] + ctx.gpr[4]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(28), ctx.gpr[5]);
    ctx.set_gpr(25, 0u + static_cast<std::uint32_t>(2));
    rt.memory().aot_store32(ctx.gpr[22] + static_cast<std::uint32_t>(29220), ctx.gpr[24]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(24), ctx.gpr[24]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(32), ctx.gpr[24]);
    ctx.set_gpr(31, 0x08B5F494u);
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(29216), ctx.gpr[25]);
    ctx.pc = 0x08B5F4FCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B5F494u) goto L_08B5F494;
    return;
L_08B5F494:
    ctx.set_gpr(14, ctx.gpr[21] + static_cast<std::uint32_t>(29272));
    ctx.set_gpr(15, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(8), ctx.gpr[15]);
    goto L_08B5F384;
L_08B5F4A4:
    ctx.set_gpr(12, ctx.gpr[5] + ctx.gpr[6]);
    ctx.set_gpr(10, ctx.gpr[12] + ctx.gpr[7]);
    ctx.set_gpr(11, ctx.gpr[10] + ctx.gpr[8]);
    ctx.set_gpr(8, ctx.gpr[11] << 6u);
    ctx.set_gpr(9, ctx.gpr[10] << 6u);
    ctx.set_gpr(20, 2272u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + ctx.gpr[9]);
    ctx.set_gpr(5, ctx.gpr[20] + static_cast<std::uint32_t>(29224));
    ctx.set_gpr(7, ctx.gpr[17] + ctx.gpr[8]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(40), ctx.gpr[7]);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(3));
    ctx.set_gpr(23, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[22] + static_cast<std::uint32_t>(29220), ctx.gpr[4]);
    ctx.set_gpr(21, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(36), ctx.gpr[4]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(44), ctx.gpr[4]);
    ctx.set_gpr(31, 0x08B5F4ECu);
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(29216), ctx.gpr[6]);
    ctx.pc = 0x08B5F4FCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B5F4ECu) goto L_08B5F4EC;
    return;
L_08B5F4EC:
    ctx.set_gpr(4, ctx.gpr[21] + static_cast<std::uint32_t>(29272));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(12), ctx.gpr[3]);
    goto L_08B5F368;
}

void sub_08B5F300(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B5F300_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B14550[5] = {
    1, 0, 0, 0, 2,
};
void sub_08B14550_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B14550u;
        entry_id = (entry_delta < 20u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B14550[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B14550;
    case 2u: goto L_08B14560;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B14550:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B14560u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(2));
    ctx.pc = 0x08B7FE2Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B14560u) goto L_08B14560;
    return;
L_08B14560:
    ctx.set_gpr(2, 0u + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B14550(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B14550_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B61228[13] = {
    1, 0, 0, 0, 2, 0, 0, 3, 0, 4, 0, 0, 5,
};
void sub_08B61228_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B61228u;
        entry_id = (entry_delta < 52u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B61228[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B61228;
    case 2u: goto L_08B61238;
    case 3u: goto L_08B61244;
    case 4u: goto L_08B6124C;
    case 5u: goto L_08B61258;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B61228:
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(3, 2272u << 16u);
    { const bool branch_taken = ctx.gpr[4] == ctx.gpr[7];
    ctx.set_gpr(6, 0u + 0u);
      if (branch_taken) {
          goto L_08B61258;
      }
      goto L_08B61238;
    }
L_08B61238:
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(4));
    { const bool branch_taken = ctx.gpr[4] == ctx.gpr[7];
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(29304));
      if (branch_taken) {
          goto L_08B6124C;
      }
      goto L_08B61244;
    }
L_08B61244:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(2, ctx.gpr[6] + 0u);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B6124C:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(4)));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[5]);
    goto L_08B61244;
L_08B61258:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(29304)));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(29304), ctx.gpr[5]);
    goto L_08B61244;
}

void sub_08B61228(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B61228_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B15DCC[27] = {
    1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 0, 0, 6,
};
void sub_08B15DCC_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B15DCCu;
        entry_id = (entry_delta < 108u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B15DCC[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B15DCC;
    case 2u: goto L_08B15DEC;
    case 3u: goto L_08B15DFC;
    case 4u: goto L_08B15E24;
    case 5u: goto L_08B15E28;
    case 6u: goto L_08B15E34;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B15DCC:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    ctx.set_gpr(5, ctx.gpr[29] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(6, ctx.gpr[29] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(7, 0u + 0u);
    ctx.set_gpr(4, ctx.gpr[29] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B15DECu);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), 0u);
    ctx.pc = 0x08B7FE4Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B15DECu) goto L_08B15DEC;
    return;
L_08B15DEC:
    ctx.set_gpr(7, 2272u << 16u);
    ctx.set_gpr(5, ctx.gpr[7] + static_cast<std::uint32_t>(21360));
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(512));
      if (branch_taken) {
          goto L_08B15E28;
      }
      goto L_08B15DFC;
    }
L_08B15DFC:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 65527u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] | 32768u);
    ctx.set_gpr(4, ctx.gpr[3] + ctx.gpr[2]);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    rt.memory().aot_store16(ctx.gpr[5] + static_cast<std::uint32_t>(2), static_cast<std::uint16_t>(ctx.gpr[6]));
    rt.memory().aot_store16(ctx.gpr[5] + static_cast<std::uint32_t>(4), static_cast<std::uint16_t>(ctx.gpr[2]));
    ctx.set_gpr(2, 1024u << 16u);
    { const bool branch_taken = ctx.gpr[3] == ctx.gpr[2];
    rt.memory().aot_store16(ctx.gpr[7] + static_cast<std::uint32_t>(21360), static_cast<std::uint16_t>(ctx.gpr[6]));
      if (branch_taken) {
          goto L_08B15E34;
      }
      goto L_08B15E24;
    }
L_08B15E24:
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(16), ctx.gpr[4]);
    goto L_08B15E28;
L_08B15E28:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B15E34:
    ctx.set_gpr(2, 1032u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] | 32768u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(16), ctx.gpr[2]);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B15DCC(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B15DCC_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B63564[20] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3,
};
void sub_08B63564_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B63564u;
        entry_id = (entry_delta < 80u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B63564[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B63564;
    case 2u: goto L_08B635A8;
    case 3u: goto L_08B635B0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B63564:
    ctx.set_gpr(7, ctx.gpr[5] + ctx.gpr[6]);
    ctx.fpr[1] = std::bit_cast<float>(ctx.gpr[7]);
    ctx.set_gpr(3, 2235u << 16u);
    ctx.fpr[3] = std::bit_cast<float>(ctx.gpr[5]);
    ctx.fpr[5] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(1)));
    ctx.fpr[2] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(8340)));
    ctx.fpr[4] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(3)));
    ctx.set_gpr(8, ctx.gpr[4] + 0u);
    { const float fs = ctx.fpr[5]; const float ft = ctx.fpr[2]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(3, 0x7FC00000u); else ctx.fpr[3] = fs * ft; }
    ctx.set_gpr(7, static_cast<std::int32_t>(ctx.gpr[5]) < static_cast<std::int32_t>(ctx.gpr[6]) ? 1u : 0u);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(216), ctx.gpr[5]);
    ctx.set_gpr(9, ctx.gpr[5] + 0u);
    ctx.fpr[0] = ctx.fpr[3] - ctx.fpr[4];
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(220), ctx.gpr[6]);
    ctx.set_gpr(4, ctx.gpr[6] + 0u);
    { const bool branch_taken = ctx.gpr[7] != 0u;
    ctx.fpr[1] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(224)));
      if (branch_taken) {
          goto L_08B635B0;
      }
      goto L_08B635A8;
    }
L_08B635A8:
    ctx.set_gpr(9, ctx.gpr[6] + 0u);
    ctx.set_gpr(4, ctx.gpr[5] + 0u);
    goto L_08B635B0;
L_08B635B0:
    ctx.fpr[7] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(1)));
    ctx.set_gpr(15, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(7, std::bit_cast<std::uint32_t>(ctx.fpr[0]));
    ctx.set_gpr(6, 54784u << 16u);
    ctx.fpr[6] = ctx.fpr[3] + ctx.fpr[7];
    ctx.set_gpr(24, 55040u << 16u);
    ctx.set_gpr(11, ctx.gpr[9] | ctx.gpr[6]);
    ctx.set_gpr(10, ctx.gpr[4] | ctx.gpr[24]);
    ctx.set_gpr(14, std::bit_cast<std::uint32_t>(ctx.fpr[6]));
    ctx.set_gpr(9, ctx.gpr[15] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(25, ctx.gpr[7] >> 8u);
    ctx.set_gpr(3, 17408u << 16u);
    ctx.set_gpr(13, ctx.gpr[14] >> 8u);
    ctx.set_gpr(4, 18176u << 16u);
    ctx.set_gpr(5, ctx.gpr[25] | ctx.gpr[3]);
    ctx.set_gpr(12, ctx.gpr[13] | ctx.gpr[4]);
    ctx.set_gpr(2, ctx.gpr[9] + static_cast<std::uint32_t>(12));
    rt.memory().aot_store32(ctx.gpr[15] + static_cast<std::uint32_t>(0), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(0), ctx.gpr[12]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(4), ctx.gpr[11]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(8), ctx.gpr[10]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B63564(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B63564_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B6360C[21] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3,
};
void sub_08B6360C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B6360Cu;
        entry_id = (entry_delta < 84u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B6360C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B6360C;
    case 2u: goto L_08B63654;
    case 3u: goto L_08B6365C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B6360C:
    ctx.set_gpr(8, ctx.gpr[4] + 0u);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(220)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(216)));
    ctx.set_gpr(3, 2235u << 16u);
    ctx.fpr[1] = std::bit_cast<float>(ctx.gpr[5]);
    ctx.set_gpr(7, ctx.gpr[4] + ctx.gpr[6]);
    ctx.fpr[2] = std::bit_cast<float>(ctx.gpr[7]);
    ctx.fpr[3] = std::bit_cast<float>(ctx.gpr[4]);
    ctx.fpr[5] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(8344)));
    ctx.fpr[6] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(2)));
    ctx.set_gpr(7, static_cast<std::int32_t>(ctx.gpr[4]) < static_cast<std::int32_t>(ctx.gpr[6]) ? 1u : 0u);
    ctx.fpr[4] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(3)));
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(224), ctx.gpr[5]);
    ctx.fpr[3] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(1)));
    { const float fs = ctx.fpr[6]; const float ft = ctx.fpr[5]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(2, 0x7FC00000u); else ctx.fpr[2] = fs * ft; }
    ctx.set_gpr(2, ctx.gpr[4] + 0u);
    { const bool branch_taken = ctx.gpr[7] != 0u;
    ctx.set_gpr(9, ctx.gpr[6] + 0u);
      if (branch_taken) {
          goto L_08B6365C;
      }
      goto L_08B63654;
    }
L_08B63654:
    ctx.set_gpr(2, ctx.gpr[6] + 0u);
    ctx.set_gpr(9, ctx.gpr[4] + 0u);
    goto L_08B6365C;
L_08B6365C:
    ctx.fpr[0] = ctx.fpr[2] - ctx.fpr[4];
    ctx.fpr[4] = ctx.fpr[2] + ctx.fpr[3];
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(4, 54784u << 16u);
    ctx.set_gpr(3, std::bit_cast<std::uint32_t>(ctx.fpr[0]));
    ctx.set_gpr(15, std::bit_cast<std::uint32_t>(ctx.fpr[4]));
    ctx.set_gpr(11, ctx.gpr[2] | ctx.gpr[4]);
    ctx.set_gpr(24, ctx.gpr[3] >> 8u);
    ctx.set_gpr(4, ctx.gpr[13] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(25, 17408u << 16u);
    ctx.set_gpr(5, ctx.gpr[15] >> 8u);
    ctx.set_gpr(6, 18176u << 16u);
    ctx.set_gpr(2, 55040u << 16u);
    ctx.set_gpr(10, ctx.gpr[9] | ctx.gpr[2]);
    ctx.set_gpr(14, ctx.gpr[24] | ctx.gpr[25]);
    ctx.set_gpr(12, ctx.gpr[5] | ctx.gpr[6]);
    ctx.set_gpr(9, ctx.gpr[4] + static_cast<std::uint32_t>(12));
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(0), ctx.gpr[14]);
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(0), ctx.gpr[12]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(4), ctx.gpr[11]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(8), ctx.gpr[10]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B6360C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B6360C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B636B8[24] = {
    1, 0, 0, 0, 0, 2, 0, 3, 0, 0, 4, 0, 0, 0, 5, 0, 6, 0, 7, 0, 0, 0, 0, 8,
};
void sub_08B636B8_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B636B8u;
        entry_id = (entry_delta < 96u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B636B8[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B636B8;
    case 2u: goto L_08B636CC;
    case 3u: goto L_08B636D4;
    case 4u: goto L_08B636E0;
    case 5u: goto L_08B636F0;
    case 6u: goto L_08B636F8;
    case 7u: goto L_08B63700;
    case 8u: goto L_08B63714;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B636B8:
    ctx.set_gpr(2, 14080u << 16u);
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(2));
    ctx.set_gpr(6, ctx.gpr[2] | 1u);
    { const bool branch_taken = ctx.gpr[5] == ctx.gpr[7];
    ctx.set_gpr(3, static_cast<std::int32_t>(ctx.gpr[5]) < 3 ? 1u : 0u);
      if (branch_taken) {
          goto L_08B63714;
      }
      goto L_08B636CC;
    }
L_08B636CC:
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B636F8;
      }
      goto L_08B636D4;
    }
L_08B636D4:
    ctx.set_gpr(6, 14080u << 16u);
    { const bool branch_taken = ctx.gpr[5] != 0u;
    ctx.set_gpr(3, ctx.gpr[6] | 2u);
      if (branch_taken) {
          goto L_08B636F0;
      }
      goto L_08B636E0;
    }
L_08B636E0:
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(11, ctx.gpr[12] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[12] + static_cast<std::uint32_t>(0), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(8), ctx.gpr[11]);
    goto L_08B636F0;
L_08B636F0:
    jump_target = ctx.gpr[31];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B636F8:
    { const bool branch_taken = ctx.gpr[5] != ctx.gpr[3];
    ctx.set_gpr(8, 14080u << 16u);
      if (branch_taken) {
          goto L_08B636F0;
      }
      goto L_08B63700;
    }
L_08B63700:
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(5, ctx.gpr[7] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(0), ctx.gpr[8]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(8), ctx.gpr[5]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B63714:
    ctx.set_gpr(10, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(9, ctx.gpr[10] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(0), ctx.gpr[6]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B636B8(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B636B8_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B5F4FC[9] = {
    1, 0, 0, 0, 2, 0, 0, 0, 3,
};
void sub_08B5F4FC_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B5F4FCu;
        entry_id = (entry_delta < 36u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B5F4FC[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B5F4FC;
    case 2u: goto L_08B5F50C;
    case 3u: goto L_08B5F51C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B5F4FC:
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(29220)));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(15));
    ctx.set_gpr(2, ctx.gpr[5] + 0u);
    goto L_08B5F50C;
L_08B5F50C:
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-1));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(0), 0u);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[3]) >= 0;
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B5F50C;
      }
      goto L_08B5F51C;
    }
L_08B5F51C:
    ctx.set_gpr(4, 2272u << 16u);
    ctx.set_gpr(10, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(29216)));
    ctx.set_gpr(2, 2235u << 16u);
    ctx.fpr[1] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(8316)));
    ctx.set_gpr(9, 2272u << 16u);
    ctx.set_gpr(7, ctx.gpr[10] << 2u);
    ctx.set_gpr(8, ctx.gpr[9] + static_cast<std::uint32_t>(29272));
    ctx.set_gpr(3, ctx.gpr[7] + ctx.gpr[8]);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(60), std::bit_cast<std::uint32_t>(ctx.fpr[1]));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(0), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[1]));
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(20), std::bit_cast<std::uint32_t>(ctx.fpr[1]));
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(40), std::bit_cast<std::uint32_t>(ctx.fpr[1]));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B5F4FC(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B5F4FC_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B175C0[164] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 4, 5, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 8,
    0, 0, 0, 0, 0, 0, 0, 9, 0, 10, 0, 0, 11, 0, 0, 12, 0, 0, 0, 0, 0, 13, 0, 14, 0, 15, 0, 0, 0, 16, 0, 17,
    18, 0, 19, 0, 20, 0, 0, 21, 0, 0, 22, 0, 0, 0, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 24, 25, 0, 0, 26, 27, 0, 0, 0, 0, 28, 0, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30, 0, 31,
    0, 32, 0, 33,
};
void sub_08B175C0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B17400u;
        entry_id = (entry_delta < 656u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B175C0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B17400;
    case 2u: goto L_08B1744C;
    case 3u: goto L_08B17454;
    case 4u: goto L_08B1748C;
    case 5u: goto L_08B17490;
    case 6u: goto L_08B174B0;
    case 7u: goto L_08B174DC;
    case 8u: goto L_08B174FC;
    case 9u: goto L_08B1751C;
    case 10u: goto L_08B17524;
    case 11u: goto L_08B17530;
    case 12u: goto L_08B1753C;
    case 13u: goto L_08B17554;
    case 14u: goto L_08B1755C;
    case 15u: goto L_08B17564;
    case 16u: goto L_08B17574;
    case 17u: goto L_08B1757C;
    case 18u: goto L_08B17580;
    case 19u: goto L_08B17588;
    case 20u: goto L_08B17590;
    case 21u: goto L_08B1759C;
    case 22u: goto L_08B175A8;
    case 23u: goto L_08B175C0;
    case 24u: goto L_08B17608;
    case 25u: goto L_08B1760C;
    case 26u: goto L_08B17618;
    case 27u: goto L_08B1761C;
    case 28u: goto L_08B17630;
    case 29u: goto L_08B17638;
    case 30u: goto L_08B17674;
    case 31u: goto L_08B1767C;
    case 32u: goto L_08B17684;
    case 33u: goto L_08B1768C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B17400:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[21]);
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(21, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    ctx.set_gpr(20, 0u + static_cast<std::uint32_t>(16));
    if (ctx.gpr[6] != 0u) ctx.set_gpr(20, ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(19, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[8] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(28), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[22]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(22636)));
    { const bool branch_taken = ctx.gpr[8] == 0u;
    if (ctx.gpr[3] != 0u) ctx.set_gpr(21, ctx.gpr[7]);
      if (branch_taken) {
          goto L_08B17564;
      }
      goto L_08B1744C;
    }
L_08B1744C:
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(3));
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(24), ctx.gpr[2]);
    goto L_08B17454;
L_08B17454:
    ctx.set_gpr(2, ctx.gpr[20] + ctx.gpr[17]);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(3, 0u - ctx.gpr[20]);
    ctx.set_gpr(4, ctx.gpr[19] + static_cast<std::uint32_t>(7));
    ctx.set_gpr(17, ctx.gpr[16] + static_cast<std::uint32_t>(16));
    ctx.set_gpr(2, ctx.gpr[2] & ctx.gpr[3]);
    ctx.set_gpr(18, ctx.gpr[4] >> 3u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[19]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(12), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(16), ctx.gpr[17]);
    { const bool branch_taken = ctx.gpr[21] != 0u;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(20), ctx.gpr[17]);
      if (branch_taken) {
          goto L_08B174DC;
      }
      goto L_08B1748C;
    }
L_08B1748C:
    ctx.set_gpr(2, 2272u << 16u);
    goto L_08B17490;
L_08B17490:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(21508)));
    ctx.set_gpr(5, ctx.gpr[16] + static_cast<std::uint32_t>(28));
    ctx.set_gpr(6, ctx.gpr[2] + static_cast<std::uint32_t>(21508));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(28), ctx.gpr[3]);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(21508)));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(32), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21508), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(4), ctx.gpr[5]);
    goto L_08B174B0;
L_08B174B0:
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(28)));
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B174DC:
    ctx.lo = ctx.gpr[18];
    { const std::int64_t acc = static_cast<std::int64_t>((static_cast<std::uint64_t>(ctx.hi) << 32u) | static_cast<std::uint64_t>(ctx.lo)) + static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[2])) * static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[19])); ctx.lo = static_cast<std::uint32_t>(acc); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(acc) >> 32u); }
    ctx.set_gpr(22, 2235u << 16u);
    ctx.set_gpr(19, 0u + 0u);
    ctx.set_gpr(2, ctx.lo);
    ctx.set_gpr(2, ctx.gpr[20] + ctx.gpr[2]);
    ctx.set_gpr(20, ctx.gpr[2] + static_cast<std::uint32_t>(7));
    goto L_08B17524;
L_08B174FC:
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(0), 0u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(16)));
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(16)));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(16), ctx.gpr[7]);
    ctx.set_gpr(31, 0x08B1751Cu);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[7]);
    ctx.pc = 0x08B73CF0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1751Cu) goto L_08B1751C;
    return;
L_08B1751C:
    { const bool branch_taken = ctx.gpr[21] == ctx.gpr[19];
    ctx.set_gpr(2, 2272u << 16u);
      if (branch_taken) {
          goto L_08B17490;
      }
      goto L_08B17524;
    }
L_08B17524:
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(97));
    ctx.set_gpr(31, 0x08B17530u);
    ctx.set_gpr(4, ctx.gpr[22] + static_cast<std::uint32_t>(-27492));
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B17530u) goto L_08B17530;
    return;
L_08B17530:
    ctx.set_gpr(4, ctx.gpr[20] + 0u);
    ctx.set_gpr(31, 0x08B1753Cu);
    ctx.set_gpr(5, 4u << 16u);
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1753Cu) goto L_08B1753C;
    return;
L_08B1753C:
    ctx.set_gpr(19, ctx.gpr[19] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(7, ctx.gpr[2] + 0u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(5, 0u + 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(6, ctx.gpr[18] + 0u);
      if (branch_taken) {
          goto L_08B174FC;
      }
      goto L_08B17554;
    }
L_08B17554:
    ctx.set_gpr(31, 0x08B1755Cu);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.pc = 0x08B17354u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1755Cu) goto L_08B1755C;
    return;
L_08B1755C:
    ctx.set_gpr(16, 0u + 0u);
    goto L_08B174B0;
L_08B17564:
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(21464)));
    { const bool branch_taken = ctx.gpr[4] == 0u;
    ctx.set_gpr(22, 2235u << 16u);
      if (branch_taken) {
          goto L_08B17590;
      }
      goto L_08B17574;
    }
L_08B17574:
    ctx.set_gpr(31, 0x08B1757Cu);
    ctx.set_gpr(5, 0u + 0u);
    ctx.pc = 0x08B17054u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1757Cu) goto L_08B1757C;
    return;
L_08B1757C:
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    goto L_08B17580;
L_08B17580:
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(2));
      if (branch_taken) {
          goto L_08B174B0;
      }
      goto L_08B17588;
    }
L_08B17588:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(24), ctx.gpr[2]);
    goto L_08B17454;
L_08B17590:
    ctx.set_gpr(4, ctx.gpr[22] + static_cast<std::uint32_t>(-27492));
    ctx.set_gpr(31, 0x08B1759Cu);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(63));
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1759Cu) goto L_08B1759C;
    return;
L_08B1759C:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(36));
    ctx.set_gpr(31, 0x08B175A8u);
    ctx.set_gpr(5, 4u << 16u);
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B175A8u) goto L_08B175A8;
    return;
L_08B175A8:
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    goto L_08B17580;
L_08B175C0:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(18, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, 2235u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[21]);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27492));
    ctx.set_gpr(21, ctx.gpr[9] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    ctx.set_gpr(20, ctx.gpr[8] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(19, ctx.gpr[7] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[5] + 0u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(339));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[6] + 0u);
    { const bool branch_taken = ctx.gpr[18] == 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[31]);
      if (branch_taken) {
          goto L_08B17684;
      }
      goto L_08B17608;
    }
L_08B17608:
    ctx.set_gpr(4, 2235u << 16u);
    goto L_08B1760C;
L_08B1760C:
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27492));
    { const bool branch_taken = ctx.gpr[17] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(340));
      if (branch_taken) {
          goto L_08B17674;
      }
      goto L_08B17618;
    }
L_08B17618:
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(-1));
    goto L_08B1761C;
L_08B1761C:
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(2, ctx.gpr[16] & ctx.gpr[2]);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27492));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(341));
      if (branch_taken) {
          goto L_08B17638;
      }
      goto L_08B17630;
    }
L_08B17630:
    ctx.set_gpr(31, 0x08B17638u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B17638u) goto L_08B17638;
    return;
L_08B17638:
    ctx.set_gpr(4, ctx.gpr[18] + 0u);
    ctx.set_gpr(5, ctx.gpr[17] + 0u);
    ctx.set_gpr(6, ctx.gpr[16] + 0u);
    ctx.set_gpr(7, ctx.gpr[19] + 0u);
    ctx.set_gpr(8, ctx.gpr[20] + 0u);
    ctx.set_gpr(9, ctx.gpr[21] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    goto L_08B17400;
L_08B17674:
    ctx.set_gpr(31, 0x08B1767Cu);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1767Cu) goto L_08B1767C;
    return;
L_08B1767C:
    ctx.set_gpr(2, ctx.gpr[16] + static_cast<std::uint32_t>(-1));
    goto L_08B1761C;
L_08B17684:
    ctx.set_gpr(31, 0x08B1768Cu);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1768Cu) goto L_08B1768C;
    return;
L_08B1768C:
    ctx.set_gpr(4, 2235u << 16u);
    goto L_08B1760C;
}

void sub_08B175C0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B175C0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B156F8[57] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 5, 0, 6, 0, 0,
    7, 0, 0, 0, 0, 0, 8, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 11,
};
void sub_08B156F8_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B15678u;
        entry_id = (entry_delta < 228u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B156F8[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B15678;
    case 2u: goto L_08B156A4;
    case 3u: goto L_08B156C4;
    case 4u: goto L_08B156DC;
    case 5u: goto L_08B156E4;
    case 6u: goto L_08B156EC;
    case 7u: goto L_08B156F8;
    case 8u: goto L_08B15710;
    case 9u: goto L_08B1571C;
    case 10u: goto L_08B15740;
    case 11u: goto L_08B15758;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B15678:
    ctx.set_gpr(3, 2284u << 16u);
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(-7744)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(2, 16838u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(7, ctx.gpr[2] | 20077u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(31));
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(0), ctx.gpr[4]);
    ctx.set_gpr(4, ctx.gpr[8] + 0u);
    goto L_08B156A4;
L_08B156A4:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(1));
    { const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[2])) * static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[7])); ctx.lo = static_cast<std::uint32_t>(product); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(product) >> 32u); }
    ctx.set_gpr(2, ctx.lo);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(12345));
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    { const bool branch_taken = ctx.gpr[5] != ctx.gpr[6];
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B156A4;
      }
      goto L_08B156C4;
    }
L_08B156C4:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(-7744)));
    ctx.set_gpr(4, ctx.gpr[8] + static_cast<std::uint32_t>(12));
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-7744));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    ctx.set_gpr(16, 0u + static_cast<std::uint32_t>(310));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
    goto L_08B156DC;
L_08B156DC:
    ctx.set_gpr(31, 0x08B156E4u);
    ctx.set_gpr(16, ctx.gpr[16] + static_cast<std::uint32_t>(-1));
    ctx.pc = 0x08B1560Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B156E4u) goto L_08B156E4;
    return;
L_08B156E4:
    { const bool branch_taken = ctx.gpr[16] != 0u;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
      if (branch_taken) {
          goto L_08B156DC;
      }
      goto L_08B156EC;
    }
L_08B156EC:
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B156F8:
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27564));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B15710u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(40));
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B15710u) goto L_08B15710;
    return;
L_08B15710:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(124));
    ctx.set_gpr(31, 0x08B1571Cu);
    ctx.set_gpr(5, 4u << 16u);
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1571Cu) goto L_08B1571C;
    return;
L_08B1571C:
    ctx.set_gpr(3, ctx.gpr[2] + 0u);
    ctx.set_gpr(7, ctx.gpr[2] + static_cast<std::uint32_t>(124));
    ctx.set_gpr(4, 39473u << 16u);
    ctx.set_gpr(6, ctx.gpr[2] + static_cast<std::uint32_t>(12));
    ctx.set_gpr(2, 2284u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] | 36921u);
    ctx.set_gpr(5, ctx.gpr[2] + static_cast<std::uint32_t>(-7744));
    { const bool branch_taken = ctx.gpr[3] == 0u;
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(-7744), ctx.gpr[3]);
      if (branch_taken) {
          goto L_08B15758;
      }
      goto L_08B15740;
    }
L_08B15740:
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(12), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(4), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(8), ctx.gpr[3]);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    goto L_08B15678;
L_08B15758:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B156F8(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B156F8_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B1560C[24] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 4, 0, 5,
};
void sub_08B1560C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B1560Cu;
        entry_id = (entry_delta < 96u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B1560C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B1560C;
    case 2u: goto L_08B1564C;
    case 3u: goto L_08B15658;
    case 4u: goto L_08B15660;
    case 5u: goto L_08B15668;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B1560C:
    ctx.set_gpr(11, 2284u << 16u);
    ctx.set_gpr(8, ctx.gpr[11] + static_cast<std::uint32_t>(-7744));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(7, ctx.gpr[5] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(9, ctx.gpr[4] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(2, ctx.gpr[2] + ctx.gpr[6]);
    ctx.set_gpr(10, ctx.gpr[9] < ctx.gpr[3] ? 1u : 0u);
    ctx.set_gpr(3, ctx.gpr[7] < ctx.gpr[3] ? 1u : 0u);
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(4), ctx.gpr[7]);
    ctx.set_gpr(6, ctx.gpr[2] >> 1u);
    { const bool branch_taken = ctx.gpr[3] != 0u;
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B15660;
      }
      goto L_08B1564C;
    }
L_08B1564C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[11] + static_cast<std::uint32_t>(-7744)));
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    goto L_08B15658;
L_08B15658:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(2, ctx.gpr[6] + 0u);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B15660:
    { const bool branch_taken = ctx.gpr[10] != 0u;
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
      if (branch_taken) {
          goto L_08B15658;
      }
      goto L_08B15668;
    }
L_08B15668:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[11] + static_cast<std::uint32_t>(-7744)));
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(2, ctx.gpr[6] + 0u);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B1560C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B1560C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08A9AD14[1] = {
    1,
};
void sub_08A9AD14_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08A9AD14u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08A9AD14[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08A9AD14;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08A9AD14:
    jump_target = ctx.gpr[31];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08A9AD14(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A9AD14_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08A9B888[142] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 5, 6, 0, 0, 0, 7, 0, 8, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 11, 0, 12, 0, 13, 0, 14, 0,
    15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 18, 0, 0, 0, 0, 0, 0, 0, 19,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 21,
};
void sub_08A9B888_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08A9B6D8u;
        entry_id = (entry_delta < 568u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08A9B888[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08A9B6D8;
    case 2u: goto L_08A9B718;
    case 3u: goto L_08A9B728;
    case 4u: goto L_08A9B72C;
    case 5u: goto L_08A9B768;
    case 6u: goto L_08A9B76C;
    case 7u: goto L_08A9B77C;
    case 8u: goto L_08A9B784;
    case 9u: goto L_08A9B78C;
    case 10u: goto L_08A9B7B0;
    case 11u: goto L_08A9B7B8;
    case 12u: goto L_08A9B7C0;
    case 13u: goto L_08A9B7C8;
    case 14u: goto L_08A9B7D0;
    case 15u: goto L_08A9B7D8;
    case 16u: goto L_08A9B888;
    case 17u: goto L_08A9B8A8;
    case 18u: goto L_08A9B8B4;
    case 19u: goto L_08A9B8D4;
    case 20u: goto L_08A9B8FC;
    case 21u: goto L_08A9B90C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08A9B6D8:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, 2234u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(31848));
    ctx.set_gpr(20, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1204));
    ctx.set_gpr(19, ctx.gpr[7] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(18, ctx.gpr[16] & 3u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[6] + 0u);
    { const bool branch_taken = ctx.gpr[16] == 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[31]);
      if (branch_taken) {
          goto L_08A9B7D0;
      }
      goto L_08A9B718;
    }
L_08A9B718:
    ctx.set_gpr(4, 2234u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(31848));
    { const bool branch_taken = ctx.gpr[18] != 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1205));
      if (branch_taken) {
          goto L_08A9B7C0;
      }
      goto L_08A9B728;
    }
L_08A9B728:
    ctx.set_gpr(6, ctx.gpr[17] & 65535u);
    goto L_08A9B72C;
L_08A9B72C:
    ctx.set_gpr(2, ctx.gpr[6] + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(24));
    ctx.set_gpr(4, ctx.gpr[4] & ctx.gpr[2]);
    ctx.set_gpr(3, ctx.gpr[6] - ctx.gpr[4]);
    ctx.set_gpr(2, ctx.gpr[2] & ctx.gpr[3]);
    ctx.set_gpr(2, ctx.gpr[2] + ctx.gpr[16]);
    ctx.set_gpr(17, ctx.gpr[2] + static_cast<std::uint32_t>(24));
    ctx.set_gpr(4, 2234u << 16u);
    ctx.set_gpr(2, ctx.gpr[17] & 3u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(31848));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1210));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(0), ctx.gpr[20]);
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(6), static_cast<std::uint16_t>(ctx.gpr[19]));
    { const bool branch_taken = ctx.gpr[2] != 0u;
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(4), static_cast<std::uint16_t>(ctx.gpr[6]));
      if (branch_taken) {
          goto L_08A9B7B0;
      }
      goto L_08A9B768;
    }
L_08A9B768:
    ctx.set_gpr(4, 2234u << 16u);
    goto L_08A9B76C;
L_08A9B76C:
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(31848));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1214));
    { const bool branch_taken = ctx.gpr[18] == 0u;
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(-4), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08A9B784;
      }
      goto L_08A9B77C;
    }
L_08A9B77C:
    ctx.set_gpr(31, 0x08A9B784u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B784u) goto L_08A9B784;
    return;
L_08A9B784:
    ctx.set_gpr(31, 0x08A9B78Cu);
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.pc = 0x08A9B50Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B78Cu) goto L_08A9B78C;
    return;
L_08A9B78C:
    ctx.set_gpr(2, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08A9B7B0:
    ctx.set_gpr(31, 0x08A9B7B8u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B7B8u) goto L_08A9B7B8;
    return;
L_08A9B7B8:
    ctx.set_gpr(4, 2234u << 16u);
    goto L_08A9B76C;
L_08A9B7C0:
    ctx.set_gpr(31, 0x08A9B7C8u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B7C8u) goto L_08A9B7C8;
    return;
L_08A9B7C8:
    ctx.set_gpr(6, ctx.gpr[17] & 65535u);
    goto L_08A9B72C;
L_08A9B7D0:
    ctx.set_gpr(31, 0x08A9B7D8u);
    ctx.set_gpr(18, 0u + 0u);
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B7D8u) goto L_08A9B7D8;
    return;
L_08A9B7D8:
    ctx.set_gpr(6, ctx.gpr[17] & 65535u);
    goto L_08A9B72C;
L_08A9B888:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(31, 0x08A9B8A8u);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    ctx.pc = 0x08A9B0A4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B8A8u) goto L_08A9B8A8;
    return;
L_08A9B8A8:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(31, 0x08A9B8B4u);
    ctx.set_gpr(18, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B73030u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B8B4u) goto L_08A9B8B4;
    return;
L_08A9B8B4:
    ctx.set_gpr(4, 2234u << 16u);
    ctx.set_gpr(6, 2234u << 16u);
    ctx.set_gpr(17, ctx.gpr[2] + 0u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(31848));
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(31880));
    ctx.set_gpr(8, ctx.gpr[16] + 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1637));
      if (branch_taken) {
          goto L_08A9B8FC;
      }
      goto L_08A9B8D4;
    }
L_08A9B8D4:
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(5, ctx.gpr[18] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(16));
    ctx.set_gpr(7, 0u + 0u);
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    goto L_08A9B6D8;
L_08A9B8FC:
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(4176)));
    ctx.set_gpr(31, 0x08A9B90Cu);
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(28)));
    ctx.pc = 0x08A9C704u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B90Cu) goto L_08A9B90C;
    return;
L_08A9B90C:
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(5, ctx.gpr[18] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(16));
    ctx.set_gpr(7, 0u + 0u);
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    goto L_08A9B6D8;
}

void sub_08A9B888(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A9B888_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08A9B0A4[1] = {
    1,
};
void sub_08A9B0A4_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08A9B0A4u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08A9B0A4[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08A9B0A4;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08A9B0A4:
    ctx.set_gpr(2, ctx.gpr[5] + ctx.gpr[4]);
    ctx.set_gpr(2, ctx.gpr[5] + ctx.gpr[2]);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(23));
    ctx.set_gpr(5, 0u - ctx.gpr[5]);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(2, ctx.gpr[2] & ctx.gpr[5]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08A9B0A4(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A9B0A4_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08A9C704[1] = {
    1,
};
void sub_08A9C704_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08A9C704u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08A9C704[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08A9C704;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08A9C704:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[8]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[10]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(28), ctx.gpr[11]);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08A9C704(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A9C704_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B60640[65] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4,
};
void sub_08B60640_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B60640u;
        entry_id = (entry_delta < 260u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B60640[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B60640;
    case 2u: goto L_08B60670;
    case 3u: goto L_08B60684;
    case 4u: goto L_08B60740;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B60640:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(6, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    ctx.set_gpr(3, 32768u << 16u);
    ctx.set_gpr(7, ctx.gpr[4] & 65535u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(16)));
    { const bool branch_taken = ctx.gpr[8] != 0u;
    ctx.set_gpr(5, ctx.gpr[3] | 33u);
      if (branch_taken) {
          goto L_08B60740;
      }
      goto L_08B60670;
    }
L_08B60670:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(2, 32768u << 16u);
    ctx.set_gpr(4, ctx.gpr[3] < static_cast<std::uint32_t>(5) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[4] == 0u;
    ctx.set_gpr(5, ctx.gpr[2] | 263u);
      if (branch_taken) {
          goto L_08B60740;
      }
      goto L_08B60684;
    }
L_08B60684:
    ctx.set_gpr(11, 2235u << 16u);
    ctx.set_gpr(9, ctx.gpr[3] << 2u);
    ctx.set_gpr(10, ctx.gpr[11] + static_cast<std::uint32_t>(17536));
    ctx.set_gpr(8, ctx.gpr[9] + ctx.gpr[10]);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[6];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B60740:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[5] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B60640(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B60640_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B606A0[47] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
    0, 0, 0, 0, 0, 6, 0, 7, 8, 0, 0, 0, 0, 0, 9,
};
void sub_08B606A0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B606A0u;
        entry_id = (entry_delta < 188u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B606A0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B606A0;
    case 2u: goto L_08B606D0;
    case 3u: goto L_08B606D8;
    case 4u: goto L_08B606F0;
    case 5u: goto L_08B6071C;
    case 6u: goto L_08B60734;
    case 7u: goto L_08B6073C;
    case 8u: goto L_08B60740;
    case 9u: goto L_08B60758;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B606A0:
    ctx.set_gpr(5, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(14, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(25, 3840u << 16u);
    ctx.set_gpr(24, ctx.gpr[7] | ctx.gpr[25]);
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[14] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(13, 3072u << 16u);
    ctx.set_gpr(12, ctx.gpr[17] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(15, ctx.gpr[12] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(0), ctx.gpr[24]);
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(8), ctx.gpr[15]);
    ctx.set_gpr(31, 0x08B606D0u);
    rt.memory().aot_store32(ctx.gpr[12] + static_cast<std::uint32_t>(0), ctx.gpr[13]);
    ctx.pc = 0x08B63C90u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B606D0u) goto L_08B606D0;
    return;
L_08B606D0:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(5, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B60740;
      }
      goto L_08B606D8;
    }
L_08B606D8:
    ctx.set_gpr(16, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(14, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[14] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[14] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(31, 0x08B606F0u);
    ctx.set_gpr(17, ctx.gpr[4] - ctx.gpr[13]);
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B606F0u) goto L_08B606F0;
    return;
L_08B606F0:
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(10, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(7, ctx.gpr[12] << 6u);
    ctx.set_gpr(11, ctx.gpr[7] - ctx.gpr[12]);
    ctx.set_gpr(3, ctx.gpr[11] << 2u);
    ctx.set_gpr(9, ctx.gpr[3] + ctx.gpr[16]);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(324)));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(324), ctx.gpr[10]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[5]) < 0;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(12), ctx.gpr[5]);
      if (branch_taken) {
          goto L_08B60758;
      }
      goto L_08B6071C;
    }
L_08B6071C:
    ctx.set_gpr(8, ctx.gpr[5] << 6u);
    ctx.set_gpr(6, ctx.gpr[8] - ctx.gpr[5]);
    ctx.set_gpr(24, ctx.gpr[6] << 2u);
    ctx.set_gpr(25, ctx.gpr[16] + static_cast<std::uint32_t>(76));
    ctx.set_gpr(15, ctx.gpr[24] + ctx.gpr[25]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(72), ctx.gpr[15]);
    goto L_08B60734;
L_08B60734:
    ctx.set_gpr(31, 0x08B6073Cu);
    // nop
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B6073Cu) goto L_08B6073C;
    return;
L_08B6073C:
    ctx.set_gpr(5, ctx.gpr[17] + 0u);
    goto L_08B60740;
L_08B60740:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[5] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B60758:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(72), 0u);
    goto L_08B60734;
}

void sub_08B606A0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B606A0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B60760[46] = {
    1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 5, 0, 0, 0, 0, 0, 0,
    6, 0, 7, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 9,
};
void sub_08B60760_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B606D8u;
        entry_id = (entry_delta < 184u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B60760[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B606D8;
    case 2u: goto L_08B606F0;
    case 3u: goto L_08B6071C;
    case 4u: goto L_08B60734;
    case 5u: goto L_08B6073C;
    case 6u: goto L_08B60758;
    case 7u: goto L_08B60760;
    case 8u: goto L_08B60774;
    case 9u: goto L_08B6078C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B606D8:
    ctx.set_gpr(16, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(14, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[14] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[14] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(31, 0x08B606F0u);
    ctx.set_gpr(17, ctx.gpr[4] - ctx.gpr[13]);
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B606F0u) goto L_08B606F0;
    return;
L_08B606F0:
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(10, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(7, ctx.gpr[12] << 6u);
    ctx.set_gpr(11, ctx.gpr[7] - ctx.gpr[12]);
    ctx.set_gpr(3, ctx.gpr[11] << 2u);
    ctx.set_gpr(9, ctx.gpr[3] + ctx.gpr[16]);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(324)));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(324), ctx.gpr[10]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[5]) < 0;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(12), ctx.gpr[5]);
      if (branch_taken) {
          goto L_08B60758;
      }
      goto L_08B6071C;
    }
L_08B6071C:
    ctx.set_gpr(8, ctx.gpr[5] << 6u);
    ctx.set_gpr(6, ctx.gpr[8] - ctx.gpr[5]);
    ctx.set_gpr(24, ctx.gpr[6] << 2u);
    ctx.set_gpr(25, ctx.gpr[16] + static_cast<std::uint32_t>(76));
    ctx.set_gpr(15, ctx.gpr[24] + ctx.gpr[25]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(72), ctx.gpr[15]);
    goto L_08B60734;
L_08B60734:
    ctx.set_gpr(31, 0x08B6073Cu);
    // nop
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B6073Cu) goto L_08B6073C;
    return;
L_08B6073C:
    ctx.set_gpr(5, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[5] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B60758:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(72), 0u);
    goto L_08B60734;
L_08B60760:
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
    { const bool branch_taken = ctx.gpr[12] == ctx.gpr[7];
    ctx.set_gpr(8, 2816u << 16u);
      if (branch_taken) {
          goto L_08B6078C;
      }
      goto L_08B60774;
    }
L_08B60774:
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(6, ctx.gpr[2] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(0), ctx.gpr[8]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(8), ctx.gpr[6]);
    goto L_08B606D8;
L_08B6078C:
    ctx.set_gpr(15, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(25, 3602u << 16u);
    ctx.set_gpr(14, 3072u << 16u);
    ctx.set_gpr(24, rt.memory().aot_load32(ctx.gpr[15] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(13, ctx.gpr[24] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(17, ctx.gpr[13] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[24] + static_cast<std::uint32_t>(0), ctx.gpr[25]);
    rt.memory().aot_store32(ctx.gpr[15] + static_cast<std::uint32_t>(8), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(0), ctx.gpr[14]);
    goto L_08B606D8;
}

void sub_08B60760(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B60760_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B607B4[56] = {
    1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 5, 0, 0, 0, 0, 0, 0,
    6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
};
void sub_08B607B4_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B606D8u;
        entry_id = (entry_delta < 224u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B607B4[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B606D8;
    case 2u: goto L_08B606F0;
    case 3u: goto L_08B6071C;
    case 4u: goto L_08B60734;
    case 5u: goto L_08B6073C;
    case 6u: goto L_08B60758;
    case 7u: goto L_08B607B4;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B606D8:
    ctx.set_gpr(16, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(14, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[14] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(13, rt.memory().aot_load32(ctx.gpr[14] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(31, 0x08B606F0u);
    ctx.set_gpr(17, ctx.gpr[4] - ctx.gpr[13]);
    ctx.pc = 0x08B7FB6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B606F0u) goto L_08B606F0;
    return;
L_08B606F0:
    ctx.set_gpr(12, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(10, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(7, ctx.gpr[12] << 6u);
    ctx.set_gpr(11, ctx.gpr[7] - ctx.gpr[12]);
    ctx.set_gpr(3, ctx.gpr[11] << 2u);
    ctx.set_gpr(9, ctx.gpr[3] + ctx.gpr[16]);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(324)));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(324), ctx.gpr[10]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[5]) < 0;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(12), ctx.gpr[5]);
      if (branch_taken) {
          goto L_08B60758;
      }
      goto L_08B6071C;
    }
L_08B6071C:
    ctx.set_gpr(8, ctx.gpr[5] << 6u);
    ctx.set_gpr(6, ctx.gpr[8] - ctx.gpr[5]);
    ctx.set_gpr(24, ctx.gpr[6] << 2u);
    ctx.set_gpr(25, ctx.gpr[16] + static_cast<std::uint32_t>(76));
    ctx.set_gpr(15, ctx.gpr[24] + ctx.gpr[25]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(72), ctx.gpr[15]);
    goto L_08B60734;
L_08B60734:
    ctx.set_gpr(31, 0x08B6073Cu);
    // nop
    ctx.pc = 0x08B7FB9Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B6073Cu) goto L_08B6073C;
    return;
L_08B6073C:
    ctx.set_gpr(5, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[5] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B60758:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(72), 0u);
    goto L_08B60734;
L_08B607B4:
    ctx.set_gpr(11, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[11] + static_cast<std::uint32_t>(72)));
    ctx.set_gpr(10, 3840u << 16u);
    ctx.set_gpr(3, ctx.gpr[7] | ctx.gpr[10]);
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, 3072u << 16u);
    ctx.set_gpr(6, ctx.gpr[9] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(8, ctx.gpr[6] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(0), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(8), ctx.gpr[8]);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
    goto L_08B606D8;
}

void sub_08B607B4(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B607B4_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08A9B50C[37] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 4, 0, 0,
    0, 0, 0, 0, 5,
};
void sub_08A9B50C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08A9B50Cu;
        entry_id = (entry_delta < 148u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08A9B50C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08A9B50C;
    case 2u: goto L_08A9B550;
    case 3u: goto L_08A9B574;
    case 4u: goto L_08A9B580;
    case 5u: goto L_08A9B59C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08A9B50C:
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(4176)));
    ctx.set_gpr(8, ctx.gpr[4] + 0u);
    ctx.set_gpr(9, ctx.gpr[4] + static_cast<std::uint32_t>(12));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(10, rt.memory().aot_load16(ctx.gpr[5] + static_cast<std::uint32_t>(52)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(28)));
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(24), ctx.gpr[2]);
    ctx.set_gpr(3, ctx.gpr[10] & 255u);
    ctx.set_gpr(2, ctx.gpr[3] << 2u);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(3, static_cast<std::int32_t>(ctx.gpr[3]) < 8 ? 1u : 0u);
    ctx.set_gpr(7, ctx.gpr[2] + ctx.gpr[5]);
    ctx.set_gpr(4, ctx.gpr[4] + ctx.gpr[6]);
    { const bool branch_taken = ctx.gpr[3] == 0u;
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(28), ctx.gpr[4]);
      if (branch_taken) {
          goto L_08A9B574;
      }
      goto L_08A9B550;
    }
L_08A9B550:
    ctx.set_gpr(2, rt.memory().aot_load16(ctx.gpr[8] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(56)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(88)));
    ctx.set_gpr(2, ctx.gpr[6] - ctx.gpr[2]);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(-24));
    ctx.set_gpr(3, ctx.gpr[3] + ctx.gpr[2]);
    ctx.set_gpr(4, ctx.gpr[6] + ctx.gpr[4]);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(88), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(56), ctx.gpr[4]);
    goto L_08A9B574;
L_08A9B574:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(0)));
    if (ctx.gpr[2] == 0u) {
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(0), ctx.gpr[9]);
        goto L_08A9B59C;
    }
    goto L_08A9B580;
L_08A9B580:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(4)));
    rt.memory().aot_store16(ctx.gpr[8] + static_cast<std::uint32_t>(8), static_cast<std::uint16_t>(ctx.gpr[10]));
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(12), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(4), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(4), ctx.gpr[9]);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(4), 0u);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08A9B59C:
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(4), ctx.gpr[9]);
    rt.memory().aot_store16(ctx.gpr[8] + static_cast<std::uint32_t>(8), static_cast<std::uint16_t>(ctx.gpr[10]));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(4), 0u);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(12), 0u);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08A9B50C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A9B50C_entry(rt, ctx, 0u, aot_mem);
}

void sub_08A9B5B0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    (void)direct_entry_id;
    std::uint32_t jump_target = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t local_transfers = 0u;
    std::uint32_t entry_id = 0u;
LOCAL_DISPATCH:
    switch (local_pc) {
    case 0x08A9B5B0u: goto L_08A9B5B0;
    case 0x08A9B5C8u: goto L_08A9B5C8;
    case 0x08A9B5E0u: goto L_08A9B5E0;
    case 0x08B7305Cu: goto L_08B7305C;
    case 0x08B73070u: goto L_08B73070;
    case 0x08B7B5D0u: goto L_08B7B5D0;
    case 0x08B7B5ECu: goto L_08B7B5EC;
    case 0x08B7B5F4u: goto L_08B7B5F4;
    case 0x08B7B620u: goto L_08B7B620;
    case 0x08B7B630u: goto L_08B7B630;
    case 0x08B7B64Cu: goto L_08B7B64C;
    case 0x08B7B65Cu: goto L_08B7B65C;
    case 0x08B7B670u: goto L_08B7B670;
    case 0x08B7B684u: goto L_08B7B684;
    case 0x08B7B688u: goto L_08B7B688;
    case 0x08B7B69Cu: goto L_08B7B69C;
    case 0x08B7B6A0u: goto L_08B7B6A0;
    case 0x08B7B6A8u: goto L_08B7B6A8;
    case 0x08B7B6B8u: goto L_08B7B6B8;
    case 0x08B7B6C4u: goto L_08B7B6C4;
    case 0x08B7B6D4u: goto L_08B7B6D4;
    case 0x08B7B6E0u: goto L_08B7B6E0;
    case 0x08B7B6ECu: goto L_08B7B6EC;
    case 0x08B7B700u: goto L_08B7B700;
    case 0x08B7B704u: goto L_08B7B704;
    case 0x08B7B714u: goto L_08B7B714;
    case 0x08B7B720u: goto L_08B7B720;
    case 0x08B7B724u: goto L_08B7B724;
    case 0x08B7B734u: goto L_08B7B734;
    case 0x08B7B74Cu: goto L_08B7B74C;
    case 0x08B7B754u: goto L_08B7B754;
    case 0x08B7B75Cu: goto L_08B7B75C;
    case 0x08B7B778u: goto L_08B7B778;
    case 0x08B7B780u: goto L_08B7B780;
    case 0x08B7B794u: goto L_08B7B794;
    case 0x08B7B7A8u: goto L_08B7B7A8;
    case 0x08B7B7C0u: goto L_08B7B7C0;
    case 0x08B7B810u: goto L_08B7B810;
    case 0x08B7B81Cu: goto L_08B7B81C;
    case 0x08B7B838u: goto L_08B7B838;
    case 0x08B7B854u: goto L_08B7B854;
    case 0x08B7B864u: goto L_08B7B864;
    case 0x08B7B86Cu: goto L_08B7B86C;
    case 0x08B7B878u: goto L_08B7B878;
    case 0x08B7B8ACu: goto L_08B7B8AC;
    case 0x08B7B8B4u: goto L_08B7B8B4;
    case 0x08B7B8C4u: goto L_08B7B8C4;
    case 0x08B7B8D0u: goto L_08B7B8D0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
L_08A9B5B0:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(31, 0x08A9B5C8u);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    ctx.pc = 0x08A9B3F4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B5C8u) goto L_08A9B5C8;
    return;
L_08A9B5C8:
    ctx.set_gpr(17, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(-4)));
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(204));
    ctx.set_gpr(31, 0x08A9B5E0u);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(0)));
    ctx.pc = 0x08B73CF0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B5E0u) goto L_08A9B5E0;
    return;
L_08A9B5E0:
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    goto L_08B7305C;
L_08B7305C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(31, 0x08B73070u);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    ctx.pc = 0x08B7300Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B73070u) goto L_08B73070;
    return;
L_08B73070:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(5, ctx.gpr[16] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    goto L_08B7B5D0;
L_08B7B5D0:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[4] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, ctx.gpr[5] + 0u);
    { const bool branch_taken = ctx.gpr[5] == 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[31]);
      if (branch_taken) {
          goto L_08B7B780;
      }
      goto L_08B7B5EC;
    }
L_08B7B5EC:
    ctx.set_gpr(31, 0x08B7B5F4u);
    // nop
    ctx.pc = 0x08B73DC8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7B5F4u) goto L_08B7B5F4;
    return;
L_08B7B5F4:
    ctx.set_gpr(9, ctx.gpr[16] + static_cast<std::uint32_t>(-8));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(11, 2244u << 16u);
    ctx.set_gpr(10, ctx.gpr[11] + static_cast<std::uint32_t>(-22984));
    ctx.set_gpr(8, ctx.gpr[3] + 0u);
    ctx.set_gpr(8, (ctx.gpr[8] & ~0x00000001u) | ((0u & 0x00000001u) << 0u));
    ctx.set_gpr(6, ctx.gpr[9] + ctx.gpr[8]);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[10] + static_cast<std::uint32_t>(8)));
    { const bool branch_taken = ctx.gpr[6] == ctx.gpr[2];
    ctx.set_gpr(4, (ctx.gpr[4] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
      if (branch_taken) {
          goto L_08B7B810;
      }
      goto L_08B7B620;
    }
L_08B7B620:
    ctx.set_gpr(2, ctx.gpr[3] & 1u);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(5, 0u + 0u);
      if (branch_taken) {
          goto L_08B7B65C;
      }
      goto L_08B7B630;
    }
L_08B7B630:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(-8)));
    ctx.set_gpr(3, ctx.gpr[10] + static_cast<std::uint32_t>(8));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(9, ctx.gpr[9] - ctx.gpr[2]);
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(8)));
    { const bool branch_taken = ctx.gpr[7] == ctx.gpr[3];
    ctx.set_gpr(8, ctx.gpr[8] + ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B7B65C;
      }
      goto L_08B7B64C;
    }
L_08B7B64C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(5, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(8), ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(12), ctx.gpr[2]);
    goto L_08B7B65C;
L_08B7B65C:
    ctx.set_gpr(3, ctx.gpr[6] + ctx.gpr[4]);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(2, ctx.gpr[2] & 1u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    // nop
      if (branch_taken) {
          goto L_08B7B74C;
      }
      goto L_08B7B670;
    }
L_08B7B670:
    ctx.set_gpr(2, ctx.gpr[8] | 1u);
    ctx.set_gpr(3, ctx.gpr[9] + ctx.gpr[8]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    { const bool branch_taken = ctx.gpr[5] == 0u;
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(0), ctx.gpr[8]);
      if (branch_taken) {
          goto L_08B7B69C;
      }
      goto L_08B7B684;
    }
L_08B7B684:
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    goto L_08B7B688;
L_08B7B688:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    ctx.pc = 0x08B73E04u; (void)rt.invoke_chained_call(ctx, &aot_mem); return;
L_08B7B69C:
    ctx.set_gpr(2, ctx.gpr[8] < static_cast<std::uint32_t>(512) ? 1u : 0u);
    goto L_08B7B6A0;
L_08B7B6A0:
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(4, ctx.gpr[8] >> 3u);
      if (branch_taken) {
          goto L_08B7B7C0;
      }
      goto L_08B7B6A8;
    }
L_08B7B6A8:
    ctx.set_gpr(7, ctx.gpr[8] >> 3u);
    ctx.set_gpr(3, ctx.gpr[8] >> 9u);
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(4, ctx.gpr[7] << 3u);
      if (branch_taken) {
          goto L_08B7B6EC;
      }
      goto L_08B7B6B8;
    }
L_08B7B6B8:
    ctx.set_gpr(2, ctx.gpr[3] < static_cast<std::uint32_t>(5) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(2, ctx.gpr[8] >> 6u);
      if (branch_taken) {
          goto L_08B7B86C;
      }
      goto L_08B7B6C4;
    }
L_08B7B6C4:
    ctx.set_gpr(7, ctx.gpr[3] + static_cast<std::uint32_t>(91));
    ctx.set_gpr(2, ctx.gpr[3] < static_cast<std::uint32_t>(21) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(4, ctx.gpr[7] << 3u);
      if (branch_taken) {
          goto L_08B7B6EC;
      }
      goto L_08B7B6D4;
    }
L_08B7B6D4:
    ctx.set_gpr(2, ctx.gpr[3] < static_cast<std::uint32_t>(85) ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(2, ctx.gpr[3] < static_cast<std::uint32_t>(341) ? 1u : 0u);
      if (branch_taken) {
          goto L_08B7B8AC;
      }
      goto L_08B7B6E0;
    }
L_08B7B6E0:
    ctx.set_gpr(2, ctx.gpr[8] >> 12u);
    ctx.set_gpr(7, ctx.gpr[2] + static_cast<std::uint32_t>(110));
    ctx.set_gpr(4, ctx.gpr[7] << 3u);
    goto L_08B7B6EC;
L_08B7B6EC:
    ctx.set_gpr(2, ctx.gpr[11] + static_cast<std::uint32_t>(-22984));
    ctx.set_gpr(6, ctx.gpr[4] + ctx.gpr[2]);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(8)));
    { const bool branch_taken = ctx.gpr[6] == ctx.gpr[5];
    ctx.set_gpr(4, ctx.gpr[7] + static_cast<std::uint32_t>(3));
      if (branch_taken) {
          goto L_08B7B878;
      }
      goto L_08B7B700;
    }
L_08B7B700:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(4)));
    goto L_08B7B704;
L_08B7B704:
    ctx.set_gpr(2, (ctx.gpr[2] & ~0x00000003u) | ((0u & 0x00000003u) << 0u));
    ctx.set_gpr(2, ctx.gpr[8] < ctx.gpr[2] ? 1u : 0u);
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(12)));
        goto L_08B7B724;
    }
    goto L_08B7B714;
L_08B7B714:
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(8)));
    if (ctx.gpr[6] != ctx.gpr[5]) {
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(4)));
        goto L_08B7B704;
    }
    goto L_08B7B720;
L_08B7B720:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(12)));
    goto L_08B7B724;
L_08B7B724:
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(8), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(12), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(12), ctx.gpr[9]);
    goto L_08B7B734;
L_08B7B734:
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    ctx.pc = 0x08B73E04u; (void)rt.invoke_chained_call(ctx, &aot_mem); return;
L_08B7B74C:
    { const bool branch_taken = ctx.gpr[5] == 0u;
    ctx.set_gpr(8, ctx.gpr[8] + ctx.gpr[4]);
      if (branch_taken) {
          goto L_08B7B794;
      }
      goto L_08B7B754;
    }
L_08B7B754:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(12)));
    goto L_08B7B75C;
L_08B7B75C:
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(8), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(12), ctx.gpr[2]);
    ctx.set_gpr(2, ctx.gpr[8] | 1u);
    ctx.set_gpr(3, ctx.gpr[9] + ctx.gpr[8]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    { const bool branch_taken = ctx.gpr[5] != 0u;
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(0), ctx.gpr[8]);
      if (branch_taken) {
          goto L_08B7B684;
      }
      goto L_08B7B778;
    }
L_08B7B778:
    ctx.set_gpr(2, ctx.gpr[8] < static_cast<std::uint32_t>(512) ? 1u : 0u);
    goto L_08B7B6A0;
L_08B7B780:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B7B794:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, ctx.gpr[11] + static_cast<std::uint32_t>(-22984));
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(8));
    if (ctx.gpr[3] != ctx.gpr[2]) {
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(12)));
        goto L_08B7B75C;
    }
    goto L_08B7B7A8;
L_08B7B7A8:
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(12), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(12), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(8), ctx.gpr[3]);
    goto L_08B7B670;
L_08B7B7C0:
    ctx.set_gpr(2, ctx.gpr[11] + static_cast<std::uint32_t>(-22984));
    ctx.set_gpr(5, ctx.gpr[4] << 3u);
    ctx.set_gpr(5, ctx.gpr[5] + ctx.gpr[2]);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[10] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(4, ctx.gpr[4] >> 2u);
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, ctx.gpr[2] << (ctx.gpr[4] & 31u));
    ctx.set_gpr(3, ctx.gpr[3] | ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(12), ctx.gpr[5]);
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(8), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(12), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    ctx.pc = 0x08B73E04u; (void)rt.invoke_chained_call(ctx, &aot_mem); return;
L_08B7B810:
    ctx.set_gpr(2, ctx.gpr[3] & 1u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(6, ctx.gpr[8] + ctx.gpr[4]);
      if (branch_taken) {
          goto L_08B7B838;
      }
      goto L_08B7B81C;
    }
L_08B7B81C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(-8)));
    ctx.set_gpr(9, ctx.gpr[9] - ctx.gpr[2]);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[9] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(6, ctx.gpr[6] + ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(8), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(12), ctx.gpr[4]);
    goto L_08B7B838;
L_08B7B838:
    ctx.set_gpr(2, 2244u << 16u);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(-22988)));
    ctx.set_gpr(4, ctx.gpr[6] | 1u);
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    ctx.set_gpr(3, ctx.gpr[6] < ctx.gpr[3] ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[3] != 0u;
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
      if (branch_taken) {
          goto L_08B7B684;
      }
      goto L_08B7B854;
    }
L_08B7B854:
    ctx.set_gpr(2, 2273u << 16u);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(3852)));
    ctx.set_gpr(31, 0x08B7B864u);
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.pc = 0x08B7B4ACu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B7B864u) goto L_08B7B864;
    return;
L_08B7B864:
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    goto L_08B7B688;
L_08B7B86C:
    ctx.set_gpr(7, ctx.gpr[2] + static_cast<std::uint32_t>(56));
    ctx.set_gpr(4, ctx.gpr[7] << 3u);
    goto L_08B7B6EC;
L_08B7B878:
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[7]) < 0 ? 1u : 0u);
    if (ctx.gpr[2] != 0u) ctx.set_gpr(7, ctx.gpr[4]);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[10] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(4, static_cast<std::uint32_t>(static_cast<std::int32_t>(ctx.gpr[7]) >> 2u));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, ctx.gpr[2] << (ctx.gpr[4] & 31u));
    ctx.set_gpr(3, ctx.gpr[3] | ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(12), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(8), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(12), ctx.gpr[9]);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    goto L_08B7B734;
L_08B7B8AC:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(2, ctx.gpr[3] < static_cast<std::uint32_t>(1365) ? 1u : 0u);
      if (branch_taken) {
          goto L_08B7B8C4;
      }
      goto L_08B7B8B4;
    }
L_08B7B8B4:
    ctx.set_gpr(2, ctx.gpr[8] >> 15u);
    ctx.set_gpr(7, ctx.gpr[2] + static_cast<std::uint32_t>(119));
    ctx.set_gpr(4, ctx.gpr[7] << 3u);
    goto L_08B7B6EC;
L_08B7B8C4:
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(126));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1008));
      if (branch_taken) {
          goto L_08B7B6EC;
      }
      goto L_08B7B8D0;
    }
L_08B7B8D0:
    ctx.set_gpr(2, ctx.gpr[8] >> 18u);
    ctx.set_gpr(7, ctx.gpr[2] + static_cast<std::uint32_t>(124));
    ctx.set_gpr(4, ctx.gpr[7] << 3u);
    goto L_08B7B6EC;
}

void sub_08A9B5B0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A9B5B0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08A9C6F0[1] = {
    1,
};
void sub_08A9C6F0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08A9C6F0u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08A9C6F0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08A9C6F0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08A9C6F0:
    jump_target = ctx.gpr[31];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08A9C6F0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A9C6F0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08A99F60[11] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
};
void sub_08A99F60_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08A99F60u;
        entry_id = (entry_delta < 44u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08A99F60[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08A99F60;
    case 2u: goto L_08A99F88;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08A99F60:
    ctx.set_gpr(8, 2261u << 16u);
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(5, ctx.gpr[4] + 0u);
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(-16212));
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(84));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08A99F88u);
    ctx.set_gpr(9, 0u + 0u);
    ctx.pc = 0x08B175C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A99F88u) goto L_08A99F88;
    return;
L_08A99F88:
    ctx.set_gpr(3, 2261u << 16u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(-16280), ctx.gpr[2]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08A99F60(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A99F60_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08A9F8F8[17] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 3,
};
void sub_08A9F8F8_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08A9F8F8u;
        entry_id = (entry_delta < 68u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08A9F8F8[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08A9F8F8;
    case 2u: goto L_08A9F920;
    case 3u: goto L_08A9F938;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08A9F8F8:
    ctx.set_gpr(5, ctx.gpr[4] + ctx.gpr[5]);
    ctx.set_gpr(2, 2281u << 16u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(24556), ctx.gpr[5]);
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(500));
    ctx.set_gpr(2, 2281u << 16u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(24560), ctx.gpr[3]);
    ctx.set_gpr(2, 2281u << 16u);
    ctx.set_gpr(5, ctx.gpr[4] + 0u);
    ctx.set_gpr(6, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(24564), ctx.gpr[4]);
    goto L_08A9F920;
L_08A9F920:
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, ctx.gpr[6] < ctx.gpr[3] ? 1u : 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(0), 0u);
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(4), 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(8));
      if (branch_taken) {
          goto L_08A9F920;
      }
      goto L_08A9F938;
    }
L_08A9F938:
    ctx.set_gpr(2, 2261u << 16u);
    ctx.set_gpr(3, 2261u << 16u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(-16160), 0u);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(-16148), 0u);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08A9F8F8(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A9F8F8_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08AB2A6C[79] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 4, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0,
    0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 8, 9, 0, 0, 0, 0, 10, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 13,
};
void sub_08AB2A6C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08AB2A6Cu;
        entry_id = (entry_delta < 316u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08AB2A6C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08AB2A6C;
    case 2u: goto L_08AB2AA0;
    case 3u: goto L_08AB2AA8;
    case 4u: goto L_08AB2AB8;
    case 5u: goto L_08AB2AC0;
    case 6u: goto L_08AB2AE4;
    case 7u: goto L_08AB2AF4;
    case 8u: goto L_08AB2B14;
    case 9u: goto L_08AB2B18;
    case 10u: goto L_08AB2B2C;
    case 11u: goto L_08AB2B34;
    case 12u: goto L_08AB2B9C;
    case 13u: goto L_08AB2BA4;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08AB2A6C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-48));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(32), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(28), ctx.gpr[23]);
    ctx.set_gpr(23, 2262u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[22]);
    ctx.set_gpr(22, 2235u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[21]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(31, 0x08AB2AA0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.pc = 0x08B7FE74u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB2AA0u) goto L_08AB2AA0;
    return;
L_08AB2AA0:
    ctx.set_gpr(31, 0x08AB2AA8u);
    ctx.set_gpr(18, ctx.gpr[2] + 0u);
    ctx.pc = 0x08B7FEB4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB2AA8u) goto L_08AB2AA8;
    return;
L_08AB2AA8:
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[23] + static_cast<std::uint32_t>(11188)));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(108));
      if (branch_taken) {
          goto L_08AB2AC0;
      }
      goto L_08AB2AB8;
    }
L_08AB2AB8:
    ctx.set_gpr(31, 0x08AB2AC0u);
    ctx.set_gpr(4, ctx.gpr[22] + static_cast<std::uint32_t>(-32152));
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB2AC0u) goto L_08AB2AC0;
    return;
L_08AB2AC0:
    ctx.set_gpr(2, 21u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] | 16384u);
    ctx.set_gpr(20, ctx.gpr[16] + ctx.gpr[2]);
    ctx.set_gpr(2, 65514u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] | 49152u);
    ctx.set_gpr(21, ctx.gpr[18] + ctx.gpr[2]);
    ctx.set_gpr(4, ctx.gpr[22] + static_cast<std::uint32_t>(-32152));
    ctx.set_gpr(31, 0x08AB2AE4u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(74));
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB2AE4u) goto L_08AB2AE4;
    return;
L_08AB2AE4:
    ctx.set_gpr(19, ctx.gpr[21] + ctx.gpr[20]);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(32));
    ctx.set_gpr(31, 0x08AB2AF4u);
    ctx.set_gpr(5, 4u << 16u);
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB2AF4u) goto L_08AB2AF4;
    return;
L_08AB2AF4:
    ctx.set_gpr(17, ctx.gpr[20] + static_cast<std::uint32_t>(7));
    ctx.set_gpr(19, (ctx.gpr[19] & ~0x00000007u) | ((0u & 0x00000007u) << 0u));
    ctx.set_gpr(17, (ctx.gpr[17] & ~0x00000007u) | ((0u & 0x00000007u) << 0u));
    ctx.set_gpr(18, ctx.gpr[19] + static_cast<std::uint32_t>(-32));
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
    ctx.set_gpr(4, ctx.gpr[22] + static_cast<std::uint32_t>(-32152));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(75));
      if (branch_taken) {
          goto L_08AB2B9C;
      }
      goto L_08AB2B14;
    }
L_08AB2B14:
    ctx.set_gpr(2, ctx.gpr[17] + static_cast<std::uint32_t>(32));
    goto L_08AB2B18;
L_08AB2B18:
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] < ctx.gpr[18] ? 1u : 0u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-32152));
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(79));
      if (branch_taken) {
          goto L_08AB2B34;
      }
      goto L_08AB2B2C;
    }
L_08AB2B2C:
    ctx.set_gpr(31, 0x08AB2B34u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB2B34u) goto L_08AB2B34;
    return;
L_08AB2B34:
    ctx.set_gpr(2, ctx.gpr[18] - ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[18] + static_cast<std::uint32_t>(4), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(16), ctx.gpr[21]);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[17] + static_cast<std::uint32_t>(4)));
    rt.memory().aot_store32(ctx.gpr[17] + static_cast<std::uint32_t>(0), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(12), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(-32), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(20), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(0), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(24), 0u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(28), 0u);
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(11188), ctx.gpr[16]);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    ctx.set_gpr(23, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(28)));
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(48));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08AB2B9C:
    ctx.set_gpr(31, 0x08AB2BA4u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB2BA4u) goto L_08AB2BA4;
    return;
L_08AB2BA4:
    ctx.set_gpr(2, ctx.gpr[17] + static_cast<std::uint32_t>(32));
    goto L_08AB2B18;
}

void sub_08AB2A6C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08AB2A6C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08AB1190[23] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3,
};
void sub_08AB1190_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08AB1190u;
        entry_id = (entry_delta < 92u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08AB1190[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08AB1190;
    case 2u: goto L_08AB11C0;
    case 3u: goto L_08AB11E8;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08AB1190:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(8, 2262u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(11112));
    ctx.set_gpr(16, ctx.gpr[5] + 0u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(5, ctx.gpr[4] + 0u);
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(48));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08AB11C0u);
    ctx.set_gpr(9, 0u + static_cast<std::uint32_t>(4098));
    ctx.pc = 0x08B175C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB11C0u) goto L_08AB11C0;
    return;
L_08AB11C0:
    ctx.set_gpr(8, 2262u << 16u);
    ctx.set_gpr(5, ctx.gpr[16] + 0u);
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(11148));
    ctx.set_gpr(16, 2262u << 16u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(48));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(9, 0u + static_cast<std::uint32_t>(4099));
    ctx.set_gpr(31, 0x08AB11E8u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(11104), ctx.gpr[2]);
    ctx.pc = 0x08B175C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB11E8u) goto L_08AB11E8;
    return;
L_08AB11E8:
    ctx.set_gpr(16, ctx.gpr[16] + static_cast<std::uint32_t>(11104));
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08AB1190(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08AB1190_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08AA155C[63] = {
    1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 5, 0, 0, 0, 6, 0,
    7, 0, 8, 0, 9, 0, 10, 0, 0, 11, 0, 12, 0, 13, 0, 0, 0, 0, 0, 14, 15, 0, 16, 0, 17, 0, 0, 0, 18, 0, 19,
};
void sub_08AA155C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08AA155Cu;
        entry_id = (entry_delta < 252u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08AA155C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08AA155C;
    case 2u: goto L_08AA156C;
    case 3u: goto L_08AA159C;
    case 4u: goto L_08AA15AC;
    case 5u: goto L_08AA15C4;
    case 6u: goto L_08AA15D4;
    case 7u: goto L_08AA15DC;
    case 8u: goto L_08AA15E4;
    case 9u: goto L_08AA15EC;
    case 10u: goto L_08AA15F4;
    case 11u: goto L_08AA1600;
    case 12u: goto L_08AA1608;
    case 13u: goto L_08AA1610;
    case 14u: goto L_08AA1628;
    case 15u: goto L_08AA162C;
    case 16u: goto L_08AA1634;
    case 17u: goto L_08AA163C;
    case 18u: goto L_08AA164C;
    case 19u: goto L_08AA1654;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08AA155C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08AA156Cu);
    // nop
    ctx.pc = 0x08AA0F8Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AA156Cu) goto L_08AA156C;
    return;
L_08AA156C:
    ctx.set_gpr(5, 2282u << 16u);
    ctx.set_gpr(4, ctx.gpr[5] + static_cast<std::uint32_t>(-3068));
    ctx.set_gpr(2, 2281u << 16u);
    ctx.set_gpr(3, 2282u << 16u);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(272), 0u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(24568));
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-3080));
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(-3068), 0u);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(292), 0u);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(296), 0u);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(276), 0u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(0), 0u);
    goto L_08AA159C;
L_08AA159C:
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(4), 0u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(148));
    if (ctx.gpr[2] != ctx.gpr[3]) {
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(0), 0u);
        goto L_08AA159C;
    }
    goto L_08AA15AC;
L_08AA15AC:
    ctx.set_gpr(4, 2234u << 16u);
    ctx.set_gpr(5, 2218u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(32432));
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(5924));
    ctx.set_gpr(31, 0x08AA15C4u);
    ctx.set_gpr(6, 0u + 0u);
    ctx.pc = 0x08B7FC54u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AA15C4u) goto L_08AA15C4;
    return;
L_08AA15C4:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, 2261u << 16u);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[4]) < 0;
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(-15840), ctx.gpr[4]);
      if (branch_taken) {
          goto L_08AA1628;
      }
      goto L_08AA15D4;
    }
L_08AA15D4:
    ctx.set_gpr(31, 0x08AA15DCu);
    // nop
    ctx.pc = 0x08B80114u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AA15DCu) goto L_08AA15DC;
    return;
L_08AA15DC:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
      if (branch_taken) {
          goto L_08AA162C;
      }
      goto L_08AA15E4;
    }
L_08AA15E4:
    ctx.set_gpr(31, 0x08AA15ECu);
    // nop
    ctx.pc = 0x08B800ECu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AA15ECu) goto L_08AA15EC;
    return;
L_08AA15EC:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(5, 2234u << 16u);
      if (branch_taken) {
          goto L_08AA1634;
      }
      goto L_08AA15F4;
    }
L_08AA15F4:
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(32440));
    ctx.set_gpr(31, 0x08AA1600u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.pc = 0x08B80124u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AA1600u) goto L_08AA1600;
    return;
L_08AA1600:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
      if (branch_taken) {
          goto L_08AA162C;
      }
      goto L_08AA1608;
    }
L_08AA1608:
    ctx.set_gpr(31, 0x08AA1610u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(32));
    ctx.pc = 0x08B8010Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AA1610u) goto L_08AA1610;
    return;
L_08AA1610:
    ctx.set_gpr(3, 2261u << 16u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(-15864), 0u);
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(3, 2261u << 16u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(4228), 0u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(-15868), 0u);
    goto L_08AA1628;
L_08AA1628:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    goto L_08AA162C;
L_08AA162C:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08AA1634:
    ctx.set_gpr(31, 0x08AA163Cu);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(2));
    ctx.pc = 0x08B8010Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AA163Cu) goto L_08AA163C;
    return;
L_08AA163C:
    ctx.set_gpr(5, 2234u << 16u);
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(32440));
    ctx.set_gpr(31, 0x08AA164Cu);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.pc = 0x08B80124u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AA164Cu) goto L_08AA164C;
    return;
L_08AA164C:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) >= 0;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
      if (branch_taken) {
          goto L_08AA1608;
      }
      goto L_08AA1654;
    }
L_08AA1654:
    // nop
    goto L_08AA162C;
}

void sub_08AA155C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08AA155C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08814E88[1] = {
    1,
};
void sub_08814E88_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08814E88u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08814E88[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08814E88;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08814E88:
    jump_target = ctx.gpr[31];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08814E88(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08814E88_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08AB2984[47] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 4, 0, 5, 0, 0, 0, 0, 0, 6, 0, 0, 7, 0, 0,
    0, 0, 8, 0, 9, 0, 0, 0, 0, 0, 0, 10, 0, 0, 11,
};
void sub_08AB2984_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08AB2984u;
        entry_id = (entry_delta < 188u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08AB2984[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08AB2984;
    case 2u: goto L_08AB29B4;
    case 3u: goto L_08AB29C4;
    case 4u: goto L_08AB29CC;
    case 5u: goto L_08AB29D4;
    case 6u: goto L_08AB29EC;
    case 7u: goto L_08AB29F8;
    case 8u: goto L_08AB2A0C;
    case 9u: goto L_08AB2A14;
    case 10u: goto L_08AB2A30;
    case 11u: goto L_08AB2A3C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08AB2984:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, 2235u << 16u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(24));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(-32176));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(19, 2262u << 16u);
    ctx.set_gpr(31, 0x08AB29B4u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB29B4u) goto L_08AB29B4;
    return;
L_08AB29B4:
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(16));
    ctx.set_gpr(31, 0x08AB29C4u);
    ctx.set_gpr(6, 4u << 16u);
    ctx.pc = 0x08A9B934u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB29C4u) goto L_08AB29C4;
    return;
L_08AB29C4:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(11184), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08AB2A30;
      }
      goto L_08AB29CC;
    }
L_08AB29CC:
    { const bool branch_taken = ctx.gpr[17] != 0u;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
      if (branch_taken) {
          goto L_08AB29EC;
      }
      goto L_08AB29D4;
    }
L_08AB29D4:
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08AB29EC:
    ctx.set_gpr(16, 0u + 0u);
    ctx.set_gpr(18, 4660u << 16u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[19] + static_cast<std::uint32_t>(11184)));
    goto L_08AB29F8;
L_08AB29F8:
    ctx.set_gpr(4, ctx.gpr[16] << 4u);
    ctx.set_gpr(5, ctx.gpr[18] | 22136u);
    ctx.set_gpr(16, ctx.gpr[16] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(31, 0x08AB2A0Cu);
    ctx.set_gpr(4, ctx.gpr[2] + ctx.gpr[4]);
    ctx.pc = 0x08AC1ED8u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB2A0Cu) goto L_08AB2A0C;
    return;
L_08AB2A0C:
    { const bool branch_taken = ctx.gpr[17] != ctx.gpr[16];
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[19] + static_cast<std::uint32_t>(11184)));
      if (branch_taken) {
          goto L_08AB29F8;
      }
      goto L_08AB2A14;
    }
L_08AB2A14:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08AB2A30:
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(-32176));
    ctx.set_gpr(31, 0x08AB2A3Cu);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(25));
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08AB2A3Cu) goto L_08AB2A3C;
    return;
L_08AB2A3C:
    // nop
    goto L_08AB29CC;
}

void sub_08AB2984(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08AB2984_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08AB3020[1] = {
    1,
};
void sub_08AB3020_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08AB3020u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08AB3020[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08AB3020;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08AB3020:
    jump_target = ctx.gpr[31];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08AB3020(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08AB3020_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B641E4[74] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4, 0, 0, 0,
    0, 0, 5, 0, 6, 0, 0, 0, 0, 7, 0, 0, 0, 0, 8, 0, 9, 0, 0, 0, 10, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0,
    12, 0, 0, 13, 0, 14, 0, 15, 0, 16,
};
void sub_08B641E4_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B641E4u;
        entry_id = (entry_delta < 296u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B641E4[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B641E4;
    case 2u: goto L_08B6420C;
    case 3u: goto L_08B64240;
    case 4u: goto L_08B64254;
    case 5u: goto L_08B6426C;
    case 6u: goto L_08B64274;
    case 7u: goto L_08B64288;
    case 8u: goto L_08B6429C;
    case 9u: goto L_08B642A4;
    case 10u: goto L_08B642B4;
    case 11u: goto L_08B642C0;
    case 12u: goto L_08B642E4;
    case 13u: goto L_08B642F0;
    case 14u: goto L_08B642F8;
    case 15u: goto L_08B64300;
    case 16u: goto L_08B64308;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B641E4:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    ctx.set_gpr(20, 2273u << 16u);
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[20] + static_cast<std::uint32_t>(-1000)));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    { const bool branch_taken = ctx.gpr[3] != 0u;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08B64308;
      }
      goto L_08B6420C;
    }
L_08B6420C:
    ctx.set_gpr(12, 2273u << 16u);
    ctx.set_gpr(9, 2273u << 16u);
    ctx.set_gpr(5, 2273u << 16u);
    ctx.set_gpr(11, ctx.gpr[12] + static_cast<std::uint32_t>(-992));
    ctx.set_gpr(10, 2273u << 16u);
    ctx.set_gpr(8, ctx.gpr[9] + static_cast<std::uint32_t>(-880));
    ctx.set_gpr(7, 2273u << 16u);
    ctx.set_gpr(4, ctx.gpr[5] + static_cast<std::uint32_t>(2720));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(512));
    ctx.set_gpr(5, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(3760), ctx.gpr[11]);
    ctx.set_gpr(31, 0x08B64240u);
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(3756), ctx.gpr[8]);
    ctx.pc = 0x08B7FB74u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B64240u) goto L_08B64240;
    return;
L_08B64240:
    ctx.set_gpr(2, 2273u << 16u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(3232));
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(31, 0x08B64254u);
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(512));
    ctx.pc = 0x08B7FB74u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B64254u) goto L_08B64254;
    return;
L_08B64254:
    ctx.set_gpr(3, 2273u << 16u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(448));
    ctx.set_gpr(6, 0u + 0u);
    ctx.set_gpr(31, 0x08B6426Cu);
    ctx.set_gpr(16, ctx.gpr[3] + static_cast<std::uint32_t>(3744));
    ctx.pc = 0x08B7FDC4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B6426Cu) goto L_08B6426C;
    return;
L_08B6426C:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B64308;
      }
      goto L_08B64274;
    }
L_08B64274:
    ctx.set_gpr(4, 2273u << 16u);
    ctx.set_gpr(18, ctx.gpr[4] + static_cast<std::uint32_t>(3768));
    ctx.set_gpr(19, ctx.gpr[16] + 0u);
    ctx.set_gpr(17, 0u + 0u);
    ctx.set_gpr(13, ctx.gpr[17] << 2u);
    goto L_08B64288;
L_08B64288:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(256));
    ctx.set_gpr(6, 0u + 0u);
    ctx.set_gpr(31, 0x08B6429Cu);
    ctx.set_gpr(16, ctx.gpr[13] + ctx.gpr[18]);
    ctx.pc = 0x08B7FDC4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B6429Cu) goto L_08B6429C;
    return;
L_08B6429C:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B642E4;
      }
      goto L_08B642A4;
    }
L_08B642A4:
    ctx.set_gpr(17, ctx.gpr[17] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(6, static_cast<std::int32_t>(ctx.gpr[17]) < 3 ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[6] != 0u;
    ctx.set_gpr(13, ctx.gpr[17] << 2u);
      if (branch_taken) {
          goto L_08B64288;
      }
      goto L_08B642B4;
    }
L_08B642B4:
    ctx.set_gpr(14, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(3, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(-1000), ctx.gpr[14]);
    goto L_08B642C0;
L_08B642C0:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[3] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B642E4:
    ctx.set_gpr(17, ctx.gpr[17] + static_cast<std::uint32_t>(-1));
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[17]) < 0;
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
      if (branch_taken) {
          goto L_08B64300;
      }
      goto L_08B642F0;
    }
L_08B642F0:
    ctx.set_gpr(31, 0x08B642F8u);
    ctx.set_gpr(17, ctx.gpr[17] + static_cast<std::uint32_t>(-1));
    ctx.pc = 0x08B7FDCCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B642F8u) goto L_08B642F8;
    return;
L_08B642F8:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[17]) >= 0;
    ctx.set_gpr(4, ctx.gpr[17] + 0u);
      if (branch_taken) {
          goto L_08B642F0;
      }
      goto L_08B64300;
    }
L_08B64300:
    ctx.set_gpr(31, 0x08B64308u);
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[19] + static_cast<std::uint32_t>(4)));
    ctx.pc = 0x08B7FDCCu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B64308u) goto L_08B64308;
    return;
L_08B64308:
    ctx.set_gpr(15, 32836u << 16u);
    ctx.set_gpr(3, ctx.gpr[15] | 1u);
    goto L_08B642C0;
}

void sub_08B641E4(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B641E4_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B64314[75] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 4, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 7, 0, 8, 0, 0, 9, 0, 10, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 12,
};
void sub_08B64314_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B64314u;
        entry_id = (entry_delta < 300u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B64314[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B64314;
    case 2u: goto L_08B64368;
    case 3u: goto L_08B64370;
    case 4u: goto L_08B6439C;
    case 5u: goto L_08B643B4;
    case 6u: goto L_08B643E0;
    case 7u: goto L_08B643F0;
    case 8u: goto L_08B643F8;
    case 9u: goto L_08B64404;
    case 10u: goto L_08B6440C;
    case 11u: goto L_08B64434;
    case 12u: goto L_08B6443C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B64314:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[21]);
    ctx.set_gpr(21, 2273u << 16u);
    ctx.set_gpr(8, 2233u << 16u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[21] + static_cast<std::uint32_t>(-1000)));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(3, 32836u << 16u);
    ctx.set_gpr(17, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, ctx.gpr[8] + static_cast<std::uint32_t>(1080));
    ctx.set_gpr(8, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(512));
    ctx.set_gpr(6, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[31]);
    ctx.set_gpr(7, 0u + 0u);
    ctx.set_gpr(19, 2273u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    ctx.set_gpr(10, ctx.gpr[3] | 3u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    { const bool branch_taken = ctx.gpr[2] != ctx.gpr[8];
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08B6440C;
      }
      goto L_08B64368;
    }
L_08B64368:
    ctx.set_gpr(31, 0x08B64370u);
    ctx.set_gpr(20, 2273u << 16u);
    ctx.pc = 0x08B7FCC4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B64370u) goto L_08B64370;
    return;
L_08B64370:
    ctx.set_gpr(4, 2230u << 16u);
    ctx.set_gpr(6, 2233u << 16u);
    ctx.set_gpr(5, ctx.gpr[4] + static_cast<std::uint32_t>(20448));
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(512));
    ctx.set_gpr(4, ctx.gpr[6] + static_cast<std::uint32_t>(1096));
    ctx.set_gpr(8, 0u + 0u);
    ctx.set_gpr(9, 0u + 0u);
    ctx.set_gpr(6, ctx.gpr[17] + 0u);
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(3764), ctx.gpr[2]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(10, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B6440C;
      }
      goto L_08B6439C;
    }
L_08B6439C:
    ctx.set_gpr(13, 32836u << 16u);
    ctx.set_gpr(12, 2273u << 16u);
    ctx.set_gpr(16, ctx.gpr[13] | 18u);
    rt.memory().aot_store32(ctx.gpr[12] + static_cast<std::uint32_t>(-996), 0u);
    ctx.set_gpr(31, 0x08B643B4u);
    ctx.set_gpr(18, ctx.gpr[20] + static_cast<std::uint32_t>(3744));
    ctx.pc = 0x08B7FCB4u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B643B4u) goto L_08B643B4;
    return;
L_08B643B4:
    ctx.set_gpr(7, 32770u << 16u);
    ctx.set_gpr(5, ctx.gpr[7] | 403u);
    ctx.set_gpr(11, 32836u << 16u);
    ctx.set_gpr(10, ctx.gpr[2] ^ ctx.gpr[5]);
    ctx.set_gpr(9, ctx.gpr[11] | 3u);
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(6, ctx.gpr[18] + 0u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(12));
    if (ctx.gpr[10] != 0u) ctx.set_gpr(16, ctx.gpr[9]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    rt.memory().aot_store32(ctx.gpr[20] + static_cast<std::uint32_t>(3744), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B643F8;
      }
      goto L_08B643E0;
    }
L_08B643E0:
    ctx.set_gpr(14, 32836u << 16u);
    rt.memory().aot_store32(ctx.gpr[18] + static_cast<std::uint32_t>(8), ctx.gpr[17]);
    ctx.set_gpr(31, 0x08B643F0u);
    ctx.set_gpr(16, ctx.gpr[14] | 3u);
    ctx.pc = 0x08B7FC74u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B643F0u) goto L_08B643F0;
    return;
L_08B643F0:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(10000));
      if (branch_taken) {
          goto L_08B64434;
      }
      goto L_08B643F8;
    }
L_08B643F8:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[19] + static_cast<std::uint32_t>(3764)));
    ctx.set_gpr(31, 0x08B64404u);
    ctx.set_gpr(17, 0u + static_cast<std::uint32_t>(-1));
    ctx.pc = 0x08B7FC6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B64404u) goto L_08B64404;
    return;
L_08B64404:
    rt.memory().aot_store32(ctx.gpr[19] + static_cast<std::uint32_t>(3764), ctx.gpr[17]);
    ctx.set_gpr(10, ctx.gpr[16] + 0u);
    goto L_08B6440C;
L_08B6440C:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[10] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B64434:
    ctx.set_gpr(31, 0x08B6443Cu);
    ctx.set_gpr(16, 0u + 0u);
    ctx.pc = 0x08B7FC34u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B6443Cu) goto L_08B6443C;
    return;
L_08B6443C:
    ctx.set_gpr(15, 0u + static_cast<std::uint32_t>(3));
    rt.memory().aot_store32(ctx.gpr[21] + static_cast<std::uint32_t>(-1000), ctx.gpr[15]);
    ctx.set_gpr(10, ctx.gpr[16] + 0u);
    goto L_08B6440C;
}

void sub_08B64314(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B64314_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B48B48[1] = {
    1,
};
void sub_08B48B48_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B48B48u;
        entry_id = (entry_delta < 4u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B48B48[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B48B48;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B48B48:
    ctx.set_gpr(2, 2272u << 16u);
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(28436), ctx.gpr[4]);
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B48B48(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B48B48_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B48B0C[6] = {
    1, 0, 0, 0, 0, 2,
};
void sub_08B48B0C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B48B0Cu;
        entry_id = (entry_delta < 24u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B48B0C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B48B0C;
    case 2u: goto L_08B48B20;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B48B0C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(31, 0x08B48B20u);
    ctx.set_gpr(16, ctx.gpr[4] + 0u);
    ctx.pc = 0x08B4B758u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B48B20u) goto L_08B48B20;
    return;
L_08B48B20:
    ctx.set_gpr(4, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(28432), ctx.gpr[16]);
    ctx.set_gpr(2, 2233u << 16u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(21404));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(3, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(28704), ctx.gpr[2]);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B48B0C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B48B0C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B177F0[35] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 4,
};
void sub_08B177F0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B177F0u;
        entry_id = (entry_delta < 140u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B177F0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B177F0;
    case 2u: goto L_08B17838;
    case 3u: goto L_08B1784C;
    case 4u: goto L_08B17878;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B177F0:
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(3, ctx.gpr[2] + static_cast<std::uint32_t>(21508));
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    ctx.set_gpr(8, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(16));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(21472));
    ctx.set_gpr(16, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21508), ctx.gpr[3]);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(36));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(7, 0u + 0u);
    ctx.set_gpr(9, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    ctx.set_gpr(31, 0x08B17838u);
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(21468), ctx.gpr[2]);
    ctx.pc = 0x08B17400u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B17838u) goto L_08B17838;
    return;
L_08B17838:
    ctx.set_gpr(5, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(6, 0u + 0u);
    { const bool branch_taken = ctx.gpr[5] == 0u;
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21464), ctx.gpr[5]);
      if (branch_taken) {
          goto L_08B17878;
      }
      goto L_08B1784C;
    }
L_08B1784C:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(32)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(28)));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(0), ctx.gpr[3]);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(28)));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
    ctx.set_gpr(2, ctx.gpr[6] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B17878:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(21468), 0u);
    ctx.set_gpr(2, ctx.gpr[6] + 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B177F0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B177F0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B13400[243] = {
    1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 3, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8,
};
void sub_08B13400_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B130ACu;
        entry_id = (entry_delta < 972u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B13400[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B130AC;
    case 2u: goto L_08B130CC;
    case 3u: goto L_08B130E8;
    case 4u: goto L_08B130F0;
    case 5u: goto L_08B13400;
    case 6u: goto L_08B13418;
    case 7u: goto L_08B13448;
    case 8u: goto L_08B13474;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B130AC:
    ctx.fpr[0] = std::bit_cast<float>(ctx.gpr[3]);
    ctx.set_gpr(2, ctx.gpr[4] >> 8u);
    ctx.set_gpr(3, ctx.gpr[2] & 255u);
    ctx.fpr[1] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(0)));
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(11628)));
    ctx.fpr[0] = ctx.fpr[1] / ctx.fpr[0];
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[3]) < 0;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
      if (branch_taken) {
          goto L_08B13448;
      }
      goto L_08B130CC;
    }
L_08B130CC:
    ctx.fpr[0] = std::bit_cast<float>(ctx.gpr[3]);
    ctx.set_gpr(3, ctx.gpr[4] & 255u);
    ctx.fpr[1] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(0)));
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(11628)));
    ctx.fpr[0] = ctx.fpr[1] / ctx.fpr[0];
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[3]) < 0;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
      if (branch_taken) {
          goto L_08B13474;
      }
      goto L_08B130E8;
    }
L_08B130E8:
    ctx.fpr[1] = std::bit_cast<float>(ctx.gpr[3]);
    ctx.fpr[0] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(1)));
    goto L_08B130F0;
L_08B130F0:
    ctx.fpr[7] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(11628)));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(6, ctx.gpr[14] + static_cast<std::uint32_t>(-8112));
    ctx.fpr[7] = ctx.fpr[0] / ctx.fpr[7];
    rt.memory().aot_store32(ctx.gpr[14] + static_cast<std::uint32_t>(-8112), ctx.gpr[2]);
    ctx.set_gpr(2, 2235u << 16u);
    ctx.set_gpr(3, 2284u << 16u);
    ctx.fpr[1] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(11632)));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-8368));
    ctx.fpr[1] = ctx.fpr[1] - ctx.fpr[9];
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    ctx.set_gpr(7, ctx.gpr[15] + static_cast<std::uint32_t>(-8080));
    ctx.set_gpr(8, ctx.gpr[10] + static_cast<std::uint32_t>(-8096));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.fpr[6] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(272)));
    ctx.fpr[3] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(256)));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    { const float fs = ctx.fpr[9]; const float ft = ctx.fpr[6]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(6, 0x7FC00000u); else ctx.fpr[6] = fs * ft; }
    { const float fs = ctx.fpr[1]; const float ft = ctx.fpr[3]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(3, 0x7FC00000u); else ctx.fpr[3] = fs * ft; }
    ctx.fpr[4] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(260)));
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(276)));
    ctx.fpr[5] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(280)));
    ctx.fpr[2] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(264)));
    { const float fs = ctx.fpr[1]; const float ft = ctx.fpr[4]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(4, 0x7FC00000u); else ctx.fpr[4] = fs * ft; }
    { const float fs = ctx.fpr[9]; const float ft = ctx.fpr[0]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(0, 0x7FC00000u); else ctx.fpr[0] = fs * ft; }
    { const float fs = ctx.fpr[1]; const float ft = ctx.fpr[2]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(2, 0x7FC00000u); else ctx.fpr[2] = fs * ft; }
    { const float fs = ctx.fpr[9]; const float ft = ctx.fpr[5]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(5, 0x7FC00000u); else ctx.fpr[5] = fs * ft; }
    ctx.set_gpr(9, rt.memory().aot_load32(ctx.gpr[15] + static_cast<std::uint32_t>(-8080)));
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(8)));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), std::bit_cast<std::uint32_t>(ctx.fpr[7]));
    ctx.fpr[3] = ctx.fpr[3] + ctx.fpr[6];
    ctx.fpr[4] = ctx.fpr[4] + ctx.fpr[0];
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(272), std::bit_cast<std::uint32_t>(ctx.fpr[6]));
    ctx.fpr[2] = ctx.fpr[2] + ctx.fpr[5];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(12), ctx.gpr[2]);
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(284)));
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(4), ctx.gpr[4]);
    { const float fs = ctx.fpr[9]; const float ft = ctx.fpr[0]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(0, 0x7FC00000u); else ctx.fpr[0] = fs * ft; }
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(8), ctx.gpr[5]);
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(-8096), ctx.gpr[9]);
    ctx.fpr[5] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(268)));
    ctx.fpr[6] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(272)));
    ctx.fpr[7] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(276)));
    ctx.fpr[8] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(280)));
    { const float fs = ctx.fpr[1]; const float ft = ctx.fpr[5]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(1, 0x7FC00000u); else ctx.fpr[1] = fs * ft; }
    { const float fs = ctx.fpr[3]; const float ft = ctx.fpr[6]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(3, 0x7FC00000u); else ctx.fpr[3] = fs * ft; }
    { const float fs = ctx.fpr[4]; const float ft = ctx.fpr[7]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(4, 0x7FC00000u); else ctx.fpr[4] = fs * ft; }
    { const float fs = ctx.fpr[2]; const float ft = ctx.fpr[8]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(2, 0x7FC00000u); else ctx.fpr[2] = fs * ft; }
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(12)));
    ctx.fpr[1] = ctx.fpr[1] + ctx.fpr[0];
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(256), std::bit_cast<std::uint32_t>(ctx.fpr[3]));
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(12), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(260), std::bit_cast<std::uint32_t>(ctx.fpr[4]));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(264), std::bit_cast<std::uint32_t>(ctx.fpr[2]));
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(284)));
    { const float fs = ctx.fpr[1]; const float ft = ctx.fpr[0]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(1, 0x7FC00000u); else ctx.fpr[1] = fs * ft; }
    jump_target = ctx.gpr[31];
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(268), std::bit_cast<std::uint32_t>(ctx.fpr[1]));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B13400:
    ctx.fpr[1] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(2)));
    ctx.set_gpr(3, ctx.gpr[2] & 255u);
    ctx.fpr[1] = ctx.fpr[1] + ctx.fpr[1];
    ctx.fpr[0] = ctx.fpr[1] / ctx.fpr[0];
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[3]) >= 0;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
      if (branch_taken) {
          goto L_08B130AC;
      }
      goto L_08B13418;
    }
L_08B13418:
    ctx.set_gpr(3, ctx.gpr[3] >> 1u);
    ctx.set_gpr(2, ctx.gpr[2] & 1u);
    ctx.set_gpr(2, ctx.gpr[2] | ctx.gpr[3]);
    ctx.fpr[2] = std::bit_cast<float>(ctx.gpr[2]);
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(11628)));
    ctx.set_gpr(2, ctx.gpr[4] >> 8u);
    ctx.fpr[1] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(2)));
    ctx.set_gpr(3, ctx.gpr[2] & 255u);
    ctx.fpr[1] = ctx.fpr[1] + ctx.fpr[1];
    ctx.fpr[0] = ctx.fpr[1] / ctx.fpr[0];
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[3]) >= 0;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
      if (branch_taken) {
          goto L_08B130CC;
      }
      goto L_08B13448;
    }
L_08B13448:
    ctx.set_gpr(3, ctx.gpr[3] >> 1u);
    ctx.set_gpr(2, ctx.gpr[2] & 1u);
    ctx.set_gpr(2, ctx.gpr[2] | ctx.gpr[3]);
    ctx.fpr[2] = std::bit_cast<float>(ctx.gpr[2]);
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[7] + static_cast<std::uint32_t>(11628)));
    ctx.set_gpr(3, ctx.gpr[4] & 255u);
    ctx.fpr[1] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(2)));
    ctx.fpr[1] = ctx.fpr[1] + ctx.fpr[1];
    ctx.fpr[0] = ctx.fpr[1] / ctx.fpr[0];
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[3]) >= 0;
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
      if (branch_taken) {
          goto L_08B130E8;
      }
      goto L_08B13474;
    }
L_08B13474:
    ctx.set_gpr(2, ctx.gpr[4] & 1u);
    ctx.set_gpr(3, ctx.gpr[3] >> 1u);
    ctx.set_gpr(2, ctx.gpr[2] | ctx.gpr[3]);
    ctx.fpr[2] = std::bit_cast<float>(ctx.gpr[2]);
    ctx.fpr[0] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(2)));
    ctx.fpr[0] = ctx.fpr[0] + ctx.fpr[0];
    goto L_08B130F0;
}

void sub_08B13400(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B13400_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08AC4794[8] = {
    1, 0, 0, 0, 0, 2, 0, 3,
};
void sub_08AC4794_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08AC4794u;
        entry_id = (entry_delta < 32u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08AC4794[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08AC4794;
    case 2u: goto L_08AC47A8;
    case 3u: goto L_08AC47B0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08AC4794:
    ctx.set_gpr(2, 2262u << 16u);
    ctx.set_gpr(25, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(17476)));
    ctx.set_gpr(4, 0u + 0u);
    { const bool branch_taken = ctx.gpr[25] == 0u;
    ctx.set_gpr(5, 0u + 0u);
      if (branch_taken) {
          goto L_08AC47B0;
      }
      goto L_08AC47A8;
    }
L_08AC47A8:
    jump_target = ctx.gpr[25];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08AC47B0:
    jump_target = ctx.gpr[31];
    // nop
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08AC4794(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08AC4794_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B15E58[28] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 5,
};
void sub_08B15E58_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B15E58u;
        entry_id = (entry_delta < 112u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B15E58[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B15E58;
    case 2u: goto L_08B15E8C;
    case 3u: goto L_08B15EA4;
    case 4u: goto L_08B15EBC;
    case 5u: goto L_08B15EC4;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B15E58:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(3, 2243u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(8, 2272u << 16u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(64));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(22632)));
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(22628)));
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(21428));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(31, 0x08B15E8Cu);
    ctx.set_gpr(9, 0u + 0u);
    ctx.pc = 0x08B175C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B15E8Cu) goto L_08B15E8C;
    return;
L_08B15E8C:
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27516));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(35));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(21416), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B15EBC;
      }
      goto L_08B15EA4;
    }
L_08B15EA4:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21416)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u < ctx.gpr[2] ? 1u : 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B15EBC:
    ctx.set_gpr(31, 0x08B15EC4u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B15EC4u) goto L_08B15EC4;
    return;
L_08B15EC4:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21416)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u < ctx.gpr[2] ? 1u : 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B15E58(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B15E58_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B15AB0[28] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 5,
};
void sub_08B15AB0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B15AB0u;
        entry_id = (entry_delta < 112u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B15AB0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B15AB0;
    case 2u: goto L_08B15AE4;
    case 3u: goto L_08B15AFC;
    case 4u: goto L_08B15B14;
    case 5u: goto L_08B15B1C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B15AB0:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(3, 2243u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(8, 2272u << 16u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(84));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(22624)));
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(22620)));
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(21380));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(31, 0x08B15AE4u);
    ctx.set_gpr(9, 0u + 0u);
    ctx.pc = 0x08B175C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B15AE4u) goto L_08B15AE4;
    return;
L_08B15AE4:
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27540));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(185));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(21356), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B15B14;
      }
      goto L_08B15AFC;
    }
L_08B15AFC:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21356)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u < ctx.gpr[2] ? 1u : 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B15B14:
    ctx.set_gpr(31, 0x08B15B1Cu);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B15B1Cu) goto L_08B15B1C;
    return;
L_08B15B1C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21356)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u < ctx.gpr[2] ? 1u : 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B15AB0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B15AB0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B1B8AC[35] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 3, 4, 0, 0, 0, 5, 0, 0, 6, 0, 0, 0, 0,
    7, 0, 8,
};
void sub_08B1B8AC_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B1B8ACu;
        entry_id = (entry_delta < 140u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B1B8AC[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B1B8AC;
    case 2u: goto L_08B1B8E0;
    case 3u: goto L_08B1B8F8;
    case 4u: goto L_08B1B8FC;
    case 5u: goto L_08B1B90C;
    case 6u: goto L_08B1B918;
    case 7u: goto L_08B1B92C;
    case 8u: goto L_08B1B934;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B1B8AC:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(3, 2243u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(8, 2272u << 16u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(224));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(22664)));
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(22660)));
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(21664));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(31, 0x08B1B8E0u);
    ctx.set_gpr(9, 0u + 0u);
    ctx.pc = 0x08B175C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1B8E0u) goto L_08B1B8E0;
    return;
L_08B1B8E0:
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27348));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(218));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(21648), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B1B92C;
      }
      goto L_08B1B8F8;
    }
L_08B1B8F8:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21648)));
    goto L_08B1B8FC;
L_08B1B8FC:
    ctx.set_gpr(5, 2272u << 16u);
    ctx.set_gpr(3, ctx.gpr[5] + static_cast<std::uint32_t>(21656));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, 0u + 0u);
      if (branch_taken) {
          goto L_08B1B918;
      }
      goto L_08B1B90C;
    }
L_08B1B90C:
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(21656), ctx.gpr[3]);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
    goto L_08B1B918;
L_08B1B918:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[4] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B1B92C:
    ctx.set_gpr(31, 0x08B1B934u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1B934u) goto L_08B1B934;
    return;
L_08B1B934:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21648)));
    goto L_08B1B8FC;
}

void sub_08B1B8AC(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B1B8AC_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B1C214[28] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 4, 0, 5,
};
void sub_08B1C214_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B1C214u;
        entry_id = (entry_delta < 112u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B1C214[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B1C214;
    case 2u: goto L_08B1C248;
    case 3u: goto L_08B1C260;
    case 4u: goto L_08B1C278;
    case 5u: goto L_08B1C280;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B1C214:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(3, 2243u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    ctx.set_gpr(8, 2272u << 16u);
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(400));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(22672)));
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(22668)));
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(21704));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(31, 0x08B1C248u);
    ctx.set_gpr(9, 0u + 0u);
    ctx.pc = 0x08B175C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1C248u) goto L_08B1C248;
    return;
L_08B1C248:
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(4, 2235u << 16u);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27324));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(24));
    { const bool branch_taken = ctx.gpr[2] == 0u;
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(21700), ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B1C278;
      }
      goto L_08B1C260;
    }
L_08B1C260:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21700)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u < ctx.gpr[2] ? 1u : 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B1C278:
    ctx.set_gpr(31, 0x08B1C280u);
    // nop
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B1C280u) goto L_08B1C280;
    return;
L_08B1C280:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(21700)));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, 0u < ctx.gpr[2] ? 1u : 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B1C214(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B1C214_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B17AC0[13] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
};
void sub_08B17AC0_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B17AC0u;
        entry_id = (entry_delta < 52u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B17AC0[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B17AC0;
    case 2u: goto L_08B17AF0;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B17AC0:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(3, 2243u << 16u);
    ctx.set_gpr(2, 2243u << 16u);
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(22652)));
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(22656)));
    ctx.set_gpr(8, 2272u << 16u);
    ctx.set_gpr(8, ctx.gpr[8] + static_cast<std::uint32_t>(21520));
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(24));
    ctx.set_gpr(6, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(31, 0x08B17AF0u);
    ctx.set_gpr(9, 0u + 0u);
    ctx.pc = 0x08B175C0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B17AF0u) goto L_08B17AF0;
    return;
L_08B17AF0:
    ctx.set_gpr(3, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(21516), ctx.gpr[2]);
    ctx.set_gpr(2, 0u < ctx.gpr[2] ? 1u : 0u);
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B17AC0(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B17AC0_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B19928[309] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 7, 0, 0, 0, 8, 0, 0,
    0, 0, 0, 9, 0, 10, 0, 0, 0, 0, 11, 0, 0, 0, 12, 0, 13, 0, 0, 14, 0, 0, 0, 15, 0, 16, 0, 17, 0, 18, 0, 0,
    0, 0, 0, 19, 0, 0, 0, 0, 20, 0, 0, 0, 0, 0, 21, 0, 0, 0, 0, 0, 22, 0, 0, 23, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 24, 0, 0, 0, 25, 0, 0, 0, 0, 26, 27, 0, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 29, 0,
    0, 0, 0, 0, 0, 30, 31, 0, 32, 0, 33, 0, 34, 0, 0, 0, 0, 0, 35, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 37,
    0, 0, 38, 0, 39, 0, 0, 0, 40, 0, 0, 0, 0, 0, 41, 0, 0, 42, 0, 0, 43,
};
void sub_08B19928_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B19928u;
        entry_id = (entry_delta < 1236u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B19928[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B19928;
    case 2u: goto L_08B19968;
    case 3u: goto L_08B19974;
    case 4u: goto L_08B19984;
    case 5u: goto L_08B19A54;
    case 6u: goto L_08B19B78;
    case 7u: goto L_08B19B8C;
    case 8u: goto L_08B19B9C;
    case 9u: goto L_08B19BB4;
    case 10u: goto L_08B19BBC;
    case 11u: goto L_08B19BD0;
    case 12u: goto L_08B19BE0;
    case 13u: goto L_08B19BE8;
    case 14u: goto L_08B19BF4;
    case 15u: goto L_08B19C04;
    case 16u: goto L_08B19C0C;
    case 17u: goto L_08B19C14;
    case 18u: goto L_08B19C1C;
    case 19u: goto L_08B19C34;
    case 20u: goto L_08B19C48;
    case 21u: goto L_08B19C60;
    case 22u: goto L_08B19C78;
    case 23u: goto L_08B19C84;
    case 24u: goto L_08B19CBC;
    case 25u: goto L_08B19CCC;
    case 26u: goto L_08B19CE0;
    case 27u: goto L_08B19CE4;
    case 28u: goto L_08B19CEC;
    case 29u: goto L_08B19D20;
    case 30u: goto L_08B19D3C;
    case 31u: goto L_08B19D40;
    case 32u: goto L_08B19D48;
    case 33u: goto L_08B19D50;
    case 34u: goto L_08B19D58;
    case 35u: goto L_08B19D70;
    case 36u: goto L_08B19D74;
    case 37u: goto L_08B19DA4;
    case 38u: goto L_08B19DB0;
    case 39u: goto L_08B19DB8;
    case 40u: goto L_08B19DC8;
    case 41u: goto L_08B19DE0;
    case 42u: goto L_08B19DEC;
    case 43u: goto L_08B19DF8;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B19928:
    ctx.set_gpr(2, 2235u << 16u);
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-64));
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(-27372));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(65));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(36), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(32), ctx.gpr[30]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(28), ctx.gpr[23]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(24), ctx.gpr[22]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[21]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[20]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[19]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(31, 0x08B19968u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(48), std::bit_cast<std::uint32_t>(ctx.fpr[20]));
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19968u) goto L_08B19968;
    return;
L_08B19968:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(12336));
    ctx.set_gpr(31, 0x08B19974u);
    ctx.set_gpr(5, 4u << 16u);
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19974u) goto L_08B19974;
    return;
L_08B19974:
    ctx.set_gpr(24, ctx.gpr[2] + 0u);
    ctx.set_gpr(2, 2272u << 16u);
    { const bool branch_taken = ctx.gpr[24] == 0u;
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21588), ctx.gpr[24]);
      if (branch_taken) {
          goto L_08B19D70;
      }
      goto L_08B19984;
    }
L_08B19984:
    ctx.set_gpr(2, 2235u << 16u);
    ctx.fpr[19] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(11888)));
    ctx.set_gpr(2, 2235u << 16u);
    ctx.fpr[20] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(11896)));
    ctx.set_gpr(2, 2235u << 16u);
    ctx.set_gpr(3, 2235u << 16u);
    ctx.fpr[12] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(11904)));
    ctx.set_gpr(2, 2235u << 16u);
    ctx.fpr[15] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(11892)));
    ctx.fpr[16] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(11912)));
    ctx.set_gpr(3, 2235u << 16u);
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(30, ctx.gpr[24] + static_cast<std::uint32_t>(1028));
    ctx.fpr[14] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(11900)));
    ctx.set_gpr(23, ctx.gpr[24] + static_cast<std::uint32_t>(2056));
    ctx.set_gpr(3, 2235u << 16u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21592), ctx.gpr[30]);
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(22, ctx.gpr[24] + static_cast<std::uint32_t>(3084));
    ctx.fpr[18] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(11908)));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21596), ctx.gpr[23]);
    ctx.set_gpr(3, 2235u << 16u);
    ctx.set_gpr(2, 2272u << 16u);
    ctx.fpr[17] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(11916)));
    ctx.set_gpr(21, ctx.gpr[24] + static_cast<std::uint32_t>(4112));
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(20, ctx.gpr[24] + static_cast<std::uint32_t>(5140));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21600), ctx.gpr[22]);
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(19, ctx.gpr[24] + static_cast<std::uint32_t>(6168));
    ctx.set_gpr(18, ctx.gpr[24] + static_cast<std::uint32_t>(7196));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(21572), ctx.gpr[21]);
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(17, ctx.gpr[24] + static_cast<std::uint32_t>(8224));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21576), ctx.gpr[20]);
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(16, ctx.gpr[24] + static_cast<std::uint32_t>(9252));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(21580), ctx.gpr[19]);
    ctx.fpr[13] = std::bit_cast<float>(0u);
    ctx.set_gpr(3, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21584), ctx.gpr[18]);
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(31, ctx.gpr[24] + static_cast<std::uint32_t>(10280));
    ctx.set_gpr(25, ctx.gpr[24] + static_cast<std::uint32_t>(11308));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(21556), ctx.gpr[17]);
    ctx.set_gpr(3, 2272u << 16u);
    ctx.set_gpr(15, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21560), ctx.gpr[16]);
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(14, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(21564), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(21568), ctx.gpr[25]);
    goto L_08B19A54;
L_08B19A54:
    ctx.fpr[1] = std::bit_cast<float>(ctx.gpr[15]);
    ctx.set_gpr(2, ctx.gpr[14] + ctx.gpr[20]);
    ctx.set_gpr(12, ctx.gpr[24] + ctx.gpr[14]);
    ctx.fpr[0] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(1)));
    ctx.set_gpr(13, ctx.gpr[14] + ctx.gpr[30]);
    ctx.set_gpr(9, ctx.gpr[14] + ctx.gpr[23]);
    ctx.set_gpr(10, ctx.gpr[14] + ctx.gpr[22]);
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[19]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(0, 0x7FC00000u); else ctx.fpr[0] = fs * ft; }
    ctx.set_gpr(11, ctx.gpr[14] + ctx.gpr[21]);
    ctx.set_gpr(3, ctx.gpr[14] + ctx.gpr[19]);
    ctx.set_gpr(4, ctx.gpr[14] + ctx.gpr[18]);
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[20]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(9, 0x7FC00000u); else ctx.fpr[9] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[14]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(10, 0x7FC00000u); else ctx.fpr[10] = fs * ft; }
    ctx.fpr[11] = ctx.fpr[0] + ctx.fpr[13];
    ctx.fpr[6] = ctx.fpr[14] - ctx.fpr[0];
    ctx.fpr[4] = ctx.fpr[9] + ctx.fpr[14];
    ctx.fpr[5] = ctx.fpr[10] - ctx.fpr[15];
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[11]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(8, 0x7FC00000u); else ctx.fpr[8] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[6]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(6, 0x7FC00000u); else ctx.fpr[6] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[4]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(4, 0x7FC00000u); else ctx.fpr[4] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[5]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(5, 0x7FC00000u); else ctx.fpr[5] = fs * ft; }
    ctx.fpr[8] = ctx.fpr[8] + ctx.fpr[13];
    ctx.fpr[6] = ctx.fpr[6] - ctx.fpr[14];
    ctx.fpr[4] = ctx.fpr[4] + ctx.fpr[14];
    ctx.fpr[5] = ctx.fpr[5] + ctx.fpr[13];
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[8]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(7, 0x7FC00000u); else ctx.fpr[7] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[6]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(6, 0x7FC00000u); else ctx.fpr[6] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[4]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(4, 0x7FC00000u); else ctx.fpr[4] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[5]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(5, 0x7FC00000u); else ctx.fpr[5] = fs * ft; }
    ctx.fpr[7] = ctx.fpr[7] + ctx.fpr[13];
    ctx.fpr[6] = ctx.fpr[6] + ctx.fpr[12];
    ctx.fpr[4] = ctx.fpr[4] + ctx.fpr[12];
    ctx.fpr[5] = ctx.fpr[5] + ctx.fpr[18];
    ctx.fpr[7] = ctx.fpr[7] / ctx.fpr[15];
    ctx.fpr[6] = ctx.fpr[6] / ctx.fpr[15];
    ctx.fpr[4] = ctx.fpr[4] / ctx.fpr[15];
    ctx.fpr[5] = ctx.fpr[5] / ctx.fpr[15];
    ctx.fpr[1] = ctx.fpr[9] + ctx.fpr[17];
    ctx.fpr[2] = ctx.fpr[10] - ctx.fpr[18];
    ctx.fpr[3] = ctx.fpr[17] - ctx.fpr[0];
    { const float fs = ctx.fpr[8]; const float ft = ctx.fpr[16]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(8, 0x7FC00000u); else ctx.fpr[8] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[1]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(1, 0x7FC00000u); else ctx.fpr[1] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[2]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(2, 0x7FC00000u); else ctx.fpr[2] = fs * ft; }
    { const float fs = ctx.fpr[0]; const float ft = ctx.fpr[3]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(3, 0x7FC00000u); else ctx.fpr[3] = fs * ft; }
    ctx.fpr[9] = ctx.fpr[9] + ctx.fpr[12];
    ctx.fpr[1] = ctx.fpr[1] + ctx.fpr[12];
    ctx.fpr[2] = ctx.fpr[2] + ctx.fpr[13];
    ctx.fpr[3] = ctx.fpr[3] - ctx.fpr[12];
    ctx.fpr[10] = ctx.fpr[10] - ctx.fpr[17];
    { const float fs = ctx.fpr[1]; const float ft = ctx.fpr[16]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(1, 0x7FC00000u); else ctx.fpr[1] = fs * ft; }
    { const float fs = ctx.fpr[2]; const float ft = ctx.fpr[16]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(2, 0x7FC00000u); else ctx.fpr[2] = fs * ft; }
    { const float fs = ctx.fpr[3]; const float ft = ctx.fpr[16]; if ((std::isinf(fs) && ft == 0.0f) || (std::isinf(ft) && fs == 0.0f)) ctx.set_fpr_bits(3, 0x7FC00000u); else ctx.fpr[3] = fs * ft; }
    ctx.fpr[0] = ctx.fpr[12] - ctx.fpr[0];
    ctx.set_gpr(5, ctx.gpr[14] + ctx.gpr[17]);
    ctx.set_gpr(6, ctx.gpr[14] + ctx.gpr[16]);
    ctx.set_gpr(7, ctx.gpr[14] + ctx.gpr[31]);
    ctx.set_gpr(8, ctx.gpr[14] + ctx.gpr[25]);
    ctx.set_gpr(15, ctx.gpr[15] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(14, ctx.gpr[14] + static_cast<std::uint32_t>(4));
    rt.memory().aot_store32(ctx.gpr[12] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[7]));
    rt.memory().aot_store32(ctx.gpr[13] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[4]));
    rt.memory().aot_store32(ctx.gpr[9] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[5]));
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[6]));
    rt.memory().aot_store32(ctx.gpr[11] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[8]));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[1]));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(257));
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[2]));
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[3]));
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[11]));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[9]));
    rt.memory().aot_store32(ctx.gpr[7] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[10]));
    { const bool branch_taken = ctx.gpr[15] != ctx.gpr[2];
    rt.memory().aot_store32(ctx.gpr[8] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
      if (branch_taken) {
          goto L_08B19A54;
      }
      goto L_08B19B78;
    }
L_08B19B78:
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(23, ctx.gpr[2] + static_cast<std::uint32_t>(21620));
    ctx.set_gpr(17, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(22, 0u + static_cast<std::uint32_t>(2));
    ctx.set_gpr(21, 0u + static_cast<std::uint32_t>(16));
    goto L_08B19B8C;
L_08B19B8C:
    ctx.set_gpr(2, 2235u << 16u);
    ctx.set_gpr(4, ctx.gpr[2] + static_cast<std::uint32_t>(-27372));
    ctx.set_gpr(31, 0x08B19B9Cu);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(219));
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19B9Cu) goto L_08B19B9C;
    return;
L_08B19B9C:
    { const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[17])) * static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[17])); ctx.lo = static_cast<std::uint32_t>(product); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(product) >> 32u); }
    ctx.set_gpr(5, 4u << 16u);
    ctx.set_gpr(2, ctx.lo);
    ctx.set_gpr(19, ctx.gpr[2] << 2u);
    ctx.set_gpr(31, 0x08B19BB4u);
    ctx.set_gpr(4, ctx.gpr[19] + 0u);
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19BB4u) goto L_08B19BB4;
    return;
L_08B19BB4:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(16, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B19D50;
      }
      goto L_08B19BBC;
    }
L_08B19BBC:
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(6, ctx.gpr[19] + 0u);
    ctx.set_gpr(31, 0x08B19BD0u);
    ctx.set_gpr(18, ctx.gpr[17] + static_cast<std::uint32_t>(-1));
    ctx.pc = 0x08B73CF0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19BD0u) goto L_08B19BD0;
    return;
L_08B19BD0:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(4));
    ctx.set_gpr(31, 0x08B19BE0u);
    ctx.set_gpr(6, ctx.gpr[18] + 0u);
    ctx.pc = 0x08B19890u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19BE0u) goto L_08B19BE0;
    return;
L_08B19BE0:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B19DA4;
      }
      goto L_08B19BE8;
    }
L_08B19BE8:
    ctx.set_gpr(2, ctx.gpr[2] + ctx.gpr[21]);
    ctx.set_gpr(31, 0x08B19BF4u);
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(-8)));
    ctx.pc = 0x08A9B5B0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19BF4u) goto L_08B19BF4;
    return;
L_08B19BF4:
    ctx.set_gpr(4, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(2));
    ctx.set_gpr(31, 0x08B19C04u);
    ctx.set_gpr(6, ctx.gpr[17] + 0u);
    ctx.pc = 0x08B19890u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19C04u) goto L_08B19C04;
    return;
L_08B19C04:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(4, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B19DA4;
      }
      goto L_08B19C0C;
    }
L_08B19C0C:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[22]) < 0;
    ctx.set_gpr(5, ctx.gpr[22] + 0u);
      if (branch_taken) {
          goto L_08B19C34;
      }
      goto L_08B19C14;
    }
L_08B19C14:
    ctx.set_gpr(2, ctx.gpr[21] + static_cast<std::uint32_t>(-8));
    ctx.set_gpr(3, ctx.gpr[4] + ctx.gpr[2]);
    goto L_08B19C1C;
L_08B19C1C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[3] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(-2));
    ctx.set_gpr(2, 0u - ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[5]) >= 0;
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-8));
      if (branch_taken) {
          goto L_08B19C1C;
      }
      goto L_08B19C34;
    }
L_08B19C34:
    ctx.set_gpr(3, ctx.gpr[17] & 1u);
    ctx.set_gpr(12, 0u + static_cast<std::uint32_t>(2));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(-2));
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[17]) <= 0;
    if (ctx.gpr[3] == 0u) ctx.set_gpr(12, ctx.gpr[2]);
      if (branch_taken) {
          goto L_08B19CCC;
      }
      goto L_08B19C48;
    }
L_08B19C48:
    ctx.fpr[0] = std::bit_cast<float>(ctx.gpr[20]);
    ctx.set_gpr(11, ctx.gpr[4] + ctx.gpr[21]);
    ctx.set_gpr(9, 0u + 0u);
    ctx.fpr[1] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(0)));
    ctx.set_gpr(10, ctx.gpr[16] + 0u);
    ctx.set_gpr(13, ctx.gpr[17] << 2u);
    goto L_08B19C60;
L_08B19C60:
    ctx.set_gpr(8, rt.memory().aot_load32(ctx.gpr[11] + static_cast<std::uint32_t>(-4)));
    ctx.fpr[2] = std::bit_cast<float>(ctx.gpr[8]);
    ctx.fpr[0] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(2)));
    ctx.fpr[0] = ctx.fpr[0] / ctx.fpr[1];
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[9]) <= 0;
    rt.memory().aot_store32(ctx.gpr[10] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
      if (branch_taken) {
          goto L_08B19CBC;
      }
      goto L_08B19C78;
    }
L_08B19C78:
    ctx.set_gpr(6, ctx.gpr[4] + 0u);
    ctx.set_gpr(5, ctx.gpr[10] + 0u);
    ctx.set_gpr(7, 0u + static_cast<std::uint32_t>(1));
    goto L_08B19C84;
L_08B19C84:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(7, ctx.gpr[7] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(3, static_cast<std::int32_t>(ctx.gpr[9]) < static_cast<std::int32_t>(ctx.gpr[7]) ? 1u : 0u);
    { const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[8])) * static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[2])); ctx.lo = static_cast<std::uint32_t>(product); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(product) >> 32u); }
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(2, ctx.lo);
    { const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[12])) * static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[2])); ctx.lo = static_cast<std::uint32_t>(product); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(product) >> 32u); }
    ctx.set_gpr(2, ctx.lo);
    ctx.fpr[2] = std::bit_cast<float>(ctx.gpr[2]);
    ctx.fpr[0] = static_cast<float>(static_cast<std::int32_t>(ctx.fpr_bits(2)));
    ctx.fpr[0] = ctx.fpr[0] / ctx.fpr[1];
    rt.memory().aot_store32(ctx.gpr[5] + static_cast<std::uint32_t>(4), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B19C84;
      }
      goto L_08B19CBC;
    }
L_08B19CBC:
    ctx.set_gpr(9, ctx.gpr[9] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(10, ctx.gpr[10] + ctx.gpr[13]);
    { const bool branch_taken = ctx.gpr[9] != ctx.gpr[17];
    ctx.set_gpr(11, ctx.gpr[11] + static_cast<std::uint32_t>(-4));
      if (branch_taken) {
          goto L_08B19C60;
      }
      goto L_08B19CCC;
    }
L_08B19CCC:
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[16] + ctx.gpr[19]);
    ctx.set_gpr(7, 0u + 0u);
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[17]) <= 0;
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(-4), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
      if (branch_taken) {
          goto L_08B19D48;
      }
      goto L_08B19CE0;
    }
L_08B19CE0:
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[7]) < static_cast<std::int32_t>(ctx.gpr[18]) ? 1u : 0u);
    goto L_08B19CE4;
L_08B19CE4:
    if (ctx.gpr[2] == 0u) {
    ctx.set_gpr(7, ctx.gpr[7] + static_cast<std::uint32_t>(1));
        goto L_08B19D40;
    }
    goto L_08B19CEC;
L_08B19CEC:
    ctx.set_gpr(2, ctx.gpr[17] - ctx.gpr[7]);
    { const std::int64_t product = static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[17])) * static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[2])); ctx.lo = static_cast<std::uint32_t>(product); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(product) >> 32u); }
    ctx.set_gpr(6, ctx.gpr[18] + 0u);
    ctx.set_gpr(2, ctx.lo);
    ctx.lo = ctx.gpr[18];
    { const std::int64_t acc = static_cast<std::int64_t>((static_cast<std::uint64_t>(ctx.hi) << 32u) | static_cast<std::uint64_t>(ctx.lo)) + static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[7])) * static_cast<std::int64_t>(static_cast<std::int32_t>(ctx.gpr[17])); ctx.lo = static_cast<std::uint32_t>(acc); ctx.hi = static_cast<std::uint32_t>(static_cast<std::uint64_t>(acc) >> 32u); }
    ctx.set_gpr(2, ctx.gpr[2] - ctx.gpr[18]);
    ctx.set_gpr(2, ctx.gpr[2] << 2u);
    ctx.set_gpr(2, ctx.gpr[16] + ctx.gpr[2]);
    ctx.set_gpr(5, ctx.gpr[2] + static_cast<std::uint32_t>(-4));
    ctx.set_gpr(3, ctx.lo);
    ctx.set_gpr(3, ctx.gpr[3] << 2u);
    ctx.set_gpr(3, ctx.gpr[16] + ctx.gpr[3]);
    goto L_08B19D20;
L_08B19D20:
    ctx.fpr[0] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(6, ctx.gpr[6] + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[7]) < static_cast<std::int32_t>(ctx.gpr[6]) ? 1u : 0u);
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(0), std::bit_cast<std::uint32_t>(ctx.fpr[0]));
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(4));
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(3, ctx.gpr[3] + static_cast<std::uint32_t>(-4));
      if (branch_taken) {
          goto L_08B19D20;
      }
      goto L_08B19D3C;
    }
L_08B19D3C:
    ctx.set_gpr(7, ctx.gpr[7] + static_cast<std::uint32_t>(1));
    goto L_08B19D40;
L_08B19D40:
    { const bool branch_taken = ctx.gpr[7] != ctx.gpr[17];
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[7]) < static_cast<std::int32_t>(ctx.gpr[18]) ? 1u : 0u);
      if (branch_taken) {
          goto L_08B19CE4;
      }
      goto L_08B19D48;
    }
L_08B19D48:
    ctx.set_gpr(31, 0x08B19D50u);
    // nop
    ctx.pc = 0x08A9B5B0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19D50u) goto L_08B19D50;
    return;
L_08B19D50:
    { const bool branch_taken = ctx.gpr[16] == 0u;
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08B19DB8;
      }
      goto L_08B19D58;
    }
L_08B19D58:
    ctx.set_gpr(17, ctx.gpr[17] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, 0u + static_cast<std::uint32_t>(11));
    ctx.set_gpr(23, ctx.gpr[23] + static_cast<std::uint32_t>(4));
    ctx.set_gpr(22, ctx.gpr[22] + static_cast<std::uint32_t>(1));
    { const bool branch_taken = ctx.gpr[17] != ctx.gpr[2];
    ctx.set_gpr(21, ctx.gpr[21] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B19B8C;
      }
      goto L_08B19D70;
    }
L_08B19D70:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(36)));
    goto L_08B19D74;
L_08B19D74:
    ctx.set_gpr(30, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(32)));
    ctx.set_gpr(23, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(28)));
    ctx.set_gpr(22, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(21, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(20, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(19, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.fpr[20] = std::bit_cast<float>(rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(48)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(64));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B19DA4:
    ctx.set_gpr(4, ctx.gpr[16] + 0u);
    ctx.set_gpr(31, 0x08B19DB0u);
    ctx.set_gpr(16, 0u + 0u);
    ctx.pc = 0x08A9B5B0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19DB0u) goto L_08B19DB0;
    return;
L_08B19DB0:
    { const bool branch_taken = ctx.gpr[16] != 0u;
    rt.memory().aot_store32(ctx.gpr[23] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
      if (branch_taken) {
          goto L_08B19D58;
      }
      goto L_08B19DB8;
    }
L_08B19DB8:
    ctx.set_gpr(3, ctx.gpr[17] + static_cast<std::uint32_t>(-1));
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[3]) < 4 ? 1u : 0u);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(36)));
      if (branch_taken) {
          goto L_08B19D74;
      }
      goto L_08B19DC8;
    }
L_08B19DC8:
    ctx.set_gpr(2, 2272u << 16u);
    ctx.set_gpr(3, ctx.gpr[3] << 2u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(21604));
    ctx.set_gpr(18, ctx.gpr[17] + static_cast<std::uint32_t>(-4));
    ctx.set_gpr(16, ctx.gpr[3] + ctx.gpr[2]);
    ctx.set_gpr(17, 0u + 0u);
    goto L_08B19DE0;
L_08B19DE0:
    ctx.set_gpr(4, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(31, 0x08B19DECu);
    ctx.set_gpr(17, ctx.gpr[17] + static_cast<std::uint32_t>(1));
    ctx.pc = 0x08A9B5B0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B19DECu) goto L_08B19DEC;
    return;
L_08B19DEC:
    rt.memory().aot_store32(ctx.gpr[16] + static_cast<std::uint32_t>(0), 0u);
    { const bool branch_taken = ctx.gpr[18] != ctx.gpr[17];
    ctx.set_gpr(16, ctx.gpr[16] + static_cast<std::uint32_t>(-4));
      if (branch_taken) {
          goto L_08B19DE0;
      }
      goto L_08B19DF8;
    }
L_08B19DF8:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(36)));
    goto L_08B19D74;
}

void sub_08B19928(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B19928_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B6094C[40] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 3, 4, 0, 5, 0,
    6, 0, 0, 0, 0, 7, 0, 8,
};
void sub_08B6094C_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B6094Cu;
        entry_id = (entry_delta < 160u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B6094C[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B6094C;
    case 2u: goto L_08B6099C;
    case 3u: goto L_08B609B8;
    case 4u: goto L_08B609BC;
    case 5u: goto L_08B609C4;
    case 6u: goto L_08B609CC;
    case 7u: goto L_08B609E0;
    case 8u: goto L_08B609E8;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B6094C:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-32));
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(16));
    ctx.set_gpr(9, ctx.gpr[7] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(16), ctx.gpr[16]);
    ctx.set_gpr(12, ctx.gpr[4] + 0u);
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(4, 2272u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[3]);
    ctx.set_gpr(3, 0u + static_cast<std::uint32_t>(1));
    ctx.set_gpr(10, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(29344), 0u);
    ctx.set_gpr(11, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(4, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(20), ctx.gpr[31]);
    ctx.set_gpr(7, ctx.gpr[29] + 0u);
    ctx.set_gpr(5, 0u + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[6]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[9]);
    { const bool branch_taken = ctx.gpr[12] == ctx.gpr[3];
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[8]);
      if (branch_taken) {
          goto L_08B609E0;
      }
      goto L_08B6099C;
    }
L_08B6099C:
    ctx.set_gpr(16, 2272u << 16u);
    ctx.set_gpr(8, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[8] + static_cast<std::uint32_t>(32)));
    ctx.set_gpr(4, ctx.gpr[10] + 0u);
    ctx.set_gpr(5, 0u + 0u);
    ctx.set_gpr(31, 0x08B609B8u);
    ctx.set_gpr(7, ctx.gpr[29] + 0u);
    ctx.pc = 0x08B7FE8Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B609B8u) goto L_08B609B8;
    return;
L_08B609B8:
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    goto L_08B609BC;
L_08B609BC:
    { const bool branch_taken = static_cast<std::int32_t>(ctx.gpr[2]) < 0;
    ctx.set_gpr(3, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B609CC;
      }
      goto L_08B609C4;
    }
L_08B609C4:
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(28), ctx.gpr[2]);
    ctx.set_gpr(3, 0u + 0u);
    goto L_08B609CC;
L_08B609CC:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(20)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(16)));
    ctx.set_gpr(2, ctx.gpr[3] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(32));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B609E0:
    ctx.set_gpr(31, 0x08B609E8u);
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[11] + static_cast<std::uint32_t>(32)));
    ctx.pc = 0x08B7FE6Cu;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B609E8u) goto L_08B609E8;
    return;
L_08B609E8:
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(29352));
    goto L_08B609BC;
}

void sub_08B6094C(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B6094C_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B63B50[12] = {
    1, 0, 0, 0, 0, 0, 2, 3, 0, 4, 0, 5,
};
void sub_08B63B50_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B63B50u;
        entry_id = (entry_delta < 48u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B63B50[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B63B50;
    case 2u: goto L_08B63B68;
    case 3u: goto L_08B63B6C;
    case 4u: goto L_08B63B74;
    case 5u: goto L_08B63B7C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B63B50:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[31]);
    ctx.set_gpr(4, ctx.gpr[4] & 65535u);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[5] + static_cast<std::uint32_t>(4)));
    { const bool branch_taken = ctx.gpr[2] != 0u;
    ctx.set_gpr(5, ctx.gpr[6] + 0u);
      if (branch_taken) {
          goto L_08B63B74;
      }
      goto L_08B63B68;
    }
L_08B63B68:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    goto L_08B63B6C;
L_08B63B6C:
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08B63B74:
    jump_target = ctx.gpr[2];
    ctx.set_gpr(31, 0x08B63B7Cu);
    // nop
    ctx.pc = jump_target;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B63B7Cu) goto L_08B63B7C;
    return;
L_08B63B7C:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    goto L_08B63B6C;
}

void sub_08B63B50(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B63B50_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08B19890[32] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 4, 0, 0, 0, 5, 0, 6, 0, 0, 0, 0, 0, 0, 0, 7,
};
void sub_08B19890_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08B19890u;
        entry_id = (entry_delta < 128u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08B19890[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08B19890;
    case 2u: goto L_08B198C0;
    case 3u: goto L_08B198CC;
    case 4u: goto L_08B198D4;
    case 5u: goto L_08B198E4;
    case 6u: goto L_08B198EC;
    case 7u: goto L_08B1990C;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08B19890:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[17]);
    ctx.set_gpr(17, ctx.gpr[4] + 0u);
    ctx.set_gpr(4, 2235u << 16u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(8), ctx.gpr[18]);
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(-27372));
    ctx.set_gpr(18, ctx.gpr[5] + 0u);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(192));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(12), ctx.gpr[31]);
    ctx.set_gpr(31, 0x08B198C0u);
    ctx.set_gpr(16, ctx.gpr[6] + 0u);
    ctx.pc = 0x08A9AD14u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B198C0u) goto L_08B198C0;
    return;
L_08B198C0:
    ctx.set_gpr(4, ctx.gpr[16] << 2u);
    ctx.set_gpr(31, 0x08B198CCu);
    ctx.set_gpr(5, 4u << 16u);
    ctx.pc = 0x08A9B888u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08B198CCu) goto L_08B198CC;
    return;
L_08B198CC:
    { const bool branch_taken = ctx.gpr[2] == 0u;
    ctx.set_gpr(6, ctx.gpr[2] + 0u);
      if (branch_taken) {
          goto L_08B1990C;
      }
      goto L_08B198D4;
    }
L_08B198D4:
    ctx.set_gpr(2, static_cast<std::int32_t>(ctx.gpr[16]) < 3 ? 1u : 0u);
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(0), ctx.gpr[17]);
    { const bool branch_taken = ctx.gpr[2] != 0u;
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(4), ctx.gpr[18]);
      if (branch_taken) {
          goto L_08B1990C;
      }
      goto L_08B198E4;
    }
L_08B198E4:
    ctx.set_gpr(4, ctx.gpr[6] + 0u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(2));
    goto L_08B198EC;
L_08B198EC:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(5, ctx.gpr[5] + static_cast<std::uint32_t>(1));
    ctx.set_gpr(2, ctx.gpr[2] << 2u);
    ctx.set_gpr(2, ctx.gpr[2] - ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(8), ctx.gpr[2]);
    { const bool branch_taken = ctx.gpr[16] != ctx.gpr[5];
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(4));
      if (branch_taken) {
          goto L_08B198EC;
      }
      goto L_08B1990C;
    }
L_08B1990C:
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(12)));
    ctx.set_gpr(18, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(17, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(2, ctx.gpr[6] + 0u);
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
}

void sub_08B19890(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08B19890_entry(rt, ctx, 0u, aot_mem);
}

static const std::uint16_t kEntryIds_sub_08A9B3F4[66] = {
    1, 0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0,
    6, 0, 0, 7, 0, 0, 8, 0, 0, 9, 0, 0, 0, 0, 10, 0, 0, 0, 0, 0, 11, 0, 0, 12, 0, 0, 13, 0, 14, 0, 0, 15,
    0, 16,
};
void sub_08A9B3F4_entry(Runtime &rt, AllegrexContext &ctx, std::uint16_t direct_entry_id, GuestMemory::AotFastView &aot_mem) {
    std::uint32_t jump_target = 0u;
    std::uint32_t local_transfers = 0u;
    std::uint32_t local_pc = ctx.pc;
    std::uint32_t entry_id = direct_entry_id;
LOCAL_DISPATCH:
    {
    if (entry_id == 0u) {
        const std::uint32_t entry_delta = local_pc - 0x08A9B3F4u;
        entry_id = (entry_delta < 264u && (entry_delta & 3u) == 0u) ? kEntryIds_sub_08A9B3F4[entry_delta >> 2u] : 0u;
    }
    switch (entry_id) {
    case 1u: goto L_08A9B3F4;
    case 2u: goto L_08A9B40C;
    case 3u: goto L_08A9B410;
    case 4u: goto L_08A9B43C;
    case 5u: goto L_08A9B468;
    case 6u: goto L_08A9B474;
    case 7u: goto L_08A9B480;
    case 8u: goto L_08A9B48C;
    case 9u: goto L_08A9B498;
    case 10u: goto L_08A9B4AC;
    case 11u: goto L_08A9B4C4;
    case 12u: goto L_08A9B4D0;
    case 13u: goto L_08A9B4DC;
    case 14u: goto L_08A9B4E4;
    case 15u: goto L_08A9B4F0;
    case 16u: goto L_08A9B4F8;
    default:
        if (local_transfers == 0u) rt.unsupported(ctx.pc, 0u, "invalid internal function entry");
        else ctx.pc = local_pc;
        return;
    }
    }
L_08A9B3F4:
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(-16));
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(4), ctx.gpr[31]);
    rt.memory().aot_store32(ctx.gpr[29] + static_cast<std::uint32_t>(0), ctx.gpr[16]);
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(-4)));
    { const bool branch_taken = ctx.gpr[16] == 0u;
    ctx.set_gpr(4, 2234u << 16u);
      if (branch_taken) {
          goto L_08A9B4E4;
      }
      goto L_08A9B40C;
    }
L_08A9B40C:
    ctx.set_gpr(2, 2243u << 16u);
    goto L_08A9B410;
L_08A9B410:
    ctx.set_gpr(6, rt.memory().aot_load32(ctx.gpr[2] + static_cast<std::uint32_t>(4176)));
    ctx.set_gpr(5, rt.memory().aot_load8(ctx.gpr[16] + static_cast<std::uint32_t>(8)));
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(24)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(28)));
    ctx.set_gpr(4, static_cast<std::int32_t>(ctx.gpr[5]) < 8 ? 1u : 0u);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(-1));
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(24), ctx.gpr[2]);
    ctx.set_gpr(7, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(0)));
    ctx.set_gpr(3, ctx.gpr[3] - ctx.gpr[7]);
    { const bool branch_taken = ctx.gpr[4] == 0u;
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(28), ctx.gpr[3]);
      if (branch_taken) {
          goto L_08A9B468;
      }
      goto L_08A9B43C;
    }
L_08A9B43C:
    ctx.set_gpr(4, ctx.gpr[5] << 2u);
    ctx.set_gpr(2, rt.memory().aot_load16(ctx.gpr[16] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(4, ctx.gpr[4] + ctx.gpr[6]);
    ctx.set_gpr(5, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(56)));
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(88)));
    ctx.set_gpr(2, ctx.gpr[7] - ctx.gpr[2]);
    ctx.set_gpr(2, ctx.gpr[2] + static_cast<std::uint32_t>(-24));
    ctx.set_gpr(3, ctx.gpr[3] - ctx.gpr[2]);
    ctx.set_gpr(5, ctx.gpr[5] - ctx.gpr[7]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(88), ctx.gpr[3]);
    rt.memory().aot_store32(ctx.gpr[4] + static_cast<std::uint32_t>(56), ctx.gpr[5]);
    goto L_08A9B468;
L_08A9B468:
    ctx.set_gpr(3, rt.memory().aot_load32(ctx.gpr[16] + static_cast<std::uint32_t>(12)));
    { const bool branch_taken = ctx.gpr[3] == 0u;
    ctx.set_gpr(4, ctx.gpr[16] + static_cast<std::uint32_t>(12));
      if (branch_taken) {
          goto L_08A9B4C4;
      }
      goto L_08A9B474;
    }
L_08A9B474:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(4)));
    if (ctx.gpr[2] == 0u) {
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(4), ctx.gpr[3]);
        goto L_08A9B4DC;
    }
    goto L_08A9B480;
L_08A9B480:
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), ctx.gpr[2]);
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(4)));
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(0), ctx.gpr[3]);
    goto L_08A9B48C;
L_08A9B48C:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[6] + static_cast<std::uint32_t>(8)));
    { const bool branch_taken = ctx.gpr[4] == ctx.gpr[2];
    ctx.set_gpr(2, ctx.gpr[16] + 0u);
      if (branch_taken) {
          goto L_08A9B4AC;
      }
      goto L_08A9B498;
    }
L_08A9B498:
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(8), static_cast<std::uint16_t>(0u));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08A9B4AC:
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(8), 0u);
    rt.memory().aot_store16(ctx.gpr[16] + static_cast<std::uint32_t>(8), static_cast<std::uint16_t>(0u));
    ctx.set_gpr(31, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(4)));
    ctx.set_gpr(16, rt.memory().aot_load32(ctx.gpr[29] + static_cast<std::uint32_t>(0)));
    jump_target = ctx.gpr[31];
    ctx.set_gpr(29, ctx.gpr[29] + static_cast<std::uint32_t>(16));
    local_pc = jump_target;
    if (++local_transfers < 2048u) { entry_id = 0u; goto LOCAL_DISPATCH; }
    ctx.pc = jump_target;
    return;
L_08A9B4C4:
    ctx.set_gpr(2, rt.memory().aot_load32(ctx.gpr[4] + static_cast<std::uint32_t>(4)));
    if (ctx.gpr[2] == 0u) {
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(4), 0u);
        goto L_08A9B4F8;
    }
    goto L_08A9B4D0;
L_08A9B4D0:
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(0), ctx.gpr[2]);
    rt.memory().aot_store32(ctx.gpr[2] + static_cast<std::uint32_t>(0), 0u);
    goto L_08A9B48C;
L_08A9B4DC:
    rt.memory().aot_store32(ctx.gpr[3] + static_cast<std::uint32_t>(4), 0u);
    goto L_08A9B48C;
L_08A9B4E4:
    ctx.set_gpr(4, ctx.gpr[4] + static_cast<std::uint32_t>(31848));
    ctx.set_gpr(31, 0x08A9B4F0u);
    ctx.set_gpr(5, 0u + static_cast<std::uint32_t>(978));
    ctx.pc = 0x08A9C6F0u;
    if (rt.invoke_chained_call(ctx, &aot_mem) && ctx.pc == 0x08A9B4F0u) goto L_08A9B4F0;
    return;
L_08A9B4F0:
    ctx.set_gpr(2, 2243u << 16u);
    goto L_08A9B410;
L_08A9B4F8:
    rt.memory().aot_store32(ctx.gpr[6] + static_cast<std::uint32_t>(0), 0u);
    goto L_08A9B48C;
}

void sub_08A9B3F4(Runtime &rt, AllegrexContext &ctx) {
    auto aot_mem = rt.memory().aot_fast_view();
    sub_08A9B3F4_entry(rt, ctx, 0u, aot_mem);
}

static void import_0(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(0u, "sceAudio", 0x01562BA3u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_1(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(1u, "sceAudio", 0x2D53F36Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_2(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(2u, "sceAudio", 0x43196845u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_3(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(3u, "sceAudio", 0x136CAF51u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_4(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(4u, "sceAudio", 0x13F592BCu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_5(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(5u, "sceAudio", 0x5EC81C55u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_6(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(6u, "sceAudio", 0x6FC46853u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_7(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(7u, "sceAudio", 0x95FD0C2Du, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_8(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(8u, "sceAudio", 0xB011922Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_9(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(9u, "sceAudio", 0xB7E1D8E7u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_10(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(10u, "sceAudio", 0xCB2E439Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_11(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(11u, "sceAudio", 0xE2D56B2Du, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_12(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(12u, "sceSasCore", 0x019B25EBu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_13(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(13u, "sceSasCore", 0x07F58C24u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_14(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(14u, "sceSasCore", 0x267A6DD2u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_15(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(15u, "sceSasCore", 0x2C8E6AB3u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_16(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(16u, "sceSasCore", 0x33D4AB37u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_17(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(17u, "sceSasCore", 0x42778A9Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_18(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(18u, "sceSasCore", 0x440CA7D8u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_19(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(19u, "sceSasCore", 0x50A14DFCu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_20(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(20u, "sceSasCore", 0x5F9529F6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_21(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(21u, "sceSasCore", 0x68A46B95u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_22(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(22u, "sceSasCore", 0x74AE582Au, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_23(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(23u, "sceSasCore", 0x76F01ACAu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_24(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(24u, "sceSasCore", 0x787D04D5u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_25(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(25u, "sceSasCore", 0x99944089u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_26(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(26u, "sceSasCore", 0x9EC3676Au, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_27(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(27u, "sceSasCore", 0xA0CF2FA4u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_28(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(28u, "sceSasCore", 0xA3589D81u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_29(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(29u, "sceSasCore", 0xAD84D37Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_30(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(30u, "sceSasCore", 0xB7660A23u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_31(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(31u, "sceSasCore", 0xBD11B7C2u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_32(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(32u, "sceSasCore", 0xCBCD4F79u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_33(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(33u, "sceSasCore", 0xD1E0A01Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_34(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(34u, "sceSasCore", 0xD5A229C9u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_35(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(35u, "sceSasCore", 0xE175EF66u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_36(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(36u, "sceSasCore", 0xE1CD9561u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_37(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(37u, "sceSasCore", 0xE855BF76u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_38(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(38u, "sceSasCore", 0xF983B186u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_39(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(39u, "sceLibFont", 0x099EF33Cu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_40(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(40u, "sceLibFont", 0x0DA7535Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_41(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(41u, "sceLibFont", 0x27F6E642u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_42(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(42u, "sceLibFont", 0x3AEA8CB6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_43(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(43u, "sceLibFont", 0x574B6FBCu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_44(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(44u, "sceLibFont", 0x67F17ED7u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_45(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(45u, "sceLibFont", 0x74B21701u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_46(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(46u, "sceLibFont", 0x980F4895u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_47(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(47u, "sceLibFont", 0xA834319Du, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_48(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(48u, "sceLibFont", 0xBC75D85Bu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_49(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(49u, "sceLibFont", 0xDCC80C2Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_50(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(50u, "sceLibFont", 0xF8F0752Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_51(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(51u, "sceDisplay", 0x0E20F177u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_52(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(52u, "sceDisplay", 0x289D82FEu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_53(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(53u, "sceDisplay", 0x46F186C3u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_54(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(54u, "sceDisplay", 0x77ED8B3Au, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_55(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(55u, "sceDisplay", 0x984C27E7u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_56(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(56u, "sceDisplay", 0x9C6EAAD7u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_57(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(57u, "sceDisplay", 0xDBA6C4C4u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_58(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(58u, "sceDisplay", 0xEEDA2E54u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_59(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(59u, "sceGe_user", 0x03444EB4u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_60(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(60u, "sceGe_user", 0x05DB22CEu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_61(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(61u, "sceGe_user", 0x1C0D95A6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_62(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(62u, "sceGe_user", 0x1F6752ADu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_63(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(63u, "sceGe_user", 0x4C06E472u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_64(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(64u, "sceGe_user", 0xA4FC06A4u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_65(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(65u, "sceGe_user", 0xAB49E76Au, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_66(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(66u, "sceGe_user", 0xB287BD61u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_67(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(67u, "sceGe_user", 0xB448EC0Du, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_68(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(68u, "sceGe_user", 0xDC93CFEFu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_69(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(69u, "sceGe_user", 0xE0D68148u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_70(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(70u, "sceGe_user", 0xE47E40E4u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_71(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(71u, "sceCtrl", 0x1F4011E6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_72(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(72u, "sceCtrl", 0x1F803938u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_73(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(73u, "sceCtrl", 0xA7144800u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_74(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(74u, "IoFileMgrForUser", 0x42EC03ACu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_75(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(75u, "IoFileMgrForUser", 0x54F5FB11u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_76(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(76u, "IoFileMgrForUser", 0x6A638D83u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_77(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(77u, "IoFileMgrForUser", 0x71B19E77u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_78(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(78u, "IoFileMgrForUser", 0x810C4BC3u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_79(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(79u, "IoFileMgrForUser", 0x89AA9906u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_80(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(80u, "IoFileMgrForUser", 0xA0B5A7C2u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_81(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(81u, "IoFileMgrForUser", 0xAB96437Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_82(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(82u, "IoFileMgrForUser", 0xACE946E8u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_83(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(83u, "IoFileMgrForUser", 0xB293727Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_84(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(84u, "IoFileMgrForUser", 0xB29DDF9Cu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_85(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(85u, "IoFileMgrForUser", 0xE23EEC33u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_86(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(86u, "IoFileMgrForUser", 0xE3EB004Cu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_87(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(87u, "IoFileMgrForUser", 0xEB092469u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_88(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(88u, "IoFileMgrForUser", 0xFF5940B6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_89(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(89u, "IoFileMgrForUser", 0x109F50BCu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_90(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(90u, "IoFileMgrForUser", 0x27EB27B8u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_91(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(91u, "IoFileMgrForUser", 0x3251EA56u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_92(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(92u, "IoFileMgrForUser", 0x35DBD746u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_93(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(93u, "IoFileMgrForUser", 0x779103A0u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_94(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(94u, "Kernel_Library", 0x092968F4u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_95(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(95u, "Kernel_Library", 0xA089ECA4u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_96(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(96u, "Kernel_Library", 0xBEA46419u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_97(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(97u, "Kernel_Library", 0x15B6446Bu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_98(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(98u, "Kernel_Library", 0x1839852Au, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_99(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(99u, "Kernel_Library", 0x293B45B8u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_100(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(100u, "Kernel_Library", 0x5F10D406u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_101(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(101u, "LoadExecForUser", 0x05572A5Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_102(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(102u, "LoadExecForUser", 0x4AC57943u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_103(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(103u, "ModuleMgrForUser", 0x2E0911AAu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_104(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(104u, "ModuleMgrForUser", 0xD1FF982Au, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_105(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(105u, "ModuleMgrForUser", 0xD8B73127u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_106(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(106u, "ModuleMgrForUser", 0x50F0C1ECu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_107(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(107u, "ModuleMgrForUser", 0x977DE386u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_108(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(108u, "ModuleMgrForUser", 0xF0A26395u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_109(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(109u, "ModuleMgrForUser", 0x8F2DF740u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_110(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(110u, "SysMemUserForUser", 0xF77D77CBu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_111(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(111u, "SysMemUserForUser", 0x35669D4Cu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_112(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(112u, "SysMemUserForUser", 0x13A5ABEFu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_113(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(113u, "SysMemUserForUser", 0x237DBD4Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_114(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(114u, "SysMemUserForUser", 0x9D9A5BA1u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_115(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(115u, "SysMemUserForUser", 0xB6D61D02u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_116(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(116u, "ThreadManForUser", 0xCEADEB47u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_117(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(117u, "ThreadManForUser", 0xD59EAD2Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_118(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(118u, "ThreadManForUser", 0xD6DA4BA1u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_119(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(119u, "ThreadManForUser", 0x1FB15A32u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_120(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(120u, "ThreadManForUser", 0xE81CAF8Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_121(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(121u, "ThreadManForUser", 0xEA748E31u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_122(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(122u, "ThreadManForUser", 0xEDBA5844u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_123(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(123u, "ThreadManForUser", 0xEF9E4C70u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_124(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(124u, "ThreadManForUser", 0xF475845Du, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_125(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(125u, "ThreadManForUser", 0xF8170FBEu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_126(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(126u, "ThreadManForUser", 0x278C0DF5u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_127(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(127u, "ThreadManForUser", 0x28B6489Cu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_128(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(128u, "ThreadManForUser", 0x349D6D6Cu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_129(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(129u, "ThreadManForUser", 0x369ED59Du, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_130(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(130u, "ThreadManForUser", 0x3F53E640u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_131(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(131u, "ThreadManForUser", 0x402FCF22u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_132(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(132u, "ThreadManForUser", 0x446D8DE6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_133(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(133u, "ThreadManForUser", 0x4E3A1105u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_134(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(134u, "ThreadManForUser", 0x55C20A00u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_135(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(135u, "ThreadManForUser", 0x58B1F937u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_136(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(136u, "ThreadManForUser", 0x60107536u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_137(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(137u, "ThreadManForUser", 0x68DA9E36u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_138(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(138u, "ThreadManForUser", 0x6B30100Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_139(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(139u, "ThreadManForUser", 0x6D212BACu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_140(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(140u, "ThreadManForUser", 0x71BC9871u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_141(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(141u, "ThreadManForUser", 0x75156E8Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_142(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(142u, "ThreadManForUser", 0x812346E4u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_143(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(143u, "ThreadManForUser", 0x17C1684Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_144(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(144u, "ThreadManForUser", 0x94AA61EEu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_145(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(145u, "ThreadManForUser", 0x9944F31Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_146(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(146u, "ThreadManForUser", 0x9ACE131Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_147(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(147u, "ThreadManForUser", 0x9FA03CD3u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_148(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(148u, "ThreadManForUser", 0xAA73C935u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_149(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(149u, "ThreadManForUser", 0x19CFF145u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_150(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(150u, "ThreadManForUser", 0xB011B11Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_151(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(151u, "ThreadManForUser", 0xB7D098C6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_152(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(152u, "ThreadManForUser", 0x328C546Au, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_153(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(153u, "ThreadManForUser", 0x5BF4DD27u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_154(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(154u, "UtilsForUser", 0x79D1C3FAu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_155(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(155u, "UtilsForUser", 0xB435DEC5u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_156(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(156u, "UtilsForUser", 0x34B9FA9Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_157(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(157u, "UtilsForUser", 0x3EE30821u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_158(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(158u, "UtilsForUser", 0x71EC4271u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_159(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(159u, "UtilsForUser", 0x91E4F6A7u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_160(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(160u, "UtilsForUser", 0x27CC57F0u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_161(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(161u, "sceSuspendForUser", 0x090CCB3Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_162(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(162u, "sceMpeg", 0x0E3C2E9Du, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_163(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(163u, "sceMpeg", 0x13407F13u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_164(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(164u, "sceMpeg", 0x167AFD9Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_165(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(165u, "sceMpeg", 0x21FF80E4u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_166(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(166u, "sceMpeg", 0x37295ED8u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_167(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(167u, "sceMpeg", 0x42560F23u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_168(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(168u, "sceMpeg", 0x4571CC64u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_169(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(169u, "sceMpeg", 0x591A4AA2u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_170(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(170u, "sceMpeg", 0x606A4649u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_171(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(171u, "sceMpeg", 0x611E9E11u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_172(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(172u, "sceMpeg", 0x682A619Bu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_173(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(173u, "sceMpeg", 0x707B7629u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_174(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(174u, "sceMpeg", 0x740FCCD1u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_175(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(175u, "sceMpeg", 0x800C44DFu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_176(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(176u, "sceMpeg", 0x874624D6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_177(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(177u, "sceMpeg", 0xA780CF7Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_178(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(178u, "sceMpeg", 0xB240A59Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_179(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(179u, "sceMpeg", 0xB5F6DC87u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_180(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(180u, "sceMpeg", 0xC132E22Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_181(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(181u, "sceMpeg", 0xCEB870B1u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_182(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(182u, "sceMpeg", 0xD7A29F46u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_183(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(183u, "sceMpeg", 0xD8C5F121u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_184(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(184u, "sceMpeg", 0xE1CE83A7u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_185(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(185u, "sceMpeg", 0xF8DCB679u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_186(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(186u, "sceMpeg", 0xFE246728u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_187(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(187u, "sceUmdUser", 0x20628E6Fu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_188(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(188u, "sceUmdUser", 0x46EBB729u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_189(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(189u, "sceUmdUser", 0x4A9E5E29u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_190(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(190u, "sceUmdUser", 0x6AF9B50Au, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_191(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(191u, "sceUmdUser", 0x6B4A146Cu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_192(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(192u, "sceUmdUser", 0x8EF08FCEu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_193(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(193u, "sceUmdUser", 0xAEE7404Du, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_194(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(194u, "sceUmdUser", 0xBD2BDE07u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_195(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(195u, "sceUmdUser", 0xC6183D47u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_196(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(196u, "sceDmac", 0x617F3FE6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_197(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(197u, "scePower", 0x04B7766Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_198(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(198u, "scePower", 0xDFA8BAF8u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_199(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(199u, "scePower", 0xEBD177D6u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_200(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(200u, "sceUtility", 0x2A2B3DE0u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_201(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(201u, "sceUtility", 0x2AD8E239u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_202(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(202u, "sceUtility", 0x50C4CD57u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_203(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(203u, "sceUtility", 0x67AF3428u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_204(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(204u, "sceUtility", 0x8874DBE0u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_205(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(205u, "sceUtility", 0x95FC253Bu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_206(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(206u, "sceUtility", 0x9790B33Cu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_207(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(207u, "sceUtility", 0x9A1C91D7u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_208(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(208u, "sceUtility", 0xD4B95FFBu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_209(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(209u, "sceUtility", 0xE49BFE92u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_210(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(210u, "sceImpose", 0x36AA6E91u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_211(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(211u, "scesupPreAcc", 0x110E318Bu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_212(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(212u, "scesupPreAcc", 0x2EC3F4D9u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_213(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(213u, "scesupPreAcc", 0x348BA3E2u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_214(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(214u, "scesupPreAcc", 0x7ADA3927u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_215(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(215u, "scesupPreAcc", 0x86DEBD66u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_216(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(216u, "scesupPreAcc", 0xA0EAF444u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_217(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(217u, "scesupPreAcc", 0xB03FF882u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_218(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(218u, "StdioForUser", 0x172D316Eu, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_219(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(219u, "StdioForUser", 0xA6BAB2E9u, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

static void import_220(Runtime &rt, AllegrexContext &ctx) {
    const RuntimeExecutionContextToken caller_context = capture_runtime_execution_context();
    const std::uint32_t import_pc = ctx.pc;
    const std::uint32_t return_address = ctx.gpr[31];
    rt.invoke_import_cached(220u, "StdioForUser", 0xF78BA90Au, ctx);
    if (!rt.stopped() && runtime_execution_context_matches(caller_context) &&
        ctx.pc == import_pc) ctx.pc = return_address;
}

void register_generated_functions(Runtime &runtime) {
    runtime.register_function(0x08804000u, &sub_08804000, "sub_08804000");
    runtime.register_function(0x08804010u, &sub_08804000, "sub_08804000");
    runtime.register_function(0x08804018u, &sub_08804000, "sub_08804000");
    runtime.register_function(0x08804024u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804028u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x0880402Cu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804030u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804034u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804038u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804040u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804044u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804048u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x0880404Cu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804054u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804058u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804060u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804064u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804068u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804070u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804074u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804078u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804080u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804084u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x0880408Cu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804090u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804094u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804098u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x0880409Cu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040A0u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040A8u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040ACu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040B0u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040B4u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040B8u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040BCu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040C0u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040C4u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040CCu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040D4u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040D8u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040DCu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040E0u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040E8u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040ECu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040F4u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040FCu, &sub_08804024, "sub_08804024");
    runtime.register_function(0x08804100u, &sub_08804024, "sub_08804024");
    runtime.register_function(0x088040A8u, &sub_088040A8, "sub_088040A8");
    runtime.register_function(0x088040CCu, &sub_088040A8, "sub_088040A8");
    runtime.register_function(0x088040D4u, &sub_088040A8, "sub_088040A8");
    runtime.register_function(0x088040E8u, &sub_088040A8, "sub_088040A8");
    runtime.register_function(0x088040F4u, &sub_088040A8, "sub_088040A8");
    runtime.register_function(0x088040FCu, &sub_088040A8, "sub_088040A8");
    runtime.register_function(0x08804108u, &module_start, "module_start");
    runtime.register_function(0x0880413Cu, &module_start, "module_start");
    runtime.register_function(0x08804148u, &module_start, "module_start");
    runtime.register_function(0x08804150u, &module_start, "module_start");
    runtime.register_function(0x08804158u, &module_start, "module_start");
    runtime.register_function(0x08804168u, &module_start, "module_start");
    runtime.register_function(0x0880417Cu, &module_start, "module_start");
    runtime.register_function(0x08804188u, &module_start, "module_start");
    runtime.register_function(0x08804198u, &module_start, "module_start");
    runtime.register_function(0x0880419Cu, &module_start, "module_start");
    runtime.register_function(0x088041ACu, &module_start, "module_start");
    runtime.register_function(0x088041B4u, &module_start, "module_start");
    runtime.register_function(0x088041C4u, &module_start, "module_start");
    runtime.register_function(0x088041D0u, &module_start, "module_start");
    runtime.register_function(0x088041E0u, &module_start, "module_start");
    runtime.register_function(0x088041F0u, &module_start, "module_start");
    runtime.register_function(0x08804210u, &module_start, "module_start");
    runtime.register_function(0x0880421Cu, &user_main, "user_main");
    runtime.register_function(0x08804254u, &user_main, "user_main");
    runtime.register_function(0x08804260u, &user_main, "user_main");
    runtime.register_function(0x08804268u, &user_main, "user_main");
    runtime.register_function(0x0880426Cu, &user_main, "user_main");
    runtime.register_function(0x08804274u, &user_main, "user_main");
    runtime.register_function(0x0880427Cu, &user_main, "user_main");
    runtime.register_function(0x08804288u, &user_main, "user_main");
    runtime.register_function(0x088042A4u, &user_main, "user_main");
    runtime.register_function(0x088042ACu, &user_main, "user_main");
    runtime.register_function(0x088042C4u, &user_main, "user_main");
    runtime.register_function(0x088042CCu, &user_main, "user_main");
    runtime.register_function(0x088042D8u, &user_main, "user_main");
    runtime.register_function(0x088042E4u, &user_main, "user_main");
    runtime.register_function(0x088042ECu, &user_main, "user_main");
    runtime.register_function(0x088042FCu, &user_main, "user_main");
    runtime.register_function(0x08804304u, &user_main, "user_main");
    runtime.register_function(0x08804314u, &user_main, "user_main");
    runtime.register_function(0x08804320u, &user_main, "user_main");
    runtime.register_function(0x08804324u, &user_main, "user_main");
    runtime.register_function(0x0880432Cu, &user_main, "user_main");
    runtime.register_function(0x08804334u, &user_main, "user_main");
    runtime.register_function(0x0880433Cu, &user_main, "user_main");
    runtime.register_function(0x08804344u, &user_main, "user_main");
    runtime.register_function(0x0880434Cu, &user_main, "user_main");
    runtime.register_function(0x08804354u, &user_main, "user_main");
    runtime.register_function(0x0880437Cu, &user_main, "user_main");
    runtime.register_function(0x088043A4u, &user_main, "user_main");
    runtime.register_function(0x088043B4u, &user_main, "user_main");
    runtime.register_function(0x088043F8u, &user_main, "user_main");
    runtime.register_function(0x0880440Cu, &user_main, "user_main");
    runtime.register_function(0x088044D8u, &user_main, "user_main");
    runtime.register_function(0x08804538u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804570u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804588u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804590u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045A0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045A8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045B0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045B8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045C0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045C8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045D0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045E0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045ECu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088045F8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804600u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804608u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804614u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x0880461Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804624u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x0880462Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804634u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x0880463Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804644u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x0880464Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804660u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088046D8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804714u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804724u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804730u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804748u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804750u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804758u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804764u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804778u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804794u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088047A0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088047C8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088047D0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088047D8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088047DCu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088047E4u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088047F0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804804u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804810u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804818u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804830u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804848u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x0880485Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804880u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804888u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804890u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804894u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x0880489Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048A8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048B8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048C0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048C8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048D0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048D8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048E0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048E8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048F0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088048F8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804900u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804908u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804910u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804918u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804920u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804928u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804930u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804938u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804950u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804954u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x0880495Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804964u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x0880496Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804974u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804978u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x0880498Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049A0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049A8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049B0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049B8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049C0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049C8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049D0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049D8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049E0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049E8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x088049F4u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A00u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A08u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A10u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A18u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A20u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A28u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A30u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A38u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A40u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A48u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A50u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A58u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A60u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A68u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A70u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A78u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A80u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A88u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A90u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804A98u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AA0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AA8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AB0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AB8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AC0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AC8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AD0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AD8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AE0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AE8u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804AF0u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804B04u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804B0Cu, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08804B14u, &sub_08804538, "sub_08804538");
    runtime.register_function(0x08B4E6A0u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E6C4u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E6D4u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E6E0u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E6ECu, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E708u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E718u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E720u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E738u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E73Cu, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E744u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E75Cu, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E770u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E778u, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4E77Cu, &sub_08B4E6A0, "sub_08B4E6A0");
    runtime.register_function(0x08B4F714u, &sub_08B4F714, "sub_08B4F714");
    runtime.register_function(0x08B4F73Cu, &sub_08B4F714, "sub_08B4F714");
    runtime.register_function(0x08B4F740u, &sub_08B4F714, "sub_08B4F714");
    runtime.register_function(0x08B4F748u, &sub_08B4F714, "sub_08B4F714");
    runtime.register_function(0x08B4F754u, &sub_08B4F714, "sub_08B4F714");
    runtime.register_function(0x08B72DF8u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E18u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E20u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E30u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E40u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E5Cu, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E60u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E7Cu, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E88u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E90u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72E9Cu, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72EA8u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72EB0u, &sub_08B72DF8, "sub_08B72DF8");
    runtime.register_function(0x08B72F04u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72F38u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72F44u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72F4Cu, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72F58u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72F74u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72F84u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72F94u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72FA4u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72FB0u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72FBCu, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72FC8u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B72FD0u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B73000u, &sub_08B72F04, "sub_08B72F04");
    runtime.register_function(0x08B7300Cu, &sub_08B7300C, "sub_08B7300C");
    runtime.register_function(0x08B73014u, &sub_08B7300C, "sub_08B7300C");
    runtime.register_function(0x08B73020u, &sub_08B7300C, "sub_08B7300C");
    runtime.register_function(0x08B73028u, &sub_08B7300C, "sub_08B7300C");
    runtime.register_function(0x08B73030u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73044u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73088u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B730C8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B730D8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B730E4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B730ECu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B730F8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73110u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7313Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73170u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73174u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7318Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73198u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B731BCu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B731C4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B731D4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B731E0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B731ECu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B731F4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B731F8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7320Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73218u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73220u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73224u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73254u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73260u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7326Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73278u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73290u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B732A8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B732B0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B732B4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B732D8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7330Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73314u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7332Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73334u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7333Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73340u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73350u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73368u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73374u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7338Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73394u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7339Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B733A8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B733B4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B733C4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B733F4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B733FCu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73408u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73444u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73468u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73474u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7348Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7349Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B734A0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B734B0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B734B8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B734C0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B734C8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B734CCu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B734E0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B734E8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B734F0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7350Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73514u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73518u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73524u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73528u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73534u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7353Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7354Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73558u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7357Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7358Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B735B4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B735C0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B735CCu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B735D4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B735E0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B735ECu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B735F8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73608u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7360Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7361Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73628u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7362Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73648u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73650u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73658u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73668u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73670u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73680u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7368Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7369Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B736A4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B736ACu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B736BCu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B736C4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B736CCu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B736DCu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7371Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73724u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73730u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73740u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7376Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73774u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73780u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73790u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B737C4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B737D4u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B737E0u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B737ECu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B737F8u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73808u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73824u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7382Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73838u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73840u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73850u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73860u, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B7386Cu, &sub_08B73030, "sub_08B73030");
    runtime.register_function(0x08B73170u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7318Cu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73198u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B731BCu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B731C4u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B731D4u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B731E0u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B731ECu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B731F4u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B731F8u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7320Cu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73218u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73220u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73224u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B734F0u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7350Cu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73514u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73518u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73524u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73528u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73534u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7353Cu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7354Cu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73558u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7357Cu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7358Cu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B735B4u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B735C0u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73724u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73730u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73740u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7376Cu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73790u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B737C4u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B737D4u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B73824u, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7382Cu, &sub_08B73170, "sub_08B73170");
    runtime.register_function(0x08B7500Cu, &sub_08B7500C, "sub_08B7500C");
    runtime.register_function(0x08B75030u, &sub_08B7500C, "sub_08B7500C");
    runtime.register_function(0x08B75040u, &sub_08B7500C, "sub_08B7500C");
    runtime.register_function(0x08B75044u, &sub_08B7500C, "sub_08B7500C");
    runtime.register_function(0x08B75058u, &sub_08B7500C, "sub_08B7500C");
    runtime.register_function(0x08B75060u, &sub_08B7500C, "sub_08B7500C");
    runtime.register_function(0x08B73DC8u, &sub_08B73DC8, "sub_08B73DC8");
    runtime.register_function(0x08B73DD8u, &sub_08B73DC8, "sub_08B73DC8");
    runtime.register_function(0x08B73DECu, &sub_08B73DC8, "sub_08B73DC8");
    runtime.register_function(0x08B73DF4u, &sub_08B73DC8, "sub_08B73DC8");
    runtime.register_function(0x08B73E04u, &sub_08B73E04, "sub_08B73E04");
    runtime.register_function(0x08B73E18u, &sub_08B73E04, "sub_08B73E04");
    runtime.register_function(0x08B73E20u, &sub_08B73E04, "sub_08B73E04");
    runtime.register_function(0x08B7FB9Cu, &sub_08B73E04, "sub_08B73E04");
    runtime.register_function(0x08B73CF0u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73CFCu, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D08u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D18u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D34u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D38u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D40u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D4Cu, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D50u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D64u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D70u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D78u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D84u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D90u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73D98u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73DA4u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73DC0u, &sub_08B73CF0, "sub_08B73CF0");
    runtime.register_function(0x08B73E2Cu, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73E3Cu, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73E48u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73E50u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73E7Cu, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73E88u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73E94u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73E9Cu, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73EACu, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73EBCu, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73EC0u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73ED8u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73EDCu, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73EE8u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73EF8u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73F00u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73F08u, &sub_08B73E2C, "sub_08B73E2C");
    runtime.register_function(0x08B73F1Cu, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73F44u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73F70u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73F74u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73F84u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73F8Cu, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73F98u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73FA0u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73FA8u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73FC8u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73FD4u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73FDCu, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B73FECu, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B74000u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B74010u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B7401Cu, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B74040u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B74048u, &sub_08B73F1C, "sub_08B73F1C");
    runtime.register_function(0x08B74100u, &sub_08B74100, "sub_08B74100");
    runtime.register_function(0x08B7413Cu, &sub_08B74100, "sub_08B74100");
    runtime.register_function(0x08B7414Cu, &sub_08B74100, "sub_08B74100");
    runtime.register_function(0x08B74198u, &sub_08B74100, "sub_08B74100");
    runtime.register_function(0x08B741ACu, &sub_08B74100, "sub_08B74100");
    runtime.register_function(0x08B74278u, &sub_08B74100, "sub_08B74100");
    runtime.register_function(0x08AB304Cu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3064u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB306Cu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3074u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3088u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3098u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30A0u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30A8u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30B0u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30B8u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30C0u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30CCu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30DCu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30E0u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30ECu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30F4u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB30FCu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3104u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB310Cu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3114u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3120u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3130u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3134u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB313Cu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3144u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3154u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3164u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB317Cu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3188u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB31C8u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB31D0u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB31E4u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB31F4u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB31FCu, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3210u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3218u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3220u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3230u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3238u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3250u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3258u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3260u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08AB3268u, &sub_08AB304C, "sub_08AB304C");
    runtime.register_function(0x08B1477Cu, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B147ACu, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B147C0u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B147C8u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B147E8u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B147F8u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B14810u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B1481Cu, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B14828u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B1483Cu, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B14848u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B14850u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B1485Cu, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B14878u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B14880u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B14890u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B14898u, &sub_08B1477C, "sub_08B1477C");
    runtime.register_function(0x08B14350u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B14360u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B14368u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B14370u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B14378u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B14380u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B14388u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B14390u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B14398u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B1D07Cu, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B1D0B0u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B1D0C8u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B1D0E0u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B1D0E8u, &sub_08B14350, "sub_08B14350");
    runtime.register_function(0x08B1D100u, &sub_08B1D100, "sub_08B1D100");
    runtime.register_function(0x08B1D12Cu, &sub_08B1D100, "sub_08B1D100");
    runtime.register_function(0x08B1D138u, &sub_08B1D100, "sub_08B1D100");
    runtime.register_function(0x08B1D150u, &sub_08B1D100, "sub_08B1D100");
    runtime.register_function(0x08B1D17Cu, &sub_08B1D100, "sub_08B1D100");
    runtime.register_function(0x08B1D190u, &sub_08B1D100, "sub_08B1D100");
    runtime.register_function(0x08B1D198u, &sub_08B1D100, "sub_08B1D100");
    runtime.register_function(0x08B1D1A0u, &sub_08B1D100, "sub_08B1D100");
    runtime.register_function(0x08B1D1A8u, &sub_08B1D100, "sub_08B1D100");
    runtime.register_function(0x08B1CE3Cu, &sub_08B1CE3C, "sub_08B1CE3C");
    runtime.register_function(0x08B1CE64u, &sub_08B1CE3C, "sub_08B1CE3C");
    runtime.register_function(0x08B1CE68u, &sub_08B1CE3C, "sub_08B1CE3C");
    runtime.register_function(0x08B1CE74u, &sub_08B1CE3C, "sub_08B1CE3C");
    runtime.register_function(0x08B1CEACu, &sub_08B1CE3C, "sub_08B1CE3C");
    runtime.register_function(0x08B1CEB4u, &sub_08B1CE3C, "sub_08B1CE3C");
    runtime.register_function(0x08B1CEECu, &sub_08B1CE3C, "sub_08B1CE3C");
    runtime.register_function(0x08B1CEF4u, &sub_08B1CE3C, "sub_08B1CE3C");
    runtime.register_function(0x08B1BB84u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BBA8u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BBB4u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BBC8u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BBE8u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BBF0u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BC30u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BC34u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BC44u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BC4Cu, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BC54u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1BC60u, &sub_08B1BB84, "sub_08B1BB84");
    runtime.register_function(0x08B1C018u, &sub_08B1C018, "sub_08B1C018");
    runtime.register_function(0x08B1C038u, &sub_08B1C018, "sub_08B1C018");
    runtime.register_function(0x08B1C040u, &sub_08B1C018, "sub_08B1C018");
    runtime.register_function(0x08B1C048u, &sub_08B1C018, "sub_08B1C018");
    runtime.register_function(0x08B1C05Cu, &sub_08B1C018, "sub_08B1C018");
    runtime.register_function(0x08B1C064u, &sub_08B1C018, "sub_08B1C018");
    runtime.register_function(0x08B1C06Cu, &sub_08B1C018, "sub_08B1C018");
    runtime.register_function(0x08B1C074u, &sub_08B1C018, "sub_08B1C018");
    runtime.register_function(0x08B1CEFCu, &sub_08B1CEFC, "sub_08B1CEFC");
    runtime.register_function(0x08B1CF24u, &sub_08B1CEFC, "sub_08B1CEFC");
    runtime.register_function(0x08B1CF30u, &sub_08B1CEFC, "sub_08B1CEFC");
    runtime.register_function(0x08B1CF48u, &sub_08B1CEFC, "sub_08B1CEFC");
    runtime.register_function(0x08B1CF50u, &sub_08B1CEFC, "sub_08B1CEFC");
    runtime.register_function(0x08B1CF5Cu, &sub_08B1CEFC, "sub_08B1CEFC");
    runtime.register_function(0x08B6029Cu, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B602CCu, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B602D4u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B602DCu, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B602F4u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B60308u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B60330u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B6034Cu, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B60354u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B60364u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B6036Cu, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B60374u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B60380u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B603A0u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B603A8u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B603B0u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B603BCu, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B603C4u, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B603CCu, &sub_08B6029C, "sub_08B6029C");
    runtime.register_function(0x08B63B84u, &sub_08B63B84, "sub_08B63B84");
    runtime.register_function(0x08B63BFCu, &sub_08B63B84, "sub_08B63B84");
    runtime.register_function(0x08B63C7Cu, &sub_08B63B84, "sub_08B63B84");
    runtime.register_function(0x08B603D8u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60414u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B6041Cu, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60438u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60454u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60464u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60478u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60480u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60488u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60498u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B604A4u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B604B0u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60520u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60524u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60548u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60550u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B6055Cu, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60564u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60574u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60580u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B6058Cu, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B605ACu, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B605B4u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B605BCu, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B605DCu, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B605E4u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B605F0u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60600u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B6060Cu, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60614u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B6061Cu, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60624u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B6062Cu, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60634u, &sub_08B603D8, "sub_08B603D8");
    runtime.register_function(0x08B60D58u, &sub_08B60D58, "sub_08B60D58");
    runtime.register_function(0x08B60F20u, &sub_08B60F20, "sub_08B60F20");
    runtime.register_function(0x08B60F4Cu, &sub_08B60F4C, "sub_08B60F4C");
    runtime.register_function(0x08B60F74u, &sub_08B60F74, "sub_08B60F74");
    runtime.register_function(0x08B60FA0u, &sub_08B60FA0, "sub_08B60FA0");
    runtime.register_function(0x08B63C90u, &sub_08B63C90, "sub_08B63C90");
    runtime.register_function(0x08B63CACu, &sub_08B63C90, "sub_08B63C90");
    runtime.register_function(0x08B63CBCu, &sub_08B63C90, "sub_08B63C90");
    runtime.register_function(0x08B63CC0u, &sub_08B63C90, "sub_08B63C90");
    runtime.register_function(0x08B63CC4u, &sub_08B63C90, "sub_08B63C90");
    runtime.register_function(0x08B63CD0u, &sub_08B63C90, "sub_08B63C90");
    runtime.register_function(0x08B63CD8u, &sub_08B63C90, "sub_08B63C90");
    runtime.register_function(0x08B63CE0u, &sub_08B63C90, "sub_08B63C90");
    runtime.register_function(0x08B60B94u, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60BC0u, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60BC4u, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60C1Cu, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60C3Cu, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60C54u, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60C60u, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60C78u, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60C7Cu, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60C84u, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60C9Cu, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B60CA4u, &sub_08B60B94, "sub_08B60B94");
    runtime.register_function(0x08B609F0u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60A30u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60A34u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60A54u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60A58u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60A68u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60A80u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60A84u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60AA4u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60AB4u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60ABCu, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B60AC4u, &sub_08B609F0, "sub_08B609F0");
    runtime.register_function(0x08B61478u, &sub_08B61478, "sub_08B61478");
    runtime.register_function(0x08B614A0u, &sub_08B61478, "sub_08B61478");
    runtime.register_function(0x08B614A4u, &sub_08B61478, "sub_08B61478");
    runtime.register_function(0x08B614ACu, &sub_08B61478, "sub_08B61478");
    runtime.register_function(0x08B614CCu, &sub_08B61478, "sub_08B61478");
    runtime.register_function(0x08B614E8u, &sub_08B614E8, "sub_08B614E8");
    runtime.register_function(0x08B61520u, &sub_08B614E8, "sub_08B614E8");
    runtime.register_function(0x08B61528u, &sub_08B614E8, "sub_08B614E8");
    runtime.register_function(0x08B6154Cu, &sub_08B614E8, "sub_08B614E8");
    runtime.register_function(0x08B61664u, &sub_08B61664, "sub_08B61664");
    runtime.register_function(0x08B61FF4u, &sub_08B61FF4, "sub_08B61FF4");
    runtime.register_function(0x08B61EFCu, &sub_08B61EFC, "sub_08B61EFC");
    runtime.register_function(0x08B61FA0u, &sub_08B61FA0, "sub_08B61FA0");
    runtime.register_function(0x08B61FC0u, &sub_08B61FA0, "sub_08B61FA0");
    runtime.register_function(0x08B610C8u, &sub_08B610C8, "sub_08B610C8");
    runtime.register_function(0x08B610D8u, &sub_08B610C8, "sub_08B610C8");
    runtime.register_function(0x08B613F0u, &sub_08B613F0, "sub_08B613F0");
    runtime.register_function(0x08B61400u, &sub_08B613F0, "sub_08B613F0");
    runtime.register_function(0x08B61424u, &sub_08B613F0, "sub_08B613F0");
    runtime.register_function(0x08B61430u, &sub_08B613F0, "sub_08B613F0");
    runtime.register_function(0x08B6141Cu, &sub_08B6141C, "sub_08B6141C");
    runtime.register_function(0x08B61424u, &sub_08B6141C, "sub_08B6141C");
    runtime.register_function(0x08B61424u, &sub_08B61430, "sub_08B61430");
    runtime.register_function(0x08B61430u, &sub_08B61430, "sub_08B61430");
    runtime.register_function(0x08B61428u, &sub_08B61438, "sub_08B61438");
    runtime.register_function(0x08B61438u, &sub_08B61438, "sub_08B61438");
    runtime.register_function(0x08B61448u, &sub_08B61438, "sub_08B61438");
    runtime.register_function(0x08B61428u, &sub_08B61450, "sub_08B61450");
    runtime.register_function(0x08B61440u, &sub_08B61450, "sub_08B61450");
    runtime.register_function(0x08B61448u, &sub_08B61450, "sub_08B61450");
    runtime.register_function(0x08B61450u, &sub_08B61450, "sub_08B61450");
    runtime.register_function(0x08B5F300u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F344u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F368u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F370u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F384u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F38Cu, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F3A0u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F3A8u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F3BCu, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F3ECu, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F414u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F41Cu, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F454u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F45Cu, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F494u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F4A4u, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B5F4ECu, &sub_08B5F300, "sub_08B5F300");
    runtime.register_function(0x08B14550u, &sub_08B14550, "sub_08B14550");
    runtime.register_function(0x08B14560u, &sub_08B14550, "sub_08B14550");
    runtime.register_function(0x08B61228u, &sub_08B61228, "sub_08B61228");
    runtime.register_function(0x08B61238u, &sub_08B61228, "sub_08B61228");
    runtime.register_function(0x08B61244u, &sub_08B61228, "sub_08B61228");
    runtime.register_function(0x08B6124Cu, &sub_08B61228, "sub_08B61228");
    runtime.register_function(0x08B61258u, &sub_08B61228, "sub_08B61228");
    runtime.register_function(0x08B15DCCu, &sub_08B15DCC, "sub_08B15DCC");
    runtime.register_function(0x08B15DECu, &sub_08B15DCC, "sub_08B15DCC");
    runtime.register_function(0x08B15DFCu, &sub_08B15DCC, "sub_08B15DCC");
    runtime.register_function(0x08B15E24u, &sub_08B15DCC, "sub_08B15DCC");
    runtime.register_function(0x08B15E28u, &sub_08B15DCC, "sub_08B15DCC");
    runtime.register_function(0x08B15E34u, &sub_08B15DCC, "sub_08B15DCC");
    runtime.register_function(0x08B63564u, &sub_08B63564, "sub_08B63564");
    runtime.register_function(0x08B635A8u, &sub_08B63564, "sub_08B63564");
    runtime.register_function(0x08B635B0u, &sub_08B63564, "sub_08B63564");
    runtime.register_function(0x08B6360Cu, &sub_08B6360C, "sub_08B6360C");
    runtime.register_function(0x08B63654u, &sub_08B6360C, "sub_08B6360C");
    runtime.register_function(0x08B6365Cu, &sub_08B6360C, "sub_08B6360C");
    runtime.register_function(0x08B636B8u, &sub_08B636B8, "sub_08B636B8");
    runtime.register_function(0x08B636CCu, &sub_08B636B8, "sub_08B636B8");
    runtime.register_function(0x08B636D4u, &sub_08B636B8, "sub_08B636B8");
    runtime.register_function(0x08B636E0u, &sub_08B636B8, "sub_08B636B8");
    runtime.register_function(0x08B636F0u, &sub_08B636B8, "sub_08B636B8");
    runtime.register_function(0x08B636F8u, &sub_08B636B8, "sub_08B636B8");
    runtime.register_function(0x08B63700u, &sub_08B636B8, "sub_08B636B8");
    runtime.register_function(0x08B63714u, &sub_08B636B8, "sub_08B636B8");
    runtime.register_function(0x08B5F4FCu, &sub_08B5F4FC, "sub_08B5F4FC");
    runtime.register_function(0x08B5F50Cu, &sub_08B5F4FC, "sub_08B5F4FC");
    runtime.register_function(0x08B5F51Cu, &sub_08B5F4FC, "sub_08B5F4FC");
    runtime.register_function(0x08B17400u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1744Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17454u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1748Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17490u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B174B0u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B174DCu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B174FCu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1751Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17524u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17530u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1753Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17554u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1755Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17564u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17574u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1757Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17580u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17588u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17590u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1759Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B175A8u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B175C0u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17608u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1760Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17618u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1761Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17630u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17638u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17674u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1767Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B17684u, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B1768Cu, &sub_08B175C0, "sub_08B175C0");
    runtime.register_function(0x08B15678u, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B156A4u, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B156C4u, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B156DCu, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B156E4u, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B156ECu, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B156F8u, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B15710u, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B1571Cu, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B15740u, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B15758u, &sub_08B156F8, "sub_08B156F8");
    runtime.register_function(0x08B1560Cu, &sub_08B1560C, "sub_08B1560C");
    runtime.register_function(0x08B1564Cu, &sub_08B1560C, "sub_08B1560C");
    runtime.register_function(0x08B15658u, &sub_08B1560C, "sub_08B1560C");
    runtime.register_function(0x08B15660u, &sub_08B1560C, "sub_08B1560C");
    runtime.register_function(0x08B15668u, &sub_08B1560C, "sub_08B1560C");
    runtime.register_function(0x08A9AD14u, &sub_08A9AD14, "sub_08A9AD14");
    runtime.register_function(0x08A9B6D8u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B718u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B728u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B72Cu, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B768u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B76Cu, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B77Cu, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B784u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B78Cu, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B7B0u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B7B8u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B7C0u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B7C8u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B7D0u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B7D8u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B888u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B8A8u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B8B4u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B8D4u, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B8FCu, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B90Cu, &sub_08A9B888, "sub_08A9B888");
    runtime.register_function(0x08A9B0A4u, &sub_08A9B0A4, "sub_08A9B0A4");
    runtime.register_function(0x08A9C704u, &sub_08A9C704, "sub_08A9C704");
    runtime.register_function(0x08B60640u, &sub_08B60640, "sub_08B60640");
    runtime.register_function(0x08B60670u, &sub_08B60640, "sub_08B60640");
    runtime.register_function(0x08B60684u, &sub_08B60640, "sub_08B60640");
    runtime.register_function(0x08B60740u, &sub_08B60640, "sub_08B60640");
    runtime.register_function(0x08B606A0u, &sub_08B606A0, "sub_08B606A0");
    runtime.register_function(0x08B606D0u, &sub_08B606A0, "sub_08B606A0");
    runtime.register_function(0x08B606D8u, &sub_08B606A0, "sub_08B606A0");
    runtime.register_function(0x08B606F0u, &sub_08B606A0, "sub_08B606A0");
    runtime.register_function(0x08B6071Cu, &sub_08B606A0, "sub_08B606A0");
    runtime.register_function(0x08B60734u, &sub_08B606A0, "sub_08B606A0");
    runtime.register_function(0x08B6073Cu, &sub_08B606A0, "sub_08B606A0");
    runtime.register_function(0x08B60740u, &sub_08B606A0, "sub_08B606A0");
    runtime.register_function(0x08B60758u, &sub_08B606A0, "sub_08B606A0");
    runtime.register_function(0x08B606D8u, &sub_08B60760, "sub_08B60760");
    runtime.register_function(0x08B606F0u, &sub_08B60760, "sub_08B60760");
    runtime.register_function(0x08B6071Cu, &sub_08B60760, "sub_08B60760");
    runtime.register_function(0x08B60734u, &sub_08B60760, "sub_08B60760");
    runtime.register_function(0x08B6073Cu, &sub_08B60760, "sub_08B60760");
    runtime.register_function(0x08B60758u, &sub_08B60760, "sub_08B60760");
    runtime.register_function(0x08B60760u, &sub_08B60760, "sub_08B60760");
    runtime.register_function(0x08B60774u, &sub_08B60760, "sub_08B60760");
    runtime.register_function(0x08B6078Cu, &sub_08B60760, "sub_08B60760");
    runtime.register_function(0x08B606D8u, &sub_08B607B4, "sub_08B607B4");
    runtime.register_function(0x08B606F0u, &sub_08B607B4, "sub_08B607B4");
    runtime.register_function(0x08B6071Cu, &sub_08B607B4, "sub_08B607B4");
    runtime.register_function(0x08B60734u, &sub_08B607B4, "sub_08B607B4");
    runtime.register_function(0x08B6073Cu, &sub_08B607B4, "sub_08B607B4");
    runtime.register_function(0x08B60758u, &sub_08B607B4, "sub_08B607B4");
    runtime.register_function(0x08B607B4u, &sub_08B607B4, "sub_08B607B4");
    runtime.register_function(0x08A9B50Cu, &sub_08A9B50C, "sub_08A9B50C");
    runtime.register_function(0x08A9B550u, &sub_08A9B50C, "sub_08A9B50C");
    runtime.register_function(0x08A9B574u, &sub_08A9B50C, "sub_08A9B50C");
    runtime.register_function(0x08A9B580u, &sub_08A9B50C, "sub_08A9B50C");
    runtime.register_function(0x08A9B59Cu, &sub_08A9B50C, "sub_08A9B50C");
    runtime.register_function(0x08A9B5B0u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08A9B5C8u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08A9B5E0u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7305Cu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B73070u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B5D0u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B5ECu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B5F4u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B620u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B630u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B64Cu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B65Cu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B670u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B684u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B688u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B69Cu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B6A0u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B6A8u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B6B8u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B6C4u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B6D4u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B6E0u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B6ECu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B700u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B704u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B714u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B720u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B724u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B734u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B74Cu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B754u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B75Cu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B778u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B780u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B794u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B7A8u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B7C0u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B810u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B81Cu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B838u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B854u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B864u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B86Cu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B878u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B8ACu, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B8B4u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B8C4u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08B7B8D0u, &sub_08A9B5B0, "sub_08A9B5B0");
    runtime.register_function(0x08A9C6F0u, &sub_08A9C6F0, "sub_08A9C6F0");
    runtime.register_function(0x08A99F60u, &sub_08A99F60, "sub_08A99F60");
    runtime.register_function(0x08A99F88u, &sub_08A99F60, "sub_08A99F60");
    runtime.register_function(0x08A9F8F8u, &sub_08A9F8F8, "sub_08A9F8F8");
    runtime.register_function(0x08A9F920u, &sub_08A9F8F8, "sub_08A9F8F8");
    runtime.register_function(0x08A9F938u, &sub_08A9F8F8, "sub_08A9F8F8");
    runtime.register_function(0x08AB2A6Cu, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2AA0u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2AA8u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2AB8u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2AC0u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2AE4u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2AF4u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2B14u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2B18u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2B2Cu, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2B34u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2B9Cu, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB2BA4u, &sub_08AB2A6C, "sub_08AB2A6C");
    runtime.register_function(0x08AB1190u, &sub_08AB1190, "sub_08AB1190");
    runtime.register_function(0x08AB11C0u, &sub_08AB1190, "sub_08AB1190");
    runtime.register_function(0x08AB11E8u, &sub_08AB1190, "sub_08AB1190");
    runtime.register_function(0x08AA155Cu, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA156Cu, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA159Cu, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA15ACu, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA15C4u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA15D4u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA15DCu, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA15E4u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA15ECu, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA15F4u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA1600u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA1608u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA1610u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA1628u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA162Cu, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA1634u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA163Cu, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA164Cu, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08AA1654u, &sub_08AA155C, "sub_08AA155C");
    runtime.register_function(0x08814E88u, &sub_08814E88, "sub_08814E88");
    runtime.register_function(0x08AB2984u, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB29B4u, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB29C4u, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB29CCu, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB29D4u, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB29ECu, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB29F8u, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB2A0Cu, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB2A14u, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB2A30u, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB2A3Cu, &sub_08AB2984, "sub_08AB2984");
    runtime.register_function(0x08AB3020u, &sub_08AB3020, "sub_08AB3020");
    runtime.register_function(0x08B641E4u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B6420Cu, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B64240u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B64254u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B6426Cu, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B64274u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B64288u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B6429Cu, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B642A4u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B642B4u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B642C0u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B642E4u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B642F0u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B642F8u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B64300u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B64308u, &sub_08B641E4, "sub_08B641E4");
    runtime.register_function(0x08B64314u, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B64368u, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B64370u, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B6439Cu, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B643B4u, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B643E0u, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B643F0u, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B643F8u, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B64404u, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B6440Cu, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B64434u, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B6443Cu, &sub_08B64314, "sub_08B64314");
    runtime.register_function(0x08B48B48u, &sub_08B48B48, "sub_08B48B48");
    runtime.register_function(0x08B48B0Cu, &sub_08B48B0C, "sub_08B48B0C");
    runtime.register_function(0x08B48B20u, &sub_08B48B0C, "sub_08B48B0C");
    runtime.register_function(0x08B177F0u, &sub_08B177F0, "sub_08B177F0");
    runtime.register_function(0x08B17838u, &sub_08B177F0, "sub_08B177F0");
    runtime.register_function(0x08B1784Cu, &sub_08B177F0, "sub_08B177F0");
    runtime.register_function(0x08B17878u, &sub_08B177F0, "sub_08B177F0");
    runtime.register_function(0x08B130ACu, &sub_08B13400, "sub_08B13400");
    runtime.register_function(0x08B130CCu, &sub_08B13400, "sub_08B13400");
    runtime.register_function(0x08B130E8u, &sub_08B13400, "sub_08B13400");
    runtime.register_function(0x08B130F0u, &sub_08B13400, "sub_08B13400");
    runtime.register_function(0x08B13400u, &sub_08B13400, "sub_08B13400");
    runtime.register_function(0x08B13418u, &sub_08B13400, "sub_08B13400");
    runtime.register_function(0x08B13448u, &sub_08B13400, "sub_08B13400");
    runtime.register_function(0x08B13474u, &sub_08B13400, "sub_08B13400");
    runtime.register_function(0x08AC4794u, &sub_08AC4794, "sub_08AC4794");
    runtime.register_function(0x08AC47A8u, &sub_08AC4794, "sub_08AC4794");
    runtime.register_function(0x08AC47B0u, &sub_08AC4794, "sub_08AC4794");
    runtime.register_function(0x08B15E58u, &sub_08B15E58, "sub_08B15E58");
    runtime.register_function(0x08B15E8Cu, &sub_08B15E58, "sub_08B15E58");
    runtime.register_function(0x08B15EA4u, &sub_08B15E58, "sub_08B15E58");
    runtime.register_function(0x08B15EBCu, &sub_08B15E58, "sub_08B15E58");
    runtime.register_function(0x08B15EC4u, &sub_08B15E58, "sub_08B15E58");
    runtime.register_function(0x08B15AB0u, &sub_08B15AB0, "sub_08B15AB0");
    runtime.register_function(0x08B15AE4u, &sub_08B15AB0, "sub_08B15AB0");
    runtime.register_function(0x08B15AFCu, &sub_08B15AB0, "sub_08B15AB0");
    runtime.register_function(0x08B15B14u, &sub_08B15AB0, "sub_08B15AB0");
    runtime.register_function(0x08B15B1Cu, &sub_08B15AB0, "sub_08B15AB0");
    runtime.register_function(0x08B1B8ACu, &sub_08B1B8AC, "sub_08B1B8AC");
    runtime.register_function(0x08B1B8E0u, &sub_08B1B8AC, "sub_08B1B8AC");
    runtime.register_function(0x08B1B8F8u, &sub_08B1B8AC, "sub_08B1B8AC");
    runtime.register_function(0x08B1B8FCu, &sub_08B1B8AC, "sub_08B1B8AC");
    runtime.register_function(0x08B1B90Cu, &sub_08B1B8AC, "sub_08B1B8AC");
    runtime.register_function(0x08B1B918u, &sub_08B1B8AC, "sub_08B1B8AC");
    runtime.register_function(0x08B1B92Cu, &sub_08B1B8AC, "sub_08B1B8AC");
    runtime.register_function(0x08B1B934u, &sub_08B1B8AC, "sub_08B1B8AC");
    runtime.register_function(0x08B1C214u, &sub_08B1C214, "sub_08B1C214");
    runtime.register_function(0x08B1C248u, &sub_08B1C214, "sub_08B1C214");
    runtime.register_function(0x08B1C260u, &sub_08B1C214, "sub_08B1C214");
    runtime.register_function(0x08B1C278u, &sub_08B1C214, "sub_08B1C214");
    runtime.register_function(0x08B1C280u, &sub_08B1C214, "sub_08B1C214");
    runtime.register_function(0x08B17AC0u, &sub_08B17AC0, "sub_08B17AC0");
    runtime.register_function(0x08B17AF0u, &sub_08B17AC0, "sub_08B17AC0");
    runtime.register_function(0x08B19928u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19968u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19974u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19984u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19A54u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19B78u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19B8Cu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19B9Cu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19BB4u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19BBCu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19BD0u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19BE0u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19BE8u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19BF4u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19C04u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19C0Cu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19C14u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19C1Cu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19C34u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19C48u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19C60u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19C78u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19C84u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19CBCu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19CCCu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19CE0u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19CE4u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19CECu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19D20u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19D3Cu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19D40u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19D48u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19D50u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19D58u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19D70u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19D74u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19DA4u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19DB0u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19DB8u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19DC8u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19DE0u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19DECu, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B19DF8u, &sub_08B19928, "sub_08B19928");
    runtime.register_function(0x08B6094Cu, &sub_08B6094C, "sub_08B6094C");
    runtime.register_function(0x08B6099Cu, &sub_08B6094C, "sub_08B6094C");
    runtime.register_function(0x08B609B8u, &sub_08B6094C, "sub_08B6094C");
    runtime.register_function(0x08B609BCu, &sub_08B6094C, "sub_08B6094C");
    runtime.register_function(0x08B609C4u, &sub_08B6094C, "sub_08B6094C");
    runtime.register_function(0x08B609CCu, &sub_08B6094C, "sub_08B6094C");
    runtime.register_function(0x08B609E0u, &sub_08B6094C, "sub_08B6094C");
    runtime.register_function(0x08B609E8u, &sub_08B6094C, "sub_08B6094C");
    runtime.register_function(0x08B63B50u, &sub_08B63B50, "sub_08B63B50");
    runtime.register_function(0x08B63B68u, &sub_08B63B50, "sub_08B63B50");
    runtime.register_function(0x08B63B6Cu, &sub_08B63B50, "sub_08B63B50");
    runtime.register_function(0x08B63B74u, &sub_08B63B50, "sub_08B63B50");
    runtime.register_function(0x08B63B7Cu, &sub_08B63B50, "sub_08B63B50");
    runtime.register_function(0x08B19890u, &sub_08B19890, "sub_08B19890");
    runtime.register_function(0x08B198C0u, &sub_08B19890, "sub_08B19890");
    runtime.register_function(0x08B198CCu, &sub_08B19890, "sub_08B19890");
    runtime.register_function(0x08B198D4u, &sub_08B19890, "sub_08B19890");
    runtime.register_function(0x08B198E4u, &sub_08B19890, "sub_08B19890");
    runtime.register_function(0x08B198ECu, &sub_08B19890, "sub_08B19890");
    runtime.register_function(0x08B1990Cu, &sub_08B19890, "sub_08B19890");
    runtime.register_function(0x08A9B3F4u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B40Cu, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B410u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B43Cu, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B468u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B474u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B480u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B48Cu, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B498u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B4ACu, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B4C4u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B4D0u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B4DCu, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B4E4u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B4F0u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08A9B4F8u, &sub_08A9B3F4, "sub_08A9B3F4");
    runtime.register_function(0x08B7FD9Cu, &import_0, "sceAudio::0x01562BA3");
    runtime.register_function(0x08B7FDA4u, &import_1, "sceAudio::0x2D53F36E");
    runtime.register_function(0x08B7FDACu, &import_2, "sceAudio::0x43196845");
    runtime.register_function(0x08B7FDB4u, &import_3, "sceAudio::0x136CAF51");
    runtime.register_function(0x08B7FDBCu, &import_4, "sceAudio::0x13F592BC");
    runtime.register_function(0x08B7FDC4u, &import_5, "sceAudio::0x5EC81C55");
    runtime.register_function(0x08B7FDCCu, &import_6, "sceAudio::0x6FC46853");
    runtime.register_function(0x08B7FDD4u, &import_7, "sceAudio::0x95FD0C2D");
    runtime.register_function(0x08B7FDDCu, &import_8, "sceAudio::0xB011922F");
    runtime.register_function(0x08B7FDE4u, &import_9, "sceAudio::0xB7E1D8E7");
    runtime.register_function(0x08B7FDECu, &import_10, "sceAudio::0xCB2E439E");
    runtime.register_function(0x08B7FDF4u, &import_11, "sceAudio::0xE2D56B2D");
    runtime.register_function(0x08B80004u, &import_12, "sceSasCore::0x019B25EB");
    runtime.register_function(0x08B8000Cu, &import_13, "sceSasCore::0x07F58C24");
    runtime.register_function(0x08B80014u, &import_14, "sceSasCore::0x267A6DD2");
    runtime.register_function(0x08B8001Cu, &import_15, "sceSasCore::0x2C8E6AB3");
    runtime.register_function(0x08B80024u, &import_16, "sceSasCore::0x33D4AB37");
    runtime.register_function(0x08B8002Cu, &import_17, "sceSasCore::0x42778A9F");
    runtime.register_function(0x08B80034u, &import_18, "sceSasCore::0x440CA7D8");
    runtime.register_function(0x08B8003Cu, &import_19, "sceSasCore::0x50A14DFC");
    runtime.register_function(0x08B80044u, &import_20, "sceSasCore::0x5F9529F6");
    runtime.register_function(0x08B8004Cu, &import_21, "sceSasCore::0x68A46B95");
    runtime.register_function(0x08B80054u, &import_22, "sceSasCore::0x74AE582A");
    runtime.register_function(0x08B8005Cu, &import_23, "sceSasCore::0x76F01ACA");
    runtime.register_function(0x08B80064u, &import_24, "sceSasCore::0x787D04D5");
    runtime.register_function(0x08B8006Cu, &import_25, "sceSasCore::0x99944089");
    runtime.register_function(0x08B80074u, &import_26, "sceSasCore::0x9EC3676A");
    runtime.register_function(0x08B8007Cu, &import_27, "sceSasCore::0xA0CF2FA4");
    runtime.register_function(0x08B80084u, &import_28, "sceSasCore::0xA3589D81");
    runtime.register_function(0x08B8008Cu, &import_29, "sceSasCore::0xAD84D37F");
    runtime.register_function(0x08B80094u, &import_30, "sceSasCore::0xB7660A23");
    runtime.register_function(0x08B8009Cu, &import_31, "sceSasCore::0xBD11B7C2");
    runtime.register_function(0x08B800A4u, &import_32, "sceSasCore::0xCBCD4F79");
    runtime.register_function(0x08B800ACu, &import_33, "sceSasCore::0xD1E0A01E");
    runtime.register_function(0x08B800B4u, &import_34, "sceSasCore::0xD5A229C9");
    runtime.register_function(0x08B800BCu, &import_35, "sceSasCore::0xE175EF66");
    runtime.register_function(0x08B800C4u, &import_36, "sceSasCore::0xE1CD9561");
    runtime.register_function(0x08B800CCu, &import_37, "sceSasCore::0xE855BF76");
    runtime.register_function(0x08B800D4u, &import_38, "sceSasCore::0xF983B186");
    runtime.register_function(0x08B7FEC4u, &import_39, "sceLibFont::0x099EF33C");
    runtime.register_function(0x08B7FECCu, &import_40, "sceLibFont::0x0DA7535E");
    runtime.register_function(0x08B7FED4u, &import_41, "sceLibFont::0x27F6E642");
    runtime.register_function(0x08B7FEDCu, &import_42, "sceLibFont::0x3AEA8CB6");
    runtime.register_function(0x08B7FEE4u, &import_43, "sceLibFont::0x574B6FBC");
    runtime.register_function(0x08B7FEECu, &import_44, "sceLibFont::0x67F17ED7");
    runtime.register_function(0x08B7FEF4u, &import_45, "sceLibFont::0x74B21701");
    runtime.register_function(0x08B7FEFCu, &import_46, "sceLibFont::0x980F4895");
    runtime.register_function(0x08B7FF04u, &import_47, "sceLibFont::0xA834319D");
    runtime.register_function(0x08B7FF0Cu, &import_48, "sceLibFont::0xBC75D85B");
    runtime.register_function(0x08B7FF14u, &import_49, "sceLibFont::0xDCC80C2F");
    runtime.register_function(0x08B7FF1Cu, &import_50, "sceLibFont::0xF8F0752E");
    runtime.register_function(0x08B7FE14u, &import_51, "sceDisplay::0x0E20F177");
    runtime.register_function(0x08B7FE1Cu, &import_52, "sceDisplay::0x289D82FE");
    runtime.register_function(0x08B7FE24u, &import_53, "sceDisplay::0x46F186C3");
    runtime.register_function(0x08B7FE2Cu, &import_54, "sceDisplay::0x77ED8B3A");
    runtime.register_function(0x08B7FE34u, &import_55, "sceDisplay::0x984C27E7");
    runtime.register_function(0x08B7FE3Cu, &import_56, "sceDisplay::0x9C6EAAD7");
    runtime.register_function(0x08B7FE44u, &import_57, "sceDisplay::0xDBA6C4C4");
    runtime.register_function(0x08B7FE4Cu, &import_58, "sceDisplay::0xEEDA2E54");
    runtime.register_function(0x08B7FE5Cu, &import_59, "sceGe_user::0x03444EB4");
    runtime.register_function(0x08B7FE64u, &import_60, "sceGe_user::0x05DB22CE");
    runtime.register_function(0x08B7FE6Cu, &import_61, "sceGe_user::0x1C0D95A6");
    runtime.register_function(0x08B7FE74u, &import_62, "sceGe_user::0x1F6752AD");
    runtime.register_function(0x08B7FE7Cu, &import_63, "sceGe_user::0x4C06E472");
    runtime.register_function(0x08B7FE84u, &import_64, "sceGe_user::0xA4FC06A4");
    runtime.register_function(0x08B7FE8Cu, &import_65, "sceGe_user::0xAB49E76A");
    runtime.register_function(0x08B7FE94u, &import_66, "sceGe_user::0xB287BD61");
    runtime.register_function(0x08B7FE9Cu, &import_67, "sceGe_user::0xB448EC0D");
    runtime.register_function(0x08B7FEA4u, &import_68, "sceGe_user::0xDC93CFEF");
    runtime.register_function(0x08B7FEACu, &import_69, "sceGe_user::0xE0D68148");
    runtime.register_function(0x08B7FEB4u, &import_70, "sceGe_user::0xE47E40E4");
    runtime.register_function(0x08B7FDFCu, &import_71, "sceCtrl::0x1F4011E6");
    runtime.register_function(0x08B7FE04u, &import_72, "sceCtrl::0x1F803938");
    runtime.register_function(0x08B7FE0Cu, &import_73, "sceCtrl::0xA7144800");
    runtime.register_function(0x08B7FACCu, &import_74, "IoFileMgrForUser::0x42EC03AC");
    runtime.register_function(0x08B7FAD4u, &import_75, "IoFileMgrForUser::0x54F5FB11");
    runtime.register_function(0x08B7FADCu, &import_76, "IoFileMgrForUser::0x6A638D83");
    runtime.register_function(0x08B7FAE4u, &import_77, "IoFileMgrForUser::0x71B19E77");
    runtime.register_function(0x08B7FAECu, &import_78, "IoFileMgrForUser::0x810C4BC3");
    runtime.register_function(0x08B7FAF4u, &import_79, "IoFileMgrForUser::0x89AA9906");
    runtime.register_function(0x08B7FAFCu, &import_80, "IoFileMgrForUser::0xA0B5A7C2");
    runtime.register_function(0x08B7FB04u, &import_81, "IoFileMgrForUser::0xAB96437F");
    runtime.register_function(0x08B7FB0Cu, &import_82, "IoFileMgrForUser::0xACE946E8");
    runtime.register_function(0x08B7FB14u, &import_83, "IoFileMgrForUser::0xB293727F");
    runtime.register_function(0x08B7FB1Cu, &import_84, "IoFileMgrForUser::0xB29DDF9C");
    runtime.register_function(0x08B7FB24u, &import_85, "IoFileMgrForUser::0xE23EEC33");
    runtime.register_function(0x08B7FB2Cu, &import_86, "IoFileMgrForUser::0xE3EB004C");
    runtime.register_function(0x08B7FB34u, &import_87, "IoFileMgrForUser::0xEB092469");
    runtime.register_function(0x08B7FB3Cu, &import_88, "IoFileMgrForUser::0xFF5940B6");
    runtime.register_function(0x08B7FB44u, &import_89, "IoFileMgrForUser::0x109F50BC");
    runtime.register_function(0x08B7FB4Cu, &import_90, "IoFileMgrForUser::0x27EB27B8");
    runtime.register_function(0x08B7FB54u, &import_91, "IoFileMgrForUser::0x3251EA56");
    runtime.register_function(0x08B7FB5Cu, &import_92, "IoFileMgrForUser::0x35DBD746");
    runtime.register_function(0x08B7FB64u, &import_93, "IoFileMgrForUser::0x779103A0");
    runtime.register_function(0x08B7FB6Cu, &import_94, "Kernel_Library::0x092968F4");
    runtime.register_function(0x08B7FB74u, &import_95, "Kernel_Library::0xA089ECA4");
    runtime.register_function(0x08B7FB7Cu, &import_96, "Kernel_Library::0xBEA46419");
    runtime.register_function(0x08B7FB84u, &import_97, "Kernel_Library::0x15B6446B");
    runtime.register_function(0x08B7FB8Cu, &import_98, "Kernel_Library::0x1839852A");
    runtime.register_function(0x08B7FB94u, &import_99, "Kernel_Library::0x293B45B8");
    runtime.register_function(0x08B7FB9Cu, &import_100, "Kernel_Library::0x5F10D406");
    runtime.register_function(0x08B7FBA4u, &import_101, "LoadExecForUser::0x05572A5F");
    runtime.register_function(0x08B7FBACu, &import_102, "LoadExecForUser::0x4AC57943");
    runtime.register_function(0x08B7FBB4u, &import_103, "ModuleMgrForUser::0x2E0911AA");
    runtime.register_function(0x08B7FBBCu, &import_104, "ModuleMgrForUser::0xD1FF982A");
    runtime.register_function(0x08B7FBC4u, &import_105, "ModuleMgrForUser::0xD8B73127");
    runtime.register_function(0x08B7FBCCu, &import_106, "ModuleMgrForUser::0x50F0C1EC");
    runtime.register_function(0x08B7FBD4u, &import_107, "ModuleMgrForUser::0x977DE386");
    runtime.register_function(0x08B7FBDCu, &import_108, "ModuleMgrForUser::0xF0A26395");
    runtime.register_function(0x08B7FBE4u, &import_109, "ModuleMgrForUser::0x8F2DF740");
    runtime.register_function(0x08B7FC04u, &import_110, "SysMemUserForUser::0xF77D77CB");
    runtime.register_function(0x08B7FC0Cu, &import_111, "SysMemUserForUser::0x35669D4C");
    runtime.register_function(0x08B7FC14u, &import_112, "SysMemUserForUser::0x13A5ABEF");
    runtime.register_function(0x08B7FC1Cu, &import_113, "SysMemUserForUser::0x237DBD4F");
    runtime.register_function(0x08B7FC24u, &import_114, "SysMemUserForUser::0x9D9A5BA1");
    runtime.register_function(0x08B7FC2Cu, &import_115, "SysMemUserForUser::0xB6D61D02");
    runtime.register_function(0x08B7FC34u, &import_116, "ThreadManForUser::0xCEADEB47");
    runtime.register_function(0x08B7FC3Cu, &import_117, "ThreadManForUser::0xD59EAD2F");
    runtime.register_function(0x08B7FC44u, &import_118, "ThreadManForUser::0xD6DA4BA1");
    runtime.register_function(0x08B7FC4Cu, &import_119, "ThreadManForUser::0x1FB15A32");
    runtime.register_function(0x08B7FC54u, &import_120, "ThreadManForUser::0xE81CAF8F");
    runtime.register_function(0x08B7FC5Cu, &import_121, "ThreadManForUser::0xEA748E31");
    runtime.register_function(0x08B7FC64u, &import_122, "ThreadManForUser::0xEDBA5844");
    runtime.register_function(0x08B7FC6Cu, &import_123, "ThreadManForUser::0xEF9E4C70");
    runtime.register_function(0x08B7FC74u, &import_124, "ThreadManForUser::0xF475845D");
    runtime.register_function(0x08B7FC7Cu, &import_125, "ThreadManForUser::0xF8170FBE");
    runtime.register_function(0x08B7FC84u, &import_126, "ThreadManForUser::0x278C0DF5");
    runtime.register_function(0x08B7FC8Cu, &import_127, "ThreadManForUser::0x28B6489C");
    runtime.register_function(0x08B7FC94u, &import_128, "ThreadManForUser::0x349D6D6C");
    runtime.register_function(0x08B7FC9Cu, &import_129, "ThreadManForUser::0x369ED59D");
    runtime.register_function(0x08B7FCA4u, &import_130, "ThreadManForUser::0x3F53E640");
    runtime.register_function(0x08B7FCACu, &import_131, "ThreadManForUser::0x402FCF22");
    runtime.register_function(0x08B7FCB4u, &import_132, "ThreadManForUser::0x446D8DE6");
    runtime.register_function(0x08B7FCBCu, &import_133, "ThreadManForUser::0x4E3A1105");
    runtime.register_function(0x08B7FCC4u, &import_134, "ThreadManForUser::0x55C20A00");
    runtime.register_function(0x08B7FCCCu, &import_135, "ThreadManForUser::0x58B1F937");
    runtime.register_function(0x08B7FCD4u, &import_136, "ThreadManForUser::0x60107536");
    runtime.register_function(0x08B7FCDCu, &import_137, "ThreadManForUser::0x68DA9E36");
    runtime.register_function(0x08B7FCE4u, &import_138, "ThreadManForUser::0x6B30100F");
    runtime.register_function(0x08B7FCECu, &import_139, "ThreadManForUser::0x6D212BAC");
    runtime.register_function(0x08B7FCF4u, &import_140, "ThreadManForUser::0x71BC9871");
    runtime.register_function(0x08B7FCFCu, &import_141, "ThreadManForUser::0x75156E8F");
    runtime.register_function(0x08B7FD04u, &import_142, "ThreadManForUser::0x812346E4");
    runtime.register_function(0x08B7FD0Cu, &import_143, "ThreadManForUser::0x17C1684E");
    runtime.register_function(0x08B7FD14u, &import_144, "ThreadManForUser::0x94AA61EE");
    runtime.register_function(0x08B7FD1Cu, &import_145, "ThreadManForUser::0x9944F31F");
    runtime.register_function(0x08B7FD24u, &import_146, "ThreadManForUser::0x9ACE131E");
    runtime.register_function(0x08B7FD2Cu, &import_147, "ThreadManForUser::0x9FA03CD3");
    runtime.register_function(0x08B7FD34u, &import_148, "ThreadManForUser::0xAA73C935");
    runtime.register_function(0x08B7FD3Cu, &import_149, "ThreadManForUser::0x19CFF145");
    runtime.register_function(0x08B7FD44u, &import_150, "ThreadManForUser::0xB011B11F");
    runtime.register_function(0x08B7FD4Cu, &import_151, "ThreadManForUser::0xB7D098C6");
    runtime.register_function(0x08B7FD54u, &import_152, "ThreadManForUser::0x328C546A");
    runtime.register_function(0x08B7FD5Cu, &import_153, "ThreadManForUser::0x5BF4DD27");
    runtime.register_function(0x08B7FD64u, &import_154, "UtilsForUser::0x79D1C3FA");
    runtime.register_function(0x08B7FD6Cu, &import_155, "UtilsForUser::0xB435DEC5");
    runtime.register_function(0x08B7FD74u, &import_156, "UtilsForUser::0x34B9FA9E");
    runtime.register_function(0x08B7FD7Cu, &import_157, "UtilsForUser::0x3EE30821");
    runtime.register_function(0x08B7FD84u, &import_158, "UtilsForUser::0x71EC4271");
    runtime.register_function(0x08B7FD8Cu, &import_159, "UtilsForUser::0x91E4F6A7");
    runtime.register_function(0x08B7FD94u, &import_160, "UtilsForUser::0x27CC57F0");
    runtime.register_function(0x08B800DCu, &import_161, "sceSuspendForUser::0x090CCB3F");
    runtime.register_function(0x08B7FF24u, &import_162, "sceMpeg::0x0E3C2E9D");
    runtime.register_function(0x08B7FF2Cu, &import_163, "sceMpeg::0x13407F13");
    runtime.register_function(0x08B7FF34u, &import_164, "sceMpeg::0x167AFD9E");
    runtime.register_function(0x08B7FF3Cu, &import_165, "sceMpeg::0x21FF80E4");
    runtime.register_function(0x08B7FF44u, &import_166, "sceMpeg::0x37295ED8");
    runtime.register_function(0x08B7FF4Cu, &import_167, "sceMpeg::0x42560F23");
    runtime.register_function(0x08B7FF54u, &import_168, "sceMpeg::0x4571CC64");
    runtime.register_function(0x08B7FF5Cu, &import_169, "sceMpeg::0x591A4AA2");
    runtime.register_function(0x08B7FF64u, &import_170, "sceMpeg::0x606A4649");
    runtime.register_function(0x08B7FF6Cu, &import_171, "sceMpeg::0x611E9E11");
    runtime.register_function(0x08B7FF74u, &import_172, "sceMpeg::0x682A619B");
    runtime.register_function(0x08B7FF7Cu, &import_173, "sceMpeg::0x707B7629");
    runtime.register_function(0x08B7FF84u, &import_174, "sceMpeg::0x740FCCD1");
    runtime.register_function(0x08B7FF8Cu, &import_175, "sceMpeg::0x800C44DF");
    runtime.register_function(0x08B7FF94u, &import_176, "sceMpeg::0x874624D6");
    runtime.register_function(0x08B7FF9Cu, &import_177, "sceMpeg::0xA780CF7E");
    runtime.register_function(0x08B7FFA4u, &import_178, "sceMpeg::0xB240A59E");
    runtime.register_function(0x08B7FFACu, &import_179, "sceMpeg::0xB5F6DC87");
    runtime.register_function(0x08B7FFB4u, &import_180, "sceMpeg::0xC132E22F");
    runtime.register_function(0x08B7FFBCu, &import_181, "sceMpeg::0xCEB870B1");
    runtime.register_function(0x08B7FFC4u, &import_182, "sceMpeg::0xD7A29F46");
    runtime.register_function(0x08B7FFCCu, &import_183, "sceMpeg::0xD8C5F121");
    runtime.register_function(0x08B7FFD4u, &import_184, "sceMpeg::0xE1CE83A7");
    runtime.register_function(0x08B7FFDCu, &import_185, "sceMpeg::0xF8DCB679");
    runtime.register_function(0x08B7FFE4u, &import_186, "sceMpeg::0xFE246728");
    runtime.register_function(0x08B800E4u, &import_187, "sceUmdUser::0x20628E6F");
    runtime.register_function(0x08B800ECu, &import_188, "sceUmdUser::0x46EBB729");
    runtime.register_function(0x08B800F4u, &import_189, "sceUmdUser::0x4A9E5E29");
    runtime.register_function(0x08B800FCu, &import_190, "sceUmdUser::0x6AF9B50A");
    runtime.register_function(0x08B80104u, &import_191, "sceUmdUser::0x6B4A146C");
    runtime.register_function(0x08B8010Cu, &import_192, "sceUmdUser::0x8EF08FCE");
    runtime.register_function(0x08B80114u, &import_193, "sceUmdUser::0xAEE7404D");
    runtime.register_function(0x08B8011Cu, &import_194, "sceUmdUser::0xBD2BDE07");
    runtime.register_function(0x08B80124u, &import_195, "sceUmdUser::0xC6183D47");
    runtime.register_function(0x08B7FE54u, &import_196, "sceDmac::0x617F3FE6");
    runtime.register_function(0x08B7FFECu, &import_197, "scePower::0x04B7766E");
    runtime.register_function(0x08B7FFF4u, &import_198, "scePower::0xDFA8BAF8");
    runtime.register_function(0x08B7FFFCu, &import_199, "scePower::0xEBD177D6");
    runtime.register_function(0x08B8012Cu, &import_200, "sceUtility::0x2A2B3DE0");
    runtime.register_function(0x08B80134u, &import_201, "sceUtility::0x2AD8E239");
    runtime.register_function(0x08B8013Cu, &import_202, "sceUtility::0x50C4CD57");
    runtime.register_function(0x08B80144u, &import_203, "sceUtility::0x67AF3428");
    runtime.register_function(0x08B8014Cu, &import_204, "sceUtility::0x8874DBE0");
    runtime.register_function(0x08B80154u, &import_205, "sceUtility::0x95FC253B");
    runtime.register_function(0x08B8015Cu, &import_206, "sceUtility::0x9790B33C");
    runtime.register_function(0x08B80164u, &import_207, "sceUtility::0x9A1C91D7");
    runtime.register_function(0x08B8016Cu, &import_208, "sceUtility::0xD4B95FFB");
    runtime.register_function(0x08B80174u, &import_209, "sceUtility::0xE49BFE92");
    runtime.register_function(0x08B7FEBCu, &import_210, "sceImpose::0x36AA6E91");
    runtime.register_function(0x08B8017Cu, &import_211, "scesupPreAcc::0x110E318B");
    runtime.register_function(0x08B80184u, &import_212, "scesupPreAcc::0x2EC3F4D9");
    runtime.register_function(0x08B8018Cu, &import_213, "scesupPreAcc::0x348BA3E2");
    runtime.register_function(0x08B80194u, &import_214, "scesupPreAcc::0x7ADA3927");
    runtime.register_function(0x08B8019Cu, &import_215, "scesupPreAcc::0x86DEBD66");
    runtime.register_function(0x08B801A4u, &import_216, "scesupPreAcc::0xA0EAF444");
    runtime.register_function(0x08B801ACu, &import_217, "scesupPreAcc::0xB03FF882");
    runtime.register_function(0x08B7FBECu, &import_218, "StdioForUser::0x172D316E");
    runtime.register_function(0x08B7FBF4u, &import_219, "StdioForUser::0xA6BAB2E9");
    runtime.register_function(0x08B7FBFCu, &import_220, "StdioForUser::0xF78BA90A");
}
} // namespace psprecomp
