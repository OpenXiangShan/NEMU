def_EHelper(flw) {
  rtl_lm(s, ddest, dsrc1, id_src2->imm, 4, MMU_DYNAMIC);
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fsw) {
  rtl_fbox(s, s0, ddest);
  rtl_sm(s, s0, dsrc1, id_src2->imm, 4, MMU_DYNAMIC);
}

#define def_fop_template(name, op, w) \
  def_EHelper(name) { \
    rtl_mv(s, s0, dsrc1); \
    rtl_hostcall(s, HOSTCALL_FP, s0, dsrc2, FPCALL_CMD(op, w)); \
    rtl_fsr(s, ddest, s0, w); \
  }

def_fop_template(fadds, FPCALL_ADD, FPCALL_W32)
def_fop_template(fsubs, FPCALL_SUB, FPCALL_W32)
def_fop_template(fmuls, FPCALL_MUL, FPCALL_W32)
def_fop_template(fdivs, FPCALL_DIV, FPCALL_W32)
