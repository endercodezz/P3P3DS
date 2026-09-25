#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int run_vcrsp_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vcrsp.t (Vector cross product)
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[3];
    float rt_ref[3];
    gen_inputs_f32(i >> 0, &rs_ref[0], 3);
    gen_inputs_f32(i >> 7, &rt_ref[0], 3);
    pfxinfo->inst_name = "vcrsp.t";
    pfxinfo->rs_tested = 1;
    pfxinfo->rt_tested = 1;
    pfxinfo->rd_tested = 1;
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000c9, 3);
      float *rt = &rt_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000c9" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdc0000c9,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 3);
      float *rt = &rt_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 3);
      float *rt = &rt_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 3);
      float *rt = &rt_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 3);
      float *rt = &rt_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 3);
      float *rt = &rt_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 3);
      float *rt = &rt_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 3);
      float *rt = &rt_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rt[3];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0000c9, 3);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0000c9" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdd0000c9,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_swz);
      }
    }
    {
      float rt[3];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0500e4, 3);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0500e4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdd0500e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_neg);
      }
    }
    {
      float rt[3];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0a00e4, 3);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0a00e4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdd0a00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_neg);
      }
    }
    {
      float rt[3];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0f00e4, 3);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0f00e4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdd0f00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_neg);
      }
    }
    {
      float rt[3];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0005e4, 3);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0005e4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdd0005e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_abs);
      }
    }
    {
      float rt[3];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd000ae4, 3);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd000ae4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdd000ae4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_abs);
      }
    }
    {
      float rt[3];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd000fe4, 3);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd000fe4" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdd000fe4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_abs);
      }
    }
    {
      float rt[3];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd00f4a7, 3);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd00f4a7" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xdd00f4a7,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
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
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xde000f00,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
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
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xde000055,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
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
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
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
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S102, 2*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[1] * rt[2] - rs[2] * rt[1];
      rd[1] = rs[2] * rt[0] - rs[0] * rt[2];
      rd[2] = rs[0] * rt[1] - rs[1] * rt[0];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xf2848010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcrsp.t",
            .instw = 0xf2848010,
            .pfxw = 0x00000000,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "rt", .size = 3},
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
