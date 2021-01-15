#include <common.h>

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction);
void difftest_exec(uint64_t n);
void qemu_write_reg(void *val, int idx);
void init_intr();

char *isa_qemu_argv[] = {
  "/usr/bin/qemu-system-i386",
  "-nographic", "-S", "-serial", "none", "-monitor", "none",
  "-cpu", "Broadwell",
  NULL
};

void init_isa() {
  uint8_t mbr[] = {
    // start16:
    0xfa,                           // cli
    0x31, 0xc0,                     // xorw   %ax,%ax
    0x8e, 0xd8,                     // movw   %ax,%ds
    0x8e, 0xc0,                     // movw   %ax,%es
    0x8e, 0xd0,                     // movw   %ax,%ss
    0x0f, 0x01, 0x16, 0x44, 0x7c,   // lgdt   gdtdesc
    0x0f, 0x20, 0xc0,               // movl   %cr0,%eax
    0x66, 0x83, 0xc8, 0x01,         // orl    $CR0_PE,%eax
    0x0f, 0x22, 0xc0,               // movl   %eax,%cr0
    0xea, 0x1d, 0x7c, 0x08, 0x00,   // ljmp   $GDT_ENTRY(1),$start32

    // start32:
    0x66, 0xb8, 0x10, 0x00,         // movw   $0x10,%ax
    0x8e, 0xd8,                     // movw   %ax, %ds
    0x8e, 0xc0,                     // movw   %ax, %es
    0x8e, 0xd0,                     // movw   %ax, %ss
    0xeb, 0xfe,                     // jmp    7c27
    0x8d, 0x76, 0x00,               // lea    0x0(%esi),%esi

    // GDT
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00,

    // GDT descriptor
    0x17, 0x00, 0x2c, 0x7c, 0x00, 0x00
  };

  // put the MBR code to QEMU to enable protected mode
  difftest_memcpy(0x7c00, mbr, sizeof(mbr), true);

  // set cs:eip to 0000:7c00
  uint32_t val = 0;
  qemu_write_reg((void *)&val, 10); // cs
  val = 0x7c00;
  qemu_write_reg((void *)&val, 8);  // pc

  // execute enough instructions to enter protected mode
  difftest_exec(20);

  init_intr();
}
