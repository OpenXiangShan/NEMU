#ifndef __ICS_EXPORT
#include "../local-include/intr.h"

static inline def_EHelper(syscall) {
  rtl_trap(s, cpu.pc, EX_SYSCALL);
  print_asm("syscall");
}

static inline def_EHelper(eret) {
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, PRIV_ERET);
  rtl_jr(s, s0);
  print_asm("eret");
}

static inline def_EHelper(mfc0) {
  rtl_hostcall(s, HOSTCALL_CSR, dsrc2, NULL, id_dest->reg);
  print_asm("mfc0 %s, %s", id_src2->str, cp0_name(id_dest->reg));
}

static inline def_EHelper(mtc0) {
  rtl_hostcall(s, HOSTCALL_CSR, NULL, dsrc2, id_dest->reg);
  print_asm("mtc0 %s, %s", id_src2->str, cp0_name(id_dest->reg));
}

static inline def_EHelper(tlbwr) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, PRIV_TLBWR);
}

static inline def_EHelper(tlbwi) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, PRIV_TLBWI);
}

static inline def_EHelper(tlbp) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, PRIV_TLBP);
}
#endif
