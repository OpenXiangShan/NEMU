#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "../local-include/intr.h"
#include "all-instr.h"
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

static inline make_EHelper(load) {
  switch (s->isa.instr.i.funct3) {
    EXW(0, lds, 1) EXW(1, lds, 2) EXW(2, lds, 4) EXW(3, ld, 8)
    EXW(4, ld, 1)  EXW(5, ld, 2)  EXW(6, ld, 4)
    default: exec_inv(s);
  }
}

static inline make_EHelper(store) {
  switch (s->isa.instr.s.funct3) {
    EXW(0, st, 1) EXW(1, st, 2) EXW(2, st, 4) EXW(3, st, 8)
  }
}

static inline make_EHelper(op_imm) {
  switch (s->isa.instr.i.funct3) {
    EX(0, add)  EX(1, sll)  EX(2, slt) EX(3, sltu)
    EX(4, xor)  EX(5, srl)  EX(6, or)  EX(7, and)
  }
}

static make_EHelper(op_imm32) {
  switch (s->isa.instr.i.funct3) {
    EX(0, addw) EX(1, sllw) EX(5, srlw)
    default: exec_inv(s);
  }
}

static inline make_EHelper(op) {
  uint32_t idx = s->isa.instr.r.funct7;
  if (idx == 32) idx = 2;
  assert(idx <= 2);
#define pair(x, y) (((x) << 3) | (y))
  switch (pair(idx, s->isa.instr.r.funct3)) {
    EX(pair(0, 0), add)  EX(pair(0, 1), sll)  EX(pair(0, 2), slt)  EX(pair(0, 3), sltu)
    EX(pair(0, 4), xor)  EX(pair(0, 5), srl)  EX(pair(0, 6), or)   EX(pair(0, 7), and)
    EX(pair(1, 0), mul)  EX(pair(1, 1), mulh) EX(pair(1,2), mulhsu)EX(pair(1, 3), mulhu)
    EX(pair(1, 4), div)  EX(pair(1, 5), divu) EX(pair(1, 6), rem)  EX(pair(1, 7), remu)
    EX(pair(2, 0), sub)  EX(pair(2, 5), sra)
    default: exec_inv(s);
  }
#undef pair
}


static make_EHelper(op32) {
  uint32_t idx = s->isa.instr.r.funct7;
  if (idx == 32) idx = 2;
  assert(idx <= 2);
#define pair(x, y) (((x) << 3) | (y))
  switch (pair(idx, s->isa.instr.r.funct3)) {
    EX(pair(0, 0), addw) EX(pair(0, 1), sllw)
                         EX(pair(0, 5), srlw)
    EX(pair(1, 0), mulw)
    EX(pair(1, 4), divw) EX(pair(1, 5), divuw) EX(pair(1, 6), remw)  EX(pair(1, 7), remuw)
    EX(pair(2, 0), subw) EX(pair(2, 5), sraw)
    default: exec_inv(s);
  }
#undef pair
}

static inline make_EHelper(branch) {
  switch (s->isa.instr.i.funct3) {
    EX(0, beq)  EX(1, bne)  EMPTY(2)   EMPTY(3)
    EX(4, blt)  EX(5, bge)  EX(6, bltu)EX(7, bgeu)
  }
}

static inline make_EHelper(system) {
  switch (s->isa.instr.i.funct3) {
    EX(0, priv)  IDEX(1, csr, csrrw)  IDEX(2, csr, csrrs)  IDEX(3, csr, csrrc)
    EMPTY(4)     IDEX(5, csri, csrrw) IDEX(6, csri, csrrs) IDEX(7, csri, csrrc)
  }
}

static inline make_EHelper(atomic) {
  cpu.amo = true;
  uint32_t funct5 = s->isa.instr.r.funct7 >> 2;
  if (funct5 == 2) cpu.amo = false; // lr is not a store
  set_width(s, 1 << s->isa.instr.r.funct3);
  switch (funct5) {
    EX(0x00, amoadd) EX(0x01, amoswap) EX(0x02, lr) EX(0x03, sc)
    EX(0x04, amoxor)
    EX(0x0c, amoand)
    EX(0x08, amoor)
    EX(0x1c, amomaxu)
  }
  cpu.amo = false;
}

static make_EHelper(fp) {
  longjmp_raise_intr(EX_II);
}

static inline void exec(DecodeExecState *s) {
  s->isa.instr.val = instr_fetch(&s->seq_pc, 4);
  assert(s->isa.instr.r.opcode1_0 == 0x3);
  switch (s->isa.instr.r.opcode6_2) {
    IDEX (0b00000, ld, load)  EX   (0b00001, fp)                                  EX   (0b00011, fence)
    IDEX (0b00100, I, op_imm) IDEX (0b00101, U, auipc)  IDEX (0b00110, I, op_imm32)
    IDEX (0b01000, st, store) EX   (0b01001, fp)                                  IDEX (0b01011, R, atomic)
    IDEX (0b01100, R, op)     IDEX (0b01101, U, lui)    IDEX (0b01110, R, op32)
    EX   (0b10000, fp)
    EX   (0b10100, fp)
    IDEX (0b11000, B, branch) IDEX (0b11001, I, jalr)   EX   (0b11010, nemu_trap) IDEX (0b11011, J, jal)
    EX   (0b11100, system)
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
  return s.seq_pc;
}

// RVC

#if 0
static make_EHelper(C_10_100) {
  static OpcodeEntry table [8] = {
    EMPTY, EMPTY, IDEX(C_rs1_rs2_0, jalr), IDEX(C_0_rs2_rd, add), EMPTY, EMPTY, IDEX(C_JALR, jalr), IDEX(C_rs1_rs2_rd, add),
  };
  uint32_t cond_c_simm12_not0 = (decinfo.isa.instr.c_simm12 != 0);
  uint32_t cond_c_rd_rs1_not0 = (decinfo.isa.instr.c_rd_rs1 != 0);
  uint32_t cond_c_rs2_not0 = (decinfo.isa.instr.c_rs2 != 0);
  uint32_t idx = (cond_c_simm12_not0 << 2) | (cond_c_rd_rs1_not0 << 1) | cond_c_rs2_not0;
  assert(idx < 8);
  idex(pc, &table[idx]);
}

static make_EHelper(C_01_011) {
  static OpcodeEntry table [2] = { IDEX(C_0_imm_rd, lui), IDEX(C_ADDI16SP, add)};
  assert(decinfo.isa.instr.c_rd_rs1 != 0);
  int idx = (decinfo.isa.instr.c_rd_rs1 == 2);
  idex(pc, &table[idx]);
}

static make_EHelper(C_01_100) {
  uint32_t func = decinfo.isa.instr.c_func6 & 0x3;
  if (func == 3) {
    decode_CR(pc);
    static OpcodeEntry table [8] = {
      EX(sub), EX(xor), EX(or), EX(and), EX(subw), EX(addw), EMPTY, EMPTY,
    };

    uint32_t idx2 = (decinfo.isa.instr.c_func6 >> 2) & 0x1;
    uint32_t idx1_0 = decinfo.isa.instr.c_func2;
    uint32_t idx = (idx2 << 2) | idx1_0;
    assert(idx < 8);
    idex(pc, &table[idx]);
  } else {
    decode_C_rs1__imm_rd_(pc);
    static OpcodeEntry table [3] = { EX(srl), EX(sra), EX(and) };
    idex(pc, &table[func]);
  }
}

static OpcodeEntry rvc_table [3][8] = {
  {IDEX(C_ADDI4SPN, add), EX(fp), IDEXW(C_LW, lds, 4), IDEXW(C_LD, ld, 8), EMPTY, EX(fp), IDEXW(C_SW, st, 4), IDEXW(C_SD, st, 8)},
  {IDEX(C_rs1_imm_rd, add), IDEX(C_rs1_imm_rd, addw), IDEX(C_0_imm_rd, add), EX(C_01_011), EX(C_01_100), IDEX(C_J, jal), IDEX(CB, beq), IDEX(CB, bne)},
  {IDEX(C_rs1_imm_rd, sll), EX(fp), IDEXW(C_LWSP, lds, 4), IDEXW(C_LDSP, ld, 8), EX(C_10_100), EX(fp), IDEXW(C_SWSP, st, 4), IDEXW(C_SDSP, st, 8)}
};

void isa_exec(vaddr_t *pc) {
  extern jmp_buf intr_buf;
  int setjmp_ret;
  if ((setjmp_ret = setjmp(intr_buf)) != 0) {
    int exception = setjmp_ret - 1;
    void raise_intr(word_t, vaddr_t);
    raise_intr(exception, cpu.pc);
    return;
  }

  cpu.fetching = true;
  if ((*pc & 0xfff) == 0xffe) {
    // instruction may accross page boundary
    uint32_t lo = instr_fetch(pc, 2);
    decinfo.isa.instr.val = lo & 0xffff;
    if (decinfo.isa.instr.opcode1_0 != 0x3) {
      // this is an RVC instruction
      cpu.fetching = false;
      goto rvc;
    }
    // this is a 4-byte instruction, should fetch the MSB part
    // NOTE: The fetch here may cause IPF.
    // If it is the case, we should have mepc = xxxffe and mtval = yyy000.
    // Refer to `mtval` in the privileged manual for more details.
    uint32_t hi = instr_fetch(pc, 2);
    decinfo.isa.instr.val |= ((hi & 0xffff) << 16);
  } else {
    // in-page instructions, fetch 4 byte and
    // see whether it is an RVC instruction later
    decinfo.isa.instr.val = instr_fetch(pc, 4);
  }
  cpu.fetching = false;
  if (decinfo.isa.instr.opcode1_0 == 0x3) {
    idex(pc, &opcode_table[decinfo.isa.instr.opcode6_2]);
  } else {
    // RVC instructions are only 2-byte
    *pc -= 2;
rvc:
    idex(pc, &rvc_table[decinfo.isa.instr.opcode1_0][decinfo.isa.instr.c_funct3]);
  }
}
#endif
