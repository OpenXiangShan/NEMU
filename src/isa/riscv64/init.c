#include <isa.h>
#include <memory/paddr.h>
#include "local-include/csr.h"

static const uint32_t img [] = {
  0x800002b7,  // lui t0,0x80000
  0x0002a023,  // sw  zero,0(t0)
  0x0002a503,  // lw  a0,0(t0)
  0x0000006b,  // nemu_trap
};

void init_csr();
#ifndef CONFIG_SHARE
void init_clint();
#endif
void init_device();

void init_isa() {
  init_csr();

#ifndef CONFIG_RESET_FROM_MMIO
  cpu.pc = RESET_VECTOR;
#else
  cpu.pc = CONFIG_MMIO_RESET_VECTOR;
#endif

  cpu.gpr[0]._64 = 0;

  cpu.mode = MODE_M;
  // For RV64 systems, the SXL and UXL fields are WARL fields that
  // control the value of XLEN for S-mode and U-mode, respectively.
  // For RV64 systems, if S-mode is not supported, then SXL is hardwired to zero.
  // For RV64 systems, if U-mode is not supported, then UXL is hardwired to zero.
  mstatus->val = 0xaUL << 32;

  pmpcfg0->val = 0;
  pmpcfg1->val = 0;
  pmpcfg2->val = 0;
  pmpcfg3->val = 0;

#ifdef CONFIG_RV_SVINVAL
  srnctl->val = 3; // enable extension 'svinval' [1]
#endif

#define ext(e) (1 << ((e) - 'a'))
  misa->extensions = ext('i') | ext('m') | ext('a') | ext('c') | ext('s') | ext('u');
  misa->extensions |= ext('d') | ext('f');
  misa->mxl = 2; // XLEN = 64

#ifdef CONFIG_RVV_010
  // vector
  vl->val = 0;
  vtype->val = 0; // actually should be 1 << 63 (set vill bit to forbidd)
#endif // CONFIG_RVV_010

#ifndef CONFIG_SHARE
  extern char *cpt_file;
  if (cpt_file == NULL) {
    memcpy(guest_to_host(RESET_VECTOR), img, sizeof(img));
  }
#else
  memcpy(guest_to_host(RESET_VECTOR), img, sizeof(img));
#endif

  IFNDEF(CONFIG_SHARE, init_clint());
  IFDEF(CONFIG_SHARE, init_device());

#ifndef CONFIG_SHARE
  Log("NEMU will start from pc 0x%lx", cpu.pc);
#endif
}
