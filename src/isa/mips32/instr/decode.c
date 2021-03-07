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

static inline def_DHelper(IU) {
  decode_op_r(s, id_src1, s->isa.instr.iu.rs, true);
  decode_op_i(s, id_src2, s->isa.instr.iu.imm, true);
  decode_op_r(s, id_dest, s->isa.instr.iu.rt, false);
}

static inline def_DHelper(ld) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs, true);
  decode_op_i(s, id_src2, s->isa.instr.i.simm, false);
  decode_op_r(s, id_dest, s->isa.instr.i.rt, false);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs, 4));
}

static inline def_DHelper(st) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs, true);
  decode_op_i(s, id_src2, s->isa.instr.i.simm, false);
  decode_op_r(s, id_dest, s->isa.instr.i.rt, true);
  print_Dop(id_src1->str, OP_STR_SIZE, "%d(%s)", id_src2->imm, reg_name(s->isa.instr.i.rs, 4));
}

static inline def_DHelper(lui) {
  decode_op_i(s, id_src1, s->isa.instr.iu.imm << 16, true);
  decode_op_r(s, id_dest, s->isa.instr.iu.rt, false);
}

#ifndef __ICS_EXPORT
static inline def_DHelper(I) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs, true);
  decode_op_i(s, id_src2, s->isa.instr.i.simm, false);
  decode_op_r(s, id_dest, s->isa.instr.i.rt, false);
}

static inline def_DHelper(J) {
  vaddr_t target = (s->pc & 0xf0000000) | (s->isa.instr.j.target << 2);
  decode_op_i(s, id_dest, target, true);
}

static inline def_DHelper(R) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rt, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
}

static inline def_DHelper(B) {
  sword_t offset = (s->isa.instr.i.simm << 2);
  decode_op_i(s, id_dest, s->pc + offset + 4, true);
  decode_op_r(s, id_src1, s->isa.instr.i.rs, true);
  decode_op_r(s, id_src2, s->isa.instr.i.rt, true);
  //s->snpc += 4; // skip the delay slot
}

static inline def_DHelper(shift) {
  decode_op_i(s, id_src1, s->isa.instr.r.sa, false);
  decode_op_r(s, id_src2, s->isa.instr.r.rt, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
}

static inline def_DHelper(cmov) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rt, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, true);
}

static inline def_DHelper(jal) {
  decode_J(s);
  id_src2->imm = s->pc + 8;
}

static inline def_DHelper(jalr) {
  decode_op_r(s, id_src1, s->isa.instr.r.rs, true);
  decode_op_r(s, id_dest, s->isa.instr.r.rd, false);
  id_src2->imm = s->pc + 8;
}

static inline def_DHelper(cp0) {
//  decode_op_r(s, id_src1, s->isa.instr.r.rs, true);
  decode_op_r(s, id_src2, s->isa.instr.r.rt, true);
  decode_op_i(s, id_dest, s->isa.instr.r.rd, false);
  print_Dop(id_dest->str, OP_STR_SIZE, "%s", cp0_name(id_dest->imm));
}

def_THelper(jr_dispatch) {
  if (s->isa.instr.r.rs == 31) return table_ret(s);
  return table_jr(s);
}
#endif

def_THelper(special) {
  switch (s->isa.instr.r.func) {
#ifndef __ICS_EXPORT
    IDTAB(000, shift, slli)                       IDTAB(002, shift, srli)IDTAB(003, shift, srai)
    IDTAB(004, R, sll)                            IDTAB(006, R, srl)     IDTAB(007, R, sra)
    IDTAB(010, R, jr_dispatch)IDTAB(011, jalr, jalr) IDTAB(012, cmov, movz) IDTAB(013, cmov, movn)
    TAB  (014, syscall)
    IDTAB(020, R, mfhi)    IDTAB(021, R, mthi)    IDTAB(022, R, mflo)    IDTAB(023, R, mtlo)

    IDTAB(030, R, mult)    IDTAB(031, R, multu)   IDTAB(032, R, div)     IDTAB(033, R, divu)

                           IDTAB(041, R, add)                            IDTAB(043, R, sub)
    IDTAB(044, R, and)     IDTAB(045, R, or)      IDTAB(046, R, xor)     IDTAB(047, R, nor)
                                                  IDTAB(052, R, slt)     IDTAB(053, R, sltu)
#endif
  }
  return EXEC_ID_inv;
}

#ifndef __ICS_EXPORT
def_THelper(special2) {
  switch (s->isa.instr.r.func) {
    IDTAB(2, R, mul)
    IDTAB(040, R, clz)
  }
  return EXEC_ID_inv;
}

def_THelper(regimm) {
  switch (s->isa.instr.r.rt) {
    IDTAB(0, B, bltz)
    IDTAB(1, B, bgez)
  }
  return EXEC_ID_inv;
}

def_THelper(cop0) {
#define pair(x, y) (((x) << 1) | (y))
  bool cop0co = (s->isa.instr.r.rs & 0x10) != 0;
  uint32_t op = pair((cop0co ? s->isa.instr.r.func : s->isa.instr.r.rs), cop0co);
  switch (op) {
    TAB  (pair(002, 1), tlbwi)
    TAB  (pair(006, 1), tlbwr)
    TAB  (pair(010, 1), tlbp)
    TAB  (pair(030, 1), eret)
    IDTAB(pair(000, 0), cp0, mfc0)
    IDTAB(pair(004, 0), cp0, mtc0)
  }
#undef pair
  return EXEC_ID_inv;
}
#endif

def_THelper(main) {
  switch (s->isa.instr.r.opcode) {
#ifdef __ICS_EXPORT
    TAB  (000, special)
    IDTAB(017, IU, lui)
    IDTAB(043, ld, lw)
    IDTAB(053, st, sw)
    TAB  (074, nemu_trap)
#else
    TAB  (000, special)    TAB  (001, regimm)     IDTAB(002, J, j)       IDTAB(003, jal, jal)
    IDTAB(004, B, beq)     IDTAB(005, B, bne)     IDTAB(006, B, blez)    IDTAB(007, B, bgtz)
                           IDTAB(011, I, addi)    IDTAB(012, I, slti)    IDTAB(013, I, sltui)
    IDTAB(014, IU, andi)   IDTAB(015, IU, ori)    IDTAB(016, IU, xori)   IDTAB(017, lui, lui)
    TAB  (020, cop0)


    TAB  (034, special2)
    IDTAB(040, ld, lb)     IDTAB(041, ld, lh)     IDTAB(042, st, lwl)    IDTAB(043, ld, lw)
    IDTAB(044, ld, lbu)    IDTAB(045, ld, lhu)    IDTAB(046, st, lwr)
    IDTAB(050, st, sb)     IDTAB(051, st, sh)     IDTAB(052, st, swl)    IDTAB(053, st, sw)
                                                  IDTAB(056, st, swr)



    TAB  (074, nemu_trap)
#endif
  }
  return table_inv(s);
}

int isa_fetch_decode(Decode *s) {
  s->isa.instr.val = instr_fetch(&s->snpc, 4);
  int idx = table_main(s);

  s->type = INSTR_TYPE_N;
  switch (idx) {
    case EXEC_ID_j:
    case EXEC_ID_jal: s->jnpc = id_dest->imm; s->type = INSTR_TYPE_J; break;
    case EXEC_ID_beq:
    case EXEC_ID_bne:
    case EXEC_ID_blez:
    case EXEC_ID_bltz:
    case EXEC_ID_bgez:
    case EXEC_ID_bgtz: s->jnpc = id_dest->imm; s->type = INSTR_TYPE_B; break;
    case EXEC_ID_ret:
    case EXEC_ID_jr:
    case EXEC_ID_jalr: s->type = INSTR_TYPE_I;
  }

  return idx;
}
