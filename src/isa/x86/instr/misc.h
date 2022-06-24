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

def_EHelper(cpuid) {
  rtl_mv(s, &cpu.eax, rz);
  rtl_mv(s, &cpu.ebx, rz);
  rtl_mv(s, &cpu.ecx, rz);
  rtl_mv(s, &cpu.edx, rz);
  difftest_skip_ref();
}

def_EHelper(rdtsc) {
#if defined(CONFIG_DETERMINISTIC) || defined(CONFIG_ENGINE_INTERPRETER)
  rtl_li(s, &cpu.edx, 0);
  rtl_li(s, &cpu.eax, 0);
#else
  uint64_t tsc = get_time();
  cpu.edx = tsc >> 32;
  cpu.eax = tsc & 0xffffffff;
#endif

  difftest_skip_ref();
}

#if 0
static inline def_EHelper(fwait) {
  print_asm("fwait");
}

static inline def_EHelper(fpu) {
  rtl_trap(s, cpu.pc, 7);
}

static inline def_EHelper(hlt) {
  rtl_trap(s, s->seq_pc, IRQ_TIMER);
  if (ref_difftest_raise_intr) ref_difftest_raise_intr(IRQ_TIMER);
  print_asm("hlt");
}
#endif
