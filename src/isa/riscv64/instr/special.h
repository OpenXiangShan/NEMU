#ifdef __ICS_EXPORT
def_EHelper(inv) {
  rtl_hostcall(s, HOSTCALL_INV, NULL, NULL, NULL, 0);
}

def_EHelper(nemu_trap) {
  rtl_hostcall(s, HOSTCALL_EXIT, NULL, &gpr(10), NULL, 0); // gpr(10) is $a0
}
#else
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
  rtl_hostcall(s, HOSTCALL_EXIT, NULL, &gpr(10), NULL, 0); // gpr(10) is $a0
  longjmp_exec(NEMU_EXEC_END);
}
#endif
