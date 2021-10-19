void x86_fp_set_rm(uint32_t rm_x86);

def_EHelper(fxam) {
  rtl_fclassd(s, s0, dfdest);
  rtl_setrelopi(s, RELOP_LTU, s0, s0, 0b10000); // less than 0 ?
  rtl_slli(s, s0, s0, 9); // sign bit
  rtl_ori(s, s0, s0, 0x4 << 8); // normal number
  rtl_andi(s, &cpu.fsw, &cpu.fsw, ~0x4700); // mask
  rtl_or(s, &cpu.fsw, &cpu.fsw, s0);
}

def_EHelper(fnstsw) {
  rtl_sr(s, R_AX, &cpu.fsw, 2);
}

def_EHelper(fnstcw) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_sm(s, &cpu.fcw, &s->isa.mbr, s->isa.moff, 2, MMU_DYNAMIC);
}

def_EHelper(fldcw) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_lm(s, &cpu.fcw, &s->isa.mbr, s->isa.moff, 2, MMU_DYNAMIC);
  x86_fp_set_rm(cpu.frm);
}

def_EHelper(fwait) {
  // empty
}

def_EHelper(fldenv) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_lm(s, &cpu.fcw, s->isa.mbase, s->isa.moff + 0, 4, MMU_DYNAMIC);
  x86_fp_set_rm(cpu.frm);
  rtl_lm(s, &cpu.fsw, s->isa.mbase, s->isa.moff + 4, 4, MMU_DYNAMIC);
  // others are not loaded
}

def_EHelper(fnstenv) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_sm(s, &cpu.fcw, s->isa.mbase, s->isa.moff + 0, 4, MMU_DYNAMIC);
  rtl_sm(s, &cpu.fsw, s->isa.mbase, s->isa.moff + 4, 4, MMU_DYNAMIC);
#if 0
  rtlreg_t ftag = 0;
  int i;
  for (i = 7; i >= 0; i --) {
    ftag <<= 2;
    if (i > cpu.ftop) { ftag |= 3; } // empty
    else {
      rtl_class387(s, s0, &cpu.fpr[i]);
      if (*s0 == 0x4200 || *s0 == 0x4000) { ftag |= 1; } // zero
      else if (*s0 == 0x700 || *s0 == 0x4600 || *s0 == 0x4400 || *s0 == 0x500 ||
          *s0 == 0x100 || *s0 == 0x300) { ftag |= 2; } // NaNs, INFs, denormal
    }
  }
  rtl_sm(s, s->isa.mbase, s->isa.moff + 8, &ftag, 4);
  rtl_sm(s, s->isa.mbase, s->isa.moff + 12, rz, 4);  // fpip
  rtl_sm(s, s->isa.mbase, s->isa.moff + 16, rz, 4);  // fpcs
  rtl_sm(s, s->isa.mbase, s->isa.moff + 20, rz, 4);  // fpoo
  rtl_sm(s, s->isa.mbase, s->isa.moff + 24, rz, 4);  // fpos
#endif
}
