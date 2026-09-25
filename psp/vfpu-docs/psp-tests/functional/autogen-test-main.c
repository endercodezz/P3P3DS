#include "func-def.h"
#include "prefix-func.h"
#include "test-utils.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
int run_vadd_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vadd_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vadd_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vadd_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsub_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsub_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsub_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsub_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmul_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmul_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmul_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmul_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vdiv_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vdiv_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vdiv_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vdiv_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmin_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmin_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmin_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmin_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmax_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmax_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmax_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmax_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vscmp_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vscmp_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vscmp_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vscmp_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsge_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsge_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsge_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsge_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vslt_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vslt_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vslt_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vslt_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcrs_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcrsp_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vqmul_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsbn_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vscl_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vscl_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vscl_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vdot_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vdot_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vdot_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vdet_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vhdp_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vhdp_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vhdp_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmov_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmov_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmov_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmov_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vabs_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vabs_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vabs_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vabs_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vneg_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vneg_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vneg_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vneg_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsat0_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsat0_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsat0_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsat0_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsat1_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsat1_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsat1_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsat1_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vrcp_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vrcp_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vrcp_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vrcp_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vrsq_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vrsq_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vrsq_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vrsq_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsin_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsin_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsin_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsin_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcos_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcos_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcos_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcos_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vexp2_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vexp2_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vexp2_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vexp2_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vlog2_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vlog2_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vlog2_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vlog2_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vlgb_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsbz_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vwbn_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsqrt_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsqrt_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsqrt_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsqrt_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vasin_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vasin_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vasin_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vasin_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vnrcp_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vnrcp_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vnrcp_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vnrcp_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vnsin_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vnsin_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vnsin_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vnsin_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vrexp2_s(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vrexp2_p(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vrexp2_t(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vrexp2_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vsrt1_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsrt2_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsrt3_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsrt4_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vbfy1_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vbfy1_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vbfy2_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsgn_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsgn_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsgn_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsgn_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vocp_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vocp_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vocp_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vocp_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vi2f_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vi2f_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vi2f_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vi2f_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vf2in_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2in_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2in_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2in_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2iz_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2iz_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2iz_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2iz_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2iu_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2iu_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2iu_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2iu_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2id_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2id_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2id_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vf2id_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vrot_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vrot_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vrot_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vsocp_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vsocp_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vavg_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vavg_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vavg_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vfad_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vfad_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vfad_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcmp_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcmp_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcmp_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcmp_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vzero_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vzero_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vzero_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vzero_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vone_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vone_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vone_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vone_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vmmul_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmmul_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmmul_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmscl_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmscl_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmscl_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmmov_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmmov_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmmov_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmidt_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmidt_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmidt_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmzero_p(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vmzero_t(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vmzero_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vmone_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmone_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vmone_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vtfm2_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vtfm3_t(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vtfm4_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vhtfm2_p(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vhtfm3_t(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vhtfm4_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vcmovf_s(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vcmovf_t(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vcmovf_p(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vcmovf_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vcmovt_s(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vcmovt_t(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vcmovt_p(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vcmovt_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vi2uc_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vi2c_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vi2us_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vi2us_q(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vi2s_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vi2s_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vf2h_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vf2h_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vs2i_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vs2i_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vus2i_s(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vus2i_p(struct test_error_info *errlist,
                struct prefix_usage_info *pfxinfo);
int run_vc2i_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vuc2ifs_s(struct test_error_info *errlist,
                  struct prefix_usage_info *pfxinfo);
int run_vh2f_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vh2f_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vt4444_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vt5551_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_vt5650_q(struct test_error_info *errlist,
                 struct prefix_usage_info *pfxinfo);
int run_viim_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vfim_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcst_s(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcst_p(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcst_t(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_vcst_q(struct test_error_info *errlist,
               struct prefix_usage_info *pfxinfo);
int run_tests(struct test_error_info *errlist,
              struct prefix_usage_info *pfxinfo) {
  int errcnt = 0;
  errcnt += run_vadd_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vadd_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vadd_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vadd_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsub_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsub_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsub_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsub_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmul_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmul_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmul_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmul_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vdiv_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vdiv_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vdiv_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vdiv_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmin_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmin_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmin_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmin_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmax_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmax_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmax_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmax_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vscmp_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vscmp_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vscmp_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vscmp_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsge_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsge_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsge_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsge_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vslt_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vslt_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vslt_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vslt_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcrs_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcrsp_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vqmul_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsbn_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vscl_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vscl_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vscl_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vdot_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vdot_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vdot_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vdet_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vhdp_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vhdp_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vhdp_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmov_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmov_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmov_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmov_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vabs_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vabs_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vabs_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vabs_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vneg_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vneg_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vneg_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vneg_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsat0_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsat0_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsat0_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsat0_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsat1_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsat1_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsat1_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsat1_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrcp_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrcp_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrcp_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrcp_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrsq_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrsq_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrsq_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrsq_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsin_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsin_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsin_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsin_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcos_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcos_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcos_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcos_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vexp2_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vexp2_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vexp2_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vexp2_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vlog2_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vlog2_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vlog2_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vlog2_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vlgb_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsbz_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vwbn_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsqrt_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsqrt_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsqrt_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsqrt_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vasin_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vasin_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vasin_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vasin_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vnrcp_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vnrcp_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vnrcp_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vnrcp_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vnsin_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vnsin_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vnsin_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vnsin_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrexp2_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrexp2_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrexp2_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrexp2_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsrt1_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsrt2_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsrt3_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsrt4_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vbfy1_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vbfy1_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vbfy2_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsgn_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsgn_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsgn_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsgn_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vocp_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vocp_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vocp_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vocp_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2f_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2f_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2f_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2f_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2in_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2in_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2in_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2in_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2iz_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2iz_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2iz_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2iz_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2iu_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2iu_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2iu_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2iu_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2id_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2id_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2id_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2id_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrot_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrot_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vrot_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsocp_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vsocp_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vavg_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vavg_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vavg_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vfad_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vfad_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vfad_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmp_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmp_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmp_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmp_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vzero_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vzero_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vzero_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vzero_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vone_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vone_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vone_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vone_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmmul_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmmul_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmmul_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmscl_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmscl_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmscl_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmmov_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmmov_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmmov_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmidt_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmidt_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmidt_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmzero_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmzero_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmzero_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmone_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmone_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vmone_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vtfm2_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vtfm3_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vtfm4_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vhtfm2_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vhtfm3_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vhtfm4_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmovf_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmovf_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmovf_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmovf_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmovt_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmovt_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmovt_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcmovt_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2uc_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2c_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2us_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2us_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2s_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vi2s_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2h_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vf2h_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vs2i_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vs2i_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vus2i_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vus2i_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vc2i_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vuc2ifs_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vh2f_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vh2f_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vt4444_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vt5551_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_vt5650_q(&errlist[errcnt], pfxinfo++);
  errcnt += run_viim_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vfim_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcst_s(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcst_p(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcst_t(&errlist[errcnt], pfxinfo++);
  errcnt += run_vcst_q(&errlist[errcnt], pfxinfo++);
  return errcnt;
}
