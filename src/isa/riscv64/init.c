#include <isa.h>
#include <memory/paddr.h>
#include "local-include/csr.h"

static const uint32_t img [] = {
  0x00000297,  // auipc t0,0
  0x0002b823,  // sw  zero,16(t0)
  0x0102b503,  // lw  a0,16(t0)
  0x0000006b,  // nemu_trap
  0xdeadbeef,  // some data
};

void init_csr();

void init_isa() {
  init_csr();

  cpu.gpr[0]._64 = 0;
  cpu.pc = RESET_VECTOR;

  cpu.mode = MODE_M;
  mstatus->val = 0;

#define ext(e) (1 << ((e) - 'a'))
  misa->extensions = ext('i') | ext('m') | ext('a') | ext('c') | ext('s') | ext('u');
  misa->extensions |= ext('d') | ext('f');

  misa->mxl = 2; // XLEN = 64

  memcpy(guest_to_host(RESET_VECTOR), img, sizeof(img));

#if !defined(CONFIG_TARGET_SHARE) && !defined(CONFIG_PA)
  void init_clint();
  init_clint();
#endif
}
