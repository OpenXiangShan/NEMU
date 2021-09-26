#include "../local-include/intr.h"

def_EHelper(syscall) {
  rtl_hostcall(s, HOSTCALL_TRAP_THIS, s0, NULL, NULL, EX_SYSCALL);
  rtl_priv_jr(s, s0);
}

def_EHelper(eret) {
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, NULL, PRIV_ERET);
  rtl_priv_jr(s, s0);
}

def_EHelper(mfc0) {
  rtl_hostcall(s, HOSTCALL_CSR, dsrc2, NULL, NULL, id_dest->imm);
  rtl_priv_next(s);
}

def_EHelper(mtc0) {
  rtl_hostcall(s, HOSTCALL_CSR, NULL, dsrc2, NULL, id_dest->imm);
  rtl_priv_next(s);
}

def_EHelper(tlbwr) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, NULL, PRIV_TLBWR);
}

def_EHelper(tlbwi) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, NULL, PRIV_TLBWI);
}

def_EHelper(tlbp) {
  rtl_hostcall(s, HOSTCALL_PRIV, NULL, NULL, NULL, PRIV_TLBP);
}
