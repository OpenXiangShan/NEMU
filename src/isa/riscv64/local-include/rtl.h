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
#include "trigger.h"

#define FBOX_MASK 0xFFFFFFFF00000000ull
#define HFBOX_MASK 0xFFFFFFFFFFFF0000ull
// The bit pattern for a default generated 32-bit floating-point NaN
#define defaultNaNF32UI 0x7FC00000

static inline def_rtl(fbox, rtlreg_t *dest, rtlreg_t *src) {
  rtl_ori(s, dest, src, FBOX_MASK);
}

static inline def_rtl(hfbox, rtlreg_t *dest, rtlreg_t *src) {
  rtl_ori(s, dest, src, HFBOX_MASK);
}

static inline def_rtl(funbox, rtlreg_t *dest, rtlreg_t *src) {
  if((*src & FBOX_MASK) == FBOX_MASK){
      rtl_andi(s, dest, src, ~FBOX_MASK);
  } else {
      *dest = defaultNaNF32UI;
  }
}

static inline def_rtl(fsr, rtlreg_t *fdest, rtlreg_t *src, int width) {
  if (width == FPCALL_W32 ) rtl_fbox(s, fdest, src);
  else if(width == FPCALL_W16 ) rtl_hfbox(s, fdest, src);
  else if (width == FPCALL_W64) rtl_mv(s, fdest, src);
  else assert(0);
  void fp_set_dirty();
  fp_set_dirty();
}

#ifdef CONFIG_RVV

static inline def_rtl(lr, rtlreg_t* dest, int r, int width) {
  rtl_mv(s, dest, &reg_l(r));
}

static inline def_rtl(sr, int r, const rtlreg_t *src1, int width) {
  if (r != 0) { rtl_mv(s, &reg_l(r), src1); }
}

#endif // CONFIG_RVV

#ifdef CONFIG_RVH

int riscv64_priv_hload(Decode *s, rtlreg_t *dest, const rtlreg_t * addr, int len, bool is_signed, bool is_hlvx);
int riscv64_priv_hstore(Decode *s, rtlreg_t *src, const rtlreg_t * addr, int len);

#endif // CONFIG_RVH

// Privileged instructions

/// @brief Do RISC-V 64 privileged instruction: SRET
/// @return the next PC after SRET
word_t riscv64_priv_sret();

/// @brief Do RISC-V 64 privileged instruction: MRET
/// @return the next PC after MRET
word_t riscv64_priv_mret();

#ifdef CONFIG_RV_SMRNMI
/// @brief Do RISC-V 64 privileged instruction: MNRET
/// @return the next PC after MNRET
word_t riscv64_priv_mnret();
#endif // CONFIG_RV_SMRNMI

#ifdef CONFIG_RV_SVINVAL
/// @brief Do RISC-V 64 privileged instruction: SFENCE.W.INVAL & SFENCE.INVAL.IR
/// Just check mode and do nothing in NEMU
/// @return no return value
void riscv64_priv_sfence_w_inval_ir();
#endif // CONFIG_RV_SVINVAL

/// @brief Do RISC-V 64 privileged instruction: WFI
/// @return no return value
void riscv64_priv_wfi();

#ifdef CONFIG_RV_ZAWRS
/// @brief Do RISC-V 64 privileged instruction: wrs.nto
/// @return no return value
void riscv64_priv_wrs_nto();
#endif // CONFIG_RV_ZAWRS

/// @brief Do RISC-V 64 privileged instruction: sfence.vma
/// @param vaddr the address to flush
/// @param asid the address space identifier
/// @return no return value
void riscv64_priv_sfence_vma(vaddr_t vaddr, word_t asid);

#ifdef CONFIG_RVH
/// @brief Do RISC-V 64 privileged instruction: hfence.vvma
/// @param vaddr the address to flush
/// @param asid the address space identifier
/// @return no return value
void riscv64_priv_hfence_vvma(vaddr_t vaddr, word_t asid);

/// @brief Do RISC-V 64 privileged instruction: hfence.gvma
/// @param vaddr the address to flush
/// @param vmid the virtual machine identifier
/// @return no return value
void riscv64_priv_hfence_gvma(vaddr_t vaddr, word_t vmid);
#endif // CONFIG_RVH

void riscv64_priv_csrrw(rtlreg_t *dest, word_t val, word_t csrid, word_t rd);
void riscv64_priv_csrrs(rtlreg_t *dest, word_t val, word_t csrid, word_t rs1);
void riscv64_priv_csrrc(rtlreg_t *dest, word_t val, word_t csrid, word_t rs1);

#endif // __RISCV64_RTL_H__
