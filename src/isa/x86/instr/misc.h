def_EHelper(cpuid) {
  rtl_mv(s, &cpu.eax, rz);
  rtl_mv(s, &cpu.ebx, rz);
  rtl_mv(s, &cpu.ecx, rz);
  rtl_mv(s, &cpu.edx, rz);
  difftest_skip_ref();
}

def_EHelper(rdtsc) {
#if defined(CONFIG_DETERMINISTIC) || defined(CONFIG_ENGINE_INTERPRETER)
  rtl_li(s, &cpu.edx, 0);
  rtl_li(s, &cpu.eax, 0);
#else
  uint64_t tsc = get_time();
  cpu.edx = tsc >> 32;
  cpu.eax = tsc & 0xffffffff;
#endif

  difftest_skip_ref();
}

#if 0
static inline def_EHelper(fwait) {
  print_asm("fwait");
}

static inline def_EHelper(fpu) {
  rtl_trap(s, cpu.pc, 7);
}

static inline def_EHelper(hlt) {
  rtl_trap(s, s->seq_pc, IRQ_TIMER);
  if (ref_difftest_raise_intr) ref_difftest_raise_intr(IRQ_TIMER);
  print_asm("hlt");
}
#endif
