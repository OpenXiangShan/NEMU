#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "all-instr.h"

#define decode_empty(s)

static inline void set_width(DecodeExecState *s, int width) {
  if (width != 0) s->width = width;
}

#define IDEXW(idx, id, ex, w) CASE_ENTRY(idx, concat(decode_, id), concat(exec_, ex), w)
#define IDEX(idx, id, ex)     IDEXW(idx, id, ex, 0)
#define EXW(idx, ex, w)       IDEXW(idx, empty, ex, w)
#define EX(idx, ex)           EXW(idx, ex, 0)
#define EMPTY(idx)            //EX(idx, inv)

#define CASE_ENTRY(idx, id, ex, w) case idx: set_width(s, w); id(s); ex(s); break;

static inline make_EHelper(load) {
  switch (s->isa.instr.i.funct3) {
    EXW  (0, lds, 1) EXW  (1, lds, 2) EXW  (2, ld, 4)
    EXW  (4, ld, 1)  EXW  (5, ld, 2)
    default: exec_inv(s);
  }
}

static inline make_EHelper(store) {
  switch (s->isa.instr.s.funct3) {
    EXW(0, st, 1) EXW(1, st, 2) EXW(2, st, 4)
  }
}

static inline make_EHelper(op_imm) {
  switch (s->isa.instr.i.funct3) {
    EX(0, addi)  EX(1, slli)  EX(2, slti) EX(3, sltui)
    EX(4, xori)  EX(5, srli)  EX(6, ori)  EX(7, andi)
    default: exec_inv(s);
  }
}

static inline make_EHelper(op) {
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

static inline make_EHelper(system) {
  switch (s->isa.instr.i.funct3) {
    EX(0, priv)  IDEX (1, csr, csrrw)  IDEX (2, csr, csrrs)
    default: exec_inv(s);
  }
}

static inline void exec(DecodeExecState *s) {
  s->isa.instr.val = instr_fetch(&s->seq_pc, 4);
  assert(s->isa.instr.r.opcode1_0 == 0x3);
  switch (s->isa.instr.r.opcode6_2) {
    IDEX (0b00000, I, load)
    IDEX (0b00100, I, op_imm)
    IDEX (0b00101, U, auipc)
    IDEX (0b01000, S, store)  IDEX (0b01100, R, op)
    IDEX (0b01101, U, lui)    IDEX (0b11000, B, branch)  IDEX (0b11001, I, jalr)  EX   (0b11010, nemu_trap)
    IDEX (0b11011, J, jal)    EX   (0b11100, system)
    default: exec_inv(s);
  }
}

vaddr_t isa_exec_once() {
  DecodeExecState s;
  s.is_jmp = 0;
  s.seq_pc = cpu.pc;

  exec(&s);
  update_pc(&s);

#if !defined(DIFF_TEST) && !_SHARE
  void query_intr(DecodeExecState *s);
  query_intr(&s);
#endif

  // reset gpr[0]
  reg_l(0) = 0;

  return s.seq_pc;
}
