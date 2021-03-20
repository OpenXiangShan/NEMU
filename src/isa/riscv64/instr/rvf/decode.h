bool fp_enable();
static int table_fop_d(Decode *s);
static int table_fop_gpr_d(Decode *s);

static inline def_DopHelper(fpr){
  op->preg = &fpreg_l(val);
  IFDEF(CONFIG_DEBUG, op->reg = val);
  print_Dop(op->str, OP_STR_SIZE, "%s", fpreg_name(val, 4));
}

static inline def_DHelper(fr) {
  decode_op_fpr(s, id_src1, s->isa.instr.fp.rs1, false);
  decode_op_fpr(s, id_src2, s->isa.instr.fp.rs2, false);
  decode_op_fpr(s, id_dest, s->isa.instr.fp.rd,  false);
}

static inline def_DHelper(fload) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, (sword_t)s->isa.instr.i.simm11_0, false);
  decode_op_fpr(s, id_dest, s->isa.instr.i.rd, false);
}

static inline def_DHelper(fstore) {
  decode_op_r(s, id_src1, s->isa.instr.s.rs1, true);
  sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
  decode_op_i(s, id_src2, simm, false);
  decode_op_fpr(s, id_dest, s->isa.instr.s.rs2, false);
}

#if 0
static inline def_DHelper(F_fpr_to_gpr){
  decode_op_fpr(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_fpr(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_r(s, id_dest, s->isa.instr.fp.rd, false);
}

static inline def_DHelper(F_gpr_to_fpr){
  decode_op_r(s, id_src1, s->isa.instr.fp.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.fp.rs2, true);
  decode_op_fpr(s, id_dest, s->isa.instr.fp.rd, false);
}
#endif


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
  }
  return EXEC_ID_inv;
}

def_THelper(fop_gpr) {
  int funct4 = s->isa.instr.fp.funct5 & 0b01111;
  switch (funct4) {
//    IDTAB(0b0100, F_fpr_to_gpr, fcmp)
//    IDTAB(0b1000, F_fpr_to_gpr, fcvt_F_to_G) IDTAB(0b1010, F_gpr_to_fpr, fcvt_G_to_F)
//    IDTAB(0b1100, F_fpr_to_gpr, fmv_F_to_G)  IDTAB(0b1110, F_gpr_to_fpr, fmv_G_to_F)
  }
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
