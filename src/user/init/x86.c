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

#ifdef CONFIG_ISA_x86
// we only maintain base of the segment here
uint32_t GDT[4] = {0};

void isa_init_user(word_t sp) {
  cpu.esp = sp;
  cpu.edx = 0; // no handler for atexit()
  cpu.sreg[CSR_CS].val = 0xb; cpu.sreg[CSR_CS].base = 0;
  cpu.sreg[CSR_DS].val = 0xb; cpu.sreg[CSR_DS].base = 0;
  cpu.sreg[CSR_ES].val = 0xb; cpu.sreg[CSR_ES].base = 0;
  cpu.sreg[CSR_FS].val = 0xb; cpu.sreg[CSR_FS].base = 0;
}
#endif
