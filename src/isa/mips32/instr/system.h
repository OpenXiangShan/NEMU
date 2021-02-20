#include "../local-include/intr.h"

def_EHelper(syscall) {
  rtl_trap(s, s->pc, EX_SYSCALL);
}

def_EHelper(eret) {
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, PRIV_ERET);
  rtl_jr(s, s0);
}

def_EHelper(mfc0) {
  rtl_hostcall(s, HOSTCALL_CSR, dsrc2, NULL, id_dest->imm);
}

def_EHelper(mtc0) {
  rtl_hostcall(s, HOSTCALL_CSR, NULL, dsrc2, id_dest->imm);
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
