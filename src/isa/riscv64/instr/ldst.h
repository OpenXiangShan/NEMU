#ifndef __ICS_EXPORT
// This file is only used when exporting to ICS
#endif
def_EHelper(ld) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 8);
}

def_EHelper(sd) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 8);
}
