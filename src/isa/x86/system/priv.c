#include "../local-include/rtl.h"
#include "../local-include/intr.h"
#include <cpu/cpu.h>

#if defined(CONFIG_ENGINE_INTERPRETER)

void set_eflags(uint32_t val);

static void load_sreg(int idx, uint16_t val) {
  cpu.sreg[idx].val = val;

  if (val == 0) return;

#ifdef CONFIG_MODE_USER
  assert(cpu.sreg[idx].ti == 0); // check the table bit
  extern uint32_t GDT[];
  cpu.sreg[idx].base = GDT[cpu.sreg[idx].idx];
#else
  uint16_t old_cpl = cpu.sreg[CSR_CS].val;
  cpu.sreg[CSR_CS].rpl = 0; // use ring 0 to index GDT

  assert(cpu.sreg[idx].ti == 0); // check the table bit
  uint32_t desc_base = cpu.gdtr.base + (cpu.sreg[idx].idx << 3);
  uint32_t desc_lo = vaddr_read(NULL, desc_base + 0, 4, MMU_DYNAMIC);
  uint32_t desc_hi = vaddr_read(NULL, desc_base + 4, 4, MMU_DYNAMIC);
  assert((desc_hi >> 15) & 0x1); // check the present bit

  cpu.sreg[CSR_CS].rpl = old_cpl; // restore CPL

  uint32_t base = (desc_hi & 0xff000000) | ((desc_hi & 0xff) << 16) | (desc_lo >> 16);
  cpu.sreg[idx].base = base;
#endif
}

static inline void csrrw(rtlreg_t *dest, const rtlreg_t *src, uint32_t csrid) {
  if (dest != NULL) {
    switch (csrid) {
#ifndef CONFIG_MODE_USER
      case 0 ... CSR_LDTR: *dest = cpu.sreg[csrid].val; break;
      case CSR_CR0 ... CSR_CR4: *dest = cpu.cr[csrid - CSR_CR0]; break;
#endif
      default: panic("Reading from CSR = %d is not supported", csrid);
    }
  }
  if (src != NULL) {
    switch (csrid) {
#ifndef CONFIG_MODE_USER
      case CSR_IDTR:
        cpu.idtr.limit = vaddr_read(NULL, *src, 2, MMU_DYNAMIC);
        cpu.idtr.base  = vaddr_read(NULL, *src + 2, 4, MMU_DYNAMIC);
        break;
      case CSR_GDTR:
        cpu.gdtr.limit = vaddr_read(NULL, *src, 2, MMU_DYNAMIC);
        cpu.gdtr.base  = vaddr_read(NULL, *src + 2, 4, MMU_DYNAMIC);
        break;
      case CSR_CR0 ... CSR_CR4: cpu.cr[csrid - CSR_CR0] = *src; break;
#endif
      case 0 ... CSR_LDTR: load_sreg(csrid, *src); break;
      default: panic("Writing to CSR = %d is not supported", csrid);
    }
    if (csrid == CSR_CR3) mmu_tlb_flush(0);
  }
}

#ifndef CONFIG_MODE_USER
static inline word_t iret() {
  int old_cpl = cpu.sreg[CSR_CS].rpl;
  uint32_t new_pc = vaddr_read(NULL, cpu.esp + 0, 4, MMU_DYNAMIC);
  uint32_t new_cs = vaddr_read(NULL, cpu.esp + 4, 4, MMU_DYNAMIC);
  uint32_t eflags = vaddr_read(NULL, cpu.esp + 8, 4, MMU_DYNAMIC);
  cpu.esp += 12;
  set_eflags(eflags);
  int new_cpl = new_cs & 0x3;
  if (new_cpl > old_cpl) {
    // return to user
    uint32_t esp3 = vaddr_read(NULL, cpu.esp + 0, 4, MMU_DYNAMIC);
    uint32_t ss3  = vaddr_read(NULL, cpu.esp + 4, 4, MMU_DYNAMIC);
    cpu.esp = esp3;
    cpu.sreg[CSR_SS].val = ss3;
  }
  cpu.sreg[CSR_CS].val = new_cs;

  set_sys_state_flag(SYS_STATE_FLUSH_TCACHE);
  return new_pc;
}

static inline word_t priv_instr(uint32_t op, const rtlreg_t *src) {
  switch (op) {
    case PRIV_IRET: return iret();
    default: panic("Unsupported privilige operation = %d", op);
  }
}
#endif

void isa_hostcall(uint32_t id, rtlreg_t *dest, const rtlreg_t *src1,
    const rtlreg_t *src2, word_t imm) {
  word_t ret = 0;
  switch (id) {
    case HOSTCALL_CSR: csrrw(dest, src1, imm); return;
#ifdef CONFIG_MODE_USER
    case HOSTCALL_TRAP:
      Assert(imm == 0x80, "Unsupport exception = %d", imm);
      uintptr_t host_syscall(uintptr_t id, uintptr_t arg1, uintptr_t arg2,
          uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t arg6);
      cpu.eax = host_syscall(cpu.eax, cpu.ebx, cpu.ecx, cpu.edx, cpu.esi, cpu.edi, cpu.ebp);
      ret = *src1;
      break;
#else
    case HOSTCALL_TRAP: ret = raise_intr(imm, *src1); break;
    case HOSTCALL_PRIV: ret = priv_instr(imm, src1); break;
#endif
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}

#endif
