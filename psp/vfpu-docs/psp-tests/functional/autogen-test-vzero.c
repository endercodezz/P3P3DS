#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int run_vzero_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vzero.s (Clear vector to zero)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vzero.s";
    pfxinfo->rd_tested = 1;
    {
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0060010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.s",
                                                    .instw = 0xd0060010,
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
      rd[0] = 0;
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0060010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.s",
                                                    .instw = 0xd0060010,
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
      rd[0] = 0;
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0060010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.s",
                                                    .instw = 0xd0060010,
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
      rd[0] = 0;
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0060010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vzero.s",
                                                    .instw = 0xd0060010,
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
int run_vzero_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vzero.p (Clear vector to zero)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vzero.p";
    pfxinfo->rd_tested = 1;
    {
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0060090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.p",
                                                    .instw = 0xd0060090,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0060090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.p",
                                                    .instw = 0xd0060090,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0060090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.p",
                                                    .instw = 0xd0060090,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 2,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0060090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vzero.p",
                                                    .instw = 0xd0060090,
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
int run_vzero_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vzero.t (Clear vector to zero)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vzero.t";
    pfxinfo->rd_tested = 1;
    {
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd0068010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.t",
                                                    .instw = 0xd0068010,
                                                    .pfxw = 0xde000f00,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd0068010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.t",
                                                    .instw = 0xd0068010,
                                                    .pfxw = 0xde000055,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd0068010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.t",
                                                    .instw = 0xd0068010,
                                                    .pfxw = 0xde0000ff,
                                                    .rdoutsz = 3,
                                                    .inputs = {}};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      rd[0] = 0;
      rd[1] = 0;
      rd[2] = 0;
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd0068010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vzero.t",
                                                    .instw = 0xd0068010,
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
int run_vzero_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vzero.q (Clear vector to zero)
  for (unsigned i = 0; i < 1; i++) {
    pfxinfo->inst_name = "vzero.q";
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
      asm volatile(".word 0xd0068090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.q",
                                                    .instw = 0xd0068090,
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
      asm volatile(".word 0xd0068090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.q",
                                                    .instw = 0xd0068090,
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
      asm volatile(".word 0xd0068090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {.inst = "vzero.q",
                                                    .instw = 0xd0068090,
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
      asm volatile(".word 0xd0068090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {.inst = "vzero.q",
                                                    .instw = 0xd0068090,
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
