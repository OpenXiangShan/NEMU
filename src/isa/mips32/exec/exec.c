#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "all-instr.h"
#include "../local-include/intr.h"
#include <setjmp.h>

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

static inline make_EHelper(special) {
  switch (s->isa.instr.r.func) {
    IDEX (000, shift, slli)                       IDEX (002, shift, srli)IDEX (003, shift, srai)
    IDEX (004, R, sll)                            IDEX (006, R, srl)     IDEX (007, R, sra)
    IDEX (010, R, jr)      IDEX (011, R, jalr)    IDEX (012, cmov, movz) IDEX (013, cmov, movn)
    EX   (014, syscall)
    IDEX (020, R, mfhi)    IDEX (021, R, mthi)    IDEX (022, R, mflo)    IDEX (023, R, mtlo)

    IDEX (030, R, mult)    IDEX (031, R, multu)   IDEX (032, R, div)     IDEX (033, R, divu)

                           IDEX (041, R, add)                            IDEX (043, R, sub)
    IDEX (044, R, and)     IDEX (045, R, or)      IDEX (046, R, xor)     IDEX (047, R, nor)
                                                  IDEX (052, R, slt)     IDEX (053, R, sltu)
    default: exec_inv(s);
  }
}

static inline make_EHelper(special2) {
  switch (s->isa.instr.r.func) {
    IDEX (2, R, mul)
    default: exec_inv(s);
  }
}

static inline make_EHelper(regimm) {
  switch (s->isa.instr.r.rt) {
    IDEX (0, B, bltz)
    IDEX (1, B, bgez)
    default: exec_inv(s);
  }
}

static inline make_EHelper(cop0) {
#define pair(x, y) (((x) << 1) | (y))
  bool cop0co = (s->isa.instr.r.rs & 0x10) != 0;
  uint32_t op = pair((cop0co ? s->isa.instr.r.func : s->isa.instr.r.rs), cop0co);
  switch (op) {
    EX   (pair(002, 1), tlbwi)
    EX   (pair(006, 1), tlbwr)
    EX   (pair(010, 1), tlbp)
    EX   (pair(030, 1), eret)
    IDEX (pair(000, 0), R, mfc0)
    IDEX (pair(004, 0), R, mtc0)
    default: exec_inv(s);
  }
#undef pair
}

static inline void exec(DecodeExecState *s) {
  s->isa.instr.val = instr_fetch(&s->seq_pc, 4);
  return_on_mem_ex();
  switch (s->isa.instr.r.opcode) {
    EX   (000, special)    EX   (001, regimm)     IDEX (002, J, j)       IDEX (003, J, jal)
    IDEX (004, B, beq)     IDEX (005, B, bne)     IDEX (006, B, blez)    IDEX (007, B, bgtz)
                           IDEX (011, I, addi)    IDEX (012, I, slti)    IDEX (013, I, sltui)
    IDEX (014, IU, andi)   IDEX (015, IU, ori)    IDEX (016, IU, xori)   IDEX (017, IU, lui)
    EX   (020, cop0)


    EX   (034, special2)
    IDEXW(040, ld, lds, 1) IDEXW(041, ld, lds, 2) IDEX (042, st, lwl)    IDEXW(043, ld, ld, 4)
    IDEXW(044, ld, ld, 1)  IDEXW(045, ld, ld, 2)  IDEX (046, st, lwr)
    IDEXW(050, st, st, 1)  IDEXW(051, st, st, 2)  IDEX (052, st, swl)    IDEXW(053, st, st, 4)
                                                  IDEX (056, st, swr)



    EX   (074, nemu_trap)
    default: exec_inv(s);
  }
}

vaddr_t isa_exec_once() {
  DecodeExecState s;
  s.is_jmp = 0;
  s.seq_pc = cpu.pc;

  exec(&s);
  if (cpu.mem_exception != MEM_OK) {
    raise_intr(&s, cpu.mem_exception, cpu.pc);
    cpu.mem_exception = MEM_OK;
  }
  update_pc(&s);

#if !defined(DIFF_TEST) && !_SHARE
  void query_intr(DecodeExecState *s);
  query_intr(&s);
#endif
  return s.seq_pc;
}
