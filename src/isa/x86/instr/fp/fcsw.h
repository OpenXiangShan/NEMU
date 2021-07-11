def_EHelper(fxam) {
  rtl_li(s, s0, ((int64_t)*dfdest < 0) << 9); // sign bit
  rtl_li(s, s1, 0x4 << 8); // normal number
  rtl_or(s, &cpu.fsw, s0, s1);
}

def_EHelper(fnstsw) {
  rtl_sr(s, R_AX, &cpu.fsw, 2);
}

def_EHelper(fnstcw) {
  rt_decode_mem(s, id_dest, false, 0);
  uint32_t x86_fp_get_rm();
  uint32_t rm_x86 = x86_fp_get_rm();
  rtl_li(s, s0, rm_x86 << 10);
  rtl_sm(s, s0, &s->isa.mbr, s->isa.moff, 2, MMU_DYNAMIC);
}

def_EHelper(fldcw) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_lm(s, s0, &s->isa.mbr, s->isa.moff, 2, MMU_DYNAMIC);

  uint32_t rm_x86 = BITS(*s0, 11, 10);
  void x86_fp_set_rm(uint32_t rm_x86);
  x86_fp_set_rm(rm_x86);
}

def_EHelper(fwait) {
  // empty
}
