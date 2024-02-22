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

#include <isa.h>
#include <cpu/cpu.h>
#include <difftest.h>
#include "../local-include/intr.h"
#include "../local-include/csr.h"

// csr_prepare() & csr_writeback() are used to maintain 
// a compact mirror of critical CSRs
// For processor difftest only 
static void csr_prepare() {
  cpu.mstatus = mstatus->val;
  cpu.mcause  = mcause->val;
  cpu.mepc    = mepc->val;

  cpu.sstatus = csrid_read(0x100); // sstatus
  cpu.scause  = scause->val;
  cpu.sepc    = sepc->val;

  cpu.satp     = satp->val;
  cpu.mip      = mip->val;
  cpu.mie      = mie->val;
  cpu.mscratch = mscratch->val;
  cpu.sscratch = sscratch->val;
  cpu.mideleg  = mideleg->val;
  cpu.medeleg  = medeleg->val;
  cpu.mtval    = mtval->val;
  cpu.stval    = stval->val;
  cpu.mtvec    = mtvec->val;
  cpu.stvec    = stvec->val;

#ifdef CONFIG_RV_DASICS
  cpu.dsmcfg    = dsmcfg->val;
  cpu.dsmbound0 = dsmbound0->val;
  cpu.dsmbound1 = dsmbound1->val;

  cpu.dumcfg    = csrid_read(0x9e0);  // dumcfg
  cpu.dumbound0 = dumbound0->val;
  cpu.dumbound1 = dumbound1->val;

  /* Yet to connect
  cpu.dlcfg0    = dlcfg0->val;
  cpu.dlbound0  = dlbound0->val;
  cpu.dlbound1  = dlbound1->val;
  cpu.dlbound2  = dlbound2->val;
  cpu.dlbound3  = dlbound3->val;
  cpu.dlbound4  = dlbound4->val;
  cpu.dlbound5  = dlbound5->val;
  cpu.dlbound6  = dlbound6->val;
  cpu.dlbound7  = dlbound7->val;
  */

  cpu.dmaincall = dmaincall->val;
  cpu.dretpc    = dretpc->val;
  cpu.dretpcfz  = dretpcfz->val;
#endif  // CONFIG_RV_DASICS

#ifdef CONFIG_RVV
  cpu.vstart  = vstart->val;
  cpu.vxsat   = vxsat->val;
  cpu.vxrm    = vxrm->val;
  cpu.vcsr    = vcsr->val;
  cpu.vl      = vl->val;
  cpu.vtype   = vtype->val;
  cpu.vlenb   = vlenb->val;
#endif // CONFIG_RVV
}

static void csr_writeback() {
  mstatus->val = cpu.mstatus;
  mcause ->val = cpu.mcause ;
  mepc   ->val = cpu.mepc   ;
  //sstatus->val = cpu.sstatus;  // sstatus is a shadow of mstatus
  scause ->val = cpu.scause ;
  sepc   ->val = cpu.sepc   ;

  satp->val     = cpu.satp;
  mip->val      = cpu.mip;
  mie->val      = cpu.mie;
  mscratch->val = cpu.mscratch;
  sscratch->val = cpu.sscratch;
  mideleg->val  = cpu.mideleg;
  medeleg->val  = cpu.medeleg;
  mtval->val    = cpu.mtval;
  stval->val    = cpu.stval;
  mtvec->val    = cpu.mtvec;
  stvec->val    = cpu.stvec;

#ifdef CONFIG_RV_DASICS
  dsmcfg->val    = cpu.dsmcfg;
  dsmbound0->val = cpu.dsmbound0;
  dsmbound1->val = cpu.dsmbound1;

  // dumcfg->val    = cpu.dumcfg;  // dumcfg is a shadow of dsmcfg
  dumbound0->val = cpu.dumbound0;
  dumbound1->val = cpu.dumbound1;

  /* Yet to connect
  dlcfg0->val    = cpu.dlcfg0;
  dlbound0->val  = cpu.dlbound0;
  dlbound1->val  = cpu.dlbound1;
  dlbound2->val  = cpu.dlbound2;
  dlbound3->val  = cpu.dlbound3;
  dlbound4->val  = cpu.dlbound4;
  dlbound5->val  = cpu.dlbound5;
  dlbound6->val  = cpu.dlbound6;
  dlbound7->val  = cpu.dlbound7;
  */

  dmaincall->val = cpu.dmaincall;
  dretpc->val    = cpu.dretpc;
  dretpcfz->val  = cpu.dretpcfz;
#endif  // CONFIG_RV_DASICS

#ifdef CONFIG_RVV
  vstart->val  = cpu.vstart;
  vxsat->val   = cpu.vxsat;
  vxrm->val    = cpu.vxrm;
  vcsr->val    = cpu.vcsr;
  vl->val      = cpu.vl;
  vtype->val   = cpu.vtype;
  vlenb->val   = cpu.vlenb;
#endif //CONFIG_RVV
}

void isa_difftest_regcpy(void *dut, bool direction) {
  if (direction == DIFFTEST_TO_REF) {
    memcpy(&cpu, dut, DIFFTEST_REG_SIZE);
    csr_writeback();
  } else {
    csr_prepare();
    memcpy(dut, &cpu, DIFFTEST_REG_SIZE);
  }
}

void isa_difftest_csrcpy(void *dut, bool direction) {
  if (direction == DIFFTEST_TO_REF) {
    memcpy(csr_array, dut, 4096 * sizeof(rtlreg_t));
  } else {
    memcpy(dut, csr_array, 4096 * sizeof(rtlreg_t));
  }
}

void isa_difftest_uarchstatus_cpy(void *dut, bool direction) {
  if (direction == DIFFTEST_TO_REF) {
    struct SyncState* ms = (struct SyncState*)dut;
    cpu.lr_valid = ms->lrscValid;
  } else {
    struct SyncState ms;
    ms.lrscValid = cpu.lr_valid;
    ms.lrscAddr = cpu.lr_addr;
    memcpy(dut, &ms, sizeof(struct SyncState));
  }
}

void isa_difftest_raise_intr(word_t NO) {
  cpu.pc = raise_intr(NO, cpu.pc);
}

#ifdef CONFIG_GUIDED_EXEC
void isa_difftest_guided_exec(void * guide) {
  memcpy(&cpu.execution_guide, guide, sizeof(struct ExecutionGuide));

  cpu.guided_exec = true;
  cpu_exec(1);
  cpu.guided_exec = false;
}
#endif

#ifdef CONFIG_QUERY_REF
void isa_difftest_query_ref(void *result_buffer, uint64_t type) {
  size_t size;
  switch(type) {
    case REF_QUERY_MEM_EVENT:
      cpu.query_mem_event.pc = cpu.debug.current_pc; // update pc
      size = sizeof(cpu.query_mem_event);
      memcpy(result_buffer, &cpu.query_mem_event, size);
      // nemu result buffer will be flushed after query 
      // printf_with_pid("mem_access %x\n", cpu.query_mem_event.mem_access);
      // printf_with_pid("mem_access_is_load %x\n", cpu.query_mem_event.mem_access_is_load);
      // printf_with_pid("mem_access_vaddr %lx\n", cpu.query_mem_event.mem_access_vaddr);
      memset(&cpu.query_mem_event, 0, size);
      break;
    default:
      panic("Invalid ref query type");
  }
}
#endif

#ifdef CONFIG_MULTICORE_DIFF
void isa_difftest_set_mhartid(int n) {
  mhartid->val = n;
}
#endif
