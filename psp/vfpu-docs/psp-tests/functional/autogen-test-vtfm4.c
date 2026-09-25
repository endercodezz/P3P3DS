#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int run_vtfm4_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vtfm4.q (Vector by matrix transform)
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[16];
    float rt_ref[4];
    gen_inputs_f32(i >> 0, &rs_ref[0], 16);
    gen_inputs_f32(i >> 7, &rt_ref[0], 4);
    pfxinfo->inst_name = "vtfm4.q";
    pfxinfo->rt_tested = 1;
    pfxinfo->rd_tested = 1;
    {
      float rt[4];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd000039, 4);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd000039" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xdd000039,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_swz);
      }
    }
    {
      float rt[4];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0500e4, 4);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0500e4" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xdd0500e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_neg);
      }
    }
    {
      float rt[4];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0a00e4, 4);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0a00e4" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xdd0a00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_neg);
      }
    }
    {
      float rt[4];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0f00e4, 4);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0f00e4" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xdd0f00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_neg);
      }
    }
    {
      float rt[4];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0005e4, 4);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0005e4" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xdd0005e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_abs);
      }
    }
    {
      float rt[4];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd000ae4, 4);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd000ae4" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xdd000ae4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_abs);
      }
    }
    {
      float rt[4];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd000fe4, 4);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd000fe4" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xdd000fe4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_abs);
      }
    }
    {
      float rt[4];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd00f4a7, 4);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd00f4a7" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xdd00f4a7,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
                {.inpname = "rt", .size = 4},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_cst);
      }
    }
    {
      float *rs = &rs_ref[0]; // No src prefix
      float *rt = &rt_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xde000f00,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xde000055,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
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
      rd[0] = rs[4 * 0] * rt[0] + rs[4 * 0 + 1] * rt[1] +
              rs[4 * 0 + 2] * rt[2] + rs[4 * 0 + 3] * rt[3];
      rd[1] = rs[4 * 1] * rt[0] + rs[4 * 1 + 1] * rt[1] +
              rs[4 * 1 + 2] * rt[2] + rs[4 * 1 + 3] * rt[3];
      rd[2] = rs[4 * 2] * rt[0] + rs[4 * 2 + 1] * rt[1] +
              rs[4 * 2 + 2] * rt[2] + rs[4 * 2 + 3] * rt[3];
      rd[3] = rs[4 * 3] * rt[0] + rs[4 * 3 + 1] * rt[1] +
              rs[4 * 3 + 2] * rt[2] + rs[4 * 3 + 3] * rt[3];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xf1848090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vtfm4.q",
            .instw = 0xf1848090,
            .pfxw = 0x00000000,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 16},
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
