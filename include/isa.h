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

#ifndef __ISA_H__
#define __ISA_H__

// Located at src/isa/$(ISA)/include/isa-def.h
#include <isa-def.h>

// The macro `__ISA__` is defined in $(CFLAGS).
// It will be expanded as "x86" or "mips32" ...
typedef concat(__ISA__, _CPU_state) CPU_state;
typedef concat(__ISA__, _ISADecodeInfo) ISADecodeInfo;

// monitor
extern char isa_logo[];
void init_isa();

// reg
extern CPU_state cpu;
extern rtlreg_t csr_array[4096];
void isa_reg_display();
word_t isa_reg_str2val(const char *name, bool *success);

// exec
struct Decode;
int isa_fetch_decode(struct Decode *s);
void isa_hostcall(uint32_t id, rtlreg_t *dest, const rtlreg_t *src1,
    const rtlreg_t *src2, word_t imm);

// memory
enum { MMU_DIRECT, MMU_TRANSLATE, MMU_DYNAMIC };
enum { MEM_TYPE_IFETCH, MEM_TYPE_READ, MEM_TYPE_WRITE, MEM_TYPE_IFETCH_READ, MEM_TYPE_WRITE_READ }; // the last two is for ptw
enum { MEM_RET_OK, MEM_RET_FAIL, MEM_RET_CROSS_PAGE };
#ifndef isa_mmu_state
int isa_mmu_state();
#endif
#ifndef isa_mmu_check
int isa_mmu_check(vaddr_t vaddr, int len, int type);
#endif
paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type);
bool isa_pmp_check_permission(paddr_t addr, int len, int type, int mode);
bool isa_spmp_check_permission(paddr_t addr, int len, int type, int mode);

// interrupt
vaddr_t raise_intr(word_t NO, vaddr_t epc);
#define INTR_EMPTY ((word_t)-1)
word_t isa_query_intr();

// difftest
  // for dut
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc);
void isa_difftest_attach();

  // for ref
void isa_difftest_regcpy(void *dut, bool direction);
void isa_difftest_csrcpy(void *dut, bool direction);
void isa_difftest_raise_intr(word_t NO);
void isa_difftest_uarchstatus_cpy(void *dut, bool direction);
void isa_difftest_guided_exec(void *guide);
void isa_difftest_query_ref(void *result_buffer, uint64_t type);
#ifdef CONFIG_MULTICORE_DIFF
void isa_difftest_set_mhartid(int n);
#endif

#endif
