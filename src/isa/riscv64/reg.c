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
#include "local-include/trigger.h"
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
  csr_prepare();
  int i;
  for (i = 0; i < 32; i ++) {
    printf("%4s: " FMT_WORD " ", regsl[i], cpu.gpr[i]._64);
    if (i % 4 == 3) {
      printf("\n");
    }
  }
#ifndef CONFIG_FPU_NONE
  for (i = 0; i < 32; i ++) {
    printf("%4s: " FMT_WORD " ", fpregsl[i], cpu.fpr[i]._64);
    if (i % 4 == 3) {
      printf("\n");
    }
  }
#endif // CONFIG_FPU_NONE
  printf("pc: " FMT_WORD " mstatus: " FMT_WORD " mcause: " FMT_WORD " mepc: " FMT_WORD "\n",
      cpu.pc, cpu.mstatus, mcause->val, mepc->val);
#ifdef CONFIG_RV_SMRNMI
  printf("%22s mnstatus: " FMT_WORD " mncause: " FMT_WORD " mnepc: " FMT_WORD "\n",
      "", mnstatus->val, mncause->val, mnepc->val);
#endif // CONFIG_RV_SMRNMI
  printf("%22s sstatus: " FMT_WORD " scause: " FMT_WORD " sepc: " FMT_WORD "\n",
      "", cpu.sstatus, scause->val, sepc->val);
  printf("satp: " FMT_WORD "\n", satp->val);
  printf("mip: " FMT_WORD " mie: " FMT_WORD " mscratch: " FMT_WORD " sscratch: " FMT_WORD "\n",
      get_mip(), mie->val, mscratch->val, sscratch->val);
  printf("mideleg: " FMT_WORD " medeleg: " FMT_WORD "\n",
      mideleg->val, medeleg->val);
  printf("mtval: " FMT_WORD " stval: " FMT_WORD " mtvec: " FMT_WORD " stvec: " FMT_WORD "\n",
      mtval->val, stval->val, mtvec->val, stvec->val);
#ifdef CONFIG_RV_IMSIC 
  printf("miselect: " FMT_WORD " siselect: " FMT_WORD " mireg: " FMT_WORD " sireg: " FMT_WORD "\n",
      miselect->val, siselect->val, mireg->val, sireg->val);
  printf("mtopi: " FMT_WORD " stopi: " FMT_WORD " mvien: " FMT_WORD " mvip: " FMT_WORD "\n",
      mtopi->val, stopi->val, mvien->val, mvip->val);
  printf("mtopei: " FMT_WORD " stopei: " FMT_WORD "\n",
      mtopei->val, stopei->val);
#endif // CONFIG_RV_IMSIC
#ifndef CONFIG_FPU_NONE
  printf("fcsr: " FMT_WORD "\n", cpu.fcsr);
#endif // CONFIG_FPU_NONE
#ifdef CONFIG_RVH
  printf("mtval2: " FMT_WORD " mtinst: " FMT_WORD " hstatus: " FMT_WORD " hideleg: " FMT_WORD "\n",
      mtval2->val, mtinst->val, hstatus->val, hideleg->val);
  printf("hedeleg: " FMT_WORD " hcounteren: " FMT_WORD " htval: " FMT_WORD " htinst: " FMT_WORD "\n",
      hedeleg->val, hcounteren->val, htval->val, htinst->val);
  printf("hgatp: " FMT_WORD " vsscratch: " FMT_WORD " vsstatus: " FMT_WORD " vstvec: " FMT_WORD "\n",
      hgatp->val, vsscratch->val, cpu.vsstatus, vstvec->val);
  printf("vsepc: " FMT_WORD " vscause: " FMT_WORD " vstval: " FMT_WORD " vsatp: " FMT_WORD "\n",
      vsepc->val, vscause->val, vstval->val, vsatp->val);
#ifdef CONFIG_RV_IMSIC
  printf("hvien: " FMT_WORD " hvictl: " FMT_WORD " hviprio1: " FMT_WORD " hviprio2: " FMT_WORD "\n",
      hvien->val, hvictl->val, hviprio1->val, hviprio2->val);
  printf("vsiselect: " FMT_WORD " vsireg: " FMT_WORD " vstopi: " FMT_WORD "\n",
      vsiselect->val, vsireg->val, vstopi->val);
  printf("vstopei: " FMT_WORD "\n", vstopei->val);
#endif // CONFIG_RV_IMSIC
  printf("virtualization mode: %ld\n", cpu.v);
#endif
  printf("privilege mode:%ld\n", cpu.mode);
#ifdef CONFIG_RV_PMP_CSR
  printf("pmp: %d entries active, details:\n", CONFIG_RV_PMP_ACTIVE_NUM);
  for (int i = 0; i < CONFIG_RV_PMP_NUM; i++) {
    printf("%2d: cfg:0x%02x addr:0x%016lx", i, pmpcfg_from_index(i), pmpaddr_from_index(i));
    if (i % 2 == 1) printf("\n");
    else printf("|");
  }
#ifndef CONFIG_RV_PMP_CHECK
  printf("pmp csr rw: enable, pmp check: disable\n");
#endif
#endif // CONFIG_RV_PMP_CSR

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

#ifdef CONFIG_RV_SDTRIG
  printf("tselect: " FMT_WORD "\n", tselect->val);
  for(i = 0; i < CONFIG_TRIGGER_NUM + 1; i++) {
    printf("%2d: tdata1: " FMT_WORD " tdata2: " FMT_WORD "\n", i, cpu.TM->triggers[i].tdata1.val, cpu.TM->triggers[i].tdata2.val);
  }
#endif // CONFIG_RV_SDTRIG
  fflush(stdout);
}

rtlreg_t isa_reg_str2val(const char *s, bool *success) {
  int i;
  *success = true;
  for (i = 0; i < 32; i ++) {
    if (strcmp(regsl[i], s) == 0) return reg_l(i);
  }

#ifndef CONFIG_FPU_NONE
  for (i = 0; i < 32; i ++) {
    if (strcmp(fpregsl[i], s) == 0) return fpreg_l(i);
  }
#endif // CONFIG_FPU_NONE

#ifdef CONFIG_RVV
  //vector register
  extern const char * vregsl[];
  for (i = 0; i < 32; i ++) {
    if (strcmp(vregsl[i], s) == 0) return cpu.vr[i]._64[0];
  }
#endif // CONFIG_RVV

  if (strcmp("pc", s) == 0) return cpu.pc;

  *success = false;
  return 0;
}

bool able_to_take_cpt() {
  return cpu.mode != MODE_M;
}