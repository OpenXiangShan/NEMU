#include "../local-include/intr.h"

def_EHelper(inv) {
  save_globals(s);
  rtl_hostcall(s, HOSTCALL_INV, NULL, NULL, NULL, 0);
  longjmp_exec(NEMU_EXEC_END);
}

def_EHelper(rt_inv) {
  save_globals(s);
  longjmp_exception(EX_II);
}

def_EHelper(nemu_trap) {
  save_globals(s);
  rtl_hostcall(s, HOSTCALL_EXIT, NULL, &cpu.gpr[10]._64, NULL, 0); // gpr[10] is $a0
  longjmp_exec(NEMU_EXEC_END);
}
