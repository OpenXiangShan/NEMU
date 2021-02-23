def_EHelper(ld) {
  save_globals(lpc, n);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 8);
}

def_EHelper(lw) {
  save_globals(lpc, n);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 4);
}

def_EHelper(lh) {
  save_globals(lpc, n);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 2);
}

def_EHelper(lb) {
  save_globals(lpc, n);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 1);
}

def_EHelper(lwu) {
  save_globals(lpc, n);
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 4);
}

def_EHelper(lhu) {
  save_globals(lpc, n);
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 2);
}

def_EHelper(lbu) {
  save_globals(lpc, n);
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 1);
}

def_EHelper(sd) {
  save_globals(lpc, n);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 8);
}

def_EHelper(sw) {
  save_globals(lpc, n);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 4);
}

def_EHelper(sh) {
  save_globals(lpc, n);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 2);
}

def_EHelper(sb) {
  save_globals(lpc, n);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 1);
}
