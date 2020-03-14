#include <rtl/rtl.h>
#include "rv_ins_def.h"

void rv64_relop(DecodeExecState *s, uint32_t relop,
    const rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2);
uint8_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest);
extern int tran_is_jmp;

static inline void rv64_zextw(uint8_t rd, uint8_t rs) {
  // x24 is set during initialization
  rv64_and(rd, rs, x24);
}

/* RTL basic instructions */

make_rtl(li, rtlreg_t* dest, rtlreg_t imm) {
  uint8_t idx = reg_ptr2idx(s, dest);
  RV_IMM rv_imm = { .val = imm };
  uint32_t lui_imm = rv_imm.imm_31_12 + (rv_imm.imm_11_0 >> 11);
  if (lui_imm == 0) {
    rv64_addiw(idx, x0, rv_imm.imm_11_0);
  } else {
    rv64_lui(idx, lui_imm);
    rv64_addiw(idx, idx, rv_imm.imm_11_0);
  }
}

make_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_src1 = reg_ptr2idx(s, src1);
  if (dest != src1) rv64_addiw(idx_dest, idx_src1, 0);
}

#define rv64_shlw rv64_sllw
#define rv64_shrw rv64_srlw
#define rv64_sarw rv64_sraw
#define rv64_mul_lo rv64_mulw
#define rv64_imul_lo rv64_mulw
#define rv64_div_q rv64_divuw
#define rv64_div_r rv64_remuw
#define rv64_idiv_q rv64_divw
#define rv64_idiv_r rv64_remw

#define make_rtl_arith_logic_w(name) \
  make_rtl(name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    concat3(rv64_, name, w) (reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), reg_ptr2idx(s, src2)); \
  }

#define make_rtl_arith_logic(name) \
  make_rtl(name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    concat(rv64_, name) (reg_ptr2idx(s, dest), reg_ptr2idx(s, src1), reg_ptr2idx(s, src2)); \
  }

make_rtl_arith_logic_w(add)
make_rtl_arith_logic_w(sub)
make_rtl_arith_logic(and)
make_rtl_arith_logic(or)
make_rtl_arith_logic(xor)
make_rtl_arith_logic_w(shl)
make_rtl_arith_logic_w(shr)
make_rtl_arith_logic_w(sar)
make_rtl_arith_logic(mul_lo)
//make_rtl_arith_logic(mul_hi)
make_rtl_arith_logic(imul_lo)
//make_rtl_arith_logic(imul_hi)
make_rtl_arith_logic(div_q)
make_rtl_arith_logic(div_r)
make_rtl_arith_logic(idiv_q)
make_rtl_arith_logic(idiv_r)

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

make_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, int len) {
  uint8_t idx_dest = reg_ptr2idx(s, dest);
  uint8_t idx_addr = reg_ptr2idx(s, addr);
  rv64_zextw(x31, idx_addr);
  switch (len) {
    case 1: rv64_lbu(idx_dest, x31, 0); break;
    case 2: rv64_lhu(idx_dest, x31, 0); break;
    case 4: rv64_lwu(idx_dest, x31, 0); break;
    case 8: rv64_ld (idx_dest, x31, 0); break;
    default: assert(0);
  }
}

make_rtl(sm, const rtlreg_t* addr, const rtlreg_t* src1, int len) {
  uint8_t idx_addr = reg_ptr2idx(s, addr);
  uint8_t idx_src1 = reg_ptr2idx(s, src1);
  rv64_zextw(x31, idx_addr);
  switch (len) {
    case 1: rv64_sb(x31, idx_src1, 0); break;
    case 2: rv64_sh(x31, idx_src1, 0); break;
    case 4: rv64_sw(x31, idx_src1, 0); break;
    case 8: rv64_sd(x31, idx_src1, 0); break;
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

make_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  rv64_relop(s, relop, dest, src1, src2);
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
  rtl_setrelop(s, relop, ddest, src1, src2);
  rtl_li(s, dsrc1, target);
  rtl_li(s, dsrc2, s->seq_pc);
  rtl_mux(s, ddest, ddest, dsrc1, dsrc2);
  rv64_addi(x30, reg_ptr2idx(s, ddest), 0);
  tran_is_jmp = true;
}
