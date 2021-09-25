#include <isa.h>
#include <memory/paddr.h>
#ifndef __ICS_EXPORT
#include "local-include/csr.h"
#endif

// this is not consistent with uint8_t
// but it is ok since we do not access the array directly
static const uint32_t img [] = {
  0x00000297,  // auipc t0,0
  0x0002b823,  // sd  zero,16(t0)
  0x0102b503,  // ld  a0,16(t0)
  0x0000006b,  // nemu_trap
  0xdeadbeef,  // some data
};

static void restart() {
  /* Set the initial program counter. */
  cpu.pc = RESET_VECTOR;

  /* The zero register is always 0. */
  cpu.gpr[0]._64 = 0;
#ifndef __ICS_EXPORT
  void init_csr();
  init_csr();

  cpu.mode = MODE_M;
  mstatus->val = 0xa00000000ull;

#define ext(e) (1 << ((e) - 'a'))
  misa->extensions = ext('i') | ext('m') | ext('a') | ext('c') | ext('s') | ext('u');
  misa->extensions |= ext('d') | ext('f');

  misa->mxl = 2; // XLEN = 64
#endif
}

void init_isa() {
  /* Load built-in image. */
  memcpy(guest_to_host(RESET_VECTOR), img, sizeof(img));

  /* Initialize this virtual computer system. */
  restart();
#if defined(CONFIG_TARGET_NATIVE_ELF) && !defined(CONFIG_PA)
  void init_clint();
  init_clint();
#endif
}
