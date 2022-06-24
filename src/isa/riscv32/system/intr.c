/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include "../local-include/rtl.h"
#include "../local-include/intr.h"

#ifndef __ICS_EXPORT
word_t raise_intr(uint32_t NO, vaddr_t epc) {
  cpu.scause = NO;
  cpu.sepc = epc;
  cpu.sstatus.spie = cpu.sstatus.sie;
  cpu.sstatus.sie = 0;
  return cpu.stvec;
}

void isa_query_intr() {
  if (cpu.INTR && cpu.sstatus.sie) {
    cpu.INTR = false;
    cpu.pc = raise_intr(0x80000005, cpu.pc);
  }
}

#else
word_t raise_intr(uint32_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  return 0;
}

void isa_query_intr() {
}
#endif
