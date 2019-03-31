#include "cpu/exec.h"
#include "all-instr.h"
#include <setjmp.h>

static OpcodeEntry special_table [64] = {
  /* b000 */ IDEX(shift, sll), EMPTY, IDEX(shift, srl), IDEX(shift, sra), IDEX(R, sll), EMPTY, IDEX(R, srl), IDEX(R, sra),
  /* b001 */ IDEX(R, jr), IDEX(R, jalr), IDEX(cmov, movz), IDEX(cmov, movn), EX(syscall), EMPTY, EMPTY, EMPTY,
  /* b010 */ IDEX(R, mfhi), IDEX(R, mthi), IDEX(R, mflo), IDEX(R, mtlo), EMPTY, EMPTY, EMPTY, EMPTY,
  /* b011 */ IDEX(R, mult), IDEX(R, multu), IDEX(R, div), IDEX(R, divu), EMPTY, EMPTY, EMPTY, EMPTY,
  /* b100 */ EMPTY, IDEX(R, add), EMPTY, IDEX(R, sub), IDEX(R, and), IDEX(R, or), IDEX(R, xor), IDEX(R, nor),
  /* b101 */ EMPTY, EMPTY, IDEX(R, slt), IDEX(R, sltu), EMPTY, EMPTY, EMPTY, EMPTY,
  /* b110 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b111 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
};

static make_EHelper(special) {
  idex(pc, &special_table[decinfo.isa.instr.func]);
}

static OpcodeEntry special2_table [64] = {
  /* b000 */ EMPTY, EMPTY, IDEX(R, mul), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b001 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b010 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b011 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b100 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b101 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b110 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b111 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
};

static make_EHelper(special2) {
  idex(pc, &special2_table[decinfo.isa.instr.func]);
}

static OpcodeEntry regimm_table [32] = {
  /* b00 */ IDEX(B, bltz), IDEX(B, bgez), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b01 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b10 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b11 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
};

static make_EHelper(regimm) {
  idex(pc, &regimm_table[decinfo.isa.instr.rt]);
}

static OpcodeEntry cop0_table [16] = {
  /* b00 */ IDEX(R, mfc0), EMPTY, EMPTY, EMPTY, IDEX(R, mtc0), EMPTY, EMPTY, EMPTY,
  /* b01 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
};

static OpcodeEntry cop0co_table [64] = {
  /* b000 */ EMPTY, EMPTY, EX(tlbwi), EMPTY, EMPTY, EMPTY, EX(tlbwr), EMPTY,
  /* b001 */ EX(tlbp), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b010 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b011 */ EX(eret), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b100 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b101 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b110 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b111 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
};

static make_EHelper(cop0) {
  if (decinfo.isa.instr.rs & 0x10) {
    idex(pc, &cop0co_table[decinfo.isa.instr.func]);
  }
  else {
    idex(pc, &cop0_table[decinfo.isa.instr.rs]);
  }
}

static OpcodeEntry opcode_table [64] = {
  /* b000 */ EX(special), EX(regimm), IDEX(J, j), IDEX(J, jal), IDEX(B, beq), IDEX(B, bne), IDEX(B, blez), IDEX(B, bgtz),
  /* b001 */ EMPTY, IDEX(I, add), IDEX(I, slt), IDEX(I, sltu), IDEX(IU, and), IDEX(IU, or), IDEX(IU, xor), IDEX(IU, lui),
  /* b010 */ EX(cop0), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b011 */ EMPTY, EMPTY, EMPTY, EMPTY, EX(special2), EMPTY, EMPTY, EMPTY,
  /* b100 */ IDEXW(ld, lds, 1), IDEXW(ld, lds, 2), IDEX(st, lwl), IDEXW(ld, ld, 4), IDEXW(ld, ld, 1), IDEXW(ld, ld, 2), IDEX(st, lwr), EMPTY,
  /* b101 */ IDEXW(st, st, 1), IDEXW(st, st, 2), IDEX(st, swl), IDEXW(st, st, 4), EMPTY, EMPTY, IDEX(st, swr), EMPTY,
  /* b110 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b111 */ EMPTY, EMPTY, EMPTY, EMPTY, EX(nemu_trap), EMPTY, EMPTY, EMPTY,
};

void isa_exec(vaddr_t *pc) {
  extern jmp_buf intr_buf;
  int setjmp_ret;
  if ((setjmp_ret = setjmp(intr_buf)) != 0) {
    // exception
    int exce_code = setjmp_ret - 1;
    void raise_intr(uint32_t, vaddr_t);
    raise_intr(exce_code, cpu.pc);
    return;
  }
  decinfo.isa.instr.val = instr_fetch(pc, 4);
  decinfo.width = opcode_table[decinfo.isa.instr.opcode].width;
  idex(pc, &opcode_table[decinfo.isa.instr.opcode]);
}
