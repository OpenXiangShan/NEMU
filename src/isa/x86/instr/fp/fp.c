#include "../local-include/intr.h"
#include <rtl/fp.h>
#include <cpu/decode.h>
#include <cpu/cpu.h>

static uint32_t g_rm = FPCALL_RM_RNE;

void x86_fp_set_rm(uint32_t rm_x86) {
  switch (rm_x86) {
    case 0b00: g_rm = FPCALL_RM_RNE; break;
    case 0b01: g_rm = FPCALL_RM_RDN; break;
    case 0b10: g_rm = FPCALL_RM_RUP; break;
    case 0b11: g_rm = FPCALL_RM_RTZ; break;
  }
}

uint32_t x86_fp_get_rm() {
  switch (g_rm) {
    case FPCALL_RM_RNE: return 0b00;
    case FPCALL_RM_RDN: return 0b01;
    case FPCALL_RM_RUP: return 0b10;
    case FPCALL_RM_RTZ: return 0b11;
  }
  return 0;
}

uint32_t isa_fp_get_rm(Decode *s) {
  return g_rm;
}

void isa_fp_set_ex(uint32_t ex) {
  assert(0);
}

#include "../lazycc.h"

def_rtl(fcmp, const fpreg_t *src1, const fpreg_t *src2) {
#ifdef CONFIG_x86_CC_LAZY
  assert(s->isa.flag_def != 0);
  if (src1 != src2) {
    rtl_fltd(s, &cpu.cc_dest, src1, src2); // cc_dest = CF
    rtl_feqd(s, &cpu.cc_src1, src1, src2); // cc_src1 = ZF
  }
  int cc = (src1 == src2 ? LAZYCC_FCMP_SAME : LAZYCC_FCMP);
  rtl_set_lazycc(s, NULL, NULL, NULL, cc, 0);
#else
  if (src1 == src2) {
    rtl_set_CF(s, rz);
    rtl_li(s, t0, 1);
  } else {
    rtl_fltd(s, t0, src1, src2);
    rtl_set_CF(s, t0);
    rtl_feqd(s, t0, src1, src2);
  }
  rtl_set_ZF(s, t0);
  rtl_set_PF(s, rz);
#endif
}

def_rtl(fcmp_fsw, rtlreg_t *dest, const fpreg_t *src1, const fpreg_t *src2) {
  if (src1 == src2) {
    rtl_li(s, dest, 0x1000);
  } else {
    rtl_feqd(s, t0, src1, src2);
    rtl_slli(s, dest, t0, 6);
    rtl_fltd(s, t0, src1, src2);
    rtl_or(s, dest, dest, t0);
    rtl_slli(s, dest, dest, 8);
  }
}
