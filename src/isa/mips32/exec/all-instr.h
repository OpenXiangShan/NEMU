#undef def_EHelper
#define finish_label exec_finish
#define def_label(l) l:
#define def_EHelper(name) \
  goto finish_label; /* this is for the previous def_EHelper() */ \
  def_label(name)
#define def_start()
#define def_finish() def_label(finish_label)

#define try
#define catch { if (cpu.mem_exception != MEM_OK) goto exception; }


typedef struct {
  const void (*DHelper)(DecodeExecState *s);
  const void *EHelper;
} OpTableEntry;

#undef IDEX
#define IDEX(id, ex) { concat(decode_, id), &&ex }
#undef EX
#define EX(ex) IDEX(empty, ex)
#undef EMPTY
#define EMPTY IDEX(empty, inv)

static const OpTableEntry special_table[64] = {
  IDEX(shift, slli), EMPTY,         IDEX(shift, srli), IDEX(shift, srai),
  IDEX(R, sll),      EMPTY,         IDEX(R, srl),      IDEX(R, sra),
  IDEX(R, jr),       IDEX(R, jalr), IDEX(cmov, movz),  IDEX(cmov, movn),
  EX  (syscall),     EMPTY,         EMPTY,             EMPTY,
  IDEX(R, mfhi),     IDEX(R, mthi), IDEX(R, mflo),     IDEX(R, mtlo),
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  IDEX(R, mult),     IDEX(R, multu),IDEX(R, div),      IDEX(R, divu),
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             IDEX(R, add),  EMPTY,             IDEX(R, sub),
  IDEX(R, and),      IDEX(R, or),   IDEX(R, xor),      IDEX(R, nor),
  EMPTY,             EMPTY,         IDEX(R, slt),      IDEX(R, sltu),
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
};

static const OpTableEntry special2_table[64] = {
  EMPTY,             EMPTY,         IDEX(R, mul),      EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
};

static const OpTableEntry regimm_table[32] = {
  IDEX(B, bltz),     IDEX(B, bgez), EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
};

static const OpTableEntry cop0_rs_table[32] = {
  IDEX(R, mfc0),     EMPTY,         EMPTY,             EMPTY,
  IDEX(R, mtc0),     EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
};

static const OpTableEntry cop0_func_table[64] = {
  EMPTY,             EMPTY,         EX  (tlbwi),       EMPTY,
  EMPTY,             EMPTY,         EX  (tlbwr),       EMPTY,
  EX  (tlbp),        EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EX  (eret),        EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
  EMPTY,             EMPTY,         EMPTY,             EMPTY,
};

static const OpTableEntry opcode_main_table[64] = {
  EX  (special),   EX  (regimm),  IDEX(J, j),     IDEX(J, jal),
  IDEX(B, beq),    IDEX(B, bne),  IDEX(B, blez),  IDEX(B, bgtz),
  EMPTY,           IDEX(I, addi), IDEX(I, slti),  IDEX(I, sltui),
  IDEX(IU, andi),  IDEX(IU, ori), IDEX(IU, xori), IDEX(IU, lui),
  EX  (cop0),      EMPTY,         EMPTY,          EMPTY,
  EMPTY,           EMPTY,         EMPTY,          EMPTY,
  EMPTY,           EMPTY,         EMPTY,          EMPTY,
  EX  (special2),  EMPTY,         EMPTY,          EMPTY,
  IDEX(ld, lb),    IDEX(ld, lh),  IDEX(st, lwl),  IDEX(ld, lw),
  IDEX(ld, lbu),   IDEX(ld, lhu), IDEX(st, lwr),  EMPTY,
  IDEX(st, sb),    IDEX(st, sh),  IDEX(st, swl),  IDEX(st, sw),
  EMPTY,           EMPTY,         EMPTY,          IDEX(st, swr),
  EMPTY,           EMPTY,         EMPTY,          EMPTY,
  EMPTY,           EMPTY,         EMPTY,          EMPTY,
  EMPTY,           EMPTY,         EMPTY,          EMPTY,
  EX  (nemu_trap), EMPTY,         EMPTY,          EMPTY,
};

#define record_and_jmp(ehelper) { \
  const void *h = (ehelper); \
  /*dccache[idx].EHelper = h;*/ \
  goto_EHelper(h); \
}

def_start() {
  try { s->isa.instr.val = instr_fetch(&s->snpc, 4); } catch;
  s->npc = s->snpc;
  const OpTableEntry *e = &opcode_main_table[s->isa.instr.r.opcode];
  e->DHelper(s);
  record_and_jmp(e->EHelper);
}

def_EHelper(special) {
  const OpTableEntry *e = &special_table[s->isa.instr.r.func];
  e->DHelper(s);
  record_and_jmp(e->EHelper);
}

def_EHelper(special2) {
  const OpTableEntry *e = &special2_table[s->isa.instr.r.func];
  e->DHelper(s);
  record_and_jmp(e->EHelper);
}

def_EHelper(regimm) {
  const OpTableEntry *e = &regimm_table[s->isa.instr.r.rt];
  e->DHelper(s);
  record_and_jmp(e->EHelper);
}

def_EHelper(cop0) {
  const OpTableEntry *e = (s->isa.instr.r.rs & 0x10) ?
    &cop0_func_table[s->isa.instr.r.func] : &cop0_rs_table[s->isa.instr.r.rs];
  e->DHelper(s);
  record_and_jmp(e->EHelper);
}

#include "compute.h"
#include "control.h"
#include "ldst.h"
#include "muldiv.h"
#include "system.h"
#include "special.h"

def_EHelper(exception) {
  s->npc = raise_intr(cpu.mem_exception, pc);
  cpu.mem_exception = MEM_OK;
}
