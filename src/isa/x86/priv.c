#include <cpu/exec.h>
#include <monitor/difftest.h>
#include "local-include/reg.h"
#include "local-include/intr.h"

#ifndef __ICS_EXPORT
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
void set_eflags(uint32_t val);

static void load_sreg(int idx, uint16_t val) {
  cpu.sreg[idx].val = val;

  if (val == 0) return;
  uint16_t old_cpl = cpu.sreg[CSR_CS].val;
  cpu.sreg[CSR_CS].rpl = 0; // use ring 0 to index GDT

  assert(cpu.sreg[idx].ti == 0); // check the table bit
  uint32_t desc_base = cpu.gdtr.base + (cpu.sreg[idx].idx << 3);
  uint32_t desc_lo = vaddr_read(desc_base + 0, 4);
  uint32_t desc_hi = vaddr_read(desc_base + 4, 4);
  assert((desc_hi >> 15) & 0x1); // check the present bit
  uint32_t base = (desc_hi & 0xff000000) | ((desc_hi & 0xff) << 16) | (desc_lo >> 16);
  cpu.sreg[idx].base = base;

  cpu.sreg[CSR_CS].rpl = old_cpl;
}

static void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid) {
  if (dest != NULL) {
    switch (csrid) {
      case 0 ... CSR_LDTR: *dest = cpu.sreg[csrid].val; break;
      case CSR_CR0 ... CSR_CR4: *dest = cpu.cr[csrid - CSR_CR0]; break;
      default: panic("Reading from CSR = %d is not supported", csrid);
    }
  }
  if (src != NULL) {
    switch (csrid) {
      case CSR_IDTR:
        cpu.idtr.limit = vaddr_read(*src, 2);
        cpu.idtr.base  = vaddr_read(*src + 2, 4);
        break;
      case CSR_GDTR:
        cpu.gdtr.limit = vaddr_read(*src, 2);
        cpu.gdtr.base  = vaddr_read(*src + 2, 4);
        break;
      case 0 ... CSR_LDTR: load_sreg(csrid, *src); break;
      case CSR_CR0 ... CSR_CR4: cpu.cr[csrid - CSR_CR0] = *src; break;
      default: panic("Writing to CSR = %d is not supported", csrid);
    }
  }
}

static inline word_t iret() {
  int old_cpl = cpu.sreg[CSR_CS].rpl;
  uint32_t new_pc = vaddr_read(cpu.esp + 0, 4);
  uint32_t new_cs = vaddr_read(cpu.esp + 4, 4);
  uint32_t eflags = vaddr_read(cpu.esp + 8, 4);
  cpu.esp += 12;
  set_eflags(eflags);
  int new_cpl = new_cs & 0x3;
  if (new_cpl > old_cpl) {
    // return to user
    uint32_t esp3 = vaddr_read(cpu.esp + 0, 4);
    uint32_t ss3  = vaddr_read(cpu.esp + 4, 4);
    cpu.esp = esp3;
    cpu.sreg[CSR_SS].val = ss3;
  }
  cpu.sreg[CSR_CS].val = new_cs;

  return new_pc;
}

static word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
    case PRIV_IRET: return iret();
    default: panic("Unsupported privilige operation = %d", op);
  }
}

word_t raise_intr(uint32_t NO, vaddr_t ret_addr) {
  assert(NO < 256);
  int old_cs = cpu.sreg[CSR_CS].val;
  // fetch the gate descriptor with ring 0
  cpu.sreg[CSR_CS].rpl = 0;
  cpu.mem_exception = 0;

  GateDesc gate;
  gate.val[0] = vaddr_read(cpu.idtr.base + NO * 8 + 0, 4);
  gate.val[1] = vaddr_read(cpu.idtr.base + NO * 8 + 4, 4);
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
    cpu.esp = vaddr_read(cpu.sreg[CSR_TR].base + 4, 4);
    cpu.sreg[CSR_SS].val = vaddr_read(cpu.sreg[CSR_TR].base + 8, 2);

    vaddr_write(cpu.esp - 4, ss3, 4);
    vaddr_write(cpu.esp - 8, esp3, 4);
    cpu.esp -= 8;
  }

  vaddr_write(cpu.esp - 4, compute_eflags(), 4);
  __attribute__((unused)) word_t eflags_esp = cpu.esp - 4;
  vaddr_write(cpu.esp - 8, old_cs, 4);
  vaddr_write(cpu.esp - 12, ret_addr, 4);
  cpu.esp -= 12;

#ifndef __PA__
  if (NO == 14) {
    // page fault has error code
    vaddr_write(cpu.esp - 4, cpu.error_code, 4);
    cpu.esp -= 4;
  }
#endif

  cpu.IF = 0;
  cpu.sreg[CSR_CS].val = new_cs;

#if defined(__DIFF_REF_KVM__)
  if (ref_difftest_raise_intr) ref_difftest_raise_intr(NO);
#elif !defined(__DIFF_REF_NEMU__)
  difftest_skip_dut(1, 2);
  void difftest_fix_eflags(void *arg);
  difftest_set_patch(difftest_fix_eflags, (void *)(uintptr_t)eflags_esp);
#endif

  return new_pc;
}

void isa_hostcall(uint32_t id, rtlreg_t *dest, const rtlreg_t *src, uint32_t imm) {
  word_t ret = 0;
  switch (id) {
    case HOSTCALL_CSR: csrrw(dest, src, imm); return;
    case HOSTCALL_TRAP: ret = raise_intr(imm, *src); break;
    case HOSTCALL_PRIV: ret = priv_instr(imm, src); break;
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}

void query_intr() {
  if (cpu.INTR && cpu.IF) {
    cpu.INTR = false;
    cpu.pc = raise_intr(IRQ_TIMER, cpu.pc);
  }
}
#else
word_t raise_intr(uint32_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  return 0;
}

void query_intr() {
}
#endif
