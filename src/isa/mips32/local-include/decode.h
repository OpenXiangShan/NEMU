#include <cpu/exec.h>
#include "rtl.h"

// decode operand helper
#define def_DopHelper(name) \
  void concat(decode_op_, name) (DecodeExecState *s, Operand *op, uint32_t val, bool load_val)

static inline def_DopHelper(i) {
  op->type = OP_TYPE_IMM;
  op->imm = val;

  print_Dop(op->str, OP_STR_SIZE, "%d", op->imm);
}

static inline def_DopHelper(r) {
  op->type = OP_TYPE_REG;
  op->reg = val;
  op->preg = &reg_l(val);

  print_Dop(op->str, OP_STR_SIZE, "%s", reg_name(op->reg, 4));
}

static inline def_DHelper(IU) {
  decode_op_r(s, id_src1, s->isa.instr.iu.rs, true);
  decode_op_i(s, id_src2, s->isa.instr.iu.imm, true);
  decode_op_r(s, id_dest, s->isa.instr.iu.rt, false);

  print_Dop(id_src2->str, OP_STR_SIZE, "0x%x", s->isa.instr.iu.imm);
}

static inline def_DHelper(ld) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs, true);
  decode_op_i(s, id_src2, s->isa.instr.i.simm, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rt, false);
}

static inline def_DHelper(st) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs, true);
  decode_op_i(s, id_src2, s->isa.instr.i.simm, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rt, true);
}

#ifndef __ICS_EXPORT
static inline def_DHelper(I) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs, true);
  decode_op_i(s, id_src2, s->isa.instr.i.simm, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rt, false);
}

static inline def_DHelper(J) {
  vaddr_t target = (cpu.pc & 0xf0000000) | (s->isa.instr.j.target << 2);
  id_dest->imm = target;
  print_Dop(id_dest->str, OP_STR_SIZE, "0x%x", id_dest->imm);
}

static inline def_DHelper(R) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rt, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
}

static inline def_DHelper(B) {
  sword_t offset = (s->isa.instr.i.simm << 2);
  id_dest->imm = cpu.pc + offset + 4;
  s->snpc += 4; // skip the delay slot

  decode_op_r(s, id_src1, s->isa.instr.i.rs, true);
  decode_op_r(s, id_src2, s->isa.instr.i.rt, true);

  print_Dop(id_dest->str, OP_STR_SIZE, "0x%x", id_dest->imm);
}

static inline def_DHelper(shift) {
  decode_op_i(s, id_src1, s->isa.instr.r.sa, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rt, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
}

static inline def_DHelper(cmov) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rt, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, true);
}
#endif
