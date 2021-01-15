#include <cpu/exec.h>

def_EHelper(nop) {
  print_asm("nop");
}

def_EHelper(inv) {
  rtl_hostcall(s, HOSTCALL_INV, NULL, NULL, NULL, 0);
  print_asm("invalid opcode");
}

def_EHelper(nemu_trap) {
  rtl_hostcall(s, HOSTCALL_EXIT, NULL, &cpu.eax, NULL, 0);
  print_asm("nemu trap");
}
