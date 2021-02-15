def_EHelper(inv) {
  rtl_hostcall(s, HOSTCALL_INV, NULL, NULL, 0);
  break;
}

def_EHelper(nemu_trap) {
  rtl_hostcall(s, HOSTCALL_EXIT, NULL, &cpu.gpr[10]._32, 0); // gpr[10] is $a0
  break;
}
