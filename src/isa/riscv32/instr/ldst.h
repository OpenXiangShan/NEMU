def_EHelper(lw) {
  save_globals(thispc, n);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 4);
}

def_EHelper(sw) {
  save_globals(thispc, n);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 4);
}

#ifndef __ICS_EXPORT
def_EHelper(lh) {
  save_globals(thispc, n);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 2);
}

def_EHelper(lb) {
  save_globals(thispc, n);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 1);
}

def_EHelper(lhu) {
  save_globals(thispc, n);
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 2);
}

def_EHelper(lbu) {
  save_globals(thispc, n);
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 1);
}

def_EHelper(sh) {
  save_globals(thispc, n);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 2);
}

def_EHelper(sb) {
  save_globals(thispc, n);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 1);
}
#endif
