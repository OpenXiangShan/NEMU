/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __RTL_PSEUDO_H__
#define __RTL_PSEUDO_H__

#ifndef __RTL_RTL_H__
#error "Should be only included by <rtl/rtl.h>"
#endif

/* RTL pseudo instructions */

static inline def_rtl(li, rtlreg_t* dest, const rtlreg_t imm) {
  rtl_addi(s, dest, rz, imm);
}

static inline def_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1) {
  rtl_addi(s, dest, src1, 0);
}

static inline def_rtl(not, rtlreg_t *dest, const rtlreg_t* src1) {
  // dest <- ~src1
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_xori(s, dest, src1, -1);
#endif
}

static inline def_rtl(neg, rtlreg_t *dest, const rtlreg_t* src1) {
  // dest <- -src1
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_sub(s, dest, rz, src1);
#endif
}

static inline def_rtl(sext, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- signext(src1[(width * 8 - 1) .. 0])
#ifdef __ICS_EXPORT
  TODO();
#else
  const int word_size = sizeof(word_t);
  if (width == word_size) {
    rtl_mv(s, dest, src1);
  } else {
#ifdef CONFIG_RT_CHECK
    assert(width == 1 || width == 2 || MUXDEF(CONFIG_ISA64, width == 4, false));
#endif
    rtl_shli(s, dest, src1, (word_size - width) * 8);
    rtl_sari(s, dest, dest, (word_size - width) * 8);
  }
#endif
}

static inline def_rtl(zext, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- zeroext(src1[(width * 8 - 1) .. 0])
#ifdef __ICS_EXPORT
  TODO();
#else
  const int word_size = sizeof(word_t);
  if (width == word_size) {
    rtl_mv(s, dest, src1);
  } else {
#ifdef CONFIG_RT_CHECK
    assert(width == 1 || width == 2 || MUXDEF(CONFIG_ISA64, width == 4, false));
#endif
    rtl_shli(s, dest, src1, (word_size - width) * 8);
    rtl_shri(s, dest, dest, (word_size - width) * 8);
  }
#endif
}

static inline def_rtl(msb, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- src1[width * 8 - 1]
#ifdef __ICS_EXPORT
  TODO();
#else
  rtl_shri(s, dest, src1, width * 8 - 1);
  if (width != 4) {
    rtl_andi(s, dest, dest, 0x1);
  }
#endif
}

static inline def_rtl(trap, vaddr_t ret_pc, word_t NO) {
  rtl_li(s, t0, ret_pc);
  rtl_hostcall(s, HOSTCALL_TRAP, t0, t0, NULL, NO);
  rtl_jr(s, t0);
}
static inline def_rtl(mux, rtlreg_t* dest, const rtlreg_t* cond, const rtlreg_t* src1, const rtlreg_t* src2) {
  // dest <- (cond ? src1 : src2)
//  TODO();
  rtl_setrelopi(s, RELOP_EQ, t0, cond, 0);
  rtl_subi(s, t0, t0, 1);
  // t0 = mask
  rtl_and(s, s2, src1, t0);
  rtl_not(s, t0, t0);
  rtl_and(s, t0, src2, t0);
  rtl_or(s, dest, t0, s2);
}

#endif
