#define def_THelper(name) \
  static inline int concat(table_, name) (DecodeExecState *s)

#undef EMPTY
#undef CASE_ENTRY
#define CASE_ENTRY(idx, id, tab) case idx: id(s); return tab(s);
#define IDTAB(idx, id, tab) CASE_ENTRY(idx, concat(decode_, id), concat(table_, tab))
#define TAB(idx, tab) IDTAB(idx, empty, tab)
#define EMPTY(idx) TAB(idx, inv)

def_THelper(load) {
  switch (s->isa.instr.i.funct3) {
    TAB(0, lb)  TAB(1, lh)  TAB(2, lw)
    TAB(4, lbu) TAB(5, lhu)
  }
  return EXEC_ID_inv;
}

def_THelper(store) {
  switch (s->isa.instr.i.funct3) {
    TAB(0, sb)  TAB(1, sh)  TAB(2, sw)
  }
  return EXEC_ID_inv;
}

def_THelper(op_imm) {
  switch (s->isa.instr.i.funct3) {
    TAB(0, addi)  TAB(1, slli)  TAB(2, slti) TAB(3, sltui)
    TAB(4, xori)  TAB(5, srli)  TAB(6, ori)  TAB(7, andi)
  }
  return EXEC_ID_inv;
};

def_THelper(op) {
#define pair(x, y) (((x) << 3) | (y))
  int index = pair(s->isa.instr.r.funct7 & 1, s->isa.instr.r.funct3);
  switch (index) {
    TAB(pair(0, 0), add)  TAB(pair(0, 1), sll)  TAB(pair(0, 2), slt)  TAB(pair(0, 3), sltu)
    TAB(pair(0, 4), xor)  TAB(pair(0, 5), srl)  TAB(pair(0, 6), or)   TAB(pair(0, 7), and)
    TAB(pair(1, 0), mul)  TAB(pair(1, 1), mulh) TAB(pair(1, 2),mulhsu)TAB(pair(1, 3), mulhu)
    TAB(pair(1, 4), div)  TAB(pair(1, 5), divu) TAB(pair(1, 6), rem)  TAB(pair(1, 7), remu)
  }
  return EXEC_ID_inv;
}

def_THelper(branch) {
  switch (s->isa.instr.i.funct3) {
    TAB(0, beq)   TAB(1, bne)
    TAB(4, blt)   TAB(5, bge)   TAB(6, bltu)   TAB(7, bgeu)
  }
  return EXEC_ID_inv;
};

def_THelper(system) {
  switch (s->isa.instr.i.funct3) {
    TAB(0, priv)  TAB (1, csrrw)  TAB (2, csrrs)
  }
  return EXEC_ID_inv;
};

def_THelper(main) {
  switch (s->isa.instr.i.opcode6_2) {
    IDTAB(000, I, load)
    IDTAB(004, I, op_imm) IDTAB(005, U, auipc)
    IDTAB(010, S, store)
    IDTAB(014, R, op)     IDTAB(015, U, lui)
    IDTAB(030, B, branch) IDTAB(031, I, jalr)  TAB  (032, nemu_trap)  IDTAB(033, J, jal)
    TAB  (034, system)
  }
  return EXEC_ID_inv;
};

const void* fetch_decode(DecodeExecState *s, const void **jmp_table) {
  cpu.pc = s->pc;
  s->snpc = s->pc;
  s->isa.instr.val = instr_fetch(&s->snpc, 4);
  int idx = EXEC_ID_inv;
  if (s->isa.instr.i.opcode1_0 == 0x3) {
    idx = table_main(s);
  }
  const void *e = jmp_table[idx];
  s->EHelper = e;
  cpu.pc = s->snpc;
  return e;
}
