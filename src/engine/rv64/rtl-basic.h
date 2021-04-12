#ifndef __RTL_BASIC_H__
#define __RTL_BASIC_H__

#include <common.h>

/* RTL basic instructions */

def_rtl(add, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(sub, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(and, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(or, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(xor, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(shl, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(shr, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(sar, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(addi, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(subi, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(andi, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(ori, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(xori, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(shli, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(shri, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(sari, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(setrelopi, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const sword_t imm);

#ifdef CONFIG_ISA64
def_rtl(addw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(subw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(shlw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(shrw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(sarw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(addiw, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(shliw, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(shriw, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
def_rtl(sariw, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
#endif

def_rtl(mulu_lo, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(mulu_hi, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(muls_hi, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(divu_q, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(divu_r, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(divs_q, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(divs_r, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(div64u_q, rtlreg_t* dest, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2);
def_rtl(div64u_r, rtlreg_t* dest, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2);
def_rtl(div64s_q, rtlreg_t* dest, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2);
def_rtl(div64s_r, rtlreg_t* dest, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2);

#ifdef CONFIG_ISA64
def_rtl(mulw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(divw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(divuw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(remw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
def_rtl(remuw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
#endif

def_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, const sword_t offset, int len);
def_rtl(sm, const rtlreg_t* addr, const sword_t offset, const rtlreg_t* src1, int len);
def_rtl(lms, rtlreg_t *dest, const rtlreg_t* addr, const sword_t offset, int len);
def_rtl(host_lm, rtlreg_t* dest, const void *addr, int len);
def_rtl(host_sm, void *addr, const rtlreg_t *src1, int len);

def_rtl(j, vaddr_t target);
def_rtl(jr, rtlreg_t *target);
def_rtl(jrelop, uint32_t relop, const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target);

#endif
