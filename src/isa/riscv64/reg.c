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

#include <stdlib.h>
#include <isa.h>
//#include <monitor/difftest.h>
#include "local-include/reg.h"
#include "local-include/csr.h"
#include "local-include/trigger.h"
#include "instr/rvv/vreg.h"
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

const char * isa_get_privilege_mode_str() {
  if (MUXDEF(CONFIG_RV_SDEXT, cpu.debug_mode, 0)) {
    return "D";
  }
  if (MUXDEF(CONFIG_RVH, cpu.v, 0)) {
    switch (cpu.mode) {
      case MODE_U: return "VU";
      case MODE_S: return "VS";
    }
  } else {
    switch (cpu.mode) {
      case MODE_U: return "U";
      case MODE_S: return "S";
      case MODE_M: return "M";
    }
  }
  return "Reserved";
}

#define FMT_CSR_NAME "%9s"

#define DISPLAY_CSR(name, val) \
  printf(" " FMT_CSR_NAME ": " FMT_WORD, name, val)

// HR: Horizontal Rule
#define DISPLAY_HR(name) \
  printf("---------------- %s ----------------\n", name)

void isa_reg_display() {
  csr_prepare();

  DISPLAY_HR("Intger Registers");
  for (int i = 0; i < 32; i ++) {
    printf("%4s: " FMT_WORD " ", regsl[i], cpu.gpr[i]._64);
    if (i % 4 == 3) {
      printf("\n");
    }
  }

  #ifndef CONFIG_FPU_NONE
    DISPLAY_HR("Float Registers");
    for (int i = 0; i < 32; i ++) {
      printf("%4s: " FMT_WORD " ", fpregsl[i], cpu.fpr[i]._64);
      if (i % 4 == 3) {
        printf("\n");
      }
    }

    #undef FMT_CSR_NAME
    #define FMT_CSR_NAME "%s"

    DISPLAY_CSR("fcsr", cpu.fcsr);
    DISPLAY_CSR("fflags", fflags->val);
    DISPLAY_CSR("frm", frm->val);
    printf("\n");
  #endif // CONFIG_FPU_NONE

  DISPLAY_HR("Privileged CSRs");

  printf("pc: " FMT_WORD "  privilege mode: %s (mode: %ld  v: %ld  debug: %d)\n",
      cpu.pc, isa_get_privilege_mode_str(),
      cpu.mode, MUXDEF(CONFIG_RVH, cpu.v, (uint64_t)0), MUXDEF(CONFIG_RV_SDEXT, cpu.debug_mode, 0));

  #undef FMT_CSR_NAME
  #define FMT_CSR_NAME "%9s"

  DISPLAY_CSR("mstatus", cpu.mstatus);
  DISPLAY_CSR("sstatus", cpu.sstatus);
  IFDEF(CONFIG_RVH, DISPLAY_CSR("vsstatus", cpu.vsstatus));
  printf("\n");

  IFDEF(CONFIG_RVH, DISPLAY_CSR("hstatus", cpu.hstatus));
  IFDEF(CONFIG_RV_SMRNMI, DISPLAY_CSR("mnstatus", mnstatus->val));
  printf("\n");

  DISPLAY_CSR("mcause", mcause->val);
  DISPLAY_CSR("mepc", mepc->val);
  DISPLAY_CSR("mtval", mtval->val);
  printf("\n");

  DISPLAY_CSR("scause", scause->val);
  DISPLAY_CSR("sepc", sepc->val);
  DISPLAY_CSR("stval", stval->val);
  printf("\n");

  #ifdef CONFIG_RVH
    DISPLAY_CSR("vscause", vscause->val);
    DISPLAY_CSR("vsepc", vsepc->val);
    DISPLAY_CSR("vstval", vstval->val);
    printf("\n");
  #endif // CONFIG_RVH

  #ifdef CONFIG_RV_SMRNMI
    DISPLAY_CSR("mncause", mncause->val);
    DISPLAY_CSR("mnepc", mnepc->val);
    DISPLAY_CSR("mnscratch", mnscratch->val);
    printf("\n");
  #endif // CONFIG_RV_SMRNMI

  #ifdef CONFIG_RVH
    DISPLAY_CSR("mtval2", mtval2->val);
    DISPLAY_CSR("htval", htval->val);
    printf("\n");
    DISPLAY_CSR("mtinst", mtinst->val);
    DISPLAY_CSR("htinst", htinst->val);
    printf("\n");
  #endif // CONFIG_RVH

  DISPLAY_CSR("mscratch", mscratch->val);
  DISPLAY_CSR("sscratch", sscratch->val);
  IFDEF(CONFIG_RVH, DISPLAY_CSR("vsscratch", vsscratch->val));
  printf("\n");

  DISPLAY_CSR("mtvec", mtvec->val);
  DISPLAY_CSR("stvec", stvec->val);
  IFDEF(CONFIG_RVH, DISPLAY_CSR("vstvec", vstvec->val));
  printf("\n");

  DISPLAY_CSR("mip", get_mip());
  DISPLAY_CSR("mie", mie->val);
  printf("\n");

  DISPLAY_CSR("mideleg", mideleg->val);
  DISPLAY_CSR("medeleg", medeleg->val);
  printf("\n");

  #ifdef CONFIG_RVH
    DISPLAY_CSR("hideleg", get_hideleg());
    DISPLAY_CSR("hedeleg", hedeleg->val);
    printf("\n");
  #endif // CONFIG_RVH

  DISPLAY_CSR("satp", satp->val);
  #ifdef CONFIG_RVH
    DISPLAY_CSR("hgatp", hgatp->val);
    DISPLAY_CSR("vsatp", vsatp->val);
  #endif // CONFIG_RVH
  printf("\n");

  DISPLAY_CSR("mcounteren", mcounteren->val);
  DISPLAY_CSR("scounteren", scounteren->val);
  IFDEF(CONFIG_RVH, DISPLAY_CSR("hcounteren", hcounteren->val));
  printf("\n");

  #ifdef CONFIG_RV_IMSIC
    DISPLAY_CSR("miselect", miselect->val);
    DISPLAY_CSR("siselect", siselect->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR("vsiselect", vsiselect->val));
    printf("\n");
    DISPLAY_CSR("mireg", mireg->val);
    DISPLAY_CSR("sireg", sireg->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR("vsireg", vsireg->val));
    printf("\n");
    DISPLAY_CSR("mtopi", mtopi->val);
    DISPLAY_CSR("stopi", stopi->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR("vstopi", vstopi->val));
    printf("\n");
    DISPLAY_CSR("mvien", mvien->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR("hvien", hvien->val));
    DISPLAY_CSR("mvip", mvip->val);
    printf("\n");
    DISPLAY_CSR("mtopei", mtopei->val);
    DISPLAY_CSR("stopei", stopei->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR("vstopei", vstopei->val));
    printf("\n");
    #ifdef CONFIG_RVH
      DISPLAY_CSR("hvictl", hvictl->val);
      DISPLAY_CSR("hviprio1", hviprio1->val);
      DISPLAY_CSR("hviprio2", hviprio2->val);
      printf("\n");
    #endif // CONFIG_RVH
  #endif // CONFIG_RV_IMSIC

   #ifdef CONFIG_RV_MBMC
     DISPLAY_CSR("mbmc", mbmc->val);
     printf("\n");
   #endif

  #ifdef CONFIG_RV_PMP_CSR
    DISPLAY_HR("PMP CSRs");
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

  #ifdef CONFIG_RV_PMA_CSR
    DISPLAY_HR("PMA CSRs");
    printf("pma: %d entries active, details:\n", CONFIG_RV_PMA_ACTIVE_NUM);
    for (int i = 0; i < CONFIG_RV_PMA_NUM; i++) {
      printf("%2d: cfg:0x%02x addr:0x%016lx", i, pmacfg_from_index(i), pmaaddr_from_index(i));
      if (i % 2 == 1) printf("\n");
      else printf("|");
    }
    #ifndef CONFIG_RV_PMA_CHECK
      printf("pma csr rw: enable, pma check: disable\n");
    #endif
  #endif // CONFIG_RV_PMA_CSR

  #ifdef CONFIG_RVV
    //vector register
    DISPLAY_HR("Vector Registers");
    extern const char * vregsl[];
    for(int i = 0; i < 32; i ++) {
      printf("%s: ", vregsl[i]);
      printf("0x%016lx_%016lx  ",
        cpu.vr[i]._64[1], cpu.vr[i]._64[0]);
      if(i%2) printf("\n");
    }

    #undef FMT_CSR_NAME
    #define FMT_CSR_NAME "%6s"

    DISPLAY_CSR("vtype", vtype->val);
    DISPLAY_CSR("vstart", vstart->val);
    DISPLAY_CSR("vxsat", vxsat->val);
    printf("\n");
    DISPLAY_CSR("vxrm", vxrm->val);
    DISPLAY_CSR("vl", vl->val);
    DISPLAY_CSR("vcsr", vcsr->val);
    printf("\n");
  #endif // CONFIG_RVV

  #ifdef CONFIG_RV_SDTRIG
    DISPLAY_HR("Triggers");
    DISPLAY_CSR("tselect", tselect->val);
    printf("\n");
    for(int i = 0; i < CONFIG_TRIGGER_NUM + 1; i++) {
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
  const char *underscore_pos = strchr(s, '_');
  if (underscore_pos != NULL) {
    size_t prefix_len = underscore_pos - s;
    int offset = atoi(underscore_pos + 1);
    extern const char * vregsl[];
    for (i = 0; i < 32; i ++) {
      if (strncmp(vregsl[i], s, prefix_len) == 0) return vreg_l(i, offset);
    }
  }
#endif // CONFIG_RVV

  if (strcmp("pc", s) == 0) return cpu.pc;

  *success = false;
  return 0;
}

bool able_to_take_cpt() {
  return cpu.mode != MODE_M;
}