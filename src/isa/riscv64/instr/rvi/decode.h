static int table_rvm(Decode *s);
static int table_rvm32(Decode *s);
static int table_op_imm_c(Decode *s);
static int table_op_imm32_c(Decode *s);

static inline def_DopHelper(i) {
  op->imm = val;
  print_Dop(op->str, OP_STR_SIZE, (flag ? "0x%lx" : "%ld"), op->imm);
}

static inline def_DopHelper(r) {
  bool load_val = flag;
  static word_t zero_null = 0;
  op->preg = (!load_val && val == 0) ? &zero_null : &reg_l(val);
  IFDEF(CONFIG_DEBUG, op->reg = val);
  print_Dop(op->str, OP_STR_SIZE, "%s", reg_name(val, 4));
}

static inline def_DHelper(I) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, (sword_t)s->isa.instr.i.simm11_0, false);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}

static inline def_DHelper(R) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rs2, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
}

static inline def_DHelper(U) {
  decode_op_i(s, id_src1, (sword_t)s->isa.instr.u.simm31_12 << 12, true);
  decode_op_r(s, id_dest, s->isa.instr.u.rd, false);
}

static inline def_DHelper(auipc) {
  decode_U(s);
  id_src1->imm += s->pc;
}

static inline def_DHelper(J) {
  sword_t offset = (s->isa.instr.j.simm20 << 20) | (s->isa.instr.j.imm19_12 << 12) |
    (s->isa.instr.j.imm11 << 11) | (s->isa.instr.j.imm10_1 << 1);
  decode_op_i(s, id_src1, s->pc + offset, true);
  decode_op_r(s, id_dest, s->isa.instr.j.rd, false);
  id_src2->imm = s->snpc;
}

static inline def_DHelper(B) {
  sword_t offset = (s->isa.instr.b.simm12 << 12) | (s->isa.instr.b.imm11 << 11) |
    (s->isa.instr.b.imm10_5 << 5) | (s->isa.instr.b.imm4_1 << 1);
  decode_op_i(s, id_dest, s->pc + offset, true);
  decode_op_r(s, id_src1, s->isa.instr.b.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.b.rs2, true);
}

static inline def_DHelper(S) {
  decode_op_r(s, id_src1, s->isa.instr.s.rs1, true);
  sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
  decode_op_i(s, id_src2, simm, false);
  decode_op_r(s, id_dest, s->isa.instr.s.rs2, true);
}

def_THelper(load) {
  print_Dop(id_src1->str, OP_STR_SIZE, "%ld(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs1, 4));
  int mmu_mode = isa_mmu_state();
  if (mmu_mode == MMU_DIRECT) {
    switch (s->isa.instr.i.funct3) {
      TAB(0, lb)  TAB(1, lh)  TAB(2, lw)  TAB(3, ld)
      TAB(4, lbu) TAB(5, lhu) TAB(6, lwu)
    }
  } else if (mmu_mode == MMU_TRANSLATE) {
    switch (s->isa.instr.i.funct3) {
      TAB(0, lb_mmu)  TAB(1, lh_mmu)  TAB(2, lw_mmu)  TAB(3, ld_mmu)
      TAB(4, lbu_mmu) TAB(5, lhu_mmu) TAB(6, lwu_mmu)
    }
  } else {
    assert(0);
  }
  return EXEC_ID_inv;
}

def_THelper(store) {
  print_Dop(id_src1->str, OP_STR_SIZE, "%ld(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs1, 4));
  int mmu_mode = isa_mmu_state();
  if (mmu_mode == MMU_DIRECT) {
    switch (s->isa.instr.i.funct3) {
      TAB(0, sb)  TAB(1, sh)  TAB(2, sw)  TAB(3, sd)
    }
  } else if (mmu_mode == MMU_TRANSLATE) {
    switch (s->isa.instr.i.funct3) {
      TAB(0, sb_mmu)  TAB(1, sh_mmu)  TAB(2, sw_mmu)  TAB(3, sd_mmu)
    }
  } else {
    assert(0);
  }
  return EXEC_ID_inv;
}

def_THelper(li_dispatch) {
  switch (id_src2->imm) {
    TAB(0, p_li_0) TAB(1, p_li_1)
    default: TAB(2, c_li);
  }
}

def_THelper(addi_dispatch) {
  if (s->isa.instr.i.rs1 == 0) return table_li_dispatch(s);
  if (id_src2->imm == 0) return table_c_mv(s);
  return table_addi(s);
}

def_THelper(op_imm) {
  if (s->isa.instr.i.rd == s->isa.instr.i.rs1) return table_op_imm_c(s);
  if ((s->isa.instr.r.funct7 & ~0x1) == 32) {
    switch (s->isa.instr.r.funct3) { TAB(5, srai) }
  }
  switch (s->isa.instr.i.funct3) {
    TAB(0, addi_dispatch)  TAB(1, slli)  TAB(2, slti) TAB(3, sltui)
    TAB(4, xori)  TAB(5, srli)  TAB(6, ori)  TAB(7, andi)
  }
  return EXEC_ID_inv;
};

def_THelper(addiw_dispatch) {
  if (id_src2->imm == 0) return table_p_sext_w(s);
  return table_addiw(s);
}

def_THelper(op_imm32) {
  if (s->isa.instr.i.rd == s->isa.instr.i.rs1) return table_op_imm32_c(s);
  if (s->isa.instr.r.funct7 == 32) {
    switch (s->isa.instr.r.funct3) { TAB(5, sraiw) }
  }
  switch (s->isa.instr.i.funct3) {
    TAB(0, addiw_dispatch) TAB(1, slliw) TAB(5, srliw)
  }
  return EXEC_ID_inv;
}

def_THelper(op) {
  if (s->isa.instr.r.funct7 == 32) {
    if (s->isa.instr.r.rd == s->isa.instr.r.rs1) {
      switch (s->isa.instr.r.funct3) {
        TAB(0, c_sub)
      }
    }
    switch (s->isa.instr.r.funct3) {
      TAB(0, sub) TAB(5, sra)
    }
    return EXEC_ID_inv;
  }
  if (s->isa.instr.r.funct7 & 1) return table_rvm(s);
  int index = s->isa.instr.r.funct3;
  if (s->isa.instr.r.rd == s->isa.instr.r.rs1) {
    switch (index) {
      TAB(0, c_add) TAB(4, c_xor) TAB(6, c_or) TAB(7, c_and)
    }
  }
  switch (index) {
    TAB(0, add)  TAB(1, sll)  TAB(2, slt)  TAB(3, sltu)
    TAB(4, xor)  TAB(5, srl)  TAB(6, or)   TAB(7, and)
  }
  return EXEC_ID_inv;
}

def_THelper(op32) {
  if (s->isa.instr.r.funct7 == 32) {
    if (s->isa.instr.r.rd == s->isa.instr.r.rs1) {
      switch (s->isa.instr.r.funct3) {
        TAB(0, c_subw)
      }
    }
    switch (s->isa.instr.r.funct3) {
      TAB(0, subw) TAB(5, sraw)
    }
    return EXEC_ID_inv;
  }
  if (s->isa.instr.r.funct7 & 1) return table_rvm32(s);
  int index = s->isa.instr.r.funct3;
  if (s->isa.instr.r.rd == s->isa.instr.r.rs1) {
    switch (index) {
      TAB(0, c_addw)
    }
  }
  switch (index) {
    TAB(0, addw) TAB(1, sllw)
                 TAB(5, srlw)
  }
  return EXEC_ID_inv;
#undef pair
}

def_THelper(branch) {
  if (s->isa.instr.r.rs2 == 0) {
    switch (s->isa.instr.i.funct3) {
      TAB(0, c_beqz)  TAB(1, c_bnez)
      TAB(4, p_bltz)  TAB(5, p_bgez)
    }
  }
  if (s->isa.instr.r.rs1 == 0) {
    switch (s->isa.instr.i.funct3) {
      TAB(4, p_bgtz)  TAB(5, p_blez)
    }
  }
  switch (s->isa.instr.i.funct3) {
    TAB(0, beq)   TAB(1, bne)
    TAB(4, blt)   TAB(5, bge)   TAB(6, bltu)   TAB(7, bgeu)
  }
  return EXEC_ID_inv;
};

def_THelper(jal_dispatch) {
  switch (s->isa.instr.j.rd) {
    TAB(0, c_j) TAB(1, p_jal)
    default: TAB(2, jal)
  }
}

def_THelper(jalr_dispatch) {
  if (s->isa.instr.i.rd == 0  && id_src2->imm == 0) {
    if (s->isa.instr.i.rs1 == 1) return table_p_ret(s);
    else return table_c_jr(s);
  }
  return table_jalr(s);
}

def_THelper(mem_fence) {
  switch (s->isa.instr.i.funct3) {
    TAB(0, fence)  TAB(1, fence_i)
  }
  return EXEC_ID_inv;
}
