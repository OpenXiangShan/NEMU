#include "../local-include/rtl.h"
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <isa-all-instr.h>

def_all_THelper();

// decode operand helper
#define def_DopHelper(name) \
  void concat(decode_op_, name) (Decode *s, Operand *op, uint32_t val, bool flag)

static inline def_DopHelper(i) {
  op->imm = val;
  print_Dop(op->str, OP_STR_SIZE, (flag ? "0x%x" : "%d"), op->imm);
}

static inline def_DopHelper(r) {
  bool load_val = flag;
  static word_t zero_null = 0;
  op->preg = (!load_val && val == 0) ? &zero_null : &reg_l(val);
  print_Dop(op->str, OP_STR_SIZE, "%s", reg_name(val, 4));
}

static inline def_DHelper(I) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.i.simm11_0, false);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}

static inline def_DHelper(U) {
  decode_op_i(s, id_src1, s->isa.instr.u.imm31_12 << 12, true);
  decode_op_r(s, id_dest, s->isa.instr.u.rd, false);
}

static inline def_DHelper(S) {
  decode_op_r(s, id_src1, s->isa.instr.s.rs1, true);
  sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
  decode_op_i(s, id_src2, simm, false);
  decode_op_r(s, id_dest, s->isa.instr.s.rs2, true);
}
#ifndef __ICS_EXPORT

static inline def_DHelper(R) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs1, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rs2, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
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

static inline def_DHelper(auipc) {
  decode_U(s);
  id_src1->imm += s->pc;
}

static inline def_DHelper(csr) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.csr.csr, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}
#endif

def_THelper(load) {
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs1, 4));
  int mmu_mode = isa_mmu_state();
  if (mmu_mode == MMU_DIRECT) {
    switch (s->isa.instr.i.funct3) {
      TAB(0, lb)  TAB(1, lh)  TAB(2, lw)
      TAB(4, lbu) TAB(5, lhu)
    }
  } else if (mmu_mode == MMU_TRANSLATE) {
    switch (s->isa.instr.i.funct3) {
      TAB(0, lb_mmu)  TAB(1, lh_mmu)  TAB(2, lw_mmu)
      TAB(4, lbu_mmu) TAB(5, lhu_mmu)
    }
  } else {
    assert(0);
  }
  return EXEC_ID_inv;
}

def_THelper(store) {
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs1, 4));
  int mmu_mode = isa_mmu_state();
  if (mmu_mode == MMU_DIRECT) {
    switch (s->isa.instr.i.funct3) {
      TAB(0, sb)  TAB(1, sh)  TAB(2, sw)
    }
  } else if (mmu_mode == MMU_TRANSLATE) {
    switch (s->isa.instr.i.funct3) {
      TAB(0, sb_mmu)  TAB(1, sh_mmu)  TAB(2, sw_mmu)
    }
  } else {
    assert(0);
  }
  return EXEC_ID_inv;
}

def_THelper(c_addi_dispatch) {
  if (id_src2->imm == 1) return table_p_inc(s);
  if (id_src2->imm == -1u) return table_p_dec(s);
  return table_c_addi(s);
}

def_THelper(addi_dispatch) {
  if (s->isa.instr.i.rs1 == 0) {
    switch (id_src2->imm) {
      TAB(0, p_li_0) TAB(1, p_li_1)
      default: TAB(2, c_li);
    }
  } else {
    switch (id_src2->imm) {
      TAB(0, c_mv)
      default: TAB(2, addi);
    }
  }
}

def_THelper(op_imm_c) {
  if (s->isa.instr.r.funct7 == 32) {
    switch (s->isa.instr.r.funct3) { TAB(5, c_srai) }
  }
  switch (s->isa.instr.i.funct3) {
    TAB(0, c_addi_dispatch)  TAB(1, c_slli)  TAB(2, slti) TAB(3, sltui)
    TAB(4, xori)  TAB(5, c_srli)  TAB(6, ori)  TAB(7, c_andi)
  }
  return EXEC_ID_inv;
}

def_THelper(op_imm) {
  if (s->isa.instr.i.rd == s->isa.instr.i.rs1) return table_op_imm_c(s);
  if (s->isa.instr.r.funct7 == 32) {
    switch (s->isa.instr.r.funct3) { TAB(5, srai) }
  }
  switch (s->isa.instr.i.funct3) {
    TAB(0, addi_dispatch)  TAB(1, slli)  TAB(2, slti) TAB(3, sltui)
    TAB(4, xori)  TAB(5, srli)  TAB(6, ori)  TAB(7, andi)
  }
  return EXEC_ID_inv;
};

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
#define pair(x, y) (((x) << 3) | (y))
  int index = pair(s->isa.instr.r.funct7 & 1, s->isa.instr.r.funct3);
  if (s->isa.instr.r.rd == s->isa.instr.r.rs1) {
    switch (index) {
      TAB(pair(0, 0), c_add)
      TAB(pair(0, 4), c_xor)
      TAB(pair(0, 6), c_or)
      TAB(pair(0, 7), c_and)
    }
  }

  switch (index) {
    TAB(pair(0, 0), add)  TAB(pair(0, 1), sll)  TAB(pair(0, 2), slt)  TAB(pair(0, 3), sltu)
    TAB(pair(0, 4), xor)  TAB(pair(0, 5), srl)  TAB(pair(0, 6), or)   TAB(pair(0, 7), and)
    TAB(pair(1, 0), mul)  TAB(pair(1, 1), mulh) TAB(pair(1, 2),mulhsu)TAB(pair(1, 3), mulhu)
    TAB(pair(1, 4), div)  TAB(pair(1, 5), divu) TAB(pair(1, 6), rem)  TAB(pair(1, 7), remu)
  }
  return EXEC_ID_inv;
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

def_THelper(priv) {
  switch (s->isa.instr.csr.csr) {
    TAB(0, ecall)  TAB (0x102, sret)  TAB (0x120, sfence_vma)
  }
  return EXEC_ID_inv;
};

def_THelper(system) {
  switch (s->isa.instr.i.funct3) {
    TAB(0, priv)  TAB (1, csrrw)  TAB (2, csrrs)
  }
  return EXEC_ID_inv;
};

def_THelper(jal_dispatch) {
  switch (s->isa.instr.j.rd) {
    TAB(0, c_j) TAB(1, c_jal)
    default: TAB(2, jal)
  }
}

def_THelper(jalr_dispatch) {
  if (s->isa.instr.i.rd == 0 && id_src2->imm == 0) {
    if (s->isa.instr.i.rs1 == 1) return table_p_ret(s);
    else return table_c_jr(s);
  }
  return table_jalr(s);
}

def_THelper(main) {
  switch (s->isa.instr.i.opcode6_2) {
    IDTAB(000, I, load)
    IDTAB(004, I, op_imm) IDTAB(005, auipc, auipc)
    IDTAB(010, S, store)
    IDTAB(014, R, op)     IDTAB(015, U, lui)
    IDTAB(030, B, branch) IDTAB(031, I, jalr_dispatch)  TAB  (032, nemu_trap)  IDTAB(033, J, jal_dispatch)
    IDTAB(034, csr, system)
  }
  return table_inv(s);
};

int isa_fetch_decode(Decode *s) {
  s->isa.instr.val = instr_fetch(&s->snpc, 4);
  int idx = EXEC_ID_inv;
  if (s->isa.instr.i.opcode1_0 == 0x3) {
    idx = table_main(s);
  }

  s->type = INSTR_TYPE_N;
  switch (idx) {
    case EXEC_ID_c_j: case EXEC_ID_c_jal: case EXEC_ID_jal:
      s->jnpc = id_src1->imm; s->type = INSTR_TYPE_J; break;

    case EXEC_ID_beq: case EXEC_ID_bne: case EXEC_ID_blt: case EXEC_ID_bge:
    case EXEC_ID_bltu: case EXEC_ID_bgeu:
    case EXEC_ID_c_beqz: case EXEC_ID_c_bnez:
    case EXEC_ID_p_bltz: case EXEC_ID_p_bgez: case EXEC_ID_p_blez: case EXEC_ID_p_bgtz:
      s->jnpc = id_dest->imm; s->type = INSTR_TYPE_B; break;

    case EXEC_ID_p_ret: case EXEC_ID_c_jr: case EXEC_ID_jalr:
      s->type = INSTR_TYPE_I;
  }

  return idx;
}
