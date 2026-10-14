/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

// RISC-V part of leader-follower boundary records.

#include <generated/autoconf.h>

#ifdef CONFIG_LF_PROFILING

#include <isa.h>
#include <cpu/difftest.h>
#include <profiling/lf_record.h>
#include "local-include/csr.h"
#include "local-include/reg.h"

extern const unsigned DIFFTEST_REG_SIZE;
extern int ifetch_mmu_state;
extern int data_mmu_state;

// Execution state outside the CSR array and the difftest register layout that
// influences later execution.
#define LF_ISA_MISC_LIST(f)                                                    \
  f(lr_addr, cpu.lr_addr)                                                      \
  f(lr_valid, cpu.lr_valid)                                                    \
  f(INTR, cpu.INTR)                                                            \
  f(platform_irp_meip, cpu.non_reg_interrupt_pending.platform_irp_meip)        \
  f(platform_irp_mtip, cpu.non_reg_interrupt_pending.platform_irp_mtip)        \
  f(platform_irp_msip, cpu.non_reg_interrupt_pending.platform_irp_msip)        \
  f(platform_irp_seip, cpu.non_reg_interrupt_pending.platform_irp_seip)        \
  f(platform_irp_stip, cpu.non_reg_interrupt_pending.platform_irp_stip)        \
  f(platform_irp_vseip, cpu.non_reg_interrupt_pending.platform_irp_vseip)      \
  f(platform_irp_vstip, cpu.non_reg_interrupt_pending.platform_irp_vstip)      \
  f(from_aia_meip, cpu.non_reg_interrupt_pending.from_aia_meip)                \
  f(from_aia_seip, cpu.non_reg_interrupt_pending.from_aia_seip)                \
  f(lcofi_req, cpu.non_reg_interrupt_pending.lcofi_req)                        \
  f(amo, cpu.amo)                                                              \
  f(pbmt, cpu.pbmt)                                                            \
  f(isVldst, cpu.isVldst)                                                      \
  f(isVecUnitStore, cpu.isVecUnitStore)                                        \
  f(mem_exception, cpu.mem_exception)                                          \
  f(ifetch_mmu_state, ifetch_mmu_state)                                        \
  f(data_mmu_state, data_mmu_state)                                            \
  IFDEF(CONFIG_TVAL_EX_II, f(instr, cpu.instr))                                \
  IFDEF(CONFIG_RV_SMRNMI, f(hasNMI, cpu.hasNMI))                               \
  IFDEF(CONFIG_RV_SDEXT, f(debug_mode, cpu.debug_mode))                        \
  IFDEF(CONFIG_RV_ZICFILP, f(elp, cpu.elp))                                    \
  IFDEF(CONFIG_RV_SMDBLTRP, f(critical_error, cpu.critical_error))             \
  IFDEF(CONFIG_RVH, f(hideleg_reg, cpu.hideleg_reg))

#define LF_MISC_NAME(name, expr) #name,
static const char *misc_names[] = { LF_ISA_MISC_LIST(LF_MISC_NAME) };
_Static_assert(sizeof(misc_names) / sizeof(misc_names[0]) <= LF_ISA_MISC_MAX,
               "too many misc fields in a boundary record");

void isa_lf_capture(lf_isa_state_t *st) {
  // csr_prepare() rewrites the difftest shadow fields of cpu; restore them so
  // that capturing a record never changes the state that continues executing.
  CPU_state saved = cpu;
  csr_prepare();
  st->regs = cpu;
  cpu = saved;
  memcpy(st->csr, csr_array, sizeof(st->csr));
  int i = 0;
#define LF_MISC_STORE(name, expr) st->misc[i++] = (uint64_t)(expr);
  LF_ISA_MISC_LIST(LF_MISC_STORE)
  (void)i;
}

bool isa_lf_compare(const lf_isa_state_t *leader, const lf_isa_state_t *mine) {
  bool ok = true;
  if (memcmp(&leader->regs.gpr, &mine->regs.gpr, DIFFTEST_REG_SIZE) != 0) {
    ok = false;
    // Fields the difftest reporter does not name.
    if (leader->regs.mode != mine->regs.mode)
      Log("privilege mode is different, leader = %lu, follower = %lu", leader->regs.mode, mine->regs.mode);
    if (leader->regs.mip != mine->regs.mip)
      Log("mip is different, leader = " FMT_WORD ", follower = " FMT_WORD, leader->regs.mip, mine->regs.mip);
    if (leader->regs.gpr[0]._64 != mine->regs.gpr[0]._64)
      Log("x0 is different, leader = " FMT_WORD ", follower = " FMT_WORD,
          leader->regs.gpr[0]._64, mine->regs.gpr[0]._64);
#if !defined(CONFIG_FPU_NONE) && !defined(CONFIG_FPU_SOFT)
    for (int r = 0; r < 32; r++)
      if (leader->regs.fpr[r]._64 != mine->regs.fpr[r]._64)
        Log("%s is different, leader = " FMT_WORD ", follower = " FMT_WORD,
            fpreg_name(r, 8), leader->regs.fpr[r]._64, mine->regs.fpr[r]._64);
#endif
    // The difftest reporter compares the live cpu (right = leader, wrong =
    // follower). The follower has not executed since capturing its record.
    // It prepares the CSR shadow of the live cpu, which is kept unchanged.
    CPU_state ref = leader->regs, saved = cpu;
    isa_difftest_checkregs(&ref, mine->regs.pc);
    cpu = saved;
  }
  for (int c = 0; c < 4096; c++) {
    if (leader->csr[c] != mine->csr[c]) {
      Log("csr 0x%03x is different, leader = " FMT_WORD ", follower = " FMT_WORD,
          c, leader->csr[c], mine->csr[c]);
      ok = false;
    }
  }
  for (int m = 0; m < ARRLEN(misc_names); m++) {
    if (leader->misc[m] != mine->misc[m]) {
      Log("%s is different, leader = 0x%lx, follower = 0x%lx", misc_names[m], leader->misc[m], mine->misc[m]);
      ok = false;
    }
  }
  return ok;
}

#endif // CONFIG_LF_PROFILING
