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

#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"
#include <difftest.h>

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
#ifndef __ICS_EXPORT
#define check_reg(r) same = difftest_check_reg(str(r), pc, ref_r->r, cpu.r) && same
  bool same = true;
  if (memcmp(&cpu, ref_r, sizeof(cpu.gpr))) {
    int i;
    for (i = 0; i < ARRLEN(cpu.gpr); i ++) {
      difftest_check_reg(reg_name(i, 4), pc, ref_r->gpr[i]._32, cpu.gpr[i]._32);
    }
    same = false;
  }
  check_reg(pc);
  check_reg(lo);
  check_reg(hi);

  return same;
#else
  return false;
#endif
}

void isa_difftest_attach() {
}
