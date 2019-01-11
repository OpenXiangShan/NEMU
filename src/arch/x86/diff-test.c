#include "nemu.h"
#include "monitor/diff-test.h"

extern void (*ref_difftest_memcpy_from_dut)(paddr_t dest, void *src, size_t n);
extern void (*ref_difftest_getregs)(void *c);
extern void (*ref_difftest_setregs)(const void *c);
extern void (*ref_difftest_exec)(uint64_t n);

#define check_reg(regs, r) \
  if (regs->r != cpu.r) { \
    Log("%s is different after executing instruction at eip = 0x%08x, right = 0x%08x, wrong = 0x%08x", \
        str(r), pc, regs->r, cpu.r); \
  }

bool arch_difftest_check_reg(CPU_state *ref_r, vaddr_t pc) {
  // TODO: Check the registers state with QEMU.
  if (memcmp(&cpu, ref_r, DIFFTEST_REG_SIZE)) {
    check_reg(ref_r, eax);
    check_reg(ref_r, ecx);
    check_reg(ref_r, edx);
    check_reg(ref_r, ebx);
    check_reg(ref_r, esp);
    check_reg(ref_r, ebp);
    check_reg(ref_r, esi);
    check_reg(ref_r, edi);
    check_reg(ref_r, eip);

    return false;
  }

  return true;
//  check_flag(ref_r, CF);
//  check_flag(ref_r, OF);
//  check_flag(ref_r, SF);
//  check_flag(ref_r, ZF);
//
//  if (eflags_skip_mask) {
//    eflags_skip_mask = 0;
//  }
}

void arch_difftest_arch_attach(void) {
  // first copy the image
  ref_difftest_memcpy_from_dut(PC_START, guest_to_host(PC_START), PMEM_SIZE - PC_START);

  // then set some special registers
  uint8_t code[] = {
    // we put this code at 0x7e00
    0xb8, 0x00, 0x00, 0x00, 0x00,    // mov $0x0, %eax
    0x0f, 0x22, 0xd8,                // mov %eax, %cr3
    0xb8, 0x00, 0x00, 0x00, 0x00,    // mov $0x0, %eax
    0x0f, 0x22, 0xc0,                // mov %eax, %cr0
    0x0f, 0x01, 0x1d, 0x40, 0x7e, 0x00, 0x00,     // lidtl (0x7e40)
  };
  uint8_t idtdesc[6];

  *(uint32_t *)(code + 1) = cpu.cr3.val;
  *(uint32_t *)(code + 9) = cpu.cr0.val;

  idtdesc[0] = cpu.idtr.limit & 0xff;
  idtdesc[1] = cpu.idtr.limit >> 8;
  *(uint32_t *)(idtdesc + 2) = cpu.idtr.base;

  assert(sizeof(code) < 0x40);
  ref_difftest_memcpy_from_dut(0x7e00, code, sizeof(code));
  ref_difftest_memcpy_from_dut(0x7e40, idtdesc, sizeof(idtdesc));

  CPU_state r = cpu;
  r.eip = 0x7e00;
  ref_difftest_setregs(&r);
  ref_difftest_exec(5);

  ref_difftest_setregs(&cpu);
}
