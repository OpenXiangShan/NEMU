#ifndef __ICS_EXPORT

def_EHelper(csrrw) {
  rtl_hostcall(s, HOSTCALL_CSR, ddest, dsrc1, NULL, id_src2->imm);
  rtl_priv_next(s);
}

def_EHelper(csrrs) {
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, NULL, id_src2->imm);
  rtl_or(s, s1, s0, dsrc1);
  rtl_mv(s, ddest, s0);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, NULL, id_src2->imm);
  rtl_priv_next(s);
}

def_EHelper(ecall) {
  rtl_trap(s, s->pc, 8 + cpu.mode);
  rtl_priv_jr(s, t0);
}

def_EHelper(mret) {
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, NULL, 0x302);
  rtl_priv_jr(s, s0);
}

def_EHelper(sfence_vma) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, dsrc1, NULL, 0x120);
  rtl_priv_next(s);
}
#endif
