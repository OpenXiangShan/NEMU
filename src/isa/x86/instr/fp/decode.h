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

static inline def_DHelper(ST0) {
  operand_freg(s, id_dest, 0);
}

static inline def_DHelper(push_ST0) {
  operand_freg(s, id_dest, -1);
}

static inline def_DHelper(ld_ST0) {
  operand_frm(s, id_src1, id_dest, -1);
}

static inline def_DHelper(st_ST0) {
  operand_frm(s, id_dest, id_src1, 0);
}

static inline def_DHelper(mem_ST0) {
  operand_frm(s, id_src1, id_dest, 0);
}

static inline def_DHelper(STi_ST0) {
  int i = get_instr(s) & 0x7;
  operand_freg(s, id_dest, i);
  operand_freg(s, id_src1, 0);
}

static inline def_DHelper(ST0_STi) {
  int i = get_instr(s) & 0x7;
  operand_freg(s, id_src1, i);
  operand_freg(s, id_dest, 0);
}

static inline def_DHelper(ST0_ST1) {
  operand_freg(s, id_src1, 1);
  operand_freg(s, id_dest, 0);
}

def_THelper(fpu_d8) {
  x86_instr_fetch(s, 1);

  if (get_instr(s) >= 0xc0) {
    def_INSTR_IDTAB("1100 0???", ST0_STi, fadd);
    def_INSTR_IDTAB("1100 1???", ST0_STi, fmul);
    def_INSTR_IDTAB("1110 0???", ST0_STi, fsub);
    def_INSTR_IDTAB("1110 1???", ST0_STi, fsubr);
    def_INSTR_IDTAB("1111 0???", ST0_STi, fdiv);
    def_INSTR_IDTAB("1111 1???", ST0_STi, fdivr);
  } else {
    def_INSTR_IDTAB("?? 000 ???", mem_ST0, fadds);
    def_INSTR_IDTAB("?? 001 ???", mem_ST0, fmuls);
    def_INSTR_IDTAB("?? 100 ???", mem_ST0, fsubs);
    def_INSTR_IDTAB("?? 101 ???", mem_ST0, fsubrs);
    def_INSTR_IDTAB("?? 110 ???", mem_ST0, fdivs);
    def_INSTR_IDTAB("?? 111 ???", mem_ST0, fdivrs);
  }

  return EXEC_ID_inv;
}

def_THelper(fpu_d9) {
  x86_instr_fetch(s, 1);

  if (get_instr(s) >= 0xc0) {
    def_hex_INSTR_IDTAB("e0", ST0     , fchs);
    def_hex_INSTR_IDTAB("e1", ST0     , fabs);
    def_hex_INSTR_IDTAB("e5", ST0     , fxam);
    def_hex_INSTR_IDTAB("e8", push_ST0, fld1);
    def_hex_INSTR_IDTAB("ea", push_ST0, fldl2e);
    def_hex_INSTR_IDTAB("ec", push_ST0, fldlg2);
    def_hex_INSTR_IDTAB("ed", push_ST0, fldln2);
    def_hex_INSTR_IDTAB("ee", push_ST0, fldz);
    def_hex_INSTR_IDTAB("f0", ST0     , f2xm1);
    def_hex_INSTR_IDTAB("f1", ST0_ST1 , fyl2x);
    def_hex_INSTR_IDTAB("f9", ST0_ST1 , fyl2xp1);
    def_hex_INSTR_IDTAB("fa", ST0     , fsqrt);
    def_hex_INSTR_IDTAB("fc", ST0     , frndint);
    def_hex_INSTR_IDTAB("fd", ST0_ST1 , fscale);
    def_INSTR_IDTAB("1100 0???", ld_ST0, fld);
    def_INSTR_IDTAB("1100 1???", STi_ST0, fxch);
  } else {
    def_INSTR_IDTAB("?? 000 ???", ld_ST0, flds);
    def_INSTR_IDTAB("?? 010 ???", st_ST0, fsts);
    def_INSTR_IDTAB("?? 011 ???", st_ST0, fstps);
    def_INSTR_IDTAB("?? 100 ???", st_ST0, fldenv);
    def_INSTR_IDTAB("?? 101 ???", st_ST0, fldcw);
    def_INSTR_IDTAB("?? 110 ???", st_ST0, fnstenv);
    def_INSTR_IDTAB("?? 111 ???", st_ST0, fnstcw);
  }

  return EXEC_ID_inv;
}

def_THelper(fpu_da) {
  x86_instr_fetch(s, 1);

  if (get_instr(s) >= 0xc0) {
    def_INSTR_IDTAB("1100 0???", ST0_STi, fcmovb);
    def_INSTR_IDTAB("1101 0???", ST0_STi, fcmovbe);
    def_hex_INSTR_IDTAB("e9", ST0_ST1 , fucompp);
  } else {
  }

  return EXEC_ID_inv;
}

def_THelper(fpu_db) {
  x86_instr_fetch(s, 1);

  if (get_instr(s) >= 0xc0) {
    def_INSTR_IDTAB("1110 1???", STi_ST0, fucomi);
    def_INSTR_IDTAB("1111 0???", STi_ST0, fcomi);
  } else {
    def_INSTR_IDTAB("?? 000 ???", ld_ST0, fildl);
    def_INSTR_IDTAB("?? 010 ???", st_ST0, fistl);
    def_INSTR_IDTAB("?? 011 ???", st_ST0, fistpl);
  }

  return EXEC_ID_inv;
}

def_THelper(fpu_dc) {
  x86_instr_fetch(s, 1);

  if (get_instr(s) >= 0xc0) {
    def_INSTR_IDTAB("1100 0???", STi_ST0, fadd);
    def_INSTR_IDTAB("1100 1???", STi_ST0, fmul);
    def_INSTR_IDTAB("1110 0???", STi_ST0, fsubr);
    def_INSTR_IDTAB("1110 1???", STi_ST0, fsub);
    def_INSTR_IDTAB("1111 0???", STi_ST0, fdivr);
    def_INSTR_IDTAB("1111 1???", STi_ST0, fdiv);
  } else {
    def_INSTR_IDTAB("?? 000 ???", mem_ST0, faddl);
    def_INSTR_IDTAB("?? 001 ???", mem_ST0, fmull);
    def_INSTR_IDTAB("?? 011 ???", mem_ST0, fcompl);
    def_INSTR_IDTAB("?? 100 ???", mem_ST0, fsubl);
    def_INSTR_IDTAB("?? 101 ???", mem_ST0, fsubrl);
    def_INSTR_IDTAB("?? 110 ???", mem_ST0, fdivl);
    def_INSTR_IDTAB("?? 111 ???", mem_ST0, fdivrl);
  }

  return EXEC_ID_inv;
}

def_THelper(fpu_dd) {
  x86_instr_fetch(s, 1);

  if (get_instr(s) >= 0xc0) {
    def_INSTR_IDTAB("1101 0???", STi_ST0, fst);
    def_INSTR_IDTAB("1101 1???", STi_ST0, fstp);
    def_INSTR_IDTAB("1110 1???", STi_ST0, fucomp);
  } else {
    def_INSTR_IDTAB("?? 000 ???", ld_ST0, fldl);
    def_INSTR_IDTAB("?? 010 ???", st_ST0, fstl);
    def_INSTR_IDTAB("?? 011 ???", st_ST0, fstpl);
  }

  return EXEC_ID_inv;
}

def_THelper(fpu_de) {
  x86_instr_fetch(s, 1);

  if (get_instr(s) >= 0xc0) {
    def_INSTR_IDTAB("1100 0???", STi_ST0, faddp);
    def_INSTR_IDTAB("1100 1???", STi_ST0, fmulp);
    def_INSTR_IDTAB("1110 0???", STi_ST0, fsubrp);
    def_INSTR_IDTAB("1110 1???", STi_ST0, fsubp);
    def_INSTR_IDTAB("1111 0???", STi_ST0, fdivrp);
    def_INSTR_IDTAB("1111 1???", STi_ST0, fdivp);
  } else {
  }

  return EXEC_ID_inv;
}

def_THelper(fpu_df) {
  x86_instr_fetch(s, 1);

  if (get_instr(s) >= 0xc0) {
    def_hex_INSTR_TAB("e0", fnstsw);

    def_INSTR_IDTAB("1110 1???", STi_ST0, fucomip);
    def_INSTR_IDTAB("1111 0???", STi_ST0, fcomip);
  } else {
    def_INSTR_IDTAB("?? 101 ???", ld_ST0, fildll);
    def_INSTR_IDTAB("?? 111 ???", st_ST0, fistpll);
  }

  return EXEC_ID_inv;
}
