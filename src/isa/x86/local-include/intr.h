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

#ifndef __X86_INTR_H__
#define __X86_INTR_H__

#include <common.h>

#ifdef CONFIG_PA
#define IRQ_TIMER 32
#else
#define IRQ_TIMER 48
#endif

word_t raise_intr(word_t NO, vaddr_t ret_addr);
#ifndef CONFIG_PA
#define return_on_mem_ex() do { if (cpu.mem_exception != 0) return; } while (0)
#else
#define return_on_mem_ex()
#endif

#endif
