#include <sys/time.h>

static inline def_EHelper(rdtsc) {
#ifndef __ENGINE_interpreter__
  panic("not support in engines other than interpreter");
#endif

  struct timeval now;
  gettimeofday(&now, NULL);
  uint64_t tsc = now.tv_sec * 1000000ull + now.tv_usec;
  cpu.edx = tsc >> 32;
  cpu.eax = tsc & 0xffffffff;

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
