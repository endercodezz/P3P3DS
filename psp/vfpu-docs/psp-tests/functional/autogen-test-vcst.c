#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
static uint32_t fpcst(uint8_t cnum) {
  const uint32_t cntlist[] = {
      0x7f7fffff, // VFPU_HUGE  [3.40282346e+38] (max exp & mantissa)
      0x3fb504f3, // SQRT(2)    [1.41421353e+00]
      0x3f3504f3, // SQRT(1/2)  [7.07106769e-01]
      0x3f906ebb, // 2/SQRT(PI) [1.12837922e+00]
      0x3f22f983, // 2/PI       [6.36619746e-01]
      0x3ea2f983, // 1/PI       [3.18309873e-01]
      0x3f490fdb, // PI/4       [7.85398185e-01]
      0x3fc90fdb, // PI/2       [1.57079637e+00]
      0x40490fdb, // PI         [3.14159274e+00]
      0x402df854, // e          [2.71828174e+00]
      0x3fb8aa3b, // LOG2(e)    [1.44269502e+00]
      0x3ede5bd9, // LOG10(e)   [4.34294492e-01]
      0x3f317218, // LOGe(2)    [6.93147182e-01]
      0x40135d8e, // LOGe(10)   [2.30258512e+00]
      0x40c90fdb, // 2PI        [6.28318548e+00]
      0x3f060a92, // PI/6       [5.23598790e-01]
      0x3e9a209b, // LOG10(2)   [3.01030009e-01]
      0x40549a78, // LOG2(10)   [3.32192802e+00]
      0x3f5db3d7, // SQRT(3)/2  [8.66025388e-01]
  };
  return cntlist[cnum - 1];
}
int run_vcst_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vcst.s (Load special constant)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vcst.s";
    pfxinfo->rd_tested = 1;
    {
      unsigned int rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0610010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0610010,
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
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0620010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0620010,
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
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0630010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0630010,
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
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0640010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0640010,
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
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0650010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0650010,
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
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0660010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0660010,
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
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0670010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0670010,
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
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0680010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0680010,
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
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0690010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0690010,
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
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06a0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06a0010,
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
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06b0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06b0010,
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
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06c0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06c0010,
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
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06d0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06d0010,
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
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06e0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06e0010,
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
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06f0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06f0010,
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
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0700010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0700010,
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
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0710010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0710010,
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
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0720010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0720010,
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
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0730010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0730010,
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
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0610010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0610010,
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
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0620010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0620010,
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
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0630010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0630010,
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
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0640010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0640010,
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
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0650010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0650010,
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
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0660010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0660010,
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
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0670010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0670010,
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
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0680010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0680010,
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
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0690010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0690010,
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
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06a0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06a0010,
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
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06b0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06b0010,
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
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06c0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06c0010,
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
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06d0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06d0010,
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
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06e0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06e0010,
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
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06f0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06f0010,
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
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0700010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0700010,
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
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0710010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0710010,
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
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0720010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0720010,
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
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0730010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0730010,
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
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0610010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0610010,
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
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0620010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0620010,
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
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0630010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0630010,
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
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0640010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0640010,
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
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0650010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0650010,
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
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0660010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0660010,
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
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0670010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0670010,
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
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0680010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0680010,
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
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0690010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0690010,
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
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06a0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06a0010,
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
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06b0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06b0010,
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
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06c0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06c0010,
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
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06d0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06d0010,
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
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06e0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06e0010,
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
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06f0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06f0010,
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
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0700010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0700010,
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
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0710010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0710010,
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
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0720010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0720010,
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
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      ;
      unsigned int rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0730010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0730010,
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
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0610010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0610010,
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
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0620010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0620010,
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
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0630010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0630010,
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
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0640010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0640010,
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
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0650010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0650010,
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
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0660010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0660010,
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
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0670010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0670010,
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
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0680010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0680010,
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
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0690010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0690010,
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
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06a0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06a0010,
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
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06b0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06b0010,
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
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06c0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06c0010,
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
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06d0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06d0010,
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
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06e0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06e0010,
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
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06f0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd06f0010,
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
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0700010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0700010,
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
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0710010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0710010,
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
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0720010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0720010,
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
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0730010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 1 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.s",
                                                    .instw = 0xd0730010,
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
int run_vcst_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vcst.p (Load special constant)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vcst.p";
    pfxinfo->rd_tested = 1;
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0610090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0610090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0620090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0620090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0630090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0630090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0640090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0640090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0650090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0650090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0660090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0660090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0670090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0670090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0680090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0680090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0690090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0690090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06a0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06a0090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06b0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06b0090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06c0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06c0090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06d0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06d0090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06e0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06e0090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06f0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06f0090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0700090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0700090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0710090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0710090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0720090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0720090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0730090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0730090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0610090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0610090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0620090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0620090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0630090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0630090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0640090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0640090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0650090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0650090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0660090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0660090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0670090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0670090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0680090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0680090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0690090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0690090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06a0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06a0090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06b0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06b0090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06c0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06c0090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06d0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06d0090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06e0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06e0090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06f0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06f0090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0700090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0700090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0710090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0710090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0720090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0720090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0730090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0730090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0610090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0610090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0620090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0620090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0630090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0630090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0640090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0640090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0650090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0650090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0660090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0660090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0670090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0670090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0680090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0680090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0690090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0690090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06a0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06a0090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06b0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06b0090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06c0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06c0090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06d0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06d0090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06e0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06e0090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06f0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06f0090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0700090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0700090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0710090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0710090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0720090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0720090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0730090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0730090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0610090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0610090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0620090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0620090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0630090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0630090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0640090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0640090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0650090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0650090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0660090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0660090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0670090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0670090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0680090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0680090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0690090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0690090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06a0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06a0090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06b0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06b0090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06c0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06c0090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06d0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06d0090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06e0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06e0090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06f0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd06f0090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0700090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0700090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0710090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0710090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0720090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0720090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0730090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 2 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.p",
                                                    .instw = 0xd0730090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
int run_vcst_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vcst.t (Load special constant)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vcst.t";
    pfxinfo->rd_tested = 1;
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0618010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0618010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0628010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0628010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0638010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0638010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0648010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0648010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0658010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0658010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0668010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0668010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0678010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0678010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0688010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0688010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0698010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0698010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06a8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06a8010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06b8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06b8010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06c8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06c8010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06d8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06d8010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06e8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06e8010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06f8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06f8010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0708010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0708010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0718010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0718010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0728010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0728010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0738010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0738010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0618010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0618010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0628010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0628010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0638010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0638010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0648010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0648010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0658010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0658010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0668010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0668010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0678010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0678010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0688010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0688010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0698010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0698010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06a8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06a8010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06b8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06b8010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06c8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06c8010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06d8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06d8010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06e8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06e8010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06f8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06f8010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0708010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0708010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0718010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0718010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0728010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0728010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0738010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0738010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0618010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0618010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0628010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0628010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0638010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0638010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0648010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0648010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0658010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0658010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0668010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0668010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0678010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0678010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0688010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0688010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0698010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0698010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06a8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06a8010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06b8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06b8010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06c8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06c8010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06d8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06d8010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06e8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06e8010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06f8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06f8010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0708010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0708010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0718010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0718010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0728010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0728010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0738010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0738010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0618010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0618010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0628010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0628010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0638010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0638010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0648010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0648010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0658010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0658010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0668010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0668010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0678010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0678010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0688010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0688010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0698010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0698010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06a8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06a8010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06b8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06b8010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06c8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06c8010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06d8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06d8010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06e8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06e8010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06f8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd06f8010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0708010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0708010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0718010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0718010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0728010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0728010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0738010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 3 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.t",
                                                    .instw = 0xd0738010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
int run_vcst_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vcst.q (Load special constant)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vcst.q";
    pfxinfo->rd_tested = 1;
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0618090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0618090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0628090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0628090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0638090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0638090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0648090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0648090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0658090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0658090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0668090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0668090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0678090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0678090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0688090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0688090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0698090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0698090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06a8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06a8090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06b8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06b8090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06c8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06c8090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06d8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06d8090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06e8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06e8090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd06f8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06f8090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0708090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0708090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0718090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0718090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0728090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0728090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0738090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0738090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0618090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0618090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0628090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0628090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0638090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0638090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0648090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0648090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0658090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0658090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0668090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0668090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0678090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0678090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0688090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0688090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0698090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0698090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06a8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06a8090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06b8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06b8090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06c8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06c8090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06d8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06d8090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06e8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06e8090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd06f8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06f8090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0708090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0708090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0718090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0718090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0728090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0728090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0738090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0738090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0618090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0618090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0628090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0628090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0638090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0638090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0648090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0648090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0658090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0658090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0668090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0668090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0678090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0678090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0688090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0688090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0698090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0698090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06a8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06a8090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06b8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06b8090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06c8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06c8090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06d8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06d8090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06e8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06e8090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd06f8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06f8090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0708090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0708090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0718090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0718090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0728090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0728090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0738090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0738090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0618090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0618090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 2;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0628090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0628090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 3;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0638090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0638090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0648090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0648090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0658090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0658090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 6;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0668090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0668090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 7;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0678090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0678090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 8;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0688090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0688090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 9;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0698090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0698090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 10;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06a8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06a8090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06b8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06b8090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 12;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06c8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06c8090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06d8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06d8090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 14;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06e8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06e8090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 15;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd06f8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd06f8090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 16;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0708090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0708090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 17;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0718090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0718090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0728090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0728090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      unsigned int rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 19;
      rd[0] = fpcst(imval);
      rd[1] = fpcst(imval);
      rd[2] = fpcst(imval);
      rd[3] = fpcst(imval);
      ;
      unsigned int *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0738090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (memcmp(&rd_final[0], &rd_out[0], 4 * sizeof(uint32_t)));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vcst.q",
                                                    .instw = 0xd0738090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
