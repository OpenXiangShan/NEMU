#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "all-instr.h"

#undef CASE_ENTRY
#define CASE_ENTRY(idx, id, ex, w) case idx: id(s); ex(s); break;

static inline def_EHelper(load) {
  switch (s->isa.instr.i.funct3) {
#ifdef __ICS_EXPORT
    EX(2, lw)
#else
    EX(0, lb)  EX(1, lh)  EX(2, lw)
    EX(4, lbu) EX(5, lhu)
#endif
    default: exec_inv(s);
  }
}

static inline def_EHelper(store) {
  switch (s->isa.instr.s.funct3) {
#ifdef __ICS_EXPORT
    EX(2, sw)
#else
    EX(0, sb) EX(1, sh) EX(2, sw)
#endif
    default: exec_inv(s);
  }
}

#ifndef __ICS_EXPORT
static inline def_EHelper(op_imm) {
  switch (s->isa.instr.i.funct3) {
    EX(0, addi)  EX(1, slli)  EX(2, slti) EX(3, sltui)
    EX(4, xori)  EX(5, srli)  EX(6, ori)  EX(7, andi)
    default: exec_inv(s);
  }
}

static inline def_EHelper(op) {
  if (s->isa.instr.r.funct7 == 32) {
    switch (s->isa.instr.r.funct3) {
      EX(0, sub) EX(5, sra)
      default: exec_inv(s);
    }
  } else {
#define pair(x, y) (((x) << 3) | (y))
    switch (pair(s->isa.instr.r.funct7, s->isa.instr.r.funct3)) {
      EX(pair(0, 0), add)  EX(pair(0, 1), sll)  EX(pair(0, 2), slt)  EX(pair(0, 3), sltu)
      EX(pair(0, 4), xor)  EX(pair(0, 5), srl)  EX(pair(0, 6), or)   EX(pair(0, 7), and)
      EX(pair(1, 0), mul)  EX(pair(1, 1), mulh) EX(pair(1,2), mulhsu)EX(pair(1, 3), mulhu)
      EX(pair(1, 4), div)  EX(pair(1, 5), divu) EX(pair(1, 6), rem)  EX(pair(1, 7), remu)
      default: exec_inv(s);
    }
#undef pair
  }
}

static inline def_EHelper(system) {
  switch (s->isa.instr.i.funct3) {
    EX(0, priv)  IDEX (1, csr, csrrw)  IDEX (2, csr, csrrs)
    default: exec_inv(s);
  }
}
#endif

static inline void fetch_decode_exec(DecodeExecState *s) {
  s->isa.instr.val = instr_fetch(&s->seq_pc, 4);
  if (s->isa.instr.i.opcode1_0 != 0x3) {
    exec_inv(s);
    return;
  }
  switch (s->isa.instr.i.opcode6_2) {
#ifdef __ICS_EXPORT
    IDEX (0b00000, I, load)
    IDEX (0b01000, S, store)
    IDEX (0b01101, U, lui)
    EX   (0b11010, nemu_trap)
#else
    IDEX (0b00000, I, load)
    IDEX (0b00100, I, op_imm)
    IDEX (0b00101, U, auipc)
    IDEX (0b01000, S, store)  IDEX (0b01100, R, op)
    IDEX (0b01101, U, lui)    IDEX (0b11000, B, branch)  IDEX (0b11001, I, jalr)  EX   (0b11010, nemu_trap)
    IDEX (0b11011, J, jal)    EX   (0b11100, system)
#endif
    default: exec_inv(s);
  }
}

static inline void reset_zero() {
  reg_l(0) = 0;
}

vaddr_t isa_exec_once() {
  DecodeExecState s;
  s.is_jmp = 0;
  s.seq_pc = cpu.pc;

  fetch_decode_exec(&s);
  update_pc(&s);
#ifndef __ICS_EXPORT

#if !defined(DIFF_TEST) && !_SHARE
  void query_intr();
  query_intr();
#endif
#endif

  reset_zero();

  return s.seq_pc;
}
