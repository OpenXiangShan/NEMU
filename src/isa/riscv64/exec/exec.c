#include "cpu/exec.h"
#include "all-instr.h"
#include <setjmp.h>
#include "../intr.h"

static make_EHelper(load) {
  static OpcodeEntry table [8] = {
    EXW(lds, 1), EXW(lds, 2), EXW(lds, 4), EXW(ld, 8), EXW(ld, 1), EXW(ld, 2), EXW(ld, 4), EMPTY
  };
  decinfo.width = table[decinfo.isa.instr.funct3].width;
  idex(pc, &table[decinfo.isa.instr.funct3]);
}

static make_EHelper(store) {
  static OpcodeEntry table [8] = {
    EXW(st, 1), EXW(st, 2), EXW(st, 4), EXW(st, 8), EMPTY, EMPTY, EMPTY, EMPTY
  };
  decinfo.width = table[decinfo.isa.instr.funct3].width;
  idex(pc, &table[decinfo.isa.instr.funct3]);
}

static make_EHelper(op_imm) {
  static OpcodeEntry table [8] = {
    EX(add), EX(sll), EX(slt), EX(sltu), EX(xor), EX(srl), EX(or), EX(and)
  };
  idex(pc, &table[decinfo.isa.instr.funct3]);
}

static make_EHelper(op_imm32) {
  static OpcodeEntry table [8] = {
    EX(addw), EX(sllw), EMPTY, EMPTY, EMPTY, EX(srlw), EMPTY, EMPTY
  };
  idex(pc, &table[decinfo.isa.instr.funct3]);
}

static make_EHelper(op) {
  static OpcodeEntry table [3][8] = {
    {EX(add), EX(sll), EX(slt), EX(sltu), EX(xor), EX(srl), EX(or), EX(and)},
    {EX(mul), EX(mulh), EX(mulhsu), EX(mulhu), EX(div), EX(divu), EX(rem), EX(remu)},
    {EX(sub), EMPTY, EMPTY, EMPTY, EMPTY, EX(sra), EMPTY, EMPTY},
  };

  uint32_t idx = decinfo.isa.instr.funct7;
  if (idx == 32) idx = 2;
  assert(idx <= 2);
  idex(pc, &table[idx][decinfo.isa.instr.funct3]);
}

static make_EHelper(op32) {
  static OpcodeEntry table [3][8] = {
    {EX(addw), EX(sllw), EMPTY, EMPTY, EMPTY, EX(srlw), EMPTY, EMPTY},
    {EX(mulw), EMPTY, EMPTY, EMPTY, EX(divw), EX(divuw), EX(remw), EX(remuw)},
    {EX(subw), EMPTY, EMPTY, EMPTY, EMPTY, EX(sraw), EMPTY, EMPTY},
  };

  uint32_t idx = decinfo.isa.instr.funct7;
  if (idx == 32) idx = 2;
  assert(idx <= 2);
  idex(pc, &table[idx][decinfo.isa.instr.funct3]);
}

static make_EHelper(system) {
  static OpcodeEntry table [8] = {
    EX(priv), IDEX(csr, csrrw), IDEX(csr, csrrs), IDEX(csr, csrrc), EMPTY, IDEX(csri, csrrw), IDEX(csri, csrrs), IDEX(csri, csrrc)
  };
  idex(pc, &table[decinfo.isa.instr.funct3]);
}

static make_EHelper(atomic) {
  cpu.amo = true;
  static OpcodeEntry table_lo [4] = {
    EMPTY, EX(amoswap), EX(lr), EX(sc)
  };
  static OpcodeEntry table_hi [8] = {
    EX(amoadd), EX(amoxor), EX(amoor), EX(amoand), EX(amomin), EX(amomax), EX(amominu), EX(amomaxu)
  };

  decinfo.width = 1 << decinfo.isa.instr.funct3;

  uint32_t funct5 = decinfo.isa.instr.funct7 >> 2;
  uint32_t idx_lo = funct5 & 0x3;
  uint32_t idx_hi = funct5 >> 2;
  if (funct5 == 2) cpu.amo = false; // lr is not a store
  if (idx_lo != 0) idex(pc, &table_lo[idx_lo]);
  else idex(pc, &table_hi[idx_hi]);
  cpu.amo = false;
}

static make_EHelper(fp_load) {
  static OpcodeEntry table [8] = {
    EMPTY,EMPTY,EXW(fp_ld,4),EXW(fp_ld,8),EMPTY,EMPTY,EMPTY,EMPTY
  };
  decinfo.width = table[decinfo.isa.instr.funct3].width;
  idex(pc, &table[decinfo.isa.instr.funct3]);
}

static make_EHelper(fp_store) {
  static OpcodeEntry table [8] = {
    EMPTY, EMPTY, EXW(fp_st, 4), EXW(fp_st, 8), EMPTY, EMPTY, EMPTY, EMPTY
  };
  decinfo.width = table[decinfo.isa.instr.funct3].width;
  idex(pc, &table[decinfo.isa.instr.funct3]);
}

static make_EHelper(op_fp) {
  static OpcodeEntry table [32] = {
    /* b00 */ EX(fadd), EX(fsub), EX(fmul), EX(fdiv), EX(fsgnj), EX(fmin_max), EMPTY, EMPTY,
    /* b01 */ EX(fcvt_F_to_F), EMPTY, EMPTY, EX(fsqrt), EMPTY, EMPTY, EMPTY, EMPTY,
    /* b10 */ EMPTY, EMPTY, EMPTY, EMPTY, IDEX(F_fpr_to_gpr, fcmp), EMPTY, EMPTY, EMPTY,
    /* b11 */ IDEX(F_fpr_to_gpr, fcvt_F_to_G), EMPTY, IDEX(F_gpr_to_fpr, fcvt_G_to_F), EMPTY, IDEX(F_fpr_to_gpr, fmv_F_to_G), EMPTY, IDEX(F_gpr_to_fpr, fmv_G_to_F), EMPTY
  };
  idex(pc, &table[decinfo.isa.instr.funct5]);
}

static OpcodeEntry opcode_table [32] = {
  /* b00 */ IDEX(ld, load), IDEX(F_ld, fp_load), EMPTY, EX(fence), IDEX(I, op_imm), IDEX(U, auipc), IDEX(I, op_imm32), EMPTY,
  /* b01 */ IDEX(st, store), IDEX(F_st, fp_store), EMPTY, IDEX(R, atomic), IDEX(R, op), IDEX(U, lui), IDEX(R, op32), EMPTY,
  /* b10 */ IDEX(F_R, fmadd), IDEX(F_R, fmsub), IDEX(F_R, fnmsub), IDEX(F_R, fnmadd), IDEX(F_R, op_fp), EMPTY, EMPTY, EMPTY,
  /* b11 */ IDEX(B, branch), IDEX(I, jalr), EX(nemu_trap), IDEX(J, jal), EX(system), EMPTY, EMPTY, EMPTY,
};

// RVC

static make_EHelper(rvc_fp){
  // TODO: implement RVC floating point instructions
  longjmp_raise_intr(EX_II);
}

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
  {IDEX(C_ADDI4SPN, add), EX(rvc_fp), IDEX(C_LW, lds), IDEX(C_LD, ld), EMPTY, EX(rvc_fp), IDEX(C_SW, st), IDEX(C_SD, st)},
  {IDEX(C_rs1_imm_rd, add), IDEX(C_rs1_imm_rd, addw), IDEX(C_0_imm_rd, add), EX(C_01_011), EX(C_01_100), IDEX(C_J, jal), IDEX(CB, beq), IDEX(CB, bne)},
  {IDEX(C_rs1_imm_rd, sll), EX(rvc_fp), IDEX(C_LWSP, lds), IDEX(C_LDSP, ld), EX(C_10_100), EX(rvc_fp), IDEX(C_SWSP, st), IDEX(C_SDSP, st)}
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
