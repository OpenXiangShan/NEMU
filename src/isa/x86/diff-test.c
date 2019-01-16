#include "nemu.h"
#include "monitor/diff-test.h"

extern void (*ref_difftest_memcpy_from_dut)(paddr_t dest, void *src, size_t n);
extern void (*ref_difftest_getregs)(void *c);
extern void (*ref_difftest_setregs)(const void *c);
extern void (*ref_difftest_exec)(uint64_t n);

void isa_difftest_attach(void) {
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
