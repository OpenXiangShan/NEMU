def_EHelper(inv) {
  rtl_hostcall(s, HOSTCALL_INV, NULL, NULL, 0);
}

def_EHelper(nemu_trap) {
  rtl_hostcall(s, HOSTCALL_EXIT, NULL, &cpu.gpr[2]._32, 0); // gpr[2] is $v0
}
