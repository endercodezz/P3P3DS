#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int run_vmmul_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vmmul.p (Matrix by matrix multiplication)
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[4];
    float rt_ref[4];
    gen_inputs_f32(i >> 0, &rs_ref[0], 4);
    gen_inputs_f32(i >> 7, &rt_ref[0], 4);
    pfxinfo->inst_name = "vmmul.p";
    pfxinfo->rd_tested = 1;
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 3*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 2] * rt[0 * 2] + rs[0 * 2 + 1] * rt[0 * 2 + 1];
      rd[1] = rs[1 * 2] * rt[0 * 2] + rs[1 * 2 + 1] * rt[0 * 2 + 1];
      rd[2] = rs[0 * 2] * rt[1 * 2] + rs[0 * 2 + 1] * rt[1 * 2 + 1];
      rd[3] = rs[1 * 2] * rt[1 * 2] + rs[1 * 2 + 1] * rt[1 * 2 + 1];
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xf0040090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.p",
            .instw = 0xf0040090,
            .pfxw = 0xde000f00,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 3*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 2] * rt[0 * 2] + rs[0 * 2 + 1] * rt[0 * 2 + 1];
      rd[1] = rs[1 * 2] * rt[0 * 2] + rs[1 * 2 + 1] * rt[0 * 2 + 1];
      rd[2] = rs[0 * 2] * rt[1 * 2] + rs[0 * 2 + 1] * rt[1 * 2 + 1];
      rd[3] = rs[1 * 2] * rt[1 * 2] + rs[1 * 2 + 1] * rt[1 * 2 + 1];
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xf0040090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.p",
            .instw = 0xf0040090,
            .pfxw = 0xde000055,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 3*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 2] * rt[0 * 2] + rs[0 * 2 + 1] * rt[0 * 2 + 1];
      rd[1] = rs[1 * 2] * rt[0 * 2] + rs[1 * 2 + 1] * rt[0 * 2 + 1];
      rd[2] = rs[0 * 2] * rt[1 * 2] + rs[0 * 2 + 1] * rt[1 * 2 + 1];
      rd[3] = rs[1 * 2] * rt[1 * 2] + rs[1 * 2 + 1] * rt[1 * 2 + 1];
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xf0040090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.p",
            .instw = 0xf0040090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 3*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 2] * rt[0 * 2] + rs[0 * 2 + 1] * rt[0 * 2 + 1];
      rd[1] = rs[1 * 2] * rt[0 * 2] + rs[1 * 2 + 1] * rt[0 * 2 + 1];
      rd[2] = rs[0 * 2] * rt[1 * 2] + rs[0 * 2 + 1] * rt[1 * 2 + 1];
      rd[3] = rs[1 * 2] * rt[1 * 2] + rs[1 * 2 + 1] * rt[1 * 2 + 1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xf0040090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S410, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S411, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.p",
            .instw = 0xf0040090,
            .pfxw = 0x00000000,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
int run_vmmul_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vmmul.t (Matrix by matrix multiplication)
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[9];
    float rt_ref[9];
    gen_inputs_f32(i >> 0, &rs_ref[0], 9);
    gen_inputs_f32(i >> 7, &rt_ref[0], 9);
    pfxinfo->inst_name = "vmmul.t";
    pfxinfo->rd_tested = 1;
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[9] = {0}, rd_out[9] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 4*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S012, 5*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S020, 6*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S021, 7*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S022, 8*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 3*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 4*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S112, 5*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S120, 6*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S121, 7*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S122, 8*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 3] * rt[0 * 3] + rs[0 * 3 + 1] * rt[0 * 3 + 1] +
              rs[0 * 3 + 2] * rt[0 * 3 + 2];
      rd[1] = rs[1 * 3] * rt[0 * 3] + rs[1 * 3 + 1] * rt[0 * 3 + 1] +
              rs[1 * 3 + 2] * rt[0 * 3 + 2];
      rd[2] = rs[2 * 3] * rt[0 * 3] + rs[2 * 3 + 1] * rt[0 * 3 + 1] +
              rs[2 * 3 + 2] * rt[0 * 3 + 2];
      rd[3] = rs[0 * 3] * rt[1 * 3] + rs[0 * 3 + 1] * rt[1 * 3 + 1] +
              rs[0 * 3 + 2] * rt[1 * 3 + 2];
      rd[4] = rs[1 * 3] * rt[1 * 3] + rs[1 * 3 + 1] * rt[1 * 3 + 1] +
              rs[1 * 3 + 2] * rt[1 * 3 + 2];
      rd[5] = rs[2 * 3] * rt[1 * 3] + rs[2 * 3 + 1] * rt[1 * 3 + 1] +
              rs[2 * 3 + 2] * rt[1 * 3 + 2];
      rd[6] = rs[0 * 3] * rt[2 * 3] + rs[0 * 3 + 1] * rt[2 * 3 + 1] +
              rs[0 * 3 + 2] * rt[2 * 3 + 2];
      rd[7] = rs[1 * 3] * rt[2 * 3] + rs[1 * 3 + 1] * rt[2 * 3 + 1] +
              rs[1 * 3 + 2] * rt[2 * 3 + 2];
      rd[8] = rs[2 * 3] * rt[2 * 3] + rs[2 * 3 + 1] * rt[2 * 3 + 1] +
              rs[2 * 3 + 2] * rt[2 * 3 + 2];
      ;
      float rd_final[9] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 9);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xf0048010" ::: "memory");
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
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.t",
            .instw = 0xf0048010,
            .pfxw = 0xde000f00,
            .rdoutsz = 9,
            .inputs = {
                {.inpname = "rs", .size = 9},
                {.inpname = "rt", .size = 9},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[9] = {0}, rd_out[9] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 4*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S012, 5*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S020, 6*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S021, 7*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S022, 8*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 3*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 4*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S112, 5*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S120, 6*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S121, 7*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S122, 8*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 3] * rt[0 * 3] + rs[0 * 3 + 1] * rt[0 * 3 + 1] +
              rs[0 * 3 + 2] * rt[0 * 3 + 2];
      rd[1] = rs[1 * 3] * rt[0 * 3] + rs[1 * 3 + 1] * rt[0 * 3 + 1] +
              rs[1 * 3 + 2] * rt[0 * 3 + 2];
      rd[2] = rs[2 * 3] * rt[0 * 3] + rs[2 * 3 + 1] * rt[0 * 3 + 1] +
              rs[2 * 3 + 2] * rt[0 * 3 + 2];
      rd[3] = rs[0 * 3] * rt[1 * 3] + rs[0 * 3 + 1] * rt[1 * 3 + 1] +
              rs[0 * 3 + 2] * rt[1 * 3 + 2];
      rd[4] = rs[1 * 3] * rt[1 * 3] + rs[1 * 3 + 1] * rt[1 * 3 + 1] +
              rs[1 * 3 + 2] * rt[1 * 3 + 2];
      rd[5] = rs[2 * 3] * rt[1 * 3] + rs[2 * 3 + 1] * rt[1 * 3 + 1] +
              rs[2 * 3 + 2] * rt[1 * 3 + 2];
      rd[6] = rs[0 * 3] * rt[2 * 3] + rs[0 * 3 + 1] * rt[2 * 3 + 1] +
              rs[0 * 3 + 2] * rt[2 * 3 + 2];
      rd[7] = rs[1 * 3] * rt[2 * 3] + rs[1 * 3 + 1] * rt[2 * 3 + 1] +
              rs[1 * 3 + 2] * rt[2 * 3 + 2];
      rd[8] = rs[2 * 3] * rt[2 * 3] + rs[2 * 3 + 1] * rt[2 * 3 + 1] +
              rs[2 * 3 + 2] * rt[2 * 3 + 2];
      ;
      float rd_final[9] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 9);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xf0048010" ::: "memory");
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
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.t",
            .instw = 0xf0048010,
            .pfxw = 0xde000055,
            .rdoutsz = 9,
            .inputs = {
                {.inpname = "rs", .size = 9},
                {.inpname = "rt", .size = 9},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[9] = {0}, rd_out[9] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 4*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S012, 5*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S020, 6*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S021, 7*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S022, 8*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 3*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 4*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S112, 5*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S120, 6*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S121, 7*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S122, 8*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 3] * rt[0 * 3] + rs[0 * 3 + 1] * rt[0 * 3 + 1] +
              rs[0 * 3 + 2] * rt[0 * 3 + 2];
      rd[1] = rs[1 * 3] * rt[0 * 3] + rs[1 * 3 + 1] * rt[0 * 3 + 1] +
              rs[1 * 3 + 2] * rt[0 * 3 + 2];
      rd[2] = rs[2 * 3] * rt[0 * 3] + rs[2 * 3 + 1] * rt[0 * 3 + 1] +
              rs[2 * 3 + 2] * rt[0 * 3 + 2];
      rd[3] = rs[0 * 3] * rt[1 * 3] + rs[0 * 3 + 1] * rt[1 * 3 + 1] +
              rs[0 * 3 + 2] * rt[1 * 3 + 2];
      rd[4] = rs[1 * 3] * rt[1 * 3] + rs[1 * 3 + 1] * rt[1 * 3 + 1] +
              rs[1 * 3 + 2] * rt[1 * 3 + 2];
      rd[5] = rs[2 * 3] * rt[1 * 3] + rs[2 * 3 + 1] * rt[1 * 3 + 1] +
              rs[2 * 3 + 2] * rt[1 * 3 + 2];
      rd[6] = rs[0 * 3] * rt[2 * 3] + rs[0 * 3 + 1] * rt[2 * 3 + 1] +
              rs[0 * 3 + 2] * rt[2 * 3 + 2];
      rd[7] = rs[1 * 3] * rt[2 * 3] + rs[1 * 3 + 1] * rt[2 * 3 + 1] +
              rs[1 * 3 + 2] * rt[2 * 3 + 2];
      rd[8] = rs[2 * 3] * rt[2 * 3] + rs[2 * 3 + 1] * rt[2 * 3 + 1] +
              rs[2 * 3 + 2] * rt[2 * 3 + 2];
      ;
      float rd_final[9] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 9);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xf0048010" ::: "memory");
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
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.t",
            .instw = 0xf0048010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 9,
            .inputs = {
                {.inpname = "rs", .size = 9},
                {.inpname = "rt", .size = 9},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[9] = {0}, rd_out[9] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 4*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S012, 5*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S020, 6*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S021, 7*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S022, 8*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 3*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 4*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S112, 5*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S120, 6*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S121, 7*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S122, 8*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 3] * rt[0 * 3] + rs[0 * 3 + 1] * rt[0 * 3 + 1] +
              rs[0 * 3 + 2] * rt[0 * 3 + 2];
      rd[1] = rs[1 * 3] * rt[0 * 3] + rs[1 * 3 + 1] * rt[0 * 3 + 1] +
              rs[1 * 3 + 2] * rt[0 * 3 + 2];
      rd[2] = rs[2 * 3] * rt[0 * 3] + rs[2 * 3 + 1] * rt[0 * 3 + 1] +
              rs[2 * 3 + 2] * rt[0 * 3 + 2];
      rd[3] = rs[0 * 3] * rt[1 * 3] + rs[0 * 3 + 1] * rt[1 * 3 + 1] +
              rs[0 * 3 + 2] * rt[1 * 3 + 2];
      rd[4] = rs[1 * 3] * rt[1 * 3] + rs[1 * 3 + 1] * rt[1 * 3 + 1] +
              rs[1 * 3 + 2] * rt[1 * 3 + 2];
      rd[5] = rs[2 * 3] * rt[1 * 3] + rs[2 * 3 + 1] * rt[1 * 3 + 1] +
              rs[2 * 3 + 2] * rt[1 * 3 + 2];
      rd[6] = rs[0 * 3] * rt[2 * 3] + rs[0 * 3 + 1] * rt[2 * 3 + 1] +
              rs[0 * 3 + 2] * rt[2 * 3 + 2];
      rd[7] = rs[1 * 3] * rt[2 * 3] + rs[1 * 3 + 1] * rt[2 * 3 + 1] +
              rs[1 * 3 + 2] * rt[2 * 3 + 2];
      rd[8] = rs[2 * 3] * rt[2 * 3] + rs[2 * 3 + 1] * rt[2 * 3 + 1] +
              rs[2 * 3 + 2] * rt[2 * 3 + 2];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xf0048010" ::: "memory");
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
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.t",
            .instw = 0xf0048010,
            .pfxw = 0x00000000,
            .rdoutsz = 9,
            .inputs = {
                {.inpname = "rs", .size = 9},
                {.inpname = "rt", .size = 9},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
int run_vmmul_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vmmul.q (Matrix by matrix multiplication)
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[16];
    float rt_ref[16];
    gen_inputs_f32(i >> 0, &rs_ref[0], 16);
    gen_inputs_f32(i >> 7, &rt_ref[0], 16);
    pfxinfo->inst_name = "vmmul.q";
    pfxinfo->rd_tested = 1;
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[16] = {0}, rd_out[16] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 4*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 5*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S012, 6*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S013, 7*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S020, 8*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S021, 9*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S022, 10*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S023, 11*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S030, 12*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S031, 13*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S032, 14*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S033, 15*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S103, 3*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 4*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 5*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S112, 6*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S113, 7*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S120, 8*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S121, 9*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S122, 10*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S123, 11*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S130, 12*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S131, 13*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S132, 14*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S133, 15*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 4] * rt[0 * 4] + rs[0 * 4 + 1] * rt[0 * 4 + 1] +
              rs[0 * 4 + 2] * rt[0 * 4 + 2] + rs[0 * 4 + 3] * rt[0 * 4 + 3];
      rd[1] = rs[1 * 4] * rt[0 * 4] + rs[1 * 4 + 1] * rt[0 * 4 + 1] +
              rs[1 * 4 + 2] * rt[0 * 4 + 2] + rs[1 * 4 + 3] * rt[0 * 4 + 3];
      rd[2] = rs[2 * 4] * rt[0 * 4] + rs[2 * 4 + 1] * rt[0 * 4 + 1] +
              rs[2 * 4 + 2] * rt[0 * 4 + 2] + rs[2 * 4 + 3] * rt[0 * 4 + 3];
      rd[3] = rs[3 * 4] * rt[0 * 4] + rs[3 * 4 + 1] * rt[0 * 4 + 1] +
              rs[3 * 4 + 2] * rt[0 * 4 + 2] + rs[3 * 4 + 3] * rt[0 * 4 + 3];
      rd[4] = rs[0 * 4] * rt[1 * 4] + rs[0 * 4 + 1] * rt[1 * 4 + 1] +
              rs[0 * 4 + 2] * rt[1 * 4 + 2] + rs[0 * 4 + 3] * rt[1 * 4 + 3];
      rd[5] = rs[1 * 4] * rt[1 * 4] + rs[1 * 4 + 1] * rt[1 * 4 + 1] +
              rs[1 * 4 + 2] * rt[1 * 4 + 2] + rs[1 * 4 + 3] * rt[1 * 4 + 3];
      rd[6] = rs[2 * 4] * rt[1 * 4] + rs[2 * 4 + 1] * rt[1 * 4 + 1] +
              rs[2 * 4 + 2] * rt[1 * 4 + 2] + rs[2 * 4 + 3] * rt[1 * 4 + 3];
      rd[7] = rs[3 * 4] * rt[1 * 4] + rs[3 * 4 + 1] * rt[1 * 4 + 1] +
              rs[3 * 4 + 2] * rt[1 * 4 + 2] + rs[3 * 4 + 3] * rt[1 * 4 + 3];
      rd[8] = rs[0 * 4] * rt[2 * 4] + rs[0 * 4 + 1] * rt[2 * 4 + 1] +
              rs[0 * 4 + 2] * rt[2 * 4 + 2] + rs[0 * 4 + 3] * rt[2 * 4 + 3];
      rd[9] = rs[1 * 4] * rt[2 * 4] + rs[1 * 4 + 1] * rt[2 * 4 + 1] +
              rs[1 * 4 + 2] * rt[2 * 4 + 2] + rs[1 * 4 + 3] * rt[2 * 4 + 3];
      rd[10] = rs[2 * 4] * rt[2 * 4] + rs[2 * 4 + 1] * rt[2 * 4 + 1] +
               rs[2 * 4 + 2] * rt[2 * 4 + 2] + rs[2 * 4 + 3] * rt[2 * 4 + 3];
      rd[11] = rs[3 * 4] * rt[2 * 4] + rs[3 * 4 + 1] * rt[2 * 4 + 1] +
               rs[3 * 4 + 2] * rt[2 * 4 + 2] + rs[3 * 4 + 3] * rt[2 * 4 + 3];
      rd[12] = rs[0 * 4] * rt[3 * 4] + rs[0 * 4 + 1] * rt[3 * 4 + 1] +
               rs[0 * 4 + 2] * rt[3 * 4 + 2] + rs[0 * 4 + 3] * rt[3 * 4 + 3];
      rd[13] = rs[1 * 4] * rt[3 * 4] + rs[1 * 4 + 1] * rt[3 * 4 + 1] +
               rs[1 * 4 + 2] * rt[3 * 4 + 2] + rs[1 * 4 + 3] * rt[3 * 4 + 3];
      rd[14] = rs[2 * 4] * rt[3 * 4] + rs[2 * 4 + 1] * rt[3 * 4 + 1] +
               rs[2 * 4 + 2] * rt[3 * 4 + 2] + rs[2 * 4 + 3] * rt[3 * 4 + 3];
      rd[15] = rs[3 * 4] * rt[3 * 4] + rs[3 * 4 + 1] * rt[3 * 4 + 1] +
               rs[3 * 4 + 2] * rt[3 * 4 + 2] + rs[3 * 4 + 3] * rt[3 * 4 + 3];
      ;
      float rd_final[16] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 16);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xf0048090" ::: "memory");
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
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.q",
            .instw = 0xf0048090,
            .pfxw = 0xde000f00,
            .rdoutsz = 16,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 16},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[16] = {0}, rd_out[16] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 4*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 5*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S012, 6*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S013, 7*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S020, 8*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S021, 9*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S022, 10*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S023, 11*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S030, 12*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S031, 13*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S032, 14*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S033, 15*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S103, 3*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 4*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 5*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S112, 6*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S113, 7*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S120, 8*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S121, 9*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S122, 10*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S123, 11*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S130, 12*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S131, 13*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S132, 14*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S133, 15*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 4] * rt[0 * 4] + rs[0 * 4 + 1] * rt[0 * 4 + 1] +
              rs[0 * 4 + 2] * rt[0 * 4 + 2] + rs[0 * 4 + 3] * rt[0 * 4 + 3];
      rd[1] = rs[1 * 4] * rt[0 * 4] + rs[1 * 4 + 1] * rt[0 * 4 + 1] +
              rs[1 * 4 + 2] * rt[0 * 4 + 2] + rs[1 * 4 + 3] * rt[0 * 4 + 3];
      rd[2] = rs[2 * 4] * rt[0 * 4] + rs[2 * 4 + 1] * rt[0 * 4 + 1] +
              rs[2 * 4 + 2] * rt[0 * 4 + 2] + rs[2 * 4 + 3] * rt[0 * 4 + 3];
      rd[3] = rs[3 * 4] * rt[0 * 4] + rs[3 * 4 + 1] * rt[0 * 4 + 1] +
              rs[3 * 4 + 2] * rt[0 * 4 + 2] + rs[3 * 4 + 3] * rt[0 * 4 + 3];
      rd[4] = rs[0 * 4] * rt[1 * 4] + rs[0 * 4 + 1] * rt[1 * 4 + 1] +
              rs[0 * 4 + 2] * rt[1 * 4 + 2] + rs[0 * 4 + 3] * rt[1 * 4 + 3];
      rd[5] = rs[1 * 4] * rt[1 * 4] + rs[1 * 4 + 1] * rt[1 * 4 + 1] +
              rs[1 * 4 + 2] * rt[1 * 4 + 2] + rs[1 * 4 + 3] * rt[1 * 4 + 3];
      rd[6] = rs[2 * 4] * rt[1 * 4] + rs[2 * 4 + 1] * rt[1 * 4 + 1] +
              rs[2 * 4 + 2] * rt[1 * 4 + 2] + rs[2 * 4 + 3] * rt[1 * 4 + 3];
      rd[7] = rs[3 * 4] * rt[1 * 4] + rs[3 * 4 + 1] * rt[1 * 4 + 1] +
              rs[3 * 4 + 2] * rt[1 * 4 + 2] + rs[3 * 4 + 3] * rt[1 * 4 + 3];
      rd[8] = rs[0 * 4] * rt[2 * 4] + rs[0 * 4 + 1] * rt[2 * 4 + 1] +
              rs[0 * 4 + 2] * rt[2 * 4 + 2] + rs[0 * 4 + 3] * rt[2 * 4 + 3];
      rd[9] = rs[1 * 4] * rt[2 * 4] + rs[1 * 4 + 1] * rt[2 * 4 + 1] +
              rs[1 * 4 + 2] * rt[2 * 4 + 2] + rs[1 * 4 + 3] * rt[2 * 4 + 3];
      rd[10] = rs[2 * 4] * rt[2 * 4] + rs[2 * 4 + 1] * rt[2 * 4 + 1] +
               rs[2 * 4 + 2] * rt[2 * 4 + 2] + rs[2 * 4 + 3] * rt[2 * 4 + 3];
      rd[11] = rs[3 * 4] * rt[2 * 4] + rs[3 * 4 + 1] * rt[2 * 4 + 1] +
               rs[3 * 4 + 2] * rt[2 * 4 + 2] + rs[3 * 4 + 3] * rt[2 * 4 + 3];
      rd[12] = rs[0 * 4] * rt[3 * 4] + rs[0 * 4 + 1] * rt[3 * 4 + 1] +
               rs[0 * 4 + 2] * rt[3 * 4 + 2] + rs[0 * 4 + 3] * rt[3 * 4 + 3];
      rd[13] = rs[1 * 4] * rt[3 * 4] + rs[1 * 4 + 1] * rt[3 * 4 + 1] +
               rs[1 * 4 + 2] * rt[3 * 4 + 2] + rs[1 * 4 + 3] * rt[3 * 4 + 3];
      rd[14] = rs[2 * 4] * rt[3 * 4] + rs[2 * 4 + 1] * rt[3 * 4 + 1] +
               rs[2 * 4 + 2] * rt[3 * 4 + 2] + rs[2 * 4 + 3] * rt[3 * 4 + 3];
      rd[15] = rs[3 * 4] * rt[3 * 4] + rs[3 * 4 + 1] * rt[3 * 4 + 1] +
               rs[3 * 4 + 2] * rt[3 * 4 + 2] + rs[3 * 4 + 3] * rt[3 * 4 + 3];
      ;
      float rd_final[16] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 16);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xf0048090" ::: "memory");
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
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.q",
            .instw = 0xf0048090,
            .pfxw = 0xde000055,
            .rdoutsz = 16,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 16},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[16] = {0}, rd_out[16] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 4*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 5*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S012, 6*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S013, 7*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S020, 8*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S021, 9*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S022, 10*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S023, 11*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S030, 12*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S031, 13*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S032, 14*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S033, 15*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S103, 3*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 4*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 5*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S112, 6*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S113, 7*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S120, 8*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S121, 9*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S122, 10*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S123, 11*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S130, 12*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S131, 13*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S132, 14*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S133, 15*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 4] * rt[0 * 4] + rs[0 * 4 + 1] * rt[0 * 4 + 1] +
              rs[0 * 4 + 2] * rt[0 * 4 + 2] + rs[0 * 4 + 3] * rt[0 * 4 + 3];
      rd[1] = rs[1 * 4] * rt[0 * 4] + rs[1 * 4 + 1] * rt[0 * 4 + 1] +
              rs[1 * 4 + 2] * rt[0 * 4 + 2] + rs[1 * 4 + 3] * rt[0 * 4 + 3];
      rd[2] = rs[2 * 4] * rt[0 * 4] + rs[2 * 4 + 1] * rt[0 * 4 + 1] +
              rs[2 * 4 + 2] * rt[0 * 4 + 2] + rs[2 * 4 + 3] * rt[0 * 4 + 3];
      rd[3] = rs[3 * 4] * rt[0 * 4] + rs[3 * 4 + 1] * rt[0 * 4 + 1] +
              rs[3 * 4 + 2] * rt[0 * 4 + 2] + rs[3 * 4 + 3] * rt[0 * 4 + 3];
      rd[4] = rs[0 * 4] * rt[1 * 4] + rs[0 * 4 + 1] * rt[1 * 4 + 1] +
              rs[0 * 4 + 2] * rt[1 * 4 + 2] + rs[0 * 4 + 3] * rt[1 * 4 + 3];
      rd[5] = rs[1 * 4] * rt[1 * 4] + rs[1 * 4 + 1] * rt[1 * 4 + 1] +
              rs[1 * 4 + 2] * rt[1 * 4 + 2] + rs[1 * 4 + 3] * rt[1 * 4 + 3];
      rd[6] = rs[2 * 4] * rt[1 * 4] + rs[2 * 4 + 1] * rt[1 * 4 + 1] +
              rs[2 * 4 + 2] * rt[1 * 4 + 2] + rs[2 * 4 + 3] * rt[1 * 4 + 3];
      rd[7] = rs[3 * 4] * rt[1 * 4] + rs[3 * 4 + 1] * rt[1 * 4 + 1] +
              rs[3 * 4 + 2] * rt[1 * 4 + 2] + rs[3 * 4 + 3] * rt[1 * 4 + 3];
      rd[8] = rs[0 * 4] * rt[2 * 4] + rs[0 * 4 + 1] * rt[2 * 4 + 1] +
              rs[0 * 4 + 2] * rt[2 * 4 + 2] + rs[0 * 4 + 3] * rt[2 * 4 + 3];
      rd[9] = rs[1 * 4] * rt[2 * 4] + rs[1 * 4 + 1] * rt[2 * 4 + 1] +
              rs[1 * 4 + 2] * rt[2 * 4 + 2] + rs[1 * 4 + 3] * rt[2 * 4 + 3];
      rd[10] = rs[2 * 4] * rt[2 * 4] + rs[2 * 4 + 1] * rt[2 * 4 + 1] +
               rs[2 * 4 + 2] * rt[2 * 4 + 2] + rs[2 * 4 + 3] * rt[2 * 4 + 3];
      rd[11] = rs[3 * 4] * rt[2 * 4] + rs[3 * 4 + 1] * rt[2 * 4 + 1] +
               rs[3 * 4 + 2] * rt[2 * 4 + 2] + rs[3 * 4 + 3] * rt[2 * 4 + 3];
      rd[12] = rs[0 * 4] * rt[3 * 4] + rs[0 * 4 + 1] * rt[3 * 4 + 1] +
               rs[0 * 4 + 2] * rt[3 * 4 + 2] + rs[0 * 4 + 3] * rt[3 * 4 + 3];
      rd[13] = rs[1 * 4] * rt[3 * 4] + rs[1 * 4 + 1] * rt[3 * 4 + 1] +
               rs[1 * 4 + 2] * rt[3 * 4 + 2] + rs[1 * 4 + 3] * rt[3 * 4 + 3];
      rd[14] = rs[2 * 4] * rt[3 * 4] + rs[2 * 4 + 1] * rt[3 * 4 + 1] +
               rs[2 * 4 + 2] * rt[3 * 4 + 2] + rs[2 * 4 + 3] * rt[3 * 4 + 3];
      rd[15] = rs[3 * 4] * rt[3 * 4] + rs[3 * 4 + 1] * rt[3 * 4 + 1] +
               rs[3 * 4 + 2] * rt[3 * 4 + 2] + rs[3 * 4 + 3] * rt[3 * 4 + 3];
      ;
      float rd_final[16] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 16);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xf0048090" ::: "memory");
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
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.q",
            .instw = 0xf0048090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 16,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 16},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[16] = {0}, rd_out[16] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 4*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 5*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S012, 6*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S013, 7*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S020, 8*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S021, 9*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S022, 10*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S023, 11*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S030, 12*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S031, 13*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S032, 14*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S033, 15*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S103, 3*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S110, 4*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S111, 5*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S112, 6*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S113, 7*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S120, 8*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S121, 9*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S122, 10*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S123, 11*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S130, 12*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S131, 13*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S132, 14*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S133, 15*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[0 * 4] * rt[0 * 4] + rs[0 * 4 + 1] * rt[0 * 4 + 1] +
              rs[0 * 4 + 2] * rt[0 * 4 + 2] + rs[0 * 4 + 3] * rt[0 * 4 + 3];
      rd[1] = rs[1 * 4] * rt[0 * 4] + rs[1 * 4 + 1] * rt[0 * 4 + 1] +
              rs[1 * 4 + 2] * rt[0 * 4 + 2] + rs[1 * 4 + 3] * rt[0 * 4 + 3];
      rd[2] = rs[2 * 4] * rt[0 * 4] + rs[2 * 4 + 1] * rt[0 * 4 + 1] +
              rs[2 * 4 + 2] * rt[0 * 4 + 2] + rs[2 * 4 + 3] * rt[0 * 4 + 3];
      rd[3] = rs[3 * 4] * rt[0 * 4] + rs[3 * 4 + 1] * rt[0 * 4 + 1] +
              rs[3 * 4 + 2] * rt[0 * 4 + 2] + rs[3 * 4 + 3] * rt[0 * 4 + 3];
      rd[4] = rs[0 * 4] * rt[1 * 4] + rs[0 * 4 + 1] * rt[1 * 4 + 1] +
              rs[0 * 4 + 2] * rt[1 * 4 + 2] + rs[0 * 4 + 3] * rt[1 * 4 + 3];
      rd[5] = rs[1 * 4] * rt[1 * 4] + rs[1 * 4 + 1] * rt[1 * 4 + 1] +
              rs[1 * 4 + 2] * rt[1 * 4 + 2] + rs[1 * 4 + 3] * rt[1 * 4 + 3];
      rd[6] = rs[2 * 4] * rt[1 * 4] + rs[2 * 4 + 1] * rt[1 * 4 + 1] +
              rs[2 * 4 + 2] * rt[1 * 4 + 2] + rs[2 * 4 + 3] * rt[1 * 4 + 3];
      rd[7] = rs[3 * 4] * rt[1 * 4] + rs[3 * 4 + 1] * rt[1 * 4 + 1] +
              rs[3 * 4 + 2] * rt[1 * 4 + 2] + rs[3 * 4 + 3] * rt[1 * 4 + 3];
      rd[8] = rs[0 * 4] * rt[2 * 4] + rs[0 * 4 + 1] * rt[2 * 4 + 1] +
              rs[0 * 4 + 2] * rt[2 * 4 + 2] + rs[0 * 4 + 3] * rt[2 * 4 + 3];
      rd[9] = rs[1 * 4] * rt[2 * 4] + rs[1 * 4 + 1] * rt[2 * 4 + 1] +
              rs[1 * 4 + 2] * rt[2 * 4 + 2] + rs[1 * 4 + 3] * rt[2 * 4 + 3];
      rd[10] = rs[2 * 4] * rt[2 * 4] + rs[2 * 4 + 1] * rt[2 * 4 + 1] +
               rs[2 * 4 + 2] * rt[2 * 4 + 2] + rs[2 * 4 + 3] * rt[2 * 4 + 3];
      rd[11] = rs[3 * 4] * rt[2 * 4] + rs[3 * 4 + 1] * rt[2 * 4 + 1] +
               rs[3 * 4 + 2] * rt[2 * 4 + 2] + rs[3 * 4 + 3] * rt[2 * 4 + 3];
      rd[12] = rs[0 * 4] * rt[3 * 4] + rs[0 * 4 + 1] * rt[3 * 4 + 1] +
               rs[0 * 4 + 2] * rt[3 * 4 + 2] + rs[0 * 4 + 3] * rt[3 * 4 + 3];
      rd[13] = rs[1 * 4] * rt[3 * 4] + rs[1 * 4 + 1] * rt[3 * 4 + 1] +
               rs[1 * 4 + 2] * rt[3 * 4 + 2] + rs[1 * 4 + 3] * rt[3 * 4 + 3];
      rd[14] = rs[2 * 4] * rt[3 * 4] + rs[2 * 4 + 1] * rt[3 * 4 + 1] +
               rs[2 * 4 + 2] * rt[3 * 4 + 2] + rs[2 * 4 + 3] * rt[3 * 4 + 3];
      rd[15] = rs[3 * 4] * rt[3 * 4] + rs[3 * 4 + 1] * rt[3 * 4 + 1] +
               rs[3 * 4 + 2] * rt[3 * 4 + 2] + rs[3 * 4 + 3] * rt[3 * 4 + 3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xf0048090" ::: "memory");
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
        static const struct test_decl_info iinfo = {
            .inst = "vmmul.q",
            .instw = 0xf0048090,
            .pfxw = 0x00000000,
            .rdoutsz = 16,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 16},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
