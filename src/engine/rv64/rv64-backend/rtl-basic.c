#include <rtl/rtl.h>
#include "rv_ins_def.h"
#include "../tran.h"
#include "../spill.h"

void rv64_relop(uint32_t relop, uint32_t idx_dest, uint32_t idx_src1, uint32_t idx_src2);
uint32_t dest2rvidx(DecodeExecState *s, const rtlreg_t* dest);
uint32_t src2rvidx(DecodeExecState *s, const rtlreg_t* src);

static inline void rv64_zextw(uint32_t rd, uint32_t rs) {
#ifndef ISA64
  // mask32 is set during initialization
  rv64_and(rd, rs, mask32);
#endif
}

static inline void rv64_sextw(uint32_t rd, uint32_t rs) {
#ifndef ISA64
  rv64_addw(rd, rs, x0);
#endif
}

// return false if `imm` can be represented within 12 bits
// else load it to `r`, and reture true
static inline bool load_imm_big(uint32_t r, const sword_t imm) {
  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;
  if (lui_imm == 0) return false;
  else {
    if (r == 0) return true;
    rv64_lui(r, lui_imm);
    if (rv_imm.imm_11_0 != 0) rv64_addi(r, r, rv_imm.imm_11_0);
    return true;
  }
}

static inline void load_imm(uint32_t r, const sword_t imm) {
  if (r == 0) return;
  if (!load_imm_big(r, imm)) rv64_addi(r, x0, imm & 0xfff);
}

static inline void load_imm_no_opt(uint32_t r, const sword_t imm) {
  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;
  rv64_lui(r, lui_imm);
  rv64_addi(r, r, rv_imm.imm_11_0);
}

/* RTL basic instructions */

#define make_rtl_compute_reg(rtl_name, rv64_name) \
  make_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    uint32_t ret = rtlreg2rvidx_pair(s, src1, true, src2, true); \
    uint32_t src1_rvidx = ret >> 16; \
    uint32_t src2_rvidx = ret & 0xffff; \
    uint32_t dest_rvidx = dest2rvidx(s, dest); \
    if (dest_rvidx == 0) return; \
    concat(rv64_, rv64_name) (dest_rvidx, src1_rvidx, src2_rvidx); \
  }

#define make_rtl_compute_imm(rtl_name, rv64_name) \
  make_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) { \
    uint32_t ret = rtlreg2rvidx_pair(s, dest, false, src1, true); \
    uint32_t dest_rvidx = ret >> 16; \
    uint32_t src1_rvidx = ret & 0xffff; \
    if (dest_rvidx == 0) return; \
    concat(rv64_, rv64_name) (dest_rvidx, src1_rvidx, imm); \
    spill_set_dirty_rvidx(dest_rvidx); \
  }

#define make_rtl_compute_imm_opt(rtl_name, rv64_name, rv64_imm_name) \
  make_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) { \
    uint32_t ret = rtlreg2rvidx_pair(s, dest, false, src1, true); \
    uint32_t dest_rvidx = ret >> 16; \
    uint32_t src1_rvidx = ret & 0xffff; \
    if (dest_rvidx == 0) return; \
    if (src1 == rz) load_imm(dest_rvidx, imm); \
    else if (load_imm_big(tmp0, imm)) concat(rv64_, rv64_name) (dest_rvidx, src1_rvidx, tmp0); \
    else concat(rv64_, rv64_imm_name) (dest_rvidx, src1_rvidx, imm); \
    spill_set_dirty_rvidx(dest_rvidx); \
  }

make_rtl_compute_reg(and, and)
make_rtl_compute_reg(or, or)
make_rtl_compute_reg(xor, xor)

make_rtl_compute_imm_opt(addi, add, addi)
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

make_rtl_compute_reg(addw, addw)
make_rtl_compute_reg(subw, subw)
make_rtl_compute_reg(shlw, sllw)
make_rtl_compute_reg(shrw, srlw)
make_rtl_compute_reg(sarw, sraw)
make_rtl_compute_imm_opt(addiw, addw, addiw)
make_rtl_compute_imm(shliw, slliw)
make_rtl_compute_imm(shriw, srliw)
make_rtl_compute_imm(sariw, sraiw)
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

make_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  uint32_t ret = rtlreg2rvidx_pair(s, src1, true, src2, true);
  uint32_t src1_rvidx = ret >> 16;
  uint32_t src2_rvidx = ret & 0xffff;
  uint32_t dest_rvidx = dest2rvidx(s, dest);
  if (dest_rvidx == 0) return;
  rv64_relop(relop, dest_rvidx, src1_rvidx, src2_rvidx);
}

make_rtl(setrelopi, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const sword_t imm) {
  int big_imm = load_imm_big(tmp0, imm);
  uint32_t ret = rtlreg2rvidx_pair(s, dest, false, src1, true);
  uint32_t dest_rvidx = ret >> 16;
  uint32_t src1_rvidx = ret & 0xffff;
  if (dest_rvidx == 0) return;
  if (!big_imm && (relop == RELOP_LT || relop == RELOP_LTU)) {
    switch (relop) {
      case RELOP_LT: rv64_slti(dest_rvidx, src1_rvidx, imm); goto finish;
      case RELOP_LTU: rv64_sltiu(dest_rvidx, src1_rvidx, imm); goto finish;
      // fall through for default cases
    }
  }
  if (!big_imm) rv64_addi(tmp0, x0, imm);
  rv64_relop(relop, dest_rvidx, src1_rvidx, tmp0);
finish:
  spill_set_dirty_rvidx(dest_rvidx);
}


#ifdef ISA64
make_rtl_compute_reg(mul_lo, mul)
make_rtl_compute_reg(mul_hi, mulhu)
make_rtl_compute_reg(imul_lo, mul)
make_rtl_compute_reg(imul_hi, mulh)
make_rtl_compute_reg(div_q, divu)
make_rtl_compute_reg(div_r, remu)
make_rtl_compute_reg(idiv_q, div)
make_rtl_compute_reg(idiv_r, rem)

make_rtl_compute_reg(mulw, mulw)
make_rtl_compute_reg(divw, divw)
make_rtl_compute_reg(divuw, divuw)
make_rtl_compute_reg(remw, remw)
make_rtl_compute_reg(remuw, remuw)
#else
make_rtl_compute_reg(mul_lo, mulw)
make_rtl_compute_reg(imul_lo, mulw)
make_rtl_compute_reg(div_q, divuw)
make_rtl_compute_reg(div_r, remuw)
make_rtl_compute_reg(idiv_q, divw)
make_rtl_compute_reg(idiv_r, remw)

make_rtl(mul_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint32_t ret = rtlreg2rvidx_pair(s, src1, true, src2, true);
  uint32_t src1_rvidx = ret >> 16;
  uint32_t src2_rvidx = ret & 0xffff;
  rv64_zextw(src1_rvidx, src1_rvidx);
  rv64_zextw(src2_rvidx, src2_rvidx);
  uint32_t dest_rvidx = dest2rvidx(s, dest);
  rv64_mul(dest_rvidx, src1_rvidx, src2_rvidx);
  rv64_srai(dest_rvidx, dest_rvidx, 32);
}

make_rtl(imul_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint32_t ret = rtlreg2rvidx_pair(s, src1, true, src2, true);
  uint32_t src1_rvidx = ret >> 16;
  uint32_t src2_rvidx = ret & 0xffff;
  rv64_sextw(src1_rvidx, src1_rvidx);
  rv64_sextw(src2_rvidx, src2_rvidx);
  uint32_t dest_rvidx = dest2rvidx(s, dest);
  rv64_mul(dest_rvidx, src1_rvidx, src2_rvidx);
  rv64_srai(dest_rvidx, dest_rvidx, 32);
}
#endif

#ifdef __ISA_x86__
#define make_x86_div64(rtl_name, ext_type, rv64_name) \
make_rtl(rtl_name, rtlreg_t* dest, \
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) { \
  uint32_t idx_dest = dest2rvidx(s, dest); \
  uint32_t idx_src1_hi = src2rvidx(s, src1_hi); \
  uint32_t idx_src1_lo = src2rvidx(s, src1_lo); \
  uint32_t idx_src2 = src2rvidx(s, src2); \
  rv64_slli(tmp0, idx_src1_hi, 32); \
  rv64_zextw(idx_src1_lo, idx_src1_lo); \
  rv64_or(tmp0, tmp0, idx_src1_lo); \
  ext_type(idx_src2, idx_src2); \
  concat(rv64_, rv64_name) (idx_dest, tmp0, idx_src2); \
}
#else
#define make_x86_div64(rtl_name, ext_type, rv64_name) \
make_rtl(rtl_name, rtlreg_t* dest, \
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) { \
  panic("only support in x86"); \
}
#endif

make_x86_div64(div64_q, rv64_zextw, divu)
make_x86_div64(div64_r, rv64_zextw, remu)
make_x86_div64(idiv64_q, rv64_sextw, div)
make_x86_div64(idiv64_r, rv64_sextw, rem)

static inline int prepare_addr(int addr_rvidx_final, int addr_rvidx, const sword_t imm) {
  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;
  if (addr_rvidx == 0) rv64_lui(addr_rvidx_final, lui_imm);
  else if (lui_imm == 0) {
#ifdef ISA64
    addr_rvidx_final = addr_rvidx;
#else
    rv64_zextw(addr_rvidx_final, addr_rvidx);
#endif
  }
  else {
    rv64_lui(tmp0, lui_imm);
    rv64_add(addr_rvidx_final, tmp0, addr_rvidx);
    rv64_zextw(addr_rvidx_final, addr_rvidx_final);
  }

  return addr_rvidx_final;
}

make_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, const sword_t imm, int len) {
  uint32_t ret = rtlreg2rvidx_pair(s, dest, false, addr, true);
  uint32_t dest_rvidx = ret >> 16;
  uint32_t addr_rvidx = ret & 0xffff;
  if (dest_rvidx == 0) return;

  uint32_t addr_rvidx_final = prepare_addr(dest_rvidx, addr_rvidx, imm);
  switch (len) {
    case 1: rv64_lbu(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 2: rv64_lhu(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
#ifdef ISA64
    case 4: rv64_lwu(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
#else
    case 4: rv64_lw (dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
#endif
    case 8: rv64_ld (dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    default: assert(0);
  }
  spill_set_dirty_rvidx(dest_rvidx);
}

make_rtl(lms, rtlreg_t *dest, const rtlreg_t* addr, const sword_t imm, int len) {
  uint32_t ret = rtlreg2rvidx_pair(s, dest, false, addr, true);
  uint32_t dest_rvidx = ret >> 16;
  uint32_t addr_rvidx = ret & 0xffff;
  if (dest_rvidx == 0) return;

  uint32_t addr_rvidx_final = prepare_addr(dest_rvidx, addr_rvidx, imm);
  switch (len) {
    case 1: rv64_lb(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 2: rv64_lh(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 4: rv64_lw(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    default: assert(0);
  }
  spill_set_dirty_rvidx(dest_rvidx);
}

make_rtl(sm, const rtlreg_t* addr, const sword_t imm, const rtlreg_t* src1, int len) {
  uint32_t ret = rtlreg2rvidx_pair(s, addr, true, src1, true);
  uint32_t addr_rvidx = ret >> 16;
  uint32_t src1_rvidx = ret & 0xffff;

  uint32_t addr_rvidx_final = prepare_addr(tmp0, addr_rvidx, imm);
  switch (len) {
    case 1: rv64_sb(src1_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 2: rv64_sh(src1_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 4: rv64_sw(src1_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 8: rv64_sd(src1_rvidx, addr_rvidx_final, imm & 0xfff); break;
    default: assert(0);
  }
}

#ifdef __ISA_x86__
make_rtl(host_lm, rtlreg_t* dest, const void *addr, int len) {
  uint32_t idx_dest = dest2rvidx(s, dest);

  // we assume that `addr` is only from cpu.gpr in x86
  uintptr_t addr_align = (uintptr_t)addr & ~(sizeof(rtlreg_t) - 1);
  uint32_t idx_r = src2rvidx(s, (void *)addr_align);
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
  uint32_t idx_src1 = dest2rvidx(s, src1);

  // we assume that `addr` is only from cpu.gpr in x86
  uintptr_t addr_align = (uintptr_t)addr & ~(sizeof(rtlreg_t) - 1);
  uint32_t idx_r = src2rvidx(s, (void *)addr_align);

  spm(sw, idx_r, SPM_X86_REG);
  if (len == 1) spm(sb, idx_src1, SPM_X86_REG + ((uintptr_t)addr & 1));
  else if (len == 2) spm(sh, idx_src1, SPM_X86_REG);
  else assert(0);
  spm(lwu, idx_r, SPM_X86_REG);
}
#else
make_rtl(host_lm, rtlreg_t* dest, const void *addr, int len) {
  panic("only used in x86\n");
}

make_rtl(host_sm, void *addr, const rtlreg_t *src1, int len) {
  panic("only used in x86\n");
}
#endif


// we use tmp0 to store x86.pc of the next basic block
make_rtl(j, vaddr_t target) {
#ifdef REG_SPILLING
  spill_writeback_all();
#endif
  load_imm(tmp0, target);
  tran_next_pc = NEXT_PC_JMP;
}

make_rtl(jr, rtlreg_t *target) {
  uint32_t rvidx = src2rvidx(s, target);
  rv64_addi(tmp0, rvidx, 0);
#ifdef REG_SPILLING
  spill_writeback_all();
#endif
  tran_next_pc = NEXT_PC_JMP;
}

make_rtl(jrelop, uint32_t relop, const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target) {
  uint32_t ret = rtlreg2rvidx_pair(s, src1, true, src2, true);
  uint32_t rs1 = ret >> 16;
  uint32_t rs2 = ret & 0xffff;
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
