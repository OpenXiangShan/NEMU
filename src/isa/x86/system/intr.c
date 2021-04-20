#include "../local-include/rtl.h"
#include "../local-include/intr.h"

#if !defined(__ICS_EXPORT) && defined(CONFIG_ENGINE_INTERPRETER)
#include <cpu/difftest.h>

typedef union GateDescriptor {
  struct {
    uint32_t offset_15_0      : 16;
    uint32_t selector         : 16;
    uint32_t dont_care1       : 15;
    uint32_t present          : 1;
    uint32_t offset_31_16     : 16;
  };
  uint32_t val[2];
} GateDesc;

uint32_t compute_eflags();

word_t raise_intr(uint32_t NO, vaddr_t ret_addr) {
  assert(NO < 256);
  int old_cs = cpu.sreg[CSR_CS].val;
  // fetch the gate descriptor with ring 0
  cpu.sreg[CSR_CS].rpl = 0;
  cpu.mem_exception = 0;

  GateDesc gate;
  gate.val[0] = vaddr_read(NULL, cpu.idtr.base + NO * 8 + 0, 4, MMU_DYNAMIC);
  gate.val[1] = vaddr_read(NULL, cpu.idtr.base + NO * 8 + 4, 4, MMU_DYNAMIC);
  assert(gate.present); // check the present bit

  uint16_t new_cs = gate.selector;
  uint32_t new_pc = (gate.offset_31_16 << 16) | gate.offset_15_0;

  if ((new_cs & 0x3) < (old_cs & 0x3)) {
    // stack switch
    assert(cpu.sreg[CSR_TR].ti == 0); // check the table bit
    assert((old_cs & 0x3) == 3); // only support switching from ring 3
    assert((new_cs & 0x3) == 0); // only support switching to ring 0

    uint32_t esp3 = cpu.esp;
    uint32_t ss3  = cpu.sreg[CSR_SS].val;
    cpu.esp = vaddr_read(NULL, cpu.sreg[CSR_TR].base + 4, 4, MMU_DYNAMIC);
    cpu.sreg[CSR_SS].val = vaddr_read(NULL, cpu.sreg[CSR_TR].base + 8, 2, MMU_DYNAMIC);

    vaddr_write(NULL, cpu.esp - 4, 4, ss3, MMU_DYNAMIC);
    vaddr_write(NULL, cpu.esp - 8, 4, esp3, MMU_DYNAMIC);
    cpu.esp -= 8;
  }

  vaddr_write(NULL, cpu.esp - 4, 4, compute_eflags(), MMU_DYNAMIC);
  __attribute__((unused)) word_t eflags_esp = cpu.esp - 4;
  vaddr_write(NULL, cpu.esp -  8, 4, old_cs, MMU_DYNAMIC);
  vaddr_write(NULL, cpu.esp - 12, 4, ret_addr, MMU_DYNAMIC);
  cpu.esp -= 12;

  if (ISNDEF(CONFIG_PA) && NO == 14) {
    // page fault has error code
    vaddr_write(NULL, cpu.esp - 4, 4, cpu.error_code, MMU_DYNAMIC);
    cpu.esp -= 4;
  }

  cpu.IF = 0;
  cpu.sreg[CSR_CS].val = new_cs;

#if defined(CONFIG_DIFFTEST_REF_KVM)
  if (ref_difftest_raise_intr) ref_difftest_raise_intr(NO);
#elif !defined(CONFIG_DIFFTEST_REF_NEMU)
  difftest_skip_dut(1, 2);
  void difftest_fix_eflags(void *arg);
  difftest_set_patch(difftest_fix_eflags, (void *)(uintptr_t)eflags_esp);
#endif

  return new_pc;
}

word_t isa_query_intr() {
  if (cpu.INTR && cpu.IF) {
    cpu.INTR = false;
    return IRQ_TIMER;
  }
  return INTR_EMPTY;
}
#else
word_t raise_intr(uint32_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

#if !defined(CONFIG_ENGINE_INTERPRETER)
  panic("not support in non-interpreter mode");
#endif
  return 0;
}

void query_intr() {
#if !defined(CONFIG_ENGINE_INTERPRETER)
  panic("not support in non-interpreter mode");
#endif
}
#endif
