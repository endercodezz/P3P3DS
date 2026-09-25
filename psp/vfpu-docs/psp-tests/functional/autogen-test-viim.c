#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int run_viim_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction viim.s (Load constant integer value)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "viim.s";
    pfxinfo->rd_tested = 1;
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 0;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf100000" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf100000,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65535;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10ffff" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10ffff,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 30504;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf107728" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf107728,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42415;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10a5af" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a5af,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50344;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10c4a8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c4a8,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54847;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10d63f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10d63f,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 41403;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10a1bb" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a1bb,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5465;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf101559" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf101559,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63857;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10f971" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f971,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59680;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10e920" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10e920,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4619;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10120b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10120b,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65518;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10ffee" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10ffee,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54372;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10d464" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10d464,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50924;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10c6ec" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c6ec,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 62351;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10f38f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f38f,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42327;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10a557" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a557,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65068;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10fe2c" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10fe2c,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11003;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf102afb" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf102afb,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13752;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf1035b8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf1035b8,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63201;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10f6e1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f6e1,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11627;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf102d6b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf102d6b,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 37317;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf1091c5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf1091c5,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 39960;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf109c18" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf109c18,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 23049;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf105a09" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf105a09,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50598;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10c5a6" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c5a6,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13344;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf103420" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf103420,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18471;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf104827" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf104827,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1051;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10041b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10041b,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 64053;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10fa35" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10fa35,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 27670;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf106c16" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf106c16,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 58274;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10e3a2" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10e3a2,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 39590;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf109aa6" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf109aa6,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 46130;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10b432" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10b432,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 26650;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xdf10681a" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10681a,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 0;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf100000" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf100000,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65535;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10ffff" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10ffff,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 30504;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf107728" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf107728,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42415;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10a5af" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a5af,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50344;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10c4a8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c4a8,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54847;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10d63f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10d63f,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 41403;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10a1bb" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a1bb,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5465;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf101559" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf101559,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63857;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10f971" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f971,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59680;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10e920" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10e920,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4619;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10120b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10120b,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65518;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10ffee" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10ffee,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54372;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10d464" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10d464,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50924;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10c6ec" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c6ec,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 62351;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10f38f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f38f,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42327;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10a557" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a557,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65068;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10fe2c" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10fe2c,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11003;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf102afb" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf102afb,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13752;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf1035b8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf1035b8,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63201;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10f6e1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f6e1,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11627;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf102d6b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf102d6b,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 37317;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf1091c5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf1091c5,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 39960;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf109c18" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf109c18,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 23049;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf105a09" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf105a09,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50598;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10c5a6" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c5a6,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13344;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf103420" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf103420,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18471;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf104827" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf104827,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1051;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10041b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10041b,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 64053;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10fa35" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10fa35,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 27670;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf106c16" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf106c16,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 58274;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10e3a2" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10e3a2,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 39590;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf109aa6" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf109aa6,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 46130;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10b432" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10b432,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 26650;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xdf10681a" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10681a,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 0;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf100000" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf100000,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65535;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10ffff" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10ffff,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 30504;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf107728" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf107728,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42415;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10a5af" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a5af,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50344;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10c4a8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c4a8,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54847;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10d63f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10d63f,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 41403;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10a1bb" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a1bb,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5465;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf101559" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf101559,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63857;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10f971" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f971,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59680;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10e920" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10e920,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4619;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10120b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10120b,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65518;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10ffee" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10ffee,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54372;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10d464" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10d464,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50924;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10c6ec" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c6ec,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 62351;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10f38f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f38f,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42327;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10a557" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a557,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65068;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10fe2c" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10fe2c,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11003;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf102afb" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf102afb,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13752;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf1035b8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf1035b8,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63201;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10f6e1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f6e1,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11627;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf102d6b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf102d6b,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 37317;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf1091c5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf1091c5,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 39960;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf109c18" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf109c18,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 23049;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf105a09" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf105a09,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50598;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10c5a6" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c5a6,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13344;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf103420" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf103420,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18471;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf104827" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf104827,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1051;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10041b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10041b,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 64053;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10fa35" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10fa35,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 27670;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf106c16" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf106c16,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 58274;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10e3a2" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10e3a2,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 39590;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf109aa6" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf109aa6,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 46130;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10b432" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10b432,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 26650;
      rd[0] = (float)(int16_t)(imval);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xdf10681a" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10681a,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 0;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf100000" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf100000,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65535;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10ffff" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10ffff,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 30504;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf107728" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf107728,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42415;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10a5af" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a5af,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50344;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10c4a8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c4a8,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54847;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10d63f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10d63f,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 41403;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10a1bb" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a1bb,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 5465;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf101559" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf101559,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63857;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10f971" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f971,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 59680;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10e920" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10e920,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 4619;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10120b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10120b,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65518;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10ffee" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10ffee,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 54372;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10d464" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10d464,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50924;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10c6ec" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c6ec,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 62351;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10f38f" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f38f,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 42327;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10a557" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10a557,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 65068;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10fe2c" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10fe2c,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11003;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf102afb" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf102afb,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13752;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf1035b8" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf1035b8,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 63201;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10f6e1" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10f6e1,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 11627;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf102d6b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf102d6b,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 37317;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf1091c5" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf1091c5,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 39960;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf109c18" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf109c18,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 23049;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf105a09" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf105a09,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 50598;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10c5a6" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10c5a6,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 13344;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf103420" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf103420,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 18471;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf104827" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf104827,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 1051;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10041b" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10041b,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 64053;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10fa35" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10fa35,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 27670;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf106c16" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf106c16,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 58274;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10e3a2" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10e3a2,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 39590;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf109aa6" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf109aa6,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 46130;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10b432" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10b432,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 1,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      unsigned imval = 26650;
      rd[0] = (float)(int16_t)(imval);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdf10681a" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "viim.s",
                                                    .instw = 0xdf10681a,
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
