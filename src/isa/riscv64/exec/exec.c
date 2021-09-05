#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "../local-include/intr.h"
#include "all-instr.h"

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

static inline make_EHelper(fp_load) {
  switch (s->isa.instr.i.funct3) {
    EXW(2, fp_ld, 4) EXW(3, fp_ld, 8)
    default: exec_inv(s);
  }
}

static inline make_EHelper(store) {
  switch (s->isa.instr.s.funct3) {
    EXW(0, st, 1) EXW(1, st, 2) EXW(2, st, 4) EXW(3, st, 8)
  }
}

static inline make_EHelper(fp_store) {
  switch (s->isa.instr.s.funct3) {
    EXW(2, fp_st, 4) EXW(3, fp_st, 8)
    default: exec_inv(s);
  }
}

static inline make_EHelper(op_fp){
  switch (s->isa.instr.fp.funct5) {
    EX(0, fadd) EX(1, fsub) EX(2, fmul) EX(3, fdiv)
    EX(4, fsgnj) EX(5, fmin_fmax)
    EX(8, fcvt_F_to_F) EX(11, fsqrt) 
    IDEX(20, F_fpr_to_gpr, fcmp) 
    IDEX(24, F_fpr_to_gpr, fcvt_F_to_G) IDEX(26, F_gpr_to_fpr, fcvt_G_to_F) 
    IDEX(28, F_fpr_to_gpr, fmv_F_to_G) IDEX(30, F_gpr_to_fpr, fmv_G_to_F)
    default: exec_inv(s);
  }
}

static inline make_EHelper(op_imm) {
  switch (s->isa.instr.i.funct3) {
    EX(0, addi)     EX(2, slti)     EX(3, sltui)
    EX(4, xori)     EX(6, ori)      EX(7, andi)

    case 1:
      switch (s->isa.instr.i.simm11_0 >> 7) {        
        EX(0, slli)   EX(1, shfli)
        EX(4, sloi)   EX(5, sbseti)
                      EX(9, sbclri)
                      EX(13, sbinvi)
        case 12:
          switch(s->isa.instr.i.simm11_0 & 0x1f) {
            EX(0, clz)      EX(1, ctz)      EX(2, pcnt)     EX(3, bmatflip)
            EX(4, sext_b)   EX(5, sext_h)
            EX(16, crc32_b) EX(17, crc32_h) EX(18, crc32_w) EX(19, crc32_d)
            EX(24,crc32c_b) EX(25,crc32c_h) EX(26,crc32c_w) EX(27, crc32c_d)  
            default: exec_inv(s);   
          }
          break;
        default: exec_inv(s);
      }
      break;
    case 5:
      if((s->isa.instr.i.simm11_0 >> 6) & 0x1){
        switch(1) {
          EX(1, fsri)
          default: exec_inv(s);
        }
        break;
      }else{
        switch (s->isa.instr.i.simm11_0 >> 7) {
          EX(0, srli)   EX(1, unshfli)
          EX(4, sroi)   EX(5, gorci)
          EX(8, srai)   EX(9, sbexti)
          EX(12,rori)   EX(13, grevi)
          default: exec_inv(s);
        }
        break;
      }
  }
}

static inline make_EHelper(op_imm32) {
  switch (s->isa.instr.i.funct3) {
    EX(0, addiw)
    EX(4, addiwu)
    case 1:
      switch (s->isa.instr.i.simm11_0 >> 7) {
        EX(0, slliw)
        EX(1, slliuw)
        EX(4, sloiw)   
        EX(5, sbsetiw)
        EX(9, sbclriw)
        EX(13, sbinviw)
        case 12:
          switch (s->isa.instr.i.simm11_0 & 0x1f){
            EX(0, clzw)   EX(1, ctzw)   EX(2, pcntw)
            default: exec_inv(s);
          }
          break;
      }
      break;
    case 5:
      if((s->isa.instr.i.simm11_0 >> 6) & 0x1) {
        switch(1) {
          EX(1, fsri)
          default: exec_inv(s);
        }
        break;
      }else{
        switch (s->isa.instr.i.simm11_0 >> 7){
          EX(0, srliw)
          EX(4, sroiw)   EX(5, gorciw)
          EX(8, sraiw)   EX(9, sbextiw)
          EX(12,roriw)   EX(13,greviw)
          default: exec_inv(s);
        }
        break;
      }
    default: exec_inv(s);
  }
}

static inline make_EHelper(op) {
  uint32_t idx = s->isa.instr.r.funct7;
  // if (idx == 32) idx = 2;
  // assert(idx <= 2);
#define pair(x, y) (((x) << 3) | (y))
  if((idx & 0x3) == 2){
    switch(s->isa.instr.r.funct3){
      EX(1, fsl)  EX(5, fsr)
      default: exec_inv(s);
    }
  }else if((idx & 0x3) == 3){
    switch(s->isa.instr.r.funct3){
      EX(1, cmix) EX(5, cmov)
      default: exec_inv(s);
    }
  }else{
    switch (pair(idx, s->isa.instr.r.funct3)) {
      EX(pair(0, 0), add)  EX(pair(0, 1), sll)  EX(pair(0, 2), slt)  EX(pair(0, 3), sltu)
      EX(pair(0, 4), xor)  EX(pair(0, 5), srl)  EX(pair(0, 6), or)   EX(pair(0, 7), and)
      EX(pair(1, 0), mul)  EX(pair(1, 1), mulh) EX(pair(1,2), mulhsu)EX(pair(1, 3), mulhu)
      EX(pair(1, 4), div)  EX(pair(1, 5), divu) EX(pair(1, 6), rem)  EX(pair(1, 7), remu)
      EX(pair(32, 0), sub) EX(pair(32, 5), sra)

      // B-extension
                              EX(pair(4, 1), shfl)                                EX(pair(4, 3), bmator)   
      EX(pair(4, 4), pack)    EX(pair(4, 5), unshfl)    EX(pair(4, 6), bext)      EX(pair(4, 7), packh)
      EX(pair(5, 1), clmul)   EX(pair(5, 2), clmulr)    EX(pair(5, 3), clmulh)    EX(pair(5, 4), min)
      EX(pair(5, 6), max)     EX(pair(5, 5), minu)      EX(pair(5, 7), maxu)

      EX(pair(16, 1), slo)
      EX(pair(16, 2), sh1add)
      EX(pair(16, 4), sh2add)
      EX(pair(16, 5), sro)
      EX(pair(16, 6), sh3add)
      EX(pair(20, 1), sbset)
      EX(pair(20, 5), gorc)
      EX(pair(48, 1), rol)
      EX(pair(48, 5), ror)
      EX(pair(52, 1), sbinv)
      EX(pair(52, 5), grev)

      EX(pair(32, 4), xnor)
      EX(pair(32, 6), orn)
      EX(pair(32, 7), andn)

      EX(pair(36, 1), sbclr)
      EX(pair(36, 3), bmatxor)
      EX(pair(36, 4), packu)
      EX(pair(36, 5), sbext)
      EX(pair(36, 6), bdep)
      EX(pair(36, 7), bfp)

      default: exec_inv(s);
    }
  }
#undef pair
}


static inline make_EHelper(op32) {
  uint32_t idx = s->isa.instr.r.funct7;
  // if (idx == 32) idx = 2;
  // assert(idx <= 2);
#define pair(x, y) (((x) << 3) | (y))
  if((idx & 0x3) == 2){
    switch (s->isa.instr.r.funct3) {
      EX(1, fsl)  EX(5, fsr)
      default: 
        exec_inv(s);
    }
  }
  else{
    switch (pair(idx, s->isa.instr.r.funct3)) {
      EX(pair(0, 0), addw) EX(pair(0, 1), sllw)
                          EX(pair(0, 5), srlw)
      EX(pair(1, 0), mulw)
      EX(pair(1, 4), divw) EX(pair(1, 5), divuw) EX(pair(1, 6), remw)  EX(pair(1, 7), remuw)
      EX(pair(32, 0), subw) EX(pair(32, 5), sraw)

      // B-extension
      EX(pair(4, 0), adduw)
      EX(pair(4, 1), shflw)
      EX(pair(4, 4), packw) 
      EX(pair(4, 5), unshflw)
      EX(pair(4, 6), bextw)
      EX(pair(5, 0), addwu)
      EX(pair(5, 1), clmulw)
      EX(pair(5, 2), clmulrw)
      EX(pair(5, 3), clmulhw)

      EX(pair(16, 1), slow)
      EX(pair(16, 2), sh1adduw)
      EX(pair(16, 4), sh2adduw)
      EX(pair(16, 5), srow)
      EX(pair(16, 6), sh3adduw)
      EX(pair(20, 1), sbsetw)
      EX(pair(20, 5), gorcw)
      EX(pair(48, 1), rolw)
      EX(pair(48, 5), rorw)
      EX(pair(52, 1), sbinvw)
      EX(pair(52, 5), grevw)

      EX(pair(36, 0), subuw)
      EX(pair(36, 1), sbclrw)
      EX(pair(36, 4), packuw)
      EX(pair(36, 5), sbextw)
      EX(pair(36, 6), bdepw)
      EX(pair(36, 7), bfpw)
      EX(pair(37, 0), subwu)

      default: exec_inv(s);
    }
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
    EMPTY(4)     IDEX(5, csri, csrrwi)IDEX(6, csri, csrrsi)IDEX(7, csri, csrrci)
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
    EX(0x10, amomin)
    EX(0x14, amomax)
    EX(0x18, amominu)
    EX(0x1c, amomaxu)
  }
  cpu.amo = false;
}

static inline make_EHelper(fp) {
  raise_intr(s, EX_II, cpu.pc);
}

// RVC

static inline make_EHelper(misc) {
  uint32_t instr = s->isa.instr.val;
  uint32_t bits12not0 = (BITS(instr, 12, 12) != 0);
  uint32_t bits11_7not0 = (BITS(instr, 11, 7) != 0);
  uint32_t bits6_2not0 = (BITS(instr, 6, 2) != 0);
  uint32_t op = (bits12not0 << 2) | (bits11_7not0 << 1) | bits6_2not0;
  switch (op) {
    IDEX (0b010, C_JR, jalr)
    IDEX (0b011, C_MOV, add)
    IDEX (0b110, C_JALR, jalr)
    IDEX (0b111, C_ADD, add)
    default: exec_inv(s);
  }
}

static inline make_EHelper(lui_addi16sp) {
  uint32_t rd = BITS(s->isa.instr.val, 11, 7);
  assert(rd != 0);
  switch (rd) {
    IDEX (2, C_ADDI16SP, addi)
    default: // and other cases
    IDEX (1, CI_simm_lui, lui)
  }
}

static inline make_EHelper(misc_alu) {
  uint32_t instr = s->isa.instr.val;
  uint32_t op = BITS(instr, 11, 10);
  if (op == 3) {
    uint32_t op2 = (BITS(instr, 12, 12) << 2) | BITS(instr, 6, 5);
    switch (op2) {
      IDEX (0, CS, sub) IDEX (1, CS, xor) IDEX (2, CS, or)  IDEX (3, CS, and)
      IDEX (4, CS, subw)IDEX (5, CS, addw)EMPTY(6)          EMPTY(7)
    }
  } else {
    switch (op) {
      IDEX (0, CB_shift, srli)
      IDEX (1, CB_shift, srai)
      IDEX (2, CB_andi, andi)
    }
  }
}

static inline void exec(DecodeExecState *s) {
#ifdef XIANGSHAN_DEBUG
  uint64_t current_pc = s->seq_pc;
#endif
  if ((s->seq_pc & 0xfff) == 0xffe) {
    // instruction may accross page boundary
    uint32_t lo = instr_fetch(&s->seq_pc, 2);
    return_on_mem_ex();
    s->isa.instr.val = lo & 0xffff;
    if (s->isa.instr.r.opcode1_0 != 0x3) {
      // this is an RVC instruction
      goto rvc;
    }
    // this is a 4-byte instruction, should fetch the MSB part
    // NOTE: The fetch here may cause IPF.
    // If it is the case, we should have mepc = xxxffe and mtval = yyy000.
    // Refer to `mtval` in the privileged manual for more details.
    uint32_t hi = instr_fetch(&s->seq_pc, 2);
    s->isa.instr.val |= ((hi & 0xffff) << 16);
  } else {
    // in-page instructions, fetch 4 byte and
    // see whether it is an RVC instruction later
    s->isa.instr.val = instr_fetch(&s->seq_pc, 4);
  }

#ifdef XIANGSHAN_DEBUG
  printf("[NEMU] exec pc = 0x%lx, instr = 0x%0x\n", current_pc, s->isa.instr.val);
#endif
  return_on_mem_ex();

  if (s->isa.instr.r.opcode1_0 == 0x3) {
    switch (s->isa.instr.r.opcode6_2) {
      IDEX (000, I, load)   IDEX (001, F_I, fp_load)                      EX   (003, fence)
      IDEX (004, I, op_imm) IDEX (005, U, auipc)  IDEX (006, I, op_imm32)
      IDEX (010, S, store)  IDEX (011, F_S, fp_store)                     IDEX (013, R, atomic)
      IDEX (014, R, op)     IDEX (015, U, lui)    IDEX (016, R, op32)
      IDEX (020, F_R, fmadd)
      IDEX (021, F_R, fmsub)
      IDEX (022, F_R, fnmsub)
      IDEX (023, F_R, fnmadd)
      IDEX (024, F_R, op_fp)
      IDEX (030, B, branch) IDEX (031, I, jalr)   EX   (032, nemu_trap)     IDEX (033, J, jal)
      EX   (034, system)
      default: exec_inv(s);
    }
  } else {
    // RVC instructions are only 2-byte
    s->seq_pc -= 2;
rvc: ;
    //idex(pc, &rvc_table[decinfo.isa.instr.opcode1_0][decinfo.isa.instr.c_funct3]);
    uint32_t rvc_opcode = (s->isa.instr.r.opcode1_0 << 3) | BITS(s->isa.instr.val, 15, 13);
    switch (rvc_opcode) {
      IDEX (000, C_ADDI4SPN, addi) IDEXW(001, C_FLD, fp_ld, 8)   IDEXW(002, C_LW, lds, 4)   IDEXW(003, C_LD, ld, 8)
                                   IDEXW(005, C_FSD, fp_st, 8)   IDEXW(006, C_SW, st, 4)    IDEXW(007, C_SD, st, 8)
      IDEX (010, CI_simm, addi)    IDEX (011, CI_simm, addiw)    IDEX (012, C_LI, addi)     EX   (013, lui_addi16sp)
      EX   (014, misc_alu)         IDEX (015, C_J, jal)          IDEX (016, CB, beq)        IDEX (017, CB, bne)
      IDEX (020, CI_uimm, slli)    IDEXW(021, C_FLDSP, fp_ld, 8) IDEXW(022, C_LWSP, lds, 4) IDEXW(023, C_LDSP, ld, 8)
      EX   (024, misc)             IDEXW(025, C_FSDSP, fp_st, 8) IDEXW(026, C_SWSP, st, 4)  IDEXW(027, C_SDSP, st, 8)
      default: exec_inv(s);
    }
  }
}

vaddr_t isa_exec_once() {
  cpu.need_disambiguate = false;
  DecodeExecState s;
  s.is_jmp = 0;
  s.seq_pc = cpu.pc;

  exec(&s);
  if (cpu.mem_exception != MEM_OK) {
    raise_intr(&s, cpu.mem_exception, cpu.pc);
    cpu.mem_exception = MEM_OK;
  }
  update_pc(&s);

#if !defined(DIFF_TEST) && !_SHARE
  void query_intr(DecodeExecState *s);
  query_intr(&s);
#endif

  // reset gpr[0]
  reg_l(0) = 0;
  //printf("%x %d\n",s.seq_pc, cpu.gpr[10]._64);
  return s.seq_pc;
}

vaddr_t isa_disambiguate_exec(void *disambiguate_para) {
  DecodeExecState s;
  s.is_jmp = 0;
  s.seq_pc = cpu.pc;
  // need_disambiguate
  cpu.need_disambiguate = true;
  struct DisambiguationState* ds = (struct DisambiguationState*) disambiguate_para;
  memcpy(&cpu.disambiguation_state, ds, sizeof(struct DisambiguationState));
  // printf("isa_disambiguate_exec %ld at pc %lx\n", ds->exceptionNo, cpu.pc);

  exec(&s);
  if (cpu.mem_exception != MEM_OK) {
    raise_intr(&s, cpu.mem_exception, cpu.pc);
    cpu.mem_exception = MEM_OK;
  }
  update_pc(&s);

#if !defined(DIFF_TEST) && !_SHARE
  void query_intr(DecodeExecState *s);
  query_intr(&s);
#endif

  // reset gpr[0]
  reg_l(0) = 0;

  cpu.need_disambiguate = false;
  return s.seq_pc;
  // return isa_exec_once();  //TODO
}
