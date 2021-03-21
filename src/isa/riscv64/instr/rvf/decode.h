bool fp_enable();
static int table_fop_d(Decode *s);
static int table_fop_gpr_d(Decode *s);

static inline def_DopHelper(fr){
  op->preg = &fpreg_l(val);
  IFDEF(CONFIG_DEBUG, op->reg = val);
  print_Dop(op->str, OP_STR_SIZE, "%s", fpreg_name(val, 4));
}

static inline def_DHelper(fr) {
  decode_op_fr(s, id_src1, s->isa.instr.fp.rs1, false);
  decode_op_fr(s, id_src2, s->isa.instr.fp.rs2, false);
  decode_op_fr(s, id_dest, s->isa.instr.fp.rd,  false);
}

static inline def_DHelper(fload) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, (sword_t)s->isa.instr.i.simm11_0, false);
  decode_op_fr(s, id_dest, s->isa.instr.i.rd, false);
}

static inline def_DHelper(fstore) {
  decode_op_r(s, id_src1, s->isa.instr.s.rs1, true);
  sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
  decode_op_i(s, id_src2, simm, false);
  decode_op_fr(s, id_dest, s->isa.instr.s.rs2, false);
}

static inline def_DHelper(fr2r){
  decode_op_fr(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_fr(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_r(s, id_dest, s->isa.instr.fp.rd, false);
}

static inline def_DHelper(r2fr){
  decode_op_r(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_fr(s, id_dest, s->isa.instr.fp.rd, false);
}

def_THelper(fload) {
  if (!fp_enable()) return EXEC_ID_rt_inv;
  print_Dop(id_src1->str, OP_STR_SIZE, "%ld(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs1, 4));
  switch (s->isa.instr.i.funct3) {
    TAB(2, flw) TAB(3, fld)
  }
  return EXEC_ID_inv;
}

def_THelper(fstore) {
  if (!fp_enable()) return EXEC_ID_rt_inv;
  print_Dop(id_src1->str, OP_STR_SIZE, "%ld(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs1, 4));
  switch (s->isa.instr.s.funct3) {
    TAB(2, fsw) TAB(3, fsd)
  }
  return EXEC_ID_inv;
}

def_THelper(fop) {
  int funct4 = s->isa.instr.fp.funct5 & 0b01111;

  switch (funct4) {
    TAB(0b0000, fadds)   TAB(0b0001, fsubs)
    TAB(0b0010, fmuls)   TAB(0b0011, fdivs)
//    TAB(0b0100, fsgnjs)  TAB(0b0101, fmin_fmax)
//    TAB(0b1000, fcvt_F_to_F)   TAB(0b1011, fsqrt)
    TAB(0b1011, fsqrts)
  }
  return EXEC_ID_inv;
}

def_THelper(fcmp_dispatch) {
  switch (s->isa.instr.fp.rm) {
    TAB(0b001, flts)
  }
  return EXEC_ID_inv;
}

def_THelper(fmv_dispatch) {
  int funct4 = s->isa.instr.fp.funct5 & 0b01111;
  int rm = s->isa.instr.fp.rm;
#define pair(x, y) (((y) << 3) | (x))
  switch (pair(rm, funct4)) {
    TAB(pair(0b000, 0b1100), fmv_x_w)
    TAB(pair(0b000, 0b1110), fmv_w_x)
//    TAB(pair(0b001, 0b1100), fclass)
  }
#undef pair
  return EXEC_ID_inv;
}

def_THelper(fop_gpr) {
  int funct4 = s->isa.instr.fp.funct5 & 0b01111;
  switch (funct4) {
    IDTAB(0b0100, fr2r, fcmp_dispatch)
  }
  assert((s->isa.instr.fp.rs2 & 0x1c) == 0);
  int sign = s->isa.instr.fp.rs2 & 0x3;
#define pair(x, y) (((y) << 2) | (x))
  switch (pair(sign, funct4)) {
    IDTAB(pair(0b00, 0b1010), r2fr, fcvt_s_w)
    IDTAB(pair(0b01, 0b1010), r2fr, fcvt_s_wu)
    IDTAB(pair(0b10, 0b1010), r2fr, fcvt_s_l)
    IDTAB(pair(0b11, 0b1010), r2fr, fcvt_s_lu)

    IDTAB(pair(0b00, 0b1000), fr2r, fcvt_w_s)
    IDTAB(pair(0b01, 0b1000), fr2r, fcvt_wu_s)
    IDTAB(pair(0b10, 0b1000), fr2r, fcvt_l_s)
    IDTAB(pair(0b11, 0b1000), fr2r, fcvt_lu_s)

    IDTAB(pair(0b00, 0b1100), fr2r, fmv_dispatch)
    IDTAB(pair(0b00, 0b1110), r2fr, fmv_dispatch)
//    IDTAB(0b1000, fr2r, fcvt_F_to_G) IDTAB(0b1010, r2fr, fcvt_G_to_F)
//    IDTAB(0b1100, fr2r, fmv_F_to_G)  IDTAB(0b1110, r2fr, fmv_G_to_F)
  }
#undef pair
  return EXEC_ID_inv;
}

def_THelper(op_fp) {
  if (!fp_enable()) return EXEC_ID_rt_inv;

  int rvd = s->isa.instr.fp.fmt;
  switch (s->isa.instr.fp.fmt) {
    // fcvt.s.d (funct5 == 0b01000) belongs to rvd
    case 0b00: rvd = (s->isa.instr.fp.funct5 == 0b01000 ? true : false); break;
    case 0b01: rvd = true; break;
    default: return EXEC_ID_inv;
  }

  int need_gpr = (s->isa.instr.fp.funct5 & 0b10000) != 0;
#define pair(x, y) (((y) << 1) | (x))
  switch (pair(rvd, need_gpr)) {
    IDTAB(pair(0, 0), fr, fop)   TAB(pair(0, 1), fop_gpr)
    IDTAB(pair(1, 0), fr, fop_d) TAB(pair(1, 1), fop_gpr_d)
  }
#undef pair
  return EXEC_ID_inv;
}
