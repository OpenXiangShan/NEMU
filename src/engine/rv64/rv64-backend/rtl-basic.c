#include <rtl/rtl.h>
#include "rv_ins_def.h"
#include "../tran.h"
#include "../spill.h"

void rv64_relop(uint32_t relop, uint32_t idx_dest, uint32_t idx_src1, uint32_t idx_src2);
uint32_t rtlreg2rvidx(DecodeExecState *s, const rtlreg_t* dest);
#ifdef REG_SPILLING
uint32_t rtlreg2varidx(DecodeExecState *s, const rtlreg_t* dest);
#endif

static inline void rv64_zextw(uint32_t rd, uint32_t rs) {
  // mask32 is set during initialization
  rv64_and(rd, rs, mask32);
}

static inline void rv64_sextw(uint32_t rd, uint32_t rs) {
  rv64_addw(rd, rs, x0);
}

// return false if `imm` can be represented within 12 bits
// else load it to `r`, and reture true
static inline bool load_imm_big(uint32_t r, const sword_t imm) {
  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;
  if (lui_imm == 0) return false;
  else {
    rv64_lui(r, lui_imm);
    if (rv_imm.imm_11_0 != 0) rv64_addiw(r, r, rv_imm.imm_11_0);
    return true;
  }
}

static inline void load_imm(uint32_t r, const sword_t imm) {
  if (imm == 0) {
    rv64_addi(r, x0, 0);
    return;
  }

  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;

  uint32_t hi = x0;
  if (lui_imm != 0) {
    rv64_lui(r, lui_imm);
    hi = r;
  }
  if (rv_imm.imm_11_0 != 0) rv64_addiw(r, hi, rv_imm.imm_11_0);
}

static inline void load_imm_no_opt(uint32_t r, const sword_t imm) {
  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;
  rv64_lui(r, lui_imm);
  rv64_addiw(r, r, rv_imm.imm_11_0);
}

/* RTL basic instructions */

#ifdef REG_SPILLING
#define make_rtl_compute_reg(rtl_name, rv64_name) \
  make_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    uint32_t dest_varidx = rtlreg2varidx(s, dest); \
    uint32_t src1_rvidx = rtlreg2rvidx(s, src1); \
    uint32_t src2_rvidx = rtlreg2rvidx(s, src2); \
    if (dest_varidx & SPMIDX_MASK) { \
      concat(rv64_, rv64_name) (tmp0, src1_rvidx, src2_rvidx); \
      rtl_kill(s, src1); \
      uint32_t dest_rvidx = rtlreg2rvidx(s, dest); \
      rv64_addi(dest_rvidx, tmp0, 0); \
      spill_set_dirty_rvidx(dest_rvidx); \
    } \
    else concat(rv64_, rv64_name) (dest_varidx, src1_rvidx, src2_rvidx); \
  }
#else
#define make_rtl_compute_reg(rtl_name, rv64_name) \
  make_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    uint32_t dest_rvidx = rtlreg2rvidx(s, dest); \
    uint32_t src1_rvidx = rtlreg2rvidx(s, src1); \
    uint32_t src2_rvidx = rtlreg2rvidx(s, src2); \
    concat(rv64_, rv64_name) (dest_rvidx, src1_rvidx, src2_rvidx); \
  }
#endif

#define make_rtl_compute_imm(rtl_name, rv64_name) \
  make_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) { \
    uint32_t dest_rvidx = rtlreg2rvidx(s, dest); \
    uint32_t src1_rvidx = rtlreg2rvidx(s, src1); \
    concat(rv64_, rv64_name) (dest_rvidx, src1_rvidx, imm); \
    spill_set_dirty_rvidx(dest_rvidx); \
  }

#define make_rtl_compute_imm_opt(rtl_name, rv64_name, rv64_imm_name) \
  make_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) { \
    uint32_t dest_rvidx = rtlreg2rvidx(s, dest); \
    uint32_t src1_rvidx = rtlreg2rvidx(s, src1); \
    if (src1 == rz) load_imm(dest_rvidx, imm); \
    else if (load_imm_big(tmp0, imm)) concat(rv64_, rv64_name) (dest_rvidx, src1_rvidx, tmp0); \
    else concat(rv64_, rv64_imm_name) (dest_rvidx, src1_rvidx, imm); \
    spill_set_dirty_rvidx(dest_rvidx); \
  }

make_rtl_compute_reg(and, and)
make_rtl_compute_reg(or, or)
make_rtl_compute_reg(xor, xor)

make_rtl_compute_imm_opt(addi, addw, addiw)
make_rtl_compute_imm_opt(andi, and, andi)
make_rtl_compute_imm_opt(xori, xor, xori)
make_rtl_compute_imm_opt(ori, or, ori)

make_rtl(subi, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) {
  rtl_addi(s, dest, src1, -imm);
}

#ifdef ISA64
make_rtl_compute_reg(add, add)
make_rtl_compute_reg(sub, sub)
make_rtl_compute_reg(shl, sll)
make_rtl_compute_reg(shr, srl)
make_rtl_compute_reg(sar, sra)
make_rtl_compute_imm(shli, slli)
make_rtl_compute_imm(shri, srli)
make_rtl_compute_imm(sari, srai)
#else
make_rtl_compute_reg(add, addw)
make_rtl_compute_reg(sub, subw)
make_rtl_compute_reg(shl, sllw)
make_rtl_compute_reg(shr, srlw)
make_rtl_compute_reg(sar, sraw)
make_rtl_compute_imm(shli, slliw)
make_rtl_compute_imm(shri, srliw)
make_rtl_compute_imm(sari, sraiw)
#endif

#ifdef REG_SPILLING
make_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  uint32_t dest_varidx = rtlreg2varidx(s, dest);
  uint32_t src1_rvidx = rtlreg2rvidx(s, src1);
  uint32_t src2_rvidx = rtlreg2rvidx(s, src2);
  if (dest_varidx & SPMIDX_MASK) {
    rv64_relop(relop, tmp0, src1_rvidx, src2_rvidx);
    rtl_kill(s, src1);
    uint32_t dest_rvidx = rtlreg2rvidx(s, dest);
    rv64_addi(dest_rvidx, tmp0, 0);
    spill_set_dirty_rvidx(dest_rvidx);
  }
  else rv64_relop(relop, dest_varidx, src1_rvidx, src2_rvidx);
}
#else
make_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  uint32_t dest_rvidx = rtlreg2rvidx(s, dest);
  uint32_t src1_rvidx = rtlreg2rvidx(s, src1);
  uint32_t src2_rvidx = rtlreg2rvidx(s, src2);
  rv64_relop(relop, dest_rvidx, src1_rvidx, src2_rvidx);
}
#endif

make_rtl(setrelopi, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const sword_t imm) {
  int big_imm = load_imm_big(tmp0, imm);
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  uint32_t idx_src1 = rtlreg2rvidx(s, src1);
  if (!big_imm && (relop == RELOP_LT || relop == RELOP_LTU)) {
    switch (relop) {
      case RELOP_LT: rv64_slti(idx_dest, idx_src1, imm); goto finish;
      case RELOP_LTU: rv64_sltiu(idx_dest, idx_src1, imm); goto finish;
      // fall through for default cases
    }
  }
  if (!big_imm) rv64_addiw(tmp0, x0, imm);
  rv64_relop(relop, idx_dest, idx_src1, tmp0);
finish:
  spill_set_dirty_rvidx(idx_dest);
}


//make_rtl_arith_logic(mul_hi)
//make_rtl_arith_logic(imul_hi)
make_rtl_compute_reg(mul_lo, mulw)
make_rtl_compute_reg(imul_lo, mulw)
make_rtl_compute_reg(div_q, divuw)
make_rtl_compute_reg(div_r, remuw)
make_rtl_compute_reg(idiv_q, divw)
make_rtl_compute_reg(idiv_r, remw)

#ifdef ISA64
# define rv64_mul_hi(c, a, b) TODO()
# define rv64_imul_hi(c, a, b) TODO()
#else

#ifdef REG_SPILLING
make_rtl(mul_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint32_t idx_src1 = rtlreg2rvidx(s, src1);
  uint32_t idx_src2 = rtlreg2rvidx(s, src2);
  rv64_zextw(idx_src1, idx_src1);
  rv64_zextw(idx_src2, idx_src2);
  rv64_mul(tmp0, idx_src1, idx_src2);
  rtl_kill(s, src1);
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  rv64_addi(idx_dest, tmp0, 0);
  rv64_srai(idx_dest, idx_dest, 32);
  spill_set_dirty_rvidx(idx_dest);
}

make_rtl(imul_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint32_t idx_src1 = rtlreg2rvidx(s, src1);
  uint32_t idx_src2 = rtlreg2rvidx(s, src2);
  rv64_sextw(idx_src1, idx_src1);
  rv64_sextw(idx_src2, idx_src2);
  rv64_mul(tmp0, idx_src1, idx_src2);
  rtl_kill(s, src1);
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  rv64_addi(idx_dest, tmp0, 0);
  rv64_srai(idx_dest, idx_dest, 32);
  spill_set_dirty_rvidx(idx_dest);
}
#else
make_rtl(mul_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  uint32_t idx_src1 = rtlreg2rvidx(s, src1);
  uint32_t idx_src2 = rtlreg2rvidx(s, src2);
  rv64_zextw(idx_src1, idx_src1);
  rv64_zextw(idx_src2, idx_src2);
  rv64_mul(idx_dest, idx_src1, idx_src2);
  rv64_srai(idx_dest, idx_dest, 32);
}

make_rtl(imul_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  uint32_t idx_src1 = rtlreg2rvidx(s, src1);
  uint32_t idx_src2 = rtlreg2rvidx(s, src2);
  rv64_sextw(idx_src1, idx_src1);
  rv64_sextw(idx_src2, idx_src2);
  rv64_mul(idx_dest, idx_src1, idx_src2);
  rv64_srai(idx_dest, idx_dest, 32);
}
#endif
#endif

make_rtl(div64_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
#ifdef REG_SPILLING
  panic("If reg-spilling enabled, you should not use div64 rtl!\n");
#endif
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  uint32_t idx_src1_hi = rtlreg2rvidx(s, src1_hi);
  uint32_t idx_src1_lo = rtlreg2rvidx(s, src1_lo);
  uint32_t idx_src2 = rtlreg2rvidx(s, src2);

  rv64_slli(tmp0, idx_src1_hi, 32);
  rv64_zextw(idx_src1_lo, idx_src1_lo);
  rv64_or(tmp0, tmp0, idx_src1_lo);
  rv64_zextw(idx_src2, idx_src2);
  rv64_divu(idx_dest, tmp0, idx_src2);
}

make_rtl(div64_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
#ifdef REG_SPILLING
  panic("If reg-spilling enabled, you should not use div64 rtl!\n");
#endif
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  uint32_t idx_src1_hi = rtlreg2rvidx(s, src1_hi);
  uint32_t idx_src1_lo = rtlreg2rvidx(s, src1_lo);
  uint32_t idx_src2 = rtlreg2rvidx(s, src2);

  rv64_slli(tmp0, idx_src1_hi, 32);
  rv64_zextw(idx_src1_lo, idx_src1_lo);
  rv64_or(tmp0, tmp0, idx_src1_lo);
  rv64_zextw(idx_src2, idx_src2);
  rv64_remu(idx_dest, tmp0, idx_src2);
}

make_rtl(idiv64_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
#ifdef REG_SPILLING
  panic("If reg-spilling enabled, you should not use div64 rtl!\n");
#endif
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  uint32_t idx_src1_hi = rtlreg2rvidx(s, src1_hi);
  uint32_t idx_src1_lo = rtlreg2rvidx(s, src1_lo);
  uint32_t idx_src2 = rtlreg2rvidx(s, src2);

  rv64_slli(tmp0, idx_src1_hi, 32);
  rv64_zextw(idx_src1_lo, idx_src1_lo);
  rv64_or(tmp0, tmp0, idx_src1_lo);
  rv64_sextw(idx_src2, idx_src2);
  rv64_div(idx_dest, tmp0, idx_src2);
}

make_rtl(idiv64_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
#ifdef REG_SPILLING
  panic("If reg-spilling enabled, you should not use div64 rtl!\n");
#endif
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  uint32_t idx_src1_hi = rtlreg2rvidx(s, src1_hi);
  uint32_t idx_src1_lo = rtlreg2rvidx(s, src1_lo);
  uint32_t idx_src2 = rtlreg2rvidx(s, src2);

  rv64_slli(tmp0, idx_src1_hi, 32);
  rv64_zextw(idx_src1_lo, idx_src1_lo);
  rv64_or(tmp0, tmp0, idx_src1_lo);
  rv64_sextw(idx_src2, idx_src2);
  rv64_rem(idx_dest, tmp0, idx_src2);
}

make_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, const sword_t imm, int len) {
  uint32_t idx_dest = rtlreg2rvidx(s, dest);
  uint32_t idx_addr = rtlreg2rvidx(s, addr);

  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;
  if (addr == rz) rv64_lui(tmp0, lui_imm);
  else if (lui_imm == 0) rv64_zextw(tmp0, idx_addr);
  else {
    rv64_lui(tmp0, lui_imm);
    rv64_add(tmp0, tmp0, idx_addr);
    rv64_zextw(tmp0, tmp0);
  }

  switch (len) {
    case 1: rv64_lbu(idx_dest, tmp0, imm & 0xfff); break;
    case 2: rv64_lhu(idx_dest, tmp0, imm & 0xfff); break;
#ifdef ISA64
    case 4: rv64_lwu(idx_dest, tmp0, imm & 0xfff); break;
#else
    case 4: rv64_lw (idx_dest, tmp0, imm & 0xfff); break;
#endif
    case 8: rv64_ld (idx_dest, tmp0, imm & 0xfff); break;
    default: assert(0);
  }
  spill_set_dirty_rvidx(idx_dest);
}

make_rtl(sm, const rtlreg_t* addr, const sword_t imm, const rtlreg_t* src1, int len) {
  uint32_t idx_addr = rtlreg2rvidx(s, addr);
  uint32_t idx_src1 = rtlreg2rvidx(s, src1);

  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;
  if (addr == rz) rv64_lui(tmp0, lui_imm);
  else if (lui_imm == 0) rv64_zextw(tmp0, idx_addr);
  else {
    rv64_lui(tmp0, lui_imm);
    rv64_add(tmp0, tmp0, idx_addr);
    rv64_zextw(tmp0, tmp0);
  }
  switch (len) {
    case 1: rv64_sb(idx_src1, tmp0, imm & 0xfff); break;
    case 2: rv64_sh(idx_src1, tmp0, imm & 0xfff); break;
    case 4: rv64_sw(idx_src1, tmp0, imm & 0xfff); break;
    case 8: rv64_sd(idx_src1, tmp0, imm & 0xfff); break;
    default: assert(0);
  }
}

make_rtl(host_lm, rtlreg_t* dest, const void *addr, int len) {
#ifndef __ISA_x86__
  panic("should not reach here");
#endif

  uint32_t idx_dest = rtlreg2rvidx(s, dest);

  // we assume that `addr` is only from cpu.gpr in x86
  uintptr_t addr_align = (uintptr_t)addr & ~(sizeof(rtlreg_t) - 1);
  uint32_t idx_r = rtlreg2rvidx(s, (void *)addr_align);
  switch (len) {
    case 1: ;
      int is_high = (uintptr_t)addr & 1;
      if (is_high) {
        rv64_srli(idx_dest, idx_r, 8);
        rv64_andi(idx_dest, idx_dest, 0xff);
      } else {
        rv64_andi(idx_dest, idx_r, 0xff);
      }
      return;
    case 2:
      rv64_and(idx_dest, idx_r, mask16); // mask16 is set during initialization
      return;
    default: assert(0);
  }
}

make_rtl(host_sm, void *addr, const rtlreg_t *src1, int len) {
#ifndef __ISA_x86__
  panic("should not reach here");
#endif

  uint32_t idx_src1 = rtlreg2rvidx(s, src1);

  // we assume that `addr` is only from cpu.gpr in x86
  uintptr_t addr_align = (uintptr_t)addr & ~(sizeof(rtlreg_t) - 1);
  uint32_t idx_r = rtlreg2rvidx(s, (void *)addr_align);

  spm(sw, idx_r, SPM_X86_REG);
  if (len == 1) spm(sb, idx_src1, SPM_X86_REG + ((uintptr_t)addr & 1));
  else if (len == 2) spm(sh, idx_src1, SPM_X86_REG);
  else assert(0);
  spm(lwu, idx_r, SPM_X86_REG);
}

// we use tmp0 to store x86.pc of the next basic block
make_rtl(j, vaddr_t target) {
#ifdef REG_SPILLING
  spill_writeback_all();
#endif
  if (!load_imm_big(tmp0, target)) rv64_addiw(tmp0, tmp0, target & 0xfff);
  tran_next_pc = NEXT_PC_JMP;
}

make_rtl(jr, rtlreg_t *target) {
  uint32_t rvidx = rtlreg2rvidx(s, target);
  rv64_addi(tmp0, rvidx, 0);
#ifdef REG_SPILLING
  spill_writeback_all();
#endif
  tran_next_pc = NEXT_PC_JMP;
}

make_rtl(jrelop, uint32_t relop, const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target) {
  uint32_t rs1 = rtlreg2rvidx(s, src1);
  uint32_t rs2 = rtlreg2rvidx(s, src2);
#ifdef REG_SPILLING
  spill_writeback_all();
#endif
  uint32_t offset = 12; // branch two instructions
  extern int trans_buffer_index;
  int old_idx = trans_buffer_index;

  // generate the branch instruciton
  switch (relop) {
    case RELOP_FALSE:
    case RELOP_TRUE: assert(0);
    case RELOP_EQ:  rv64_beq(rs1, rs2, offset); break;
    case RELOP_NE:  rv64_bne(rs1, rs2, offset); break;
    case RELOP_LT:  rv64_blt(rs1, rs2, offset); break;
    case RELOP_GE:  rv64_bge(rs1, rs2, offset); break;
    case RELOP_LTU: rv64_bltu(rs1, rs2, offset); break;
    case RELOP_GEU: rv64_bgeu(rs1, rs2, offset); break;

    case RELOP_LE:  rv64_bge(rs2, rs1, offset); break;
    case RELOP_GT:  rv64_blt(rs2, rs1, offset); break;
    case RELOP_LEU: rv64_bgeu(rs2, rs1, offset); break;
    case RELOP_GTU: rv64_bltu(rs2, rs1, offset); break;
    default: panic("unsupport relop = %d", relop);
  }

  // generate instrutions to load the not-taken target
  load_imm_no_opt(tmp0, s->seq_pc);  // only two instructions
  // generate instrutions to load the taken target
  load_imm_no_opt(tmp0, target);     // only two instructions

  tran_next_pc = NEXT_PC_BRANCH;

  int new_idx = trans_buffer_index;
  Assert(new_idx - old_idx == 5, "if this condition is broken, "
      "you should also modify rv64_exec_trans_buffer() in exec.c");
}

make_rtl(kill, const rtlreg_t* src1) {
#ifdef REG_SPILLING
  uint32_t varidx = rtlreg2varidx(s, src1);
  spill_flush(varidx);
#endif
}
