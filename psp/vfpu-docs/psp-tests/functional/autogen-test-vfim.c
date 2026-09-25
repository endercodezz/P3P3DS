#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
static uint32_t ifloat16(uint16_t fp16) {
  // Format is S.EEEEE.MMMMMMMMMM
  uint32_t exponent = (fp16 >> 10) & 0x1F;
  uint32_t mantissa = (fp16 & 0x3FF);
  uint32_t sign = (fp16 & 0x8000) << 16;

  if (!exponent)
    return sign; // Denormals rounded to zero

  if (exponent == 31) { // NaN/Inf
    exponent = 255;
  } else {
    mantissa <<= 13;
    exponent += 127 - 15;
  }

  // Direct conversion, no mantissa/exponent conversion
  return sign | (exponent << 23) | mantissa;
}
int run_vfim_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vfim.s (Load constant float value)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vfim.s";
    pfxinfo->rd_tested = 1;
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 0;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf900000" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf900000,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65535;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90ffff" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90ffff,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59180;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90e72c" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e72c,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 35709;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf908b7d" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf908b7d,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 27306;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf906aaa" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906aaa,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 56406;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90dc56" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90dc56,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 56517;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90dcc5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90dcc5,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2125;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90084d" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90084d,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 44212;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90acb4" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90acb4,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 38341;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf9095c5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9095c5,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8243;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf902033" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902033,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63840;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90f960" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90f960,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8226;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf902022" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902022,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42792;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90a728" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a728,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7774;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf901e5e" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901e5e,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5472;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf901560" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901560,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6065;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf9017b1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9017b1,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 26005;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf906595" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906595,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 64607;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90fc5f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90fc5f,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59853;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90e9cd" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e9cd,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 28578;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf906fa2" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906fa2,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59175;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90e727" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e727,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11098;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf902b5a" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902b5a,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 35233;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf9089a1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9089a1,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5816;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf9016b8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9016b8,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54101;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90d355" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90d355,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42389;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90a595" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a595,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7042;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf901b82" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901b82,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 53909;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90d295" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90d295,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 34146;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf908562" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf908562,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9311;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90245f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90245f,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 48112;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90bbf0" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90bbf0,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42068;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf90a454" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a454,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18196;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf904714" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf904714,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 0;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf900000" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf900000,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65535;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90ffff" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90ffff,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59180;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90e72c" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e72c,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 35709;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf908b7d" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf908b7d,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 27306;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf906aaa" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906aaa,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 56406;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90dc56" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90dc56,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 56517;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90dcc5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90dcc5,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2125;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90084d" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90084d,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 44212;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90acb4" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90acb4,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 38341;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf9095c5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9095c5,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8243;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf902033" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902033,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63840;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90f960" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90f960,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8226;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf902022" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902022,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42792;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90a728" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a728,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7774;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf901e5e" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901e5e,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5472;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf901560" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901560,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6065;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf9017b1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9017b1,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 26005;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf906595" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906595,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 64607;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90fc5f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90fc5f,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59853;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90e9cd" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e9cd,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 28578;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf906fa2" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906fa2,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59175;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90e727" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e727,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11098;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf902b5a" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902b5a,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 35233;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf9089a1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9089a1,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5816;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf9016b8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9016b8,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54101;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90d355" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90d355,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42389;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90a595" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a595,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7042;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf901b82" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901b82,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 53909;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90d295" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90d295,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 34146;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf908562" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf908562,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9311;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90245f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90245f,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 48112;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90bbf0" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90bbf0,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42068;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf90a454" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a454,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18196;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf904714" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf904714,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 0;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf900000" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf900000,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65535;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90ffff" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90ffff,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59180;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90e72c" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e72c,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 35709;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf908b7d" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf908b7d,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 27306;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf906aaa" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906aaa,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 56406;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90dc56" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90dc56,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 56517;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90dcc5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90dcc5,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2125;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90084d" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90084d,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 44212;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90acb4" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90acb4,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 38341;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf9095c5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9095c5,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8243;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf902033" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902033,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63840;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90f960" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90f960,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8226;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf902022" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902022,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42792;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90a728" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a728,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7774;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf901e5e" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901e5e,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5472;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf901560" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901560,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6065;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf9017b1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9017b1,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 26005;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf906595" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906595,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 64607;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90fc5f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90fc5f,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59853;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90e9cd" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e9cd,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 28578;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf906fa2" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906fa2,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59175;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90e727" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e727,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11098;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf902b5a" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902b5a,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 35233;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf9089a1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9089a1,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5816;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf9016b8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9016b8,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54101;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90d355" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90d355,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42389;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90a595" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a595,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7042;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf901b82" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901b82,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 53909;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90d295" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90d295,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 34146;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf908562" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf908562,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9311;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90245f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90245f,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 48112;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90bbf0" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90bbf0,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42068;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf90a454" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a454,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18196;
      rd[0] = ifloat16(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf904714" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf904714,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 0;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf900000" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf900000,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65535;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90ffff" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90ffff,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59180;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90e72c" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e72c,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 35709;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf908b7d" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf908b7d,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 27306;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf906aaa" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906aaa,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 56406;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90dc56" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90dc56,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 56517;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90dcc5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90dcc5,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2125;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90084d" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90084d,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 44212;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90acb4" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90acb4,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 38341;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf9095c5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9095c5,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8243;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf902033" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902033,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63840;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90f960" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90f960,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8226;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf902022" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902022,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42792;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90a728" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a728,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7774;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf901e5e" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901e5e,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5472;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf901560" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901560,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6065;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf9017b1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9017b1,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 26005;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf906595" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906595,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 64607;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90fc5f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90fc5f,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59853;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90e9cd" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e9cd,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 28578;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf906fa2" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf906fa2,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59175;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90e727" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90e727,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11098;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf902b5a" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf902b5a,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 35233;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf9089a1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9089a1,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5816;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf9016b8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf9016b8,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54101;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90d355" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90d355,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42389;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90a595" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a595,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7042;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf901b82" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf901b82,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 53909;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90d295" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90d295,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 34146;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf908562" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf908562,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9311;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90245f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90245f,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 48112;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90bbf0" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90bbf0,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42068;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf90a454" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf90a454,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18196;
      rd[0] = ifloat16(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf904714" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vfim.s",
                                                    .instw = 0xdf904714,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
