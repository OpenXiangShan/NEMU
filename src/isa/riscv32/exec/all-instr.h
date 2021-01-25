#undef def_EHelper
#define finish_label exec_finish
#define def_label(l) l:
#define def_EHelper(name) \
  goto finish_label; /* this is for the previous def_EHelper() */ \
  def_label(name)
#define def_start()
#define def_finish() def_label(finish_label)

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

typedef struct {
  const void (*DHelper)(DecodeExecState *s);
  void *EHelper;
} MainEntry;

#undef IDEX
#define IDEX(id, ex) { concat(decode_, id), &&ex }
#define EMPTY2 IDEX(empty, inv)

static MainEntry opcode_main_table[32] = {
  IDEX(I, load),    EMPTY2,     EMPTY2,    EMPTY2,
  IDEX(I, op_imm),IDEX(U, auipc),EMPTY2, EMPTY2,
  IDEX(S, store), EMPTY2,     EMPTY2,    EMPTY2,
  IDEX(R, op),      IDEX(U, lui),     EMPTY2,    EMPTY2,
  EMPTY2,     EMPTY2,     EMPTY2,    EMPTY2,
  EMPTY2,     EMPTY2,     EMPTY2,    EMPTY2,
  IDEX(B, branch),  IDEX(I, jalr),    IDEX(empty, nemu_trap), IDEX(J, jal),
  IDEX(csr, system),  EMPTY2,     EMPTY2,    EMPTY2,
};

#define record_and_jmp(ehelper) { \
  const void *h = (ehelper); \
  s->EHelper = h; \
  goto_EHelper(h); \
}

def_start() {
  s->snpc = pc;
  s->isa.instr.val = instr_fetch(&s->snpc, 4);
  s->npc = s->snpc;
  if (s->isa.instr.i.opcode1_0 != 0x3) goto inv;
  MainEntry *e = &opcode_main_table[s->isa.instr.i.opcode6_2];
  e->DHelper(s);
  record_and_jmp(e->EHelper);
}

def_EHelper(load) {
  record_and_jmp(load_table[s->isa.instr.i.funct3]);
}

def_EHelper(store) {
  record_and_jmp(store_table[s->isa.instr.s.funct3]);
}

def_EHelper(op_imm) {
  record_and_jmp(op_imm_table[s->isa.instr.i.funct3]);
}

#define pair(x, y) (((x) << 3) | (y))
def_EHelper(op) {
  int index = pair(s->isa.instr.r.funct7 & 1, s->isa.instr.r.funct3);
  record_and_jmp(op_table[index]);
}
#undef pair

def_EHelper(branch) {
  record_and_jmp(branch_table[s->isa.instr.b.funct3]);
}

def_EHelper(system) {
  record_and_jmp(system_table[s->isa.instr.i.funct3]);
}

#include "compute.h"
#include "control.h"
#include "ldst.h"
#include "muldiv.h"
#include "system.h"
#include "special.h"
