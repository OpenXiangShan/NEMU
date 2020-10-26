#include <sys/time.h>

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

  print_asm("cpuid");
}

static inline def_EHelper(fwait) {
  print_asm("fwait");
}

static inline def_EHelper(fpu) {
  void raise_intr(DecodeExecState *s, uint32_t, vaddr_t);
  raise_intr(s, 7, cpu.pc);
}

static inline def_EHelper(hlt) {
  print_asm("hlt");
}
