#ifndef __X86_RTL_H__
#define __X86_RTL_H__

#include <rtl/rtl.h>
#include "reg.h"
#include "intr.h"

/* RTL pseudo instructions */

static inline def_rtl(lr, rtlreg_t* dest, int r, int width) {
  switch (width) {
    case 4: rtl_mv(s, dest, &reg_l(r)); return;
    case 1: rtl_host_lm(s, dest, &reg_b(r), 1); return;
    case 2: rtl_host_lm(s, dest, &reg_w(r), 2); return;
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

static inline def_rtl(sr, int r, const rtlreg_t* src1, int width) {
  switch (width) {
    case 4: rtl_mv(s, &reg_l(r), src1); return;
    case 1: rtl_host_sm(s, &reg_b(r), src1, 1); return;
    case 2: rtl_host_sm(s, &reg_w(r), src1, 2); return;
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

#ifndef __ICS_EXPORT
static inline def_rtl(push, const rtlreg_t* src1) {
  // esp <- esp - 4
  // M[esp] <- src1
  rtl_sm(s, src1, &cpu.esp, -4, 4, MMU_DYNAMIC);
  rtl_subi(s, &cpu.esp, &cpu.esp, 4);
}

static inline def_rtl(pop, rtlreg_t* dest) {
  // dest <- M[esp]
  // esp <- esp + 4
  rtl_lm(s, dest, &cpu.esp, 0, 4, MMU_DYNAMIC);
  rtl_addi(s, &cpu.esp, &cpu.esp, 4);
}

static inline def_rtl(is_sub_overflow, rtlreg_t* dest,
    const rtlreg_t* res, const rtlreg_t* src1, const rtlreg_t* src2, int width) {
  // dest <- is_overflow(src1 - src2)
  rtl_xor(s, t0, src1, src2);
  rtl_xor(s, dest, src1, res);
  rtl_and(s, dest, t0, dest);
  rtl_msb(s, dest, dest, width);
}

static inline def_rtl(is_sub_carry, rtlreg_t* dest,
    const rtlreg_t* src1, const rtlreg_t* src2) {
  // dest <- is_carry(src1 - src2)
  rtl_setrelop(s, RELOP_LTU, dest, src1, src2);
}

static inline def_rtl(is_add_overflow, rtlreg_t* dest,
    const rtlreg_t* res, const rtlreg_t* src1, const rtlreg_t* src2, int width) {
  // dest <- is_overflow(src1 + src2)
  rtl_is_sub_overflow(s, dest, src1, res, src2, width);
}

static inline def_rtl(is_add_carry, rtlreg_t* dest,
    const rtlreg_t* res, const rtlreg_t* src1) {
  // dest <- is_carry(src1 + src2)
  rtl_is_sub_carry(s, dest, res, src1);
}

#else
static inline def_rtl(push, const rtlreg_t* src1) {
  // esp <- esp - 4
  // M[esp] <- src1
  TODO();
}

static inline def_rtl(pop, rtlreg_t* dest) {
  // dest <- M[esp]
  // esp <- esp + 4
  TODO();
}

static inline def_rtl(is_sub_overflow, rtlreg_t* dest,
    const rtlreg_t* res, const rtlreg_t* src1, const rtlreg_t* src2, int width) {
  // dest <- is_overflow(src1 - src2)
  TODO();
}

static inline def_rtl(is_sub_carry, rtlreg_t* dest,
    const rtlreg_t* src1, const rtlreg_t* src2) {
  // dest <- is_carry(src1 - src2)
  TODO();
}

static inline def_rtl(is_add_overflow, rtlreg_t* dest,
    const rtlreg_t* res, const rtlreg_t* src1, const rtlreg_t* src2, int width) {
  // dest <- is_overflow(src1 + src2)
  TODO();
}

static inline def_rtl(is_add_carry, rtlreg_t* dest,
    const rtlreg_t* res, const rtlreg_t* src1) {
  // dest <- is_carry(src1 + src2)
  TODO();
}
#endif

#ifndef __ICS_EXPORT
#define def_rtl_setget_eflags(f) \
  static inline def_rtl(concat(set_, f), const rtlreg_t* src) { \
    rtl_mv(s, &cpu.f, src); \
  } \
  static inline def_rtl(concat(get_, f), rtlreg_t* dest) { \
    rtl_mv(s, dest, &cpu.f); \
  }

def_rtl_setget_eflags(CF)
def_rtl_setget_eflags(OF)
def_rtl_setget_eflags(ZF)
def_rtl_setget_eflags(SF)
def_rtl_setget_eflags(DF)
def_rtl_setget_eflags(IF)
def_rtl_setget_eflags(PF)

static inline def_rtl(update_ZF, const rtlreg_t* result, int width) {
  // eflags.ZF <- is_zero(result[width * 8 - 1 .. 0])
  if (width != 4) {
    rtl_andi(s, t0, result, 0xffffffffu >> ((4 - width) * 8));
    rtl_setrelopi(s, RELOP_EQ, t0, t0, 0);
  }
  else {
    rtl_setrelopi(s, RELOP_EQ, t0, result, 0);
  }
  rtl_set_ZF(s, t0);
}

static inline def_rtl(update_SF, const rtlreg_t* result, int width) {
  // eflags.SF <- is_sign(result[width * 8 - 1 .. 0])
  rtl_msb(s, t0, result, width);
  rtl_set_SF(s, t0);
}

static inline def_rtl(update_PF, const rtlreg_t* result) {
  // eflags.PF <- is_parity(result[7 .. 0])
  // HACK: `s2` is rarely used, so we use it here
  rtl_shri(s, t0, result, 4);
  rtl_xor(s, t0, t0, result);
  rtl_shri(s, s2, t0, 2);
  rtl_xor(s, t0, t0, s2);
  rtl_shri(s, s2, t0, 1);
  rtl_xor(s, t0, t0, s2);
  rtl_not(s, t0, t0);
  rtl_andi(s, t0, t0, 1);
  rtl_set_PF(s, t0);
}

static inline def_rtl(update_ZFSF, const rtlreg_t* result, int width) {
  rtl_update_ZF(s, result, width);
  rtl_update_SF(s, result, width);
#ifndef CONFIG_PA
  rtl_update_PF(s, result);
#endif
}
#else
#define def_rtl_setget_eflags(f) \
  static inline def_rtl(concat(set_, f), const rtlreg_t* src) { \
    TODO(); \
  } \
  static inline def_rtl(concat(get_, f), rtlreg_t* dest) { \
    TODO(); \
  }

def_rtl_setget_eflags(CF)
def_rtl_setget_eflags(OF)
def_rtl_setget_eflags(ZF)
def_rtl_setget_eflags(SF)

static inline def_rtl(update_ZF, const rtlreg_t* result, int width) {
  // eflags.ZF <- is_zero(result[width * 8 - 1 .. 0])
  TODO();
}

static inline def_rtl(update_SF, const rtlreg_t* result, int width) {
  // eflags.SF <- is_sign(result[width * 8 - 1 .. 0])
  TODO();
}

static inline def_rtl(update_ZFSF, const rtlreg_t* result, int width) {
  rtl_update_ZF(s, result, width);
  rtl_update_SF(s, result, width);
}
#endif
#endif
