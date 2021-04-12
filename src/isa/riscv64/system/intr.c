#include <cpu/difftest.h>
#include "../local-include/csr.h"
#include "../local-include/intr.h"

void update_mmu_state();

#define INTR_BIT (1ULL << 63)
enum {
  IRQ_USIP, IRQ_SSIP, IRQ_HSIP, IRQ_MSIP,
  IRQ_UTIP, IRQ_STIP, IRQ_HTIP, IRQ_MTIP,
  IRQ_UEIP, IRQ_SEIP, IRQ_HEIP, IRQ_MEIP
};

word_t raise_intr(word_t NO, vaddr_t epc) {
  switch (NO) {
    case EX_II:
    case EX_IPF:
    case EX_LPF:
    case EX_SPF: difftest_skip_dut(1, 2); break;
  }

  word_t deleg = (NO & INTR_BIT ? mideleg->val : medeleg->val);
  bool delegS = ((deleg & (1 << (NO & 0xf))) != 0) && (cpu.mode < MODE_M);

  if (delegS) {
    scause->val = NO;
    sepc->val = epc;
    mstatus->spp = cpu.mode;
    mstatus->spie = mstatus->sie;
    mstatus->sie = 0;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
        break;
      default: stval->val = 0;
    }
    cpu.mode = MODE_S;
    update_mmu_state();
    return stvec->val;
  } else {
    mcause->val = NO;
    mepc->val = epc;
    mstatus->mpp = cpu.mode;
    mstatus->mpie = mstatus->mie;
    mstatus->mie = 0;
    switch (NO) {
      case EX_IPF: case EX_LPF: case EX_SPF:
      case EX_LAM: case EX_SAM:
        break;
      default: mtval->val = 0;
    }
    cpu.mode = MODE_M;
    update_mmu_state();
    return mtvec->val;
  }
}

word_t isa_query_intr() {
  word_t intr_vec = mie->val & mip->val;
  if (!intr_vec) return INTR_EMPTY;

  const int priority [] = {
    IRQ_MEIP, IRQ_MSIP, IRQ_MTIP,
    IRQ_SEIP, IRQ_SSIP, IRQ_STIP,
    IRQ_UEIP, IRQ_USIP, IRQ_UTIP
  };
  int i;
  for (i = 0; i < 9; i ++) {
    int irq = priority[i];
    if (intr_vec & (1 << irq)) {
      bool deleg = (mideleg->val & (1 << irq)) != 0;
      bool global_enable = (deleg ? ((cpu.mode == MODE_S) && mstatus->sie) || (cpu.mode < MODE_S) :
          ((cpu.mode == MODE_M) && mstatus->mie) || (cpu.mode < MODE_M));
      if (global_enable) return irq | INTR_BIT;
    }
  }
  return INTR_EMPTY;
}
