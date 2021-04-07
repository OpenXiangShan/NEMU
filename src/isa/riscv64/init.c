#include <isa.h>
#include <memory/paddr.h>
#include "local-include/csr.h"

static const uint32_t img [] = {
  0x800002b7,  // lui t0,0x80000
  0x0002a023,  // sw  zero,0(t0)
  0x0002a503,  // lw  a0,0(t0)
  0x0000006b,  // nemu_trap
};

void init_clint(void);

void init_isa(void) {
  cpu.gpr[0]._64 = 0;
  cpu.pc = PMEM_BASE + IMAGE_START;

  cpu.mode = MODE_M;
#ifndef __DIFF_REF_QEMU__
  // QEMU seems to initialize mstatus with 0
  mstatus->val = 0x0;
#endif

#define ext(e) (1 << ((e) - 'a'))
  misa->extensions = ext('i') | ext('m') | ext('a') | ext('c') | ext('s') | ext('u');
  misa->extensions |= ext('d') | ext('f');
  misa->mxl = 2; // XLEN = 64

  memcpy(guest_to_host(IMAGE_START), img, sizeof(img));

  init_clint();
  extern void init_sdcard();
  init_sdcard();
}
