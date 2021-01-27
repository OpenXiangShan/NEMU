def_EHelper(inv) {
  rtl_hostcall(s, HOSTCALL_INV, NULL, NULL, 0);
  remain_instr = n;
  n = 1;
  print_asm("invalid opcode");
}

def_EHelper(nemu_trap) {
  rtl_hostcall(s, HOSTCALL_EXIT, NULL, &cpu.gpr[10]._32, 0); // gpr[10] is $a0
  remain_instr = n;
  n = 1;
  print_asm("nemu trap");
}
