#include "cpu/exec.h"
#include "all-instr.h"

static OpcodeEntry special_table [64] = {
  /* b000 */ IDEX(shift, sll), EMPTY, IDEX(shift, srl), IDEX(shift, sra), IDEX(R, sll), EMPTY, IDEX(R, srl), IDEX(R, sra),
  /* b001 */ IDEX(R, jr), IDEX(R, jalr), IDEX(cmov, movz), IDEX(cmov, movn), EMPTY, EMPTY, EMPTY, EMPTY,
  /* b010 */ IDEX(R, mfhi), EMPTY, IDEX(R, mflo), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b011 */ IDEX(R, mult), IDEX(R, multu), IDEX(R, div), IDEX(R, divu), EMPTY, EMPTY, EMPTY, EMPTY,
  /* b100 */ EMPTY, IDEX(R, add), EMPTY, IDEX(R, sub), IDEX(R, and), IDEX(R, or), IDEX(R, xor), IDEX(R, nor),
  /* b101 */ EMPTY, EMPTY, IDEX(R, slt), IDEX(R, sltu), EMPTY, EMPTY, EMPTY, EMPTY,
  /* b110 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b111 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
};

static make_EHelper(special) {
  idex(eip, &special_table[decinfo.arch.instr.func]);
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
  idex(eip, &special2_table[decinfo.arch.instr.func]);
}

static OpcodeEntry regimm_table [32] = {
  /* b00 */ IDEX(B, bltz), IDEX(B, bgez), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b01 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b10 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b11 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
};

static make_EHelper(regimm) {
  idex(eip, &regimm_table[decinfo.arch.instr.rt]);
}

static OpcodeEntry opcode_table [64] = {
  /* b000 */ EX(special), EX(regimm), IDEX(J, j), IDEX(J, jal), IDEX(B, beq), IDEX(B, bne), IDEX(B, blez), IDEX(B, bgtz),
  /* b001 */ EMPTY, IDEX(I, add), IDEX(I, slt), IDEX(I, sltu), IDEX(IU, and), IDEX(IU, or), IDEX(IU, xor), IDEX(I, lui),
  /* b010 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b011 */ EMPTY, EMPTY, EMPTY, EMPTY, EX(special2), EMPTY, EMPTY, EMPTY,
  /* b100 */ IDEXW(I, loads, 1), IDEXW(I, loads, 2), IDEX(store, lwl), IDEXW(I, load, 4), IDEXW(I, load, 1), IDEXW(I, load, 2), IDEX(store, lwr), EMPTY,
  /* b101 */ IDEXW(store, store, 1), IDEXW(store, store, 2), IDEX(store, swl), IDEXW(store, store, 4), EMPTY, EMPTY, IDEX(store, swr), EMPTY,
  /* b110 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b111 */ EMPTY, EMPTY, EMPTY, EMPTY, EX(nemu_trap), EMPTY, EMPTY, EMPTY,
};

make_EHelper(arch) {
  decinfo.arch.instr.val = instr_fetch(eip, 4);
  decinfo.width = opcode_table[decinfo.arch.instr.opcode].width;
  idex(eip, &opcode_table[decinfo.arch.instr.opcode]);
}
