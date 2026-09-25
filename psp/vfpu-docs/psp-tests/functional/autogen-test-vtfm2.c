#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int run_vtfm2_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vtfm2.p (Vector by matrix transform)
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[4];
    float rt_ref[2];
    gen_inputs_f32(i >> 0, &rs_ref[0], 4);
    gen_inputs_f32(i >> 7, &rt_ref[0], 2);
    pfxinfo->inst_name = "vtfm2.p";
    pfxinfo->rt_tested = 1;
    pfxinfo->rd_tested = 1;
    {
      float rt[2];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0000e1, 2);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0000e1" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xdd0000e1,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_swz);
      }
    }
    {
      float rt[2];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0500e4, 2);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0500e4" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xdd0500e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_neg);
      }
    }
    {
      float rt[2];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0a00e4, 2);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0a00e4" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xdd0a00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_neg);
      }
    }
    {
      float rt[2];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0f00e4, 2);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0f00e4" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xdd0f00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_neg);
      }
    }
    {
      float rt[2];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd0005e4, 2);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd0005e4" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xdd0005e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_abs);
      }
    }
    {
      float rt[2];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd000ae4, 2);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd000ae4" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xdd000ae4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_abs);
      }
    }
    {
      float rt[2];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd000fe4, 2);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd000fe4" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xdd000fe4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_reg(rep->inputs[1], rt_ref);
        pfxinfo->rt_errors |= (1 << enum_abs);
      }
    }
    {
      float rt[2];
      pfx_src((uint32_t *)&rt[0], (uint32_t *)&rt_ref[0], 0xdd00f4a7, 2);
      float *rs = &rs_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdd00f4a7" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rt_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xdd00f4a7,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
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
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xde000f00,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
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
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xde000055,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
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
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
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
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S010, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S011, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S100, 0*4(%0)" ::"r"(&rt_ref[0]));
      asm volatile("lv.s S101, 1*4(%0)" ::"r"(&rt_ref[0]));
      rd[0] = rs[2 * 0] * rt[0] + rs[2 * 0 + 1] * rt[1];
      rd[1] = rs[2 * 1] * rt[0] + rs[2 * 1 + 1] * rt[1];
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xf0840090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vtfm2.p",
            .instw = 0xf0840090,
            .pfxw = 0x00000000,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "rt", .size = 2},
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
