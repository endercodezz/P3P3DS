#pragma once

#include "psprecomp/runtime.hpp"

#include <bit>
#include <cstdint>

namespace psprecomp {

// Bit-exact host lowering of the leaf routine at 0x088B1780.  This is the
// same Allegrex/VFPU sequence emitted by psp_recomp, isolated so a caller in
// another AOT unit can execute it without an extra runtime redispatch.
inline void vcs_fast_088B1780(Runtime &rt, AllegrexContext &ctx) {
    // Four compact vector loads dominate this verified leaf. Cache the AOT RAM
    // view once so all sixteen scalar words share the same pre-resolved RAM base.
    const auto memory = rt.memory().aot_fast_view();
    { const std::uint32_t vfpu_address = ctx.gpr[4];
      float vfpu_value[4]{
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 0u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 4u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 8u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 12u))};
      ctx.write_vfpu_vector_ct<0u, 4u>(vfpu_value); }
    { const std::uint32_t vfpu_address = ctx.gpr[4] + 16u;
      float vfpu_value[4]{
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 0u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 4u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 8u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 12u))};
      ctx.write_vfpu_vector_ct<1u, 4u>(vfpu_value); }
    { const std::uint32_t vfpu_address = ctx.gpr[5];
      float vfpu_value[4]{
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 0u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 4u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 8u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 12u))};
      ctx.write_vfpu_vector_ct<4u, 4u>(vfpu_value); }
    { const std::uint32_t vfpu_address = ctx.gpr[5] + 16u;
      float vfpu_value[4]{
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 0u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 4u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 8u)),
        std::bit_cast<float>(memory.aot_load32(vfpu_address + 12u))};
      ctx.write_vfpu_vector_ct<5u, 4u>(vfpu_value); }
    ctx.execute_vfpu_vminmax(2u, 0u, 1u, 3u, false);
    ctx.execute_vfpu_vminmax(3u, 0u, 1u, 3u, true);
    ctx.execute_vfpu_compare3(12u, 5u, 2u, 3u, 7u);
    ctx.execute_vfpu_compare3(13u, 3u, 4u, 3u, 7u);
    { float vfpu_value[4]{};
      ctx.write_vfpu_vector_with_destination_prefix_ct<28u, 3u>(vfpu_value); }
    { float vfpu_s[4]{}, vfpu_t[4]{}, vfpu_d[4]{};
      ctx.read_vfpu_vector_with_source_prefix_ct<12u, 3u, 0u>(vfpu_s);
      ctx.read_vfpu_vector_with_source_prefix_ct<13u, 3u, 1u>(vfpu_t);
      for (std::uint32_t i = 0; i < 3u; ++i) vfpu_d[i] = vfpu_s[i] + vfpu_t[i];
      ctx.write_vfpu_vector_with_destination_prefix_ct<12u, 3u>(vfpu_d); }
    ctx.execute_vfpu_vcmp(12u, 28u, 3u, 7u);
    { const bool branch_taken = ((ctx.vfpu_ctrl[3] >> 4u) & 1u) != 0u;
      { float vfpu_s[4]{}, vfpu_t[4]{}, vfpu_d[4]{};
        ctx.read_vfpu_vector_with_source_prefix_ct<5u, 3u, 0u>(vfpu_s);
        ctx.read_vfpu_vector_with_source_prefix_ct<4u, 3u, 1u>(vfpu_t);
        for (std::uint32_t i = 0; i < 3u; ++i) vfpu_d[i] = vfpu_s[i] + vfpu_t[i];
        ctx.write_vfpu_vector_with_destination_prefix_ct<15u, 3u>(vfpu_d); }
      if (branch_taken) goto finish;
    }
    { float vfpu_s[4]{}, vfpu_t[4]{}, vfpu_d[4]{};
      ctx.read_vfpu_vector_with_source_prefix_ct<0u, 3u, 0u>(vfpu_s);
      ctx.read_vfpu_vector_with_source_prefix_ct<1u, 3u, 1u>(vfpu_t);
      for (std::uint32_t i = 0; i < 3u; ++i) vfpu_d[i] = vfpu_s[i] + vfpu_t[i];
      ctx.write_vfpu_vector_with_destination_prefix_ct<14u, 3u>(vfpu_d); }
    { float vfpu_s[4]{}, vfpu_t[4]{}, vfpu_d[4]{};
      ctx.read_vfpu_vector_with_source_prefix_ct<5u, 3u, 0u>(vfpu_s);
      ctx.read_vfpu_vector_with_source_prefix_ct<4u, 3u, 1u>(vfpu_t);
      for (std::uint32_t i = 0; i < 3u; ++i) vfpu_d[i] = vfpu_s[i] - vfpu_t[i];
      ctx.write_vfpu_vector_with_destination_prefix_ct<8u, 3u>(vfpu_d); }
    { float vfpu_s[4]{}, vfpu_t[4]{}, vfpu_d[4]{};
      ctx.read_vfpu_vector_with_source_prefix_ct<1u, 3u, 0u>(vfpu_s);
      ctx.read_vfpu_vector_with_source_prefix_ct<0u, 3u, 1u>(vfpu_t);
      for (std::uint32_t i = 0; i < 3u; ++i) vfpu_d[i] = vfpu_s[i] - vfpu_t[i];
      ctx.write_vfpu_vector_with_destination_prefix_ct<12u, 3u>(vfpu_d); }
    { float vfpu_s[4]{}, vfpu_t[4]{}, vfpu_d[4]{};
      ctx.read_vfpu_vector_with_source_prefix_ct<15u, 3u, 0u>(vfpu_s);
      ctx.read_vfpu_vector_with_source_prefix_ct<14u, 3u, 1u>(vfpu_t);
      for (std::uint32_t i = 0; i < 3u; ++i) vfpu_d[i] = vfpu_s[i] - vfpu_t[i];
      ctx.write_vfpu_vector_with_destination_prefix_ct<9u, 3u>(vfpu_d); }
    ctx.vfpu_ctrl[0u] = 0x000040C9u;
    ctx.vfpu_ctrl[1u] = 0x000043C6u;
    ctx.execute_vfpu_vdot(16u, 8u, 12u, 3u);
    ctx.vfpu_ctrl[0u] = 0x000040C2u;
    ctx.vfpu_ctrl[1u] = 0x000043C8u;
    ctx.execute_vfpu_vdot(48u, 8u, 12u, 3u);
    ctx.vfpu_ctrl[1u] = 0x000003E1u;
    ctx.execute_vfpu_vdot(80u, 8u, 12u, 2u);
    ctx.vfpu_ctrl[0u] = 0x000040C9u;
    ctx.vfpu_ctrl[1u] = 0x000240C6u;
    ctx.execute_vfpu_vdot(17u, 9u, 12u, 3u);
    ctx.vfpu_ctrl[0u] = 0x000040C2u;
    ctx.vfpu_ctrl[1u] = 0x000240C8u;
    ctx.execute_vfpu_vdot(49u, 9u, 12u, 3u);
    ctx.vfpu_ctrl[1u] = 0x000200E1u;
    ctx.execute_vfpu_vdot(81u, 9u, 12u, 2u);
    ctx.vfpu_ctrl[0u] = 0x000007E4u;
    ctx.execute_vfpu_vcmp(17u, 16u, 3u, 7u);
finish:
    // vflush is an architectural no-op that retains the VFPU prefixes.
    ctx.set_gpr(4, ctx.vfpu_scalar_bits_ct<131u>());
    ctx.set_gpr(2, ctx.gpr[4] & 16u);
    ctx.set_gpr(2, ctx.gpr[2] < 1u ? 1u : 0u);
    ctx.pc = ctx.gpr[31];
}

} // namespace psprecomp
