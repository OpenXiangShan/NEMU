def_EHelper(lw) {
  update_gpc(thispc);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 4);
}

def_EHelper(sw) {
  update_gpc(thispc);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 4);
}

#ifndef __ICS_EXPORT
def_EHelper(lh) {
  update_gpc(thispc);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 2);
}

def_EHelper(lb) {
  update_gpc(thispc);
  rtl_lms(s, ddest, dsrc1, id_src2->imm, 1);
}

def_EHelper(lhu) {
  update_gpc(thispc);
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 2);
}

def_EHelper(lbu) {
  update_gpc(thispc);
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 1);
}

def_EHelper(sh) {
  update_gpc(thispc);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 2);
}

def_EHelper(sb) {
  update_gpc(thispc);
  rtl_sm(s, dsrc1, id_src2->imm, ddest, 1);
}
#endif
