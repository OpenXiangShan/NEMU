#ifdef CONFIG_RV_ZFA
def_EHelper(fli_s) {
  rtl_li(s, s0, s->isa.instr.i.rs1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, s0, rz, FPCALL_CMD(FPCALL_FLI, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fli_d) {
  rtl_li(s, s0, s->isa.instr.i.rs1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, s0, rz, FPCALL_CMD(FPCALL_FLI, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fminm_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MINM, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fminm_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MINM, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fmaxm_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MAXM, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fmaxm_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MAXM, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fround_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FROUND, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(fround_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FROUND, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(froundnx_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FROUNDNX, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}

def_EHelper(froundnx_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FROUNDNX, FPCALL_W64));
  rtl_fsr(s, ddest, ddest, FPCALL_W64);
}

def_EHelper(fcvtmod_w_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FCVTMOD, FPCALL_W64));
  rtl_sext(s, ddest, ddest, 4);
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fleq_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_FLEQ, FPCALL_W32));
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fleq_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_FLEQ, FPCALL_W64));
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fltq_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_FLTQ, FPCALL_W32));
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fltq_d) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_FLTQ, FPCALL_W64));
  void fp_set_dirty();
  fp_set_dirty();
}

#ifdef CONFIG_RV_ZFH
def_EHelper(fli_h) {
  rtl_li(s, s0, s->isa.instr.i.rs1);
  rtl_hostcall(s, HOSTCALL_FP, ddest, s0, rz, FPCALL_CMD(FPCALL_FLI, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fminm_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MINM, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fmaxm_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_MAXM, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fround_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FROUND, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(froundnx_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_FROUNDNX, FPCALL_W16));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fleq_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_FLEQ, FPCALL_W16));
  void fp_set_dirty();
  fp_set_dirty();
}

def_EHelper(fltq_h) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, dsrc2, FPCALL_CMD(FPCALL_FLTQ, FPCALL_W16));
  void fp_set_dirty();
  fp_set_dirty();
}
#endif // CONFIG_RV_ZFH
#endif // CONFIG_RV_ZFA
