#include <cpu/exec.h>

def_EHelper(inv) {
  rtl_hostcall(s, HOSTCALL_INV, NULL, NULL, 0);
  print_asm("invalid opcode");
}

def_EHelper(nemu_trap) {
  rtl_hostcall(s, HOSTCALL_EXIT, NULL, &cpu.gpr[10]._32, 0); // gpr[10] is $a0
  print_asm("nemu trap");
}
