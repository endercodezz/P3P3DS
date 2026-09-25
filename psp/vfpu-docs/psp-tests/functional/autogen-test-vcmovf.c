#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int run_vcmovf_s(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vcmovf.s (Conditional move (false))
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[1];
    unsigned int vfpu_cc_ref[6];
    gen_inputs_f32(i >> 0, &rs_ref[0], 1);
    gen_inputs_bits(i >> 7, &vfpu_cc_ref[0], 6);
    pfxinfo->inst_name = "vcmovf.s";
    pfxinfo->rs_tested = 1;
    pfxinfo->rd_tested = 1;
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e4" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xdc0000e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e4" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xdc0000e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e4" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xdc0000e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e4" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xdc0000e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e4" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xdc0000e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e4" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xdc0000e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e4" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xdc0000e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[1];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 1);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xde000f00,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xde000f00,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xde000f00,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xde000f00,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xde000f00,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xde000f00,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 1);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xde000f00,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xde000055,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xde000055,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xde000055,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xde000055,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xde000055,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xde000055,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 1);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xde000055,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float rd_final[1] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 1);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2a80010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a80010,
            .pfxw = 0x00000000,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2a90010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2a90010,
            .pfxw = 0x00000000,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2aa0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2aa0010,
            .pfxw = 0x00000000,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ab0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ab0010,
            .pfxw = 0x00000000,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ac0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ac0010,
            .pfxw = 0x00000000,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ad0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ad0010,
            .pfxw = 0x00000000,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[1] = {0}, rd_out[1] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ae0010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 1));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.s",
            .instw = 0xd2ae0010,
            .pfxw = 0x00000000,
            .rdoutsz = 1,
            .inputs = {
                {.inpname = "rs", .size = 1},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
int run_vcmovf_t(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vcmovf.t (Conditional move (false))
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[3];
    unsigned int vfpu_cc_ref[6];
    gen_inputs_f32(i >> 0, &rs_ref[0], 3);
    gen_inputs_bits(i >> 7, &vfpu_cc_ref[0], 6);
    pfxinfo->inst_name = "vcmovf.t";
    pfxinfo->rs_tested = 1;
    pfxinfo->rd_tested = 1;
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000c9, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000c9" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xdc0000c9,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000c9, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000c9" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xdc0000c9,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000c9, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000c9" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xdc0000c9,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000c9, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000c9" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xdc0000c9,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000c9, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000c9" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xdc0000c9,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000c9, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000c9" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xdc0000c9,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000c9, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000c9" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xdc0000c9,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[3];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 3);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xde000f00,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xde000f00,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xde000f00,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xde000f00,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xde000f00,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xde000f00,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 3);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xde000f00,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xde000055,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xde000055,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xde000055,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xde000055,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xde000055,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xde000055,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 3);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xde000055,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float rd_final[3] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 3);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0xde0000ff,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2a88010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a88010,
            .pfxw = 0x00000000,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2a98010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2a98010,
            .pfxw = 0x00000000,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2aa8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2aa8010,
            .pfxw = 0x00000000,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ab8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ab8010,
            .pfxw = 0x00000000,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ac8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ac8010,
            .pfxw = 0x00000000,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ad8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ad8010,
            .pfxw = 0x00000000,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[3] = {0}, rd_out[3] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ae8010" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 3));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.t",
            .instw = 0xd2ae8010,
            .pfxw = 0x00000000,
            .rdoutsz = 3,
            .inputs = {
                {.inpname = "rs", .size = 3},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
int run_vcmovf_p(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vcmovf.p (Conditional move (false))
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[2];
    unsigned int vfpu_cc_ref[6];
    gen_inputs_f32(i >> 0, &rs_ref[0], 2);
    gen_inputs_bits(i >> 7, &vfpu_cc_ref[0], 6);
    pfxinfo->inst_name = "vcmovf.p";
    pfxinfo->rs_tested = 1;
    pfxinfo->rd_tested = 1;
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e1, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e1" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xdc0000e1,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e1, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e1" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xdc0000e1,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e1, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e1" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xdc0000e1,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e1, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e1" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xdc0000e1,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e1, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e1" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xdc0000e1,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e1, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e1" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xdc0000e1,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0000e1, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0000e1" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xdc0000e1,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[2];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 2);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xde000f00,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xde000f00,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xde000f00,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xde000f00,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xde000f00,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xde000f00,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 2);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xde000f00,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xde000055,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xde000055,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xde000055,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xde000055,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xde000055,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xde000055,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 2);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xde000055,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float rd_final[2] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 2);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2a80090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a80090,
            .pfxw = 0x00000000,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2a90090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2a90090,
            .pfxw = 0x00000000,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2aa0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2aa0090,
            .pfxw = 0x00000000,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ab0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ab0090,
            .pfxw = 0x00000000,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ac0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ac0090,
            .pfxw = 0x00000000,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ad0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ad0090,
            .pfxw = 0x00000000,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[2] = {0}, rd_out[2] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ae0090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 2));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.p",
            .instw = 0xd2ae0090,
            .pfxw = 0x00000000,
            .rdoutsz = 2,
            .inputs = {
                {.inpname = "rs", .size = 2},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
int run_vcmovf_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  // Tests for instruction vcmovf.q (Conditional move (false))
  for (unsigned i = 0; i < 16384; i++) {
    float rs_ref[4];
    unsigned int vfpu_cc_ref[6];
    gen_inputs_f32(i >> 0, &rs_ref[0], 4);
    gen_inputs_bits(i >> 7, &vfpu_cc_ref[0], 6);
    pfxinfo->inst_name = "vcmovf.q";
    pfxinfo->rs_tested = 1;
    pfxinfo->rd_tested = 1;
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000039, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000039" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xdc000039,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000039, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000039" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xdc000039,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000039, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000039" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xdc000039,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000039, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000039" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xdc000039,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000039, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000039" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xdc000039,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000039, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000039" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xdc000039,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000039, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000039" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_swz_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xdc000039,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_swz);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0500e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0500e4" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xdc0500e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0a00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0a00e4" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xdc0a00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0f00e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0f00e4" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_neg_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xdc0f00e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_neg);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc0005e4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc0005e4" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xdc0005e4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000ae4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000ae4" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xdc000ae4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc000fe4, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc000fe4" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_abs_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xdc000fe4,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_abs);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float rs[4];
      pfx_src((uint32_t *)&rs[0], (uint32_t *)&rs_ref[0], 0xdc00f4a7, 4);
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xdc00f4a7" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rs_cst_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xdc00f4a7,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rs_errors |= (1 << enum_cst);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xde000f00,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xde000f00,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xde000f00,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xde000f00,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xde000f00,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xde000f00,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000f00, 4);
      asm volatile(".word 0xde000f00" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_msk_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xde000f00,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_msk);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xde000055,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xde000055,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xde000055,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xde000055,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xde000055,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xde000055,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde000055, 4);
      asm volatile(".word 0xde000055" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xde000055,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float rd_final[4] = {0};
      pfx_dst((uint32_t *)&rd_final[0], (uint32_t *)&rd[0], 0xde0000ff, 4);
      asm volatile(".word 0xde0000ff" ::: "memory");
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &pfxinfo->rd_sat_err;
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0xde0000ff,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        pfxinfo->rd_errors |= (1 << enum_sat);
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 0;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2a88090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a88090,
            .pfxw = 0x00000000,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 1;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2a98090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2a98090,
            .pfxw = 0x00000000,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 2;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2aa8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2aa8090,
            .pfxw = 0x00000000,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 3;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ab8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ab8090,
            .pfxw = 0x00000000,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 4;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ac8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ac8090,
            .pfxw = 0x00000000,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 5;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ad8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ad8090,
            .pfxw = 0x00000000,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
    {
      float *rs = &rs_ref[0];                  // No src prefix
      unsigned int *vfpu_cc = &vfpu_cc_ref[0]; // No src prefix
      float rd[4] = {0}, rd_out[4] = {0};
      asm volatile("mtvc $0, $131;vmzero.q M400");
      asm volatile("lv.s S000, 0*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S001, 1*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S002, 2*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("lv.s S003, 3*4(%0)" ::"r"(&rs_ref[0]));
      asm volatile("mtvc %0, $131" ::"r"(pack6(vfpu_cc)));
      unsigned cc_sel = 6;
      rd[0] = (cc_sel == 6) ? (vfpu_cc[0] ? rd[0] : rs[0])
                            : (vfpu_cc[cc_sel] ? rd[0] : rs[0]);
      rd[1] = (cc_sel == 6) ? (vfpu_cc[1] ? rd[1] : rs[1])
                            : (vfpu_cc[cc_sel] ? rd[1] : rs[1]);
      rd[2] = (cc_sel == 6) ? (vfpu_cc[2] ? rd[2] : rs[2])
                            : (vfpu_cc[cc_sel] ? rd[2] : rs[2]);
      rd[3] = (cc_sel == 6) ? (vfpu_cc[3] ? rd[3] : rs[3])
                            : (vfpu_cc[cc_sel] ? rd[3] : rs[3]);
      ;
      float *rd_final = &rd[0]; // No dst prefix
      asm volatile(".word 0xd2ae8090" ::: "memory");
      asm volatile("sv.s S400, 0*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S401, 1*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S402, 2*4(%0)" ::"r"(&rd_out[0]));
      asm volatile("sv.s S403, 3*4(%0)" ::"r"(&rd_out[0]));
      int mm = (fpdifn(&rd_final[0], &rd_out[0], 4));
      if (mm) {
        struct test_error_info *rep = &errlist[errcnt];
        static const struct test_decl_info iinfo = {
            .inst = "vcmovf.q",
            .instw = 0xd2ae8090,
            .pfxw = 0x00000000,
            .rdoutsz = 4,
            .inputs = {
                {.inpname = "rs", .size = 4},
                {.inpname = "vfpu_cc", .size = 6},
            }};
        fill_errinfo(rep, &iinfo, rd_final, rd_out);
        fill_reg(rep->inputs[0], rs_ref);
        fill_cc(rep->inputs[1], pack6(vfpu_cc));
        errcnt++;
        break;
      }
    }
  }

  return errcnt;
}
