#undef def_EHelper
#define def_EHelper(name, body) name: body; goto exec_finish;
#define def_start()
#define call_DHelper(name) concat(decode_, name)(s)

#undef EMPTY
#define EMPTY &&inv

static const void *load_table[8] = {
  &&lb,      &&lh,      &&lw,     EMPTY,
  &&lbu,     &&lhu,     EMPTY,    EMPTY,
};

static const void *store_table[8] = {
  &&sb,      &&sh,      &&sw,     EMPTY,
  EMPTY,     EMPTY,     EMPTY,    EMPTY,
};

static const void *op_imm_table[8] = {
  &&addi,    &&slli,    &&slti,   &&sltui,
  &&xori,    &&srli,    &&ori,    &&andi,
};

static const void *op_table[16] = {
  &&add,     &&sll,     &&slt,    &&sltu,
  &&xor,     &&srl,     &&or,     &&and,
  &&mul,     &&mulh,    &&mulhsu, &&mulhu,
  &&div,     &&divu,    &&rem,    &&remu,
};

static const void *branch_table[8] = {
  &&beq,     &&bne,     EMPTY,    EMPTY,
  &&blt,     &&bge,     &&bltu,   &&bgeu,
};

static const void *system_table[8] = {
  &&priv,    &&csrrw,   &&csrrs,  EMPTY,
  EMPTY,     EMPTY,     EMPTY,    EMPTY,
};

static const void *opcode_main_table[32] = {
  &&load,    EMPTY,     EMPTY,    EMPTY,    &&op_imm,  &&auipc,   EMPTY,    EMPTY,
  &&store,   EMPTY,     EMPTY,    EMPTY,    &&op,      &&lui,     EMPTY,    EMPTY,
  EMPTY,     EMPTY,     EMPTY,    EMPTY,    EMPTY,     EMPTY,     EMPTY,    EMPTY,
  &&branch,  &&jalr,    &&nemu_trap, &&jal, &&system,  EMPTY,     EMPTY,    EMPTY,
};

def_start() {
  if (s->isa.instr.i.opcode1_0 != 0x3) goto inv;
  last_helper = opcode_main_table[s->isa.instr.i.opcode6_2];
  goto *last_helper;
}

def_EHelper(load, {
  call_DHelper(I);
  last_helper = load_table[s->isa.instr.i.funct3];
  goto *last_helper;
})

def_EHelper(store, {
  call_DHelper(S);
  last_helper = store_table[s->isa.instr.s.funct3];
  goto *last_helper;
})

def_EHelper(op_imm, {
  call_DHelper(I);
  last_helper = op_imm_table[s->isa.instr.i.funct3];
  goto *last_helper;
})

#define pair(x, y) (((x) << 3) | (y))
def_EHelper(op, {
  call_DHelper(R);
  int idx = pair(s->isa.instr.r.funct7 & 1, s->isa.instr.r.funct3);
  last_helper = op_table[idx];
  goto *last_helper;
})
#undef pair

def_EHelper(branch, {
  call_DHelper(B);
  last_helper = branch_table[s->isa.instr.b.funct3];
  goto *last_helper;
})

def_EHelper(system, {
  call_DHelper(csr);
  last_helper = system_table[s->isa.instr.i.funct3];
  goto *last_helper;
})

#include "compute.h"
#include "control.h"
#include "ldst.h"
#include "muldiv.h"
#include "system.h"
#include "special.h"
