#include <cpu/exec.h>
#include "rtl.h"

// decode operand helper
#define def_DopHelper(name) \
  void concat(decode_op_, name) (DecodeExecState *s, Operand *op, uint32_t val)

static inline def_DopHelper(i) {
  op->imm = val;
  print_Dop(op->str, OP_STR_SIZE, "%d", op->imm);
}

static inline def_DopHelper(r) {
  op->preg = &reg_l(val);
  print_Dop(op->str, OP_STR_SIZE, "%s", reg_name(op->reg, 4));
}

static inline def_DHelper(I) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1);
  decode_op_i(s, id_src2, s->isa.instr.i.simm11_0);
  decode_op_r(s, id_dest, s->isa.instr.i.rd);
}

static inline def_DHelper(U) {
  decode_op_i(s, id_src1, s->isa.instr.u.imm31_12 << 12);
  decode_op_r(s, id_dest, s->isa.instr.u.rd);

  print_Dop(id_src1->str, OP_STR_SIZE, "0x%x", s->isa.instr.u.imm31_12);
}

static inline def_DHelper(S) {
  decode_op_r(s, id_src1, s->isa.instr.s.rs1);
  sword_t simm = (s->isa.instr.s.simm11_5 << 5) | s->isa.instr.s.imm4_0;
  decode_op_i(s, id_src2, simm);
  decode_op_r(s, id_dest, s->isa.instr.s.rs2);
}
#ifndef __ICS_EXPORT

static inline def_DHelper(R) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs1);
  decode_op_r(s, id_src2, s->isa.instr.r.rs2);
  decode_op_r(s, id_dest, s->isa.instr.r.rd);
}

static inline def_DHelper(J) {
  sword_t offset = (s->isa.instr.j.simm20 << 20) | (s->isa.instr.j.imm19_12 << 12) |
    (s->isa.instr.j.imm11 << 11) | (s->isa.instr.j.imm10_1 << 1);
  id_src1->imm = cpu.pc + offset;
  print_Dop(id_src1->str, OP_STR_SIZE, "0x%x", s->npc);

  decode_op_r(s, id_dest, s->isa.instr.j.rd);
}

static inline def_DHelper(B) {
  sword_t offset = (s->isa.instr.b.simm12 << 12) | (s->isa.instr.b.imm11 << 11) |
    (s->isa.instr.b.imm10_5 << 5) | (s->isa.instr.b.imm4_1 << 1);
  id_dest->imm = cpu.pc + offset;

  decode_op_r(s, id_src1, s->isa.instr.b.rs1);
  decode_op_r(s, id_src2, s->isa.instr.b.rs2);
}

static inline def_DHelper(csr) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1);
  decode_op_i(s, id_src2, s->isa.instr.csr.csr);
  decode_op_r(s, id_dest, s->isa.instr.i.rd);
}
#endif
