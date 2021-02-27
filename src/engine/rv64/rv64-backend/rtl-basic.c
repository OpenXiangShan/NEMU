#include <rtl/rtl.h>
#include "rv_ins_def.h"
#include "../tran.h"
#include "../spill.h"

void rv64_relop(uint32_t relop, uint32_t idx_dest, uint32_t idx_src1, uint32_t idx_src2);
uint32_t dest2rvidx(Decode *s, const rtlreg_t* dest);
uint32_t src2rvidx(Decode *s, const rtlreg_t* src);
int rtlreg_is_zero(Decode *s, const rtlreg_t *r);

static inline void rv64_zextw(uint32_t rd, uint32_t rs) {
  // mask32 is set during initialization
  IFUNDEF(CONFIG_ISA64, rv64_and(rd, rs, mask32));
}

static inline void rv64_sextw(uint32_t rd, uint32_t rs) {
  IFUNDEF(CONFIG_ISA64, rv64_addw(rd, rs, x0));
}

static inline sword_t get_imm_hi(const sword_t imm) {
  RV_IMM rv_imm = { .val = imm };
  uint32_t imm_hi = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;
  return imm_hi;
}

static inline sword_t get_imm_lo(const sword_t imm) {
  return imm & 0xfffu;
}

static inline bool is_imm_big(const sword_t imm) {
  return get_imm_hi(imm) != 0;
}

static inline void load_imm(uint32_t r, const sword_t imm) {
  if (r == 0) return;
  if (!is_imm_big(imm)) rv64_addi(r, x0, imm & 0xfff);
  else {
    sword_t hi = get_imm_hi(imm);
    sword_t lo = get_imm_lo(imm);
    rv64_lui(r, hi);
    if (lo != 0) rv64_addi(r, r, lo);
  }
}

static inline void load_imm_no_opt(uint32_t r, const sword_t imm) {
  sword_t hi = get_imm_hi(imm);
  sword_t lo = get_imm_lo(imm);
  rv64_lui(r, hi);
  rv64_addi(r, r, lo);
}

/* RTL basic instructions */

#define def_rtl_compute_reg(rtl_name, rv64_name) \
  def_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    if (rtlreg_is_zero(s, dest)) return; \
    uint32_t ret = rtlreg2rvidx_pair(s, src1, true, src2, true); \
    uint32_t src1_rvidx = ret >> 16; \
    uint32_t src2_rvidx = ret & 0xffff; \
    uint32_t dest_rvidx = dest2rvidx(s, dest); \
    concat(rv64_, rv64_name) (dest_rvidx, src1_rvidx, src2_rvidx); \
    spill_set_dirty_rvidx(dest_rvidx); \
  }

#define def_rtl_compute_imm_small(rtl_name, rv64_name) \
  def_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) { \
    if (rtlreg_is_zero(s, dest)) return; \
    uint32_t ret = rtlreg2rvidx_pair(s, dest, false, src1, true); \
    uint32_t dest_rvidx = ret >> 16; \
    uint32_t src1_rvidx = ret & 0xffff; \
    concat(rv64_, rv64_name) (dest_rvidx, src1_rvidx, imm); \
    spill_set_dirty_rvidx(dest_rvidx); \
  }

#define def_rtl_compute_imm_opt(rtl_name, rv64_name, rv64_imm_name) \
  def_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) { \
    if (rtlreg_is_zero(s, dest)) return; \
    uint32_t ret = rtlreg2rvidx_pair(s, dest, false, src1, true); \
    uint32_t dest_rvidx = ret >> 16; \
    uint32_t src1_rvidx = ret & 0xffff; \
    if (src1 == rz) load_imm(dest_rvidx, imm); \
    else if (is_imm_big(imm)) { \
      load_imm(tmp0, imm); \
      concat(rv64_, rv64_name) (dest_rvidx, src1_rvidx, tmp0); \
    } \
    else concat(rv64_, rv64_imm_name) (dest_rvidx, src1_rvidx, imm); \
    spill_set_dirty_rvidx(dest_rvidx); \
  }

def_rtl_compute_reg(and, and)
def_rtl_compute_reg(or, or)
def_rtl_compute_reg(xor, xor)

def_rtl_compute_imm_opt(addi, add, addi)
def_rtl_compute_imm_opt(addi, add, MUXDEF(CONFIG_USA64, addi, addiw))
def_rtl_compute_imm_opt(andi, and, andi)
def_rtl_compute_imm_opt(xori, xor, xori)
def_rtl_compute_imm_opt(ori, or, ori)

def_rtl(subi, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) {
  rtl_addi(s, dest, src1, -imm);
}

#ifdef CONFIG_ISA64
def_rtl_compute_reg(add, add)
def_rtl_compute_reg(sub, sub)
def_rtl_compute_reg(shl, sll)
def_rtl_compute_reg(shr, srl)
def_rtl_compute_reg(sar, sra)
def_rtl_compute_imm_small(shli, slli)
def_rtl_compute_imm_small(shri, srli)
def_rtl_compute_imm_small(sari, srai)

def_rtl_compute_reg(addw, addw)
def_rtl_compute_reg(subw, subw)
def_rtl_compute_reg(shlw, sllw)
def_rtl_compute_reg(shrw, srlw)
def_rtl_compute_reg(sarw, sraw)
def_rtl_compute_imm_opt(addiw, addw, addiw)
def_rtl_compute_imm_small(shliw, slliw)
def_rtl_compute_imm_small(shriw, srliw)
def_rtl_compute_imm_small(sariw, sraiw)
#else
def_rtl_compute_reg(add, addw)
def_rtl_compute_reg(sub, subw)
def_rtl_compute_reg(shl, sllw)
def_rtl_compute_reg(shr, srlw)
def_rtl_compute_reg(sar, sraw)
def_rtl_compute_imm_small(shli, slliw)
def_rtl_compute_imm_small(shri, srliw)
def_rtl_compute_imm_small(sari, sraiw)
#endif

def_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  if (rtlreg_is_zero(s, dest)) return;
  uint32_t ret = rtlreg2rvidx_pair(s, src1, true, src2, true);
  uint32_t src1_rvidx = ret >> 16;
  uint32_t src2_rvidx = ret & 0xffff;
  uint32_t dest_rvidx = dest2rvidx(s, dest);
  rv64_relop(relop, dest_rvidx, src1_rvidx, src2_rvidx);
}

def_rtl(setrelopi, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const sword_t imm) {
  if (rtlreg_is_zero(s, dest)) return;
  uint32_t ret = rtlreg2rvidx_pair(s, dest, false, src1, true);
  uint32_t dest_rvidx = ret >> 16;
  uint32_t src1_rvidx = ret & 0xffff;
  int big_imm = is_imm_big(imm);
  if (!big_imm && (relop == RELOP_LT || relop == RELOP_LTU)) {
    switch (relop) {
      case RELOP_LT: rv64_slti(dest_rvidx, src1_rvidx, imm); goto finish;
      case RELOP_LTU: rv64_sltiu(dest_rvidx, src1_rvidx, imm); goto finish;
      // fall through for default cases
    }
  }
  load_imm(tmp0, imm);
  rv64_relop(relop, dest_rvidx, src1_rvidx, tmp0);
finish:
  spill_set_dirty_rvidx(dest_rvidx);
}


#ifdef CONFIG_ISA64
def_rtl_compute_reg(mulu_lo, mul)
def_rtl_compute_reg(mulu_hi, mulhu)
def_rtl_compute_reg(muls_hi, mulh)
def_rtl_compute_reg(divu_q, divu)
def_rtl_compute_reg(divu_r, remu)
def_rtl_compute_reg(divs_q, div)
def_rtl_compute_reg(divs_r, rem)

def_rtl_compute_reg(mulw, mulw)
def_rtl_compute_reg(divw, divw)
def_rtl_compute_reg(divuw, divuw)
def_rtl_compute_reg(remw, remw)
def_rtl_compute_reg(remuw, remuw)
#else
def_rtl_compute_reg(mulu_lo, mulw)
def_rtl_compute_reg(divu_q, divuw)
def_rtl_compute_reg(divu_r, remuw)
def_rtl_compute_reg(divs_q, divw)
def_rtl_compute_reg(divs_r, remw)

def_rtl(mulu_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint32_t ret = rtlreg2rvidx_pair(s, src1, true, src2, true);
  uint32_t src1_rvidx = ret >> 16;
  uint32_t src2_rvidx = ret & 0xffff;
  rv64_zextw(src1_rvidx, src1_rvidx);
  rv64_zextw(src2_rvidx, src2_rvidx);
  uint32_t dest_rvidx = dest2rvidx(s, dest);
  rv64_mul(dest_rvidx, src1_rvidx, src2_rvidx);
  rv64_srai(dest_rvidx, dest_rvidx, 32);
}

def_rtl(muls_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
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
def_rtl(rtl_name, rtlreg_t* dest, \
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
def_rtl(rtl_name, rtlreg_t* dest, \
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) { \
  panic("only support in x86"); \
}
#endif

make_x86_div64(div64u_q, rv64_zextw, divu)
make_x86_div64(div64u_r, rv64_zextw, remu)
make_x86_div64(div64s_q, rv64_sextw, div)
make_x86_div64(div64s_r, rv64_sextw, rem)

static inline int prepare_addr(int addr_rvidx_final, int addr_rvidx, const sword_t imm) {
  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = (rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11)) & 0xfffffu;
  if (addr_rvidx == 0) rv64_lui(addr_rvidx_final, lui_imm);
  else if (lui_imm == 0) {
#ifdef CONFIG_ISA64
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

def_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, const sword_t imm, int len) {
  if (rtlreg_is_zero(s, dest)) return;
  uint32_t ret = rtlreg2rvidx_pair(s, dest, false, addr, true);
  uint32_t dest_rvidx = ret >> 16;
  uint32_t addr_rvidx = ret & 0xffff;

  uint32_t addr_rvidx_final = prepare_addr(dest_rvidx, addr_rvidx, imm);
  switch (len) {
    case 1: rv64_lbu(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 2: rv64_lhu(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 4: MUXDEF(CONFIG_ISA64, rv64_lwu, rv64_lw)(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 8: rv64_ld (dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    default: assert(0);
  }
  spill_set_dirty_rvidx(dest_rvidx);
}

def_rtl(lms, rtlreg_t *dest, const rtlreg_t* addr, const sword_t imm, int len) {
  if (rtlreg_is_zero(s, dest)) return;
  uint32_t ret = rtlreg2rvidx_pair(s, dest, false, addr, true);
  uint32_t dest_rvidx = ret >> 16;
  uint32_t addr_rvidx = ret & 0xffff;

  uint32_t addr_rvidx_final = prepare_addr(dest_rvidx, addr_rvidx, imm);
  switch (len) {
    case 1: rv64_lb(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 2: rv64_lh(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    case 4: rv64_lw(dest_rvidx, addr_rvidx_final, imm & 0xfff); break;
    default: assert(0);
  }
  spill_set_dirty_rvidx(dest_rvidx);
}

def_rtl(sm, const rtlreg_t* addr, const sword_t imm, const rtlreg_t* src1, int len) {
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
def_rtl(host_lm, rtlreg_t* dest, const void *addr, int len) {
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

def_rtl(host_sm, void *addr, const rtlreg_t *src1, int len) {
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
def_rtl(host_lm, rtlreg_t* dest, const void *addr, int len) {
  panic("only used in x86\n");
}

def_rtl(host_sm, void *addr, const rtlreg_t *src1, int len) {
  panic("only used in x86\n");
}
#endif


// we use tmp0 to store x86.pc of the next basic block
def_rtl(j, vaddr_t target) {
#ifdef REG_SPILLING
  spill_writeback_all();
#endif
  load_imm(tmp0, target);
  tran_next_pc = NEXT_PC_JMP;
}

def_rtl(jr, rtlreg_t *target) {
  uint32_t rvidx = src2rvidx(s, target);
  rv64_addi(tmp0, rvidx, 0);
#ifdef REG_SPILLING
  spill_writeback_all();
#endif
  tran_next_pc = NEXT_PC_JMP;
}

def_rtl(jrelop, uint32_t relop, const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target) {
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
