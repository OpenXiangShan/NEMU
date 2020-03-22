#include <rtl/rtl.h>
#include "rv_ins_def.h"

void rv64_relop(uint32_t relop, uint32_t idx_dest, uint32_t idx_src1, uint32_t idx_src2);
uint8_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest);
extern int tran_is_jmp;

static inline void rv64_zextw(uint8_t rd, uint8_t rs) {
  // x24 is set during initialization
  rv64_and(rd, rs, x24);
}

// return true if `imm` can be represented within 12 bits
// else load it to x31, and reture false
static inline bool load_imm(const sword_t imm) {
  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11);
  if (lui_imm == 0) return true;
  else {
    rv64_lui(x31, lui_imm);
    if (rv_imm.imm_11_0 != 0) rv64_addiw(x31, x31, rv_imm.imm_11_0);
    return false;
  }
}

/* RTL basic instructions */

#define make_rtl_compute_reg(rtl_name, rv64_name) \
  make_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    concat(rv64_, rv64_name) (reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), reg_ptr2idx(s, src2)); \
  }

#define make_rtl_compute_imm(rtl_name, rv64_name) \
  make_rtl(rtl_name, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) { \
    concat(rv64_, rv64_name) (reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), imm); \
  }

make_rtl_compute_reg(and, and)
make_rtl_compute_reg(or, or)
make_rtl_compute_reg(xor, xor)

make_rtl(addi, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) {
  if (load_imm(imm)) rv64_addiw(reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), imm);
  else rv64_addw(reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), x31);
}

make_rtl(subi, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) {
  rtl_addi(s, dest, src1, -imm);
}

make_rtl(andi, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) {
  if (load_imm(imm)) rv64_andi(reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), imm);
  else rv64_and(reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), x31);
}

make_rtl(xori, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) {
  if (load_imm(imm)) rv64_xori(reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), imm);
  else rv64_xor(reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), x31);
}

make_rtl(ori, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) {
  if (load_imm(imm)) rv64_ori(reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), imm);
  else rv64_or(reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), x31);
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

make_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  rv64_relop(relop, reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), reg_ptr2idx(s, src2));
}

make_rtl(setrelopi, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const sword_t imm) {
  int small_imm = load_imm(imm);
  uint32_t idx_dest = reg_ptr2idx(s, dest);
  uint32_t idx_src1 = reg_ptr2idx(s, src1);
  if (small_imm && (relop == RELOP_LT || relop == RELOP_LTU)) {
    switch (relop) {
      case RELOP_LT: rv64_slt(idx_dest, idx_src1, imm); return;
      case RELOP_LTU: rv64_sltu(idx_dest, idx_src1, imm); return;
      // fall through for default cases
    }
  }
  rv64_addiw(x31, x0, imm);
  rv64_relop(relop, idx_dest, idx_src1, x31);
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
make_rtl(mul_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_src1 = reg_ptr2idx(s, src1);
  uint8_t idx_src2 = reg_ptr2idx(s, src2);
  rv64_zextw(x30, idx_src1);
  rv64_zextw(x31, idx_src2);
  rv64_mul(idx_dest, x30, x31);
  rv64_srai(idx_dest, idx_dest, 32);
}

make_rtl(imul_hi, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_src1 = reg_ptr2idx(s, src1);
  uint8_t idx_src2 = reg_ptr2idx(s, src2);
  rv64_mul(idx_dest, idx_src1, idx_src2);
  rv64_srai(idx_dest, idx_dest, 32);
}
#endif

make_rtl(div64_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_src1_hi = reg_ptr2idx(s, src1_hi);
  uint8_t idx_src1_lo = reg_ptr2idx(s, src1_lo);
  uint8_t idx_src2 = reg_ptr2idx(s, src2);

  rv64_slli(x30, idx_src1_hi, 32);
  rv64_zextw(x31, idx_src1_lo);
  rv64_or(x30, x30, x31);
  rv64_zextw(x31, idx_src2);
  rv64_divu(idx_dest, x30, x31);
}

make_rtl(div64_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_src1_hi = reg_ptr2idx(s, src1_hi);
  uint8_t idx_src1_lo = reg_ptr2idx(s, src1_lo);
  uint8_t idx_src2 = reg_ptr2idx(s, src2);

  rv64_slli(x30, idx_src1_hi, 32);
  rv64_zextw(x31, idx_src1_lo);
  rv64_or(x30, x30, x31);
  rv64_zextw(x31, idx_src2);
  rv64_remu(idx_dest, x30, x31);
}

make_rtl(idiv64_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_src1_hi = reg_ptr2idx(s, src1_hi);
  uint8_t idx_src1_lo = reg_ptr2idx(s, src1_lo);
  uint8_t idx_src2 = reg_ptr2idx(s, src2);

  rv64_slli(x30, idx_src1_hi, 32);
  rv64_zextw(x31, idx_src1_lo);
  rv64_or(x30, x30, x31);
  rv64_div(idx_dest, x30, idx_src2);
}

make_rtl(idiv64_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_src1_hi = reg_ptr2idx(s, src1_hi);
  uint8_t idx_src1_lo = reg_ptr2idx(s, src1_lo);
  uint8_t idx_src2 = reg_ptr2idx(s, src2);

  rv64_slli(x30, idx_src1_hi, 32);
  rv64_zextw(x31, idx_src1_lo);
  rv64_or(x30, x30, x31);
  rv64_rem(idx_dest, x30, idx_src2);
}

make_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, const sword_t imm, int len) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_addr = reg_ptr2idx(s, addr);

  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11);
  if (lui_imm != 0) {
    rv64_lui(x31, lui_imm);
    rv64_add(x31, x31, idx_addr);
    rv64_zextw(x31, x31);
  } else {
    rv64_zextw(x31, idx_addr);
  }
  switch (len) {
    case 1: rv64_lbu(idx_dest, x31, imm & 0xfff); break;
    case 2: rv64_lhu(idx_dest, x31, imm & 0xfff); break;
    case 4: rv64_lwu(idx_dest, x31, imm & 0xfff); break;
    case 8: rv64_ld (idx_dest, x31, imm & 0xfff); break;
    default: assert(0);
  }
}

make_rtl(sm, const rtlreg_t* addr, const sword_t imm, const rtlreg_t* src1, int len) {
  uint8_t idx_addr = reg_ptr2idx(s, addr);
  uint8_t idx_src1 = reg_ptr2idx(s, src1);

  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11);
  if (lui_imm != 0) {
    rv64_lui(x31, lui_imm);
    rv64_add(x31, x31, idx_addr);
    rv64_zextw(x31, x31);
  } else {
    rv64_zextw(x31, idx_addr);
  }
  switch (len) {
    case 1: rv64_sb(x31, idx_src1, imm & 0xfff); break;
    case 2: rv64_sh(x31, idx_src1, imm & 0xfff); break;
    case 4: rv64_sw(x31, idx_src1, imm & 0xfff); break;
    case 8: rv64_sd(x31, idx_src1, imm & 0xfff); break;
    default: assert(0);
  }
}

make_rtl(host_lm, rtlreg_t* dest, const void *addr, int len) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);

  // we assume that `addr` is only from cpu.gpr in x86
  uintptr_t addr_align = (uintptr_t)addr & ~(sizeof(rtlreg_t) - 1);
  uint8_t idx_r = reg_ptr2idx(s, (void *)addr_align);
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
      rv64_and(idx_dest, idx_r, x25); // x25 is set during initialization
      return;
    default: assert(0);
  }
}

make_rtl(host_sm, void *addr, const rtlreg_t *src1, int len) {
  uint8_t idx_src1 = reg_ptr2idx(s, src1);

  // we assume that `addr` is only from cpu.gpr in x86
  uintptr_t addr_align = (uintptr_t)addr & ~(sizeof(rtlreg_t) - 1);
  uint8_t idx_r = reg_ptr2idx(s, (void *)addr_align);
  switch (len) {
    case 1:
      if((uintptr_t)addr & 1){//high
        rv64_addi(x31, x0, 0xff);
        rv64_slliw(x31, x31, 8);
        rv64_slliw(x30, idx_src1, 8);
        rv64_and(x30, x30, x31);
        rv64_xori(x31, x31, -1);
        rv64_and(idx_r, idx_r, x31);
        rv64_or(idx_r, idx_r, x30);
      }else{//low
        rv64_addi(x31, x0, 0xff);
        rv64_xori(x30, x31, -1);
        rv64_and(x31, x31, idx_src1);
        rv64_and(x30, x30, idx_r);
        rv64_or(idx_r, x30, x31);
      }
      return;
    case 2:
      rv64_lui(x31, 0xffff0);
      rv64_xori(x30, x31, -1);
      rv64_and(x31, x31, idx_r);
      rv64_and(x30, x30, idx_src1);
      rv64_or(idx_r, x30, x31);
      return;
    default: assert(0);
  }
}

// we use x30 to store x86.pc of the next basic block
make_rtl(j, vaddr_t target) {
  rtl_li(s, ir, target);
  rv64_addi(x30, reg_ptr2idx(s, ir), 0);
  tran_is_jmp = true;
}

make_rtl(jr, rtlreg_t *target) {
  rv64_addi(x30, reg_ptr2idx(s, target), 0);
  tran_is_jmp = true;
}

make_rtl(jrelop, uint32_t relop, const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target) {
  rtl_setrelop(s, relop, &id_dest->val, src1, src2);
  rtl_li(s, &id_src1->val, target);
  rtl_li(s, &id_src2->val, s->seq_pc);
  rtl_mux(s, &id_dest->val, &id_dest->val, &id_src1->val, &id_src2->val);
  rv64_addi(x30, reg_ptr2idx(s, &id_dest->val), 0);
  tran_is_jmp = true;
}
