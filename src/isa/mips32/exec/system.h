def_EHelper(syscall) {
  rtl_trap(s, cpu.pc, EX_SYSCALL);
  print_asm("syscall");
}

def_EHelper(eret) {
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, PRIV_ERET);
  rtl_jr(s, s0);
  print_asm("eret");
}

def_EHelper(mfc0) {
  rtl_hostcall(s, HOSTCALL_CSR, dsrc2, NULL, id_dest->reg);
  print_asm("mfc0 %s, %s", id_src2->str, cp0_name(id_dest->reg));
}

def_EHelper(mtc0) {
  rtl_hostcall(s, HOSTCALL_CSR, NULL, dsrc2, id_dest->reg);
  print_asm("mtc0 %s, %s", id_src2->str, cp0_name(id_dest->reg));
}

def_EHelper(tlbwr) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, PRIV_TLBWR);
}

def_EHelper(tlbwi) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, PRIV_TLBWI);
}

def_EHelper(tlbp) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, PRIV_TLBP);
}
