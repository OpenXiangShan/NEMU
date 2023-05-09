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
//#include <monitor/difftest.h>
#include "local-include/reg.h"
#include "local-include/csr.h"
//#include "local-include/intr.h"

const char *regsl[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

const char *fpregsl[] = {
  "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",
  "fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",
  "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",
  "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"
};

void isa_reg_display() {
  int i;
  for (i = 0; i < 32; i ++) {
    printf("%4s: " FMT_WORD " ", regsl[i], cpu.gpr[i]._64);
    if (i % 4 == 3) {
      printf("\n");
    }
  }
  for (i = 0; i < 32; i ++) {
    printf("%4s: " FMT_WORD " ", fpregsl[i], cpu.fpr[i]._64);
    if (i % 4 == 3) {
      printf("\n");
    }
  }
  printf("pc: " FMT_WORD " mstatus: " FMT_WORD " mcause: " FMT_WORD " mepc: " FMT_WORD "\n",
      cpu.pc, mstatus->val, mcause->val, mepc->val);
  printf("%22s sstatus: " FMT_WORD " scause: " FMT_WORD " sepc: " FMT_WORD "\n",
      "", csrid_read(0x100), scause->val, sepc->val);
  printf("satp: " FMT_WORD "\n", satp->val);
  printf("mip: " FMT_WORD " mie: " FMT_WORD " mscratch: " FMT_WORD " sscratch: " FMT_WORD "\n",
      mip->val, mie->val, mscratch->val, sscratch->val);
  printf("mideleg: " FMT_WORD " medeleg: " FMT_WORD "\n",
      mideleg->val, medeleg->val);
  printf("mtval: " FMT_WORD " stval: " FMT_WORD " mtvec: " FMT_WORD " stvec: " FMT_WORD "\n",
      mtval->val, stval->val, mtvec->val, stvec->val);
#ifdef CONFIG_RV_PMP_CSR
  printf("privilege mode:%ld  pmp: below\n", cpu.mode);
  for (int i = 0; i < CONFIG_RV_PMP_NUM; i++) {
    printf("%2d: cfg:0x%02x addr:0x%016lx", i, pmpcfg_from_index(i), pmpaddr_from_index(i));
    if (i % 2 == 1) printf("\n");
    else printf("|");
  }
#ifndef CONFIG_RV_PMP_CHECK
  printf("pmp csr rw: enable, pmp check: disable\n");
#endif
#else
  printf("privilege mode:%ld\n", cpu.mode);
#endif

#ifdef CONFIG_RV_SPMP_CSR
  printf("privilege mode:%ld  spmp: below\n", cpu.mode);
  for (int i = 0; i < CONFIG_RV_SPMP_NUM; i++) {
    printf("%2d: cfg:0x%02x addr:0x%016lx", i, spmpcfg_from_index(i), spmpaddr_from_index(i));
    if (i % 2 == 1) printf("\n");
    else printf("|");
  }
#ifndef CONFIG_RV_SPMP_CHECK
  printf("spmp csr rw: enable, spmp check: disable\n");
#endif
#else
  printf("privilege mode:%ld\n", cpu.mode);
#endif

#ifdef CONFIG_RVV
  //vector register
  extern const char * vregsl[];
  for(i = 0; i < 32; i ++) {
    printf("%s: ", vregsl[i]);
    printf("0x%016lx_%016lx  ",
      cpu.vr[i]._64[1], cpu.vr[i]._64[0]);
    if(i%2) printf("\n");
  }
  printf("vtype: " FMT_WORD " vstart: " FMT_WORD " vxsat: " FMT_WORD "\n", vtype->val, vstart->val, vxsat->val);
  printf("vxrm: " FMT_WORD " vl: " FMT_WORD " vcsr: " FMT_WORD "\n", vxrm->val, vl->val, vcsr->val);
#endif // CONFIG_RVV
  fflush(stdout);
}

rtlreg_t isa_reg_str2val(const char *s, bool *success) {
  int i;
  *success = true;
  for (i = 0; i < 32; i ++) {
    if (strcmp(regsl[i], s) == 0) return reg_l(i);
  }

  if (strcmp("pc", s) == 0) return cpu.pc;

  *success = false;
  return 0;
}

bool able_to_take_cpt() {
  return cpu.mode != MODE_M;
}