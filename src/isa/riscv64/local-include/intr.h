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

#ifndef __INTR_H__
#define __INTR_H__

#include <cpu/decode.h>
#include "csr.h"
enum {
  EX_IAM, // instruction address misaligned
  EX_IAF, // instruction address fault
  EX_II,  // illegal instruction
  EX_BP,  // breakpoint
  EX_LAM, // load address misaligned
  EX_LAF, // load address fault
  EX_SAM, // store/amo address misaligned
  EX_SAF, // store/amo address fault
  EX_ECU, // ecall from U-mode or VU-mode
  EX_ECS, // ecall from HS-mode
  EX_ECVS,// ecall from VS-mode, H-extention
  EX_ECM, // ecall from M-mode
  EX_IPF, // instruction page fault
  EX_LPF, // load page fault
  EX_RS0, // reserved
  EX_SPF, // store/amo page fault
  EX_IGPF = 20,// instruction guest-page fault, H-extention
  EX_LGPF,// load guest-page fault, H-extention
  EX_VI,  // virtual instruction, H-extention
  EX_SGPF // store/amo guest-page fault, H-extention
};

enum {
  IRQ_USIP,  // reserved yet
  IRQ_SSIP,
  IRQ_VSSIP,
  IRQ_MSIP,
  IRQ_UTIP,  // reserved yet
  IRQ_STIP,
  IRQ_VSTIP,
  IRQ_MTIP,
  IRQ_UEIP,  // reserved yet
  IRQ_SEIP,
  IRQ_VSEIP,
  IRQ_MEIP,
  IRQ_SGEI,  // Supervisor guest external interrupt
  IRQ_LCOFI, // Local counter overflow interrupt
};

#define INTR_BIT (1ULL << 63)

#define MIP_SSIP   (1ULL << IRQ_SSIP)
#define MIP_VSSIP  (1ULL << IRQ_VSSIP)
#define MIP_MSIP   (1ULL << IRQ_MSIP)
#define MIP_STIP   (1ULL << IRQ_STIP)
#define MIP_VSTIP  (1ULL << IRQ_VSTIP)
#define MIP_MTIP   (1ULL << IRQ_MTIP)
#define MIP_SEIP   (1ULL << IRQ_SEIP)
#define MIP_VSEIP  (1ULL << IRQ_VSEIP)
#define MIP_MEIP   (1ULL << IRQ_MEIP)
#define MIP_SGEIP  (1ULL << IRQ_SGEI)
#define MIP_LCOFIP (1ULL << IRQ_LCOFI)

#define VSI_MASK   (MIP_VSSIP | MIP_VSTIP | MIP_VSEIP)
#define HSI_MASK   (VSI_MASK | MIP_SGEIP)
#define SI_MASK    (MIP_SSIP | MIP_STIP | MIP_SEIP)
#define LCI_MASK   (~0x1FFFULL)

// now NEMU does not support EX_IAM,
// so it may ok to use EX_IAM to indicate a successful memory access
#define MEM_OK 0

word_t raise_intr(word_t NO, vaddr_t epc);
#define return_on_mem_ex() do { if (cpu.mem_exception != MEM_OK) return; } while (0)
bool intr_deleg_S(word_t exceptionNO);
bool intr_deleg_VS(word_t exceptionNO);
#ifdef CONFIG_RVH
#define INTR_TVAL_REG(ex) (*((intr_deleg_VS(ex)) ? (word_t *)vstval :(intr_deleg_S(ex)) ? (word_t *)stval : (word_t *)mtval))
#else
#define INTR_TVAL_REG(ex) (*((intr_deleg_S(ex)) ? (word_t *)stval : (word_t *)mtval))
#endif
#endif

#ifdef CONFIG_USE_XS_ARCH_CSRS
word_t INTR_TVAL_SV39_SEXT(word_t vaddr);
#endif
