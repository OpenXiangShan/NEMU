def_EHelper(fld) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 8, MMU_DYNAMIC);
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fsd) {
  rtl_sm(s, ddest, dsrc1, id_src2->imm, 8, MMU_DYNAMIC);
}

def_fop_template(faddd, FPCALL_ADD, FPCALL_W64)
def_fop_template(fsubd, FPCALL_SUB, FPCALL_W64)
def_fop_template(fmuld, FPCALL_MUL, FPCALL_W64)
def_fop_template(fdivd, FPCALL_DIV, FPCALL_W64)
