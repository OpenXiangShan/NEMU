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
