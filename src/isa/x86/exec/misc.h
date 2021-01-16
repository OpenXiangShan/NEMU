#include <sys/time.h>
#include "../local-include/intr.h"

static inline def_EHelper(rdtsc) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

#ifdef DETERMINISTIC
  cpu.edx = 0;
  cpu.eax = 0;
#else
  struct timeval now;
  gettimeofday(&now, NULL);
  uint64_t tsc = now.tv_sec * 1000000ull + now.tv_usec;
  cpu.edx = tsc >> 32;
  cpu.eax = tsc & 0xffffffff;
#endif

  difftest_skip_ref();

  print_asm("rdtsc");
}

static inline def_EHelper(cpuid) {
  rtl_mv(s, &cpu.eax, rz);
  rtl_mv(s, &cpu.ebx, rz);
  rtl_mv(s, &cpu.ecx, rz);
  rtl_mv(s, &cpu.edx, rz);

  difftest_skip_ref();
  print_asm("cpuid");
}

static inline def_EHelper(fwait) {
  print_asm("fwait");
}

static inline def_EHelper(fpu) {
  rtl_li(s, s0, cpu.pc);
  rtl_hostcall(s, HOSTCALL_TRAP, s0, s0, 7);
  rtl_jr(s, s0);
}

static inline def_EHelper(hlt) {
  rtl_li(s, s0, s->seq_pc);
  rtl_hostcall(s, HOSTCALL_TRAP, s0, s0, IRQ_TIMER);
  rtl_jr(s, s0);
  if (ref_difftest_raise_intr) ref_difftest_raise_intr(IRQ_TIMER);
  print_asm("hlt");
}
