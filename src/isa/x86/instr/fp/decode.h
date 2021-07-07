static inline void operand_freg(Decode *s, Operand *op, int i) {
  op->type = OP_TYPE_REG;
//  op->reg = i;
  op->pfreg = &cpu.fpr[(cpu.ftop + i) & 0x7];
  print_Dop(op->str, OP_STR_SIZE, "%%st(%d)", i);
}

static inline void operand_frm(Decode *s, Operand *rm, Operand *reg, int i) {
  ModR_M m;
  m.val = get_instr(s);
  if (reg != NULL) operand_freg(s, reg, i); // load ST(i)
  if (m.mod == 3) operand_freg(s, rm, m.R_M);
  else { load_addr(s, &m, rm); }
}

static inline def_DHelper(push_ST0) {
  operand_freg(s, id_dest, -1);
}

static inline def_DHelper(ld_ST0) {
  operand_frm(s, id_src1, id_dest, -1);
}

static inline def_DHelper(st_ST0) {
  operand_frm(s, id_src1, id_dest, 0);
}

static inline def_DHelper(STi_ST0) {
  int i = get_instr(s) & 0x7;
  operand_freg(s, id_dest, i);
  operand_freg(s, id_src1, 0);
}

def_THelper(fpu_d9) {
  x86_instr_fetch(s, 1);

  def_hex_INSTR_IDTAB("e8", push_ST0, fld1);

  def_INSTR_IDTAB("?? 000 ???", ld_ST0, flds);
  def_INSTR_IDTAB("?? 011 ???", st_ST0, fstps);

  return EXEC_ID_inv;
}

def_THelper(fpu_de) {
  x86_instr_fetch(s, 1);

  def_INSTR_IDTAB("1100 0???", STi_ST0, faddp);

  return EXEC_ID_inv;
}
