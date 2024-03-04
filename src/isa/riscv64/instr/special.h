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

#include "../local-include/intr.h"
#include <profiling/profiling_control.h>
#include "cpu/difftest.h"

def_EHelper(inv) {
  save_globals(s);
#ifdef CONFIG_REPORT_ILLEGAL_INSTR
  rtl_hostcall(s, HOSTCALL_INV, NULL, NULL, NULL, 0);
  longjmp_exec(NEMU_EXEC_END);
#else
  longjmp_exception(EX_II);
#endif
}

def_EHelper(rt_inv) {
  save_globals(s);
  longjmp_exception(EX_II);
}

def_EHelper(nemu_trap) {
  save_globals(s);
  Log("nemu_trap case %lx",cpu.gpr[10]._64);
  if (cpu.gpr[10]._64 == 0x100) {
      extern void disable_time_intr();
      disable_time_intr();
      cpu.mie = mie->val;
      difftest_skip_ref();
  } else if (cpu.gpr[10]._64 == 0x101) {
    if (!workload_loaded) {
      reset_inst_counters();
      difftest_skip_ref();
    }
  } else {
      rtl_hostcall(s, HOSTCALL_EXIT,NULL, &cpu.gpr[10]._64, NULL, 0); // gpr[10] is $a0
      longjmp_exec(NEMU_EXEC_END);
  }
}
