#include <isa.h>
#include <memory/paddr.h>
#include "local-include/reg.h"

static const uint8_t img []  = {
  0xb8, 0x34, 0x12, 0x00, 0x00,        // 100000:  movl  $0x1234,%eax
  0xb9, 0x27, 0x00, 0x10, 0x00,        // 100005:  movl  $0x100027,%ecx
  0x89, 0x01,                          // 10000a:  movl  %eax,(%ecx)
  0x66, 0xc7, 0x41, 0x04, 0x01, 0x00,  // 10000c:  movw  $0x1,0x4(%ecx)
  0xbb, 0x02, 0x00, 0x00, 0x00,        // 100012:  movl  $0x2,%ebx
  0x66, 0xc7, 0x84, 0x99, 0x00, 0xe0,  // 100017:  movw  $0x1,-0x2000(%ecx,%ebx,4)
  0xff, 0xff, 0x01, 0x00,
  0xb8, 0x00, 0x00, 0x00, 0x00,        // 100021:  movl  $0x0,%eax
  0xd6,                                // 100026:  nemu_trap
};

static void restart() {
  /* Set the initial instruction pointer. */
  cpu.pc = PMEM_BASE + IMAGE_START;
#ifndef __ICS_EXPORT
  cpu.sreg[CSR_CS].val = 0x8;
  cpu.cr0.val = 0x60000011;
  cpu.fcw = 0x37f;
#endif
}

void init_i8259a();
void init_mc146818rtc();
void init_i8253();
void init_ioport80();
void init_i8237a();
void init_sdcard(const char *img);

void init_isa() {
  /* Test the implementation of the `CPU_state' structure. */
  void reg_test();
#ifndef DETERMINISTIC
  reg_test();
#endif

  /* Load built-in image. */
  memcpy(guest_to_host(IMAGE_START), img, sizeof(img));

  /* Initialize this virtual computer system. */
  restart();

#ifndef USER_MODE
  init_i8259a();
  init_mc146818rtc();
  init_i8253();
  init_ioport80();
  init_i8237a();
  init_sdcard("/home/yzh/sdi/debian-16G.img");
#endif
}

#ifdef USER_MODE

// we only maintain base of the segment here
uint32_t GDT[4] = {0};

void isa_init_user(word_t sp) {
  cpu.esp = sp;
  cpu.edx = 0; // no handler for atexit()
  cpu.sreg[CSR_CS].val = 0xb; cpu.sreg[CSR_CS].base = 0;
  cpu.sreg[CSR_DS].val = 0xb; cpu.sreg[CSR_DS].base = 0;
  cpu.sreg[CSR_ES].val = 0xb; cpu.sreg[CSR_ES].base = 0;
  cpu.sreg[CSR_FS].val = 0xb; cpu.sreg[CSR_FS].base = 0;
}
#endif
