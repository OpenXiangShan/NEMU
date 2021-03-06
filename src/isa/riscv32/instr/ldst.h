def_EHelper(lw) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 4);
}

def_EHelper(sw) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 4);
}

#ifndef __ICS_EXPORT
def_EHelper(lh) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 2);
}

def_EHelper(lb) {
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 1);
}

def_EHelper(lhu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 2);
}

def_EHelper(lbu) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 1);
}

def_EHelper(sh) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 2);
}

def_EHelper(sb) {
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 1);
}
#endif
