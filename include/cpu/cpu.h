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

#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#include <common.h>

#define AHEAD_LENGTH 500

enum {
  NEMU_EXEC_RUNNING = 0, // unused by longjmp()
  NEMU_EXEC_END,
  NEMU_EXEC_AGAIN,
  NEMU_EXEC_EXCEPTION
};

void cpu_exec(uint64_t n);
__attribute__((noreturn)) void longjmp_exec(int cause);
__attribute__((noreturn)) void longjmp_exception(int ex_cause);

enum {
  SYS_STATE_UPDATE = 1,
  SYS_STATE_FLUSH_TCACHE = 2,
};
void set_sys_state_flag(int flag);
void mmu_tlb_flush(vaddr_t vaddr);

struct Decode;
void save_globals(struct Decode *s);
void fetch_decode(struct Decode *s, vaddr_t pc);
void lightqs_take_reg_snapshot();
void clint_take_snapshot();
void lightqs_take_spec_reg_snapshot();
void clint_take_spec_snapshot();
uint64_t lightqs_restore_reg_snapshot(uint64_t n);
void pmem_record_restore(uint64_t restore_inst_cnt);
void clint_restore_snapshot(uint64_t restore_inst_cnt);

struct lightqs_reg_ss {
  uint64_t inst_cnt;
  uint64_t br_cnt;
  // snapshot stores GPR CSR
  uint64_t mstatus, mcause, mepc, sstatus, scause, sepc,
  satp, mip, mie, mscratch, sscratch, mideleg, medeleg,
  mtval, stval, mtvec, stvec;
#ifdef CONFIG_RVV
  uint64_t vtype, vstart, vxsat, vxrm, vl;
#endif // CONFIG_RVV
  uint64_t gpr[32], fpr[32];
  uint64_t mode;
  uint64_t pc;
  uint64_t lr_addr, lr_valid;
  // RAM is store-logged at another position

  int ifetch_mmu_state;
  int data_mmu_state;
};
#endif
