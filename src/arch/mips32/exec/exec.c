#include "cpu/exec.h"
#include "all-instr.h"

static OpcodeEntry special_table [64] = {
  /* b000 */ IDEX(shift, sll), EMPTY, IDEX(shift, srl), IDEX(shift, sra), EMPTY, EMPTY, IDEX(R, srl), IDEX(R, sra),
  /* b001 */ IDEX(R, jr), IDEX(R, jalr), IDEX(cmov, movz), IDEX(cmov, movn), EMPTY, EMPTY, EMPTY, EMPTY,
  /* b010 */ IDEX(R, mfhi), EMPTY, IDEX(R, mflo), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b011 */ IDEX(R, mult), EMPTY, IDEX(R, div), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b100 */ EMPTY, IDEX(R, add), EMPTY, IDEX(R, sub), EMPTY, IDEX(R, or), IDEX(R, xor), EMPTY,
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

static OpcodeEntry opcode_table [64] = {
  /* b000 */ EX(special), EMPTY, IDEX(J, j), IDEX(J, jal), IDEX(B, beq), IDEX(B, bne), IDEX(B, blez), EMPTY,
  /* b001 */ EMPTY, IDEX(I, add), IDEX(I, slt), IDEX(I, sltu), IDEX(IU, and), IDEX(IU, or), IDEX(IU, xor), IDEX(I, lui),
  /* b010 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b011 */ EMPTY, EMPTY, EMPTY, EMPTY, EX(special2), EMPTY, EMPTY, EMPTY,
  /* b100 */ EMPTY, EMPTY, EMPTY, IDEX(I, load), EMPTY, EMPTY, EMPTY, EMPTY,
  /* b101 */ EMPTY, EMPTY, EMPTY, IDEX(store, store), EMPTY, EMPTY, EMPTY, EMPTY,
  /* b110 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b111 */ EMPTY, EMPTY, EMPTY, EMPTY, EX(nemu_trap), EMPTY, EMPTY, EMPTY,
};

make_EHelper(arch) {
  decinfo.arch.instr.val = instr_fetch(eip, 4);
  idex(eip, &opcode_table[decinfo.arch.instr.opcode]);
}
