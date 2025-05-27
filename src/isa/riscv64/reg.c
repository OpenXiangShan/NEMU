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

extern FILE *log_fp;

#define DISPLAY_CSR(log_fp, name, val) \
  fprintf(log_fp, " " FMT_CSR_NAME ": " FMT_WORD, name, val)

// HR: Horizontal Rule
#define DISPLAY_HR(log_fp, name) \
  fprintf(log_fp, "---------------- %s ----------------\n", name)

void isa_reg_display() {
  csr_prepare();

  DISPLAY_HR(log_fp, "Intger Registers");
  for (int i = 0; i < 32; i ++) {
    fprintf(log_fp, "%4s: " FMT_WORD " ", regsl[i], cpu.gpr[i]._64);
    if (i % 4 == 3) {
      fprintf(log_fp, "\n");
    }
  }

  #ifndef CONFIG_FPU_NONE
    DISPLAY_HR(log_fp, "Float Registers");
    for (int i = 0; i < 32; i ++) {
      fprintf(log_fp, "%4s: " FMT_WORD " ", fpregsl[i], cpu.fpr[i]._64);
      if (i % 4 == 3) {
        fprintf(log_fp, "\n");
      }
    }

    #undef FMT_CSR_NAME
    #define FMT_CSR_NAME "%s"

    DISPLAY_CSR(log_fp, "fcsr", cpu.fcsr);
    DISPLAY_CSR(log_fp, "fflags", fflags->val);
    DISPLAY_CSR(log_fp, "frm", frm->val);
    fprintf(log_fp, "\n");
  #endif // CONFIG_FPU_NONE

  DISPLAY_HR(log_fp, "Privileged CSRs");

  fprintf(log_fp, "pc: " FMT_WORD "  privilege mode: %s (mode: %ld  v: %ld  debug: %d)\n",
      cpu.pc, isa_get_privilege_mode_str(),
      cpu.mode, MUXDEF(CONFIG_RVH, cpu.v, (uint64_t)0), MUXDEF(CONFIG_RV_SDEXT, cpu.debug_mode, 0));

  #undef FMT_CSR_NAME
  #define FMT_CSR_NAME "%9s"

  DISPLAY_CSR(log_fp, "mstatus", cpu.mstatus);
  DISPLAY_CSR(log_fp, "sstatus", cpu.sstatus);
  IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "vsstatus", cpu.vsstatus));
  fprintf(log_fp, "\n");

  IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "hstatus", cpu.hstatus));
  IFDEF(CONFIG_RV_SMRNMI, DISPLAY_CSR(log_fp, "mnstatus", mnstatus->val));
  fprintf(log_fp, "\n");

  DISPLAY_CSR(log_fp, "mcause", mcause->val);
  DISPLAY_CSR(log_fp, "mepc", mepc->val);
  DISPLAY_CSR(log_fp, "mtval", mtval->val);
  fprintf(log_fp, "\n");

  DISPLAY_CSR(log_fp, "scause", scause->val);
  DISPLAY_CSR(log_fp, "sepc", sepc->val);
  DISPLAY_CSR(log_fp, "stval", stval->val);
  fprintf(log_fp, "\n");

  #ifdef CONFIG_RVH
    DISPLAY_CSR(log_fp, "vscause", vscause->val);
    DISPLAY_CSR(log_fp, "vsepc", vsepc->val);
    DISPLAY_CSR(log_fp, "vstval", vstval->val);
    fprintf(log_fp, "\n");
  #endif // CONFIG_RVH

  #ifdef CONFIG_RV_SMRNMI
    DISPLAY_CSR(log_fp, "mncause", mncause->val);
    DISPLAY_CSR(log_fp, "mnepc", mnepc->val);
    DISPLAY_CSR(log_fp, "mnscratch", mnscratch->val);
    fprintf(log_fp, "\n");
  #endif // CONFIG_RV_SMRNMI

  #ifdef CONFIG_RVH
    DISPLAY_CSR(log_fp, "mtval2", mtval2->val);
    DISPLAY_CSR(log_fp, "htval", htval->val);
    fprintf(log_fp, "\n");
    DISPLAY_CSR(log_fp, "mtinst", mtinst->val);
    DISPLAY_CSR(log_fp, "htinst", htinst->val);
    fprintf(log_fp, "\n");
  #endif // CONFIG_RVH

  DISPLAY_CSR(log_fp, "mscratch", mscratch->val);
  DISPLAY_CSR(log_fp, "sscratch", sscratch->val);
  IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "vsscratch", vsscratch->val));
  fprintf(log_fp, "\n");

  DISPLAY_CSR(log_fp, "mtvec", mtvec->val);
  DISPLAY_CSR(log_fp, "stvec", stvec->val);
  IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "vstvec", vstvec->val));
  fprintf(log_fp, "\n");

  DISPLAY_CSR(log_fp, "mip", get_mip());
  DISPLAY_CSR(log_fp, "mie", mie->val);
  fprintf(log_fp, "\n");

  DISPLAY_CSR(log_fp, "mideleg", mideleg->val);
  DISPLAY_CSR(log_fp, "medeleg", medeleg->val);
  fprintf(log_fp, "\n");

  #ifdef CONFIG_RVH
    DISPLAY_CSR(log_fp, "hideleg", get_hideleg());
    DISPLAY_CSR(log_fp, "hedeleg", hedeleg->val);
    fprintf(log_fp, "\n");
  #endif // CONFIG_RVH

  DISPLAY_CSR(log_fp, "satp", satp->val);
  #ifdef CONFIG_RVH
    DISPLAY_CSR(log_fp, "hgatp", hgatp->val);
    DISPLAY_CSR(log_fp, "vsatp", vsatp->val);
  #endif // CONFIG_RVH
  fprintf(log_fp, "\n");

  DISPLAY_CSR(log_fp, "mcounteren", mcounteren->val);
  DISPLAY_CSR(log_fp, "scounteren", scounteren->val);
  IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "hcounteren", hcounteren->val));
  fprintf(log_fp, "\n");

  #ifdef CONFIG_RV_IMSIC
    DISPLAY_CSR(log_fp, "miselect", miselect->val);
    DISPLAY_CSR(log_fp, "siselect", siselect->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "vsiselect", vsiselect->val));
    fprintf(log_fp, "\n");
    DISPLAY_CSR(log_fp, "mireg", mireg->val);
    DISPLAY_CSR(log_fp, "sireg", sireg->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "vsireg", vsireg->val));
    fprintf(log_fp, "\n");
    DISPLAY_CSR(log_fp, "mtopi", mtopi->val);
    DISPLAY_CSR(log_fp, "stopi", stopi->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "vstopi", vstopi->val));
    fprintf(log_fp, "\n");
    DISPLAY_CSR(log_fp, "mvien", mvien->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "hvien", hvien->val));
    DISPLAY_CSR(log_fp, "mvip", mvip->val);
    fprintf(log_fp, "\n");
    DISPLAY_CSR(log_fp, "mtopei", mtopei->val);
    DISPLAY_CSR(log_fp, "stopei", stopei->val);
    IFDEF(CONFIG_RVH, DISPLAY_CSR(log_fp, "vstopei", vstopei->val));
    fprintf(log_fp, "\n");
    #ifdef CONFIG_RVH
      DISPLAY_CSR(log_fp, "hvictl", hvictl->val);
      DISPLAY_CSR(log_fp, "hviprio1", hviprio1->val);
      DISPLAY_CSR(log_fp, "hviprio2", hviprio2->val);
      fprintf(log_fp, "\n");
    #endif // CONFIG_RVH
  #endif // CONFIG_RV_IMSIC

   #ifdef CONFIG_RV_MBMC
     DISPLAY_CSR(log_fp, "mbmc", mbmc->val);
     fprintf(log_fp, "\n");
   #endif

  #ifdef CONFIG_RV_PMP_CSR
    DISPLAY_HR(log_fp, "PMP CSRs");
    fprintf(log_fp, "pmp: %d entries active, details:\n", CONFIG_RV_PMP_ACTIVE_NUM);
    for (int i = 0; i < CONFIG_RV_PMP_NUM; i++) {
      fprintf(log_fp, "%2d: cfg:0x%02x addr:0x%016lx", i, pmpcfg_from_index(i), pmpaddr_from_index(i));
      if (i % 2 == 1) fprintf(log_fp, "\n");
      else fprintf(log_fp, "|");
    }
    #ifndef CONFIG_RV_PMP_CHECK
      fprintf(log_fp, "pmp csr rw: enable, pmp check: disable\n");
    #endif
  #endif // CONFIG_RV_PMP_CSR

  #ifdef CONFIG_RV_PMA_CSR
    DISPLAY_HR(log_fp, "PMA CSRs");
    fprintf(log_fp, "pma: %d entries active, details:\n", CONFIG_RV_PMA_ACTIVE_NUM);
    for (int i = 0; i < CONFIG_RV_PMA_NUM; i++) {
      fprintf(log_fp, "%2d: cfg:0x%02x addr:0x%016lx", i, pmacfg_from_index(i), pmaaddr_from_index(i));
      if (i % 2 == 1) fprintf(log_fp, "\n");
      else fprintf(log_fp, "|");
    }
    #ifndef CONFIG_RV_PMA_CHECK
      fprintf(log_fp, "pma csr rw: enable, pma check: disable\n");
    #endif
  #endif // CONFIG_RV_PMA_CSR

  #ifdef CONFIG_RVV
    //vector register
    DISPLAY_HR(log_fp, "Vector Registers");
    extern const char * vregsl[];
    for(int i = 0; i < 32; i ++) {
      fprintf(log_fp, "%s: ", vregsl[i]);
      fprintf(log_fp, "0x%016lx_%016lx  ",
        cpu.vr[i]._64[1], cpu.vr[i]._64[0]);
      if(i%2) fprintf(log_fp, "\n");
    }

    #undef FMT_CSR_NAME
    #define FMT_CSR_NAME "%6s"

    DISPLAY_CSR(log_fp, "vtype", vtype->val);
    DISPLAY_CSR(log_fp, "vstart", vstart->val);
    DISPLAY_CSR(log_fp, "vxsat", vxsat->val);
    fprintf(log_fp, "\n");
    DISPLAY_CSR(log_fp, "vxrm", vxrm->val);
    DISPLAY_CSR(log_fp, "vl", vl->val);
    DISPLAY_CSR(log_fp, "vcsr", vcsr->val);
    fprintf(log_fp, "\n");
  #endif // CONFIG_RVV

  #ifdef CONFIG_RV_SDTRIG
    DISPLAY_HR(log_fp, "Triggers");
    DISPLAY_CSR(log_fp, "tselect", tselect->val);
    fprintf(log_fp, "\n");
    for(int i = 0; i < CONFIG_TRIGGER_NUM + 1; i++) {
      fprintf(log_fp, "%2d: tdata1: " FMT_WORD " tdata2: " FMT_WORD "\n", i, cpu.TM->triggers[i].tdata1.val, cpu.TM->triggers[i].tdata2.val);
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