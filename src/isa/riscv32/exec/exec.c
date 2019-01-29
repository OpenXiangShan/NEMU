#include "cpu/exec.h"
#include "all-instr.h"

static OpcodeEntry load_table [8] = {
  EMPTY, EMPTY, EXW(ld, 4), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY
};

static make_EHelper(load) {
  decinfo.width = load_table[decinfo.isa.instr.funct3].width;
  idex(eip, &load_table[decinfo.isa.instr.funct3]);
}

static OpcodeEntry store_table [8] = {
  EMPTY, EMPTY, EXW(st, 4), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY
};

static make_EHelper(store) {
  decinfo.width = store_table[decinfo.isa.instr.funct3].width;
  idex(eip, &store_table[decinfo.isa.instr.funct3]);
}

static OpcodeEntry op_imm_table [8] = {
  EX(add), EX(sll), EMPTY, EX(sltu), EMPTY, EMPTY, EMPTY, EX(and)
};

static make_EHelper(op_imm) {
  idex(eip, &op_imm_table[decinfo.isa.instr.funct3]);
}

static OpcodeEntry op_table [8] = {
  EX(add), EMPTY, EX(slt), EX(sltu), EX(xor), EMPTY, EX(or), EMPTY
};

static OpcodeEntry op2_table [8] = {
  EX(sub), EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY
};

static make_EHelper(op) {
  switch (decinfo.isa.instr.funct7) {
    case 0:  idex(eip, &op_table[decinfo.isa.instr.funct3]); break;
    case 32: idex(eip, &op2_table[decinfo.isa.instr.funct3]); break;
    default: assert(0);
  }
}

static OpcodeEntry opcode_table [32] = {
  /* b00 */ IDEX(ld, load), EMPTY, EMPTY, EMPTY, IDEX(I, op_imm), IDEX(U, auipc), EMPTY, EMPTY,
  /* b01 */ IDEX(st, store), EMPTY, EMPTY, EMPTY, IDEX(R, op), IDEX(U, lui), EMPTY, EMPTY,
  /* b10 */ EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY,
  /* b11 */ IDEX(B, branch), IDEX(I, jalr), EX(nemu_trap), IDEX(J, jal), EMPTY, EMPTY, EMPTY, EMPTY,
};

make_EHelper(isa) {
  decinfo.isa.instr.val = instr_fetch(eip, 4);
  assert(decinfo.isa.instr.opcode1_0 == 0x3);
  idex(eip, &opcode_table[decinfo.isa.instr.opcode6_2]);
}
