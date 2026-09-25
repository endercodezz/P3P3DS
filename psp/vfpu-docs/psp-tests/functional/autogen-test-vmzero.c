#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int run_vmzero_p(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vmzero.p (Clear matrix to zero)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vmzero.p";
    pfxinfo->rd_tested = 1;
    {
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xf3860090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vmzero.p",
                                                    .instw = 0xf3860090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xf3860090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vmzero.p",
                                                    .instw = 0xf3860090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xf3860090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vmzero.p",
                                                    .instw = 0xf3860090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 4,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xf3860090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vmzero.p",
                                                    .instw = 0xf3860090,
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
int run_vmzero_t(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vmzero.t (Clear matrix to zero)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vmzero.t";
    pfxinfo->rd_tested = 1;
    {
      float rd[9] = {0}, rd_out[9] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      rd[4] = 0;
      rd[5] = 0;
      rd[6] = 0;
      rd[7] = 0;
      rd[8] = 0;
      ;
      float rd_final[9] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 9);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xf3868010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 3*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 4*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S412, 5*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S420, 6*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S421, 7*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S422, 8*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 9));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vmzero.t",
                                                    .instw = 0xf3868010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 9,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[9] = {0}, rd_out[9] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      rd[4] = 0;
      rd[5] = 0;
      rd[6] = 0;
      rd[7] = 0;
      rd[8] = 0;
      ;
      float rd_final[9] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 9);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xf3868010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 3*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 4*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S412, 5*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S420, 6*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S421, 7*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S422, 8*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 9));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vmzero.t",
                                                    .instw = 0xf3868010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 9,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[9] = {0}, rd_out[9] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      rd[4] = 0;
      rd[5] = 0;
      rd[6] = 0;
      rd[7] = 0;
      rd[8] = 0;
      ;
      float rd_final[9] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 9);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xf3868010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 3*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 4*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S412, 5*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S420, 6*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S421, 7*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S422, 8*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 9));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vmzero.t",
                                                    .instw = 0xf3868010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 9,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[9] = {0}, rd_out[9] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      rd[4] = 0;
      rd[5] = 0;
      rd[6] = 0;
      rd[7] = 0;
      rd[8] = 0;
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xf3868010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 3*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 4*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S412, 5*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S420, 6*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S421, 7*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S422, 8*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 9));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vmzero.t",
                                                    .instw = 0xf3868010,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 9,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
int run_vmzero_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vmzero.q (Clear matrix to zero)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vmzero.q";
    pfxinfo->rd_tested = 1;
    {
      float rd[16] = {0}, rd_out[16] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      rd[4] = 0;
      rd[5] = 0;
      rd[6] = 0;
      rd[7] = 0;
      rd[8] = 0;
      rd[9] = 0;
      rd[10] = 0;
      rd[11] = 0;
      rd[12] = 0;
      rd[13] = 0;
      rd[14] = 0;
      rd[15] = 0;
      ;
      float rd_final[16] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 16);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xf3868090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 4*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 5*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S412, 6*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S413, 7*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S420, 8*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S421, 9*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S422, 10*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S423, 11*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S430, 12*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S431, 13*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S432, 14*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S433, 15*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 16));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vmzero.q",
                                                    .instw = 0xf3868090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 16,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[16] = {0}, rd_out[16] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      rd[4] = 0;
      rd[5] = 0;
      rd[6] = 0;
      rd[7] = 0;
      rd[8] = 0;
      rd[9] = 0;
      rd[10] = 0;
      rd[11] = 0;
      rd[12] = 0;
      rd[13] = 0;
      rd[14] = 0;
      rd[15] = 0;
      ;
      float rd_final[16] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 16);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xf3868090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 4*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 5*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S412, 6*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S413, 7*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S420, 8*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S421, 9*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S422, 10*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S423, 11*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S430, 12*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S431, 13*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S432, 14*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S433, 15*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 16));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vmzero.q",
                                                    .instw = 0xf3868090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 16,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[16] = {0}, rd_out[16] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      rd[4] = 0;
      rd[5] = 0;
      rd[6] = 0;
      rd[7] = 0;
      rd[8] = 0;
      rd[9] = 0;
      rd[10] = 0;
      rd[11] = 0;
      rd[12] = 0;
      rd[13] = 0;
      rd[14] = 0;
      rd[15] = 0;
      ;
      float rd_final[16] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 16);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xf3868090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 4*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 5*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S412, 6*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S413, 7*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S420, 8*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S421, 9*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S422, 10*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S423, 11*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S430, 12*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S431, 13*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S432, 14*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S433, 15*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 16));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vmzero.q",
                                                    .instw = 0xf3868090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 16,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[16] = {0}, rd_out[16] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      rd[3] = 0;
      rd[4] = 0;
      rd[5] = 0;
      rd[6] = 0;
      rd[7] = 0;
      rd[8] = 0;
      rd[9] = 0;
      rd[10] = 0;
      rd[11] = 0;
      rd[12] = 0;
      rd[13] = 0;
      rd[14] = 0;
      rd[15] = 0;
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xf3868090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 4*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 5*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S412, 6*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S413, 7*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S420, 8*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S421, 9*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S422, 10*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S423, 11*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S430, 12*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S431, 13*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S432, 14*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S433, 15*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 16));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vmzero.q",
                                                    .instw = 0xf3868090,
                                                    .pfxw = 0x00000000,
                                                    .rdoutsz = 16,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
