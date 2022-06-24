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

#ifndef __RISCV64_RTL_H__
#define __RISCV64_RTL_H__

#include <rtl/rtl.h>
#include "reg.h"
#include "csr.h"

#define FBOX_MASK 0xFFFFFFFF00000000ull
// The bit pattern for a default generated 32-bit floating-point NaN
#define defaultNaNF32UI 0x7FC00000

static inline def_rtl(fbox, rtlreg_t *dest, rtlreg_t *src) {
  rtl_ori(s, dest, src, FBOX_MASK);
}

static inline def_rtl(funbox, rtlreg_t *dest, rtlreg_t *src) {
  if((*src & FBOX_MASK) == FBOX_MASK){
      rtl_andi(s, dest, src, ~FBOX_MASK);
  } else {
      *dest = defaultNaNF32UI;
  }
}

static inline def_rtl(fsr, rtlreg_t *fdest, rtlreg_t *src, int width) {
  if (width == FPCALL_W32) rtl_fbox(s, fdest, src);
  else if (width == FPCALL_W64) rtl_mv(s, fdest, src);
  else assert(0);
  void fp_set_dirty();
  fp_set_dirty();
}

#ifdef CONFIG_RVV_010

static inline def_rtl(lr, rtlreg_t* dest, int r, int width) {
  rtl_mv(s, dest, &reg_l(r));
}

static inline def_rtl(sr, int r, const rtlreg_t *src1, int width) {
  if (r != 0) { rtl_mv(s, &reg_l(r), src1); }
}

#endif // CONFIG_RVV_010


#endif
