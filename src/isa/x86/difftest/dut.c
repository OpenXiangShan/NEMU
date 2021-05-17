#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"
#include <difftest.h>

#ifndef __ICS_EXPORT
#include <memory/paddr.h>

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  if (memcmp(&cpu, ref_r, DIFFTEST_REG_SIZE)) {
    int i;
    for (i = 0; i < ARRLEN(cpu.gpr); i ++) {
      difftest_check_reg(reg_name(i, 4), pc, ref_r->gpr[i]._32, cpu.gpr[i]._32);
    }
    difftest_check_reg("pc", pc, ref_r->pc, cpu.pc);
    return false;
  }
  return true;
}

void isa_difftest_attach() {
  // first copy the image
  ref_difftest_memcpy(CONFIG_MBASE, guest_to_host(CONFIG_MBASE), CONFIG_MSIZE, DIFFTEST_TO_REF);

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
  ref_difftest_memcpy(0x7e00, code, sizeof(code), DIFFTEST_TO_REF);
  ref_difftest_memcpy(0x7e40, idtdesc, sizeof(idtdesc), DIFFTEST_TO_REF);

  CPU_state r = cpu;
  r.pc = 0x7e00;
  ref_difftest_regcpy(&r, DIFFTEST_TO_REF);
  ref_difftest_exec(5);

  ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);
}
#else
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  return false;
}

void isa_difftest_attach() {
}
#endif
