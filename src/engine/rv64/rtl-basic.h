#ifndef __RTL_BASIC_H__
#define __RTL_BASIC_H__

#include <common.h>

/* RTL basic instructions */

make_rtl(add, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(sub, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(and, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(or, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(xor, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(shl, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(shr, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(sar, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(addi, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(subi, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(andi, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(ori, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(xori, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(shli, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(shri, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(sari, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(setrelopi, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const sword_t imm);

#ifdef ISA64
make_rtl(addw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(subw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(shlw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(shrw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(sarw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(addiw, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(shliw, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(shriw, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
make_rtl(sariw, rtlreg_t* dest, const rtlreg_t *src1, const sword_t imm);
#endif

make_rtl(mul_lo, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(mul_hi, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(imul_lo, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(imul_hi, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(div_q, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(div_r, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(idiv_q, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(idiv_r, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(div64_q, rtlreg_t* dest, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2);
make_rtl(div64_r, rtlreg_t* dest, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2);
make_rtl(idiv64_q, rtlreg_t* dest, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2);
make_rtl(idiv64_r, rtlreg_t* dest, const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2);

#ifdef ISA64
make_rtl(mulw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(divw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(divuw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(remw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(remuw, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
#endif

make_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, const sword_t offset, int len);
make_rtl(sm, const rtlreg_t* addr, const sword_t offset, const rtlreg_t* src1, int len);
make_rtl(lms, rtlreg_t *dest, const rtlreg_t* addr, const sword_t offset, int len);
make_rtl(host_lm, rtlreg_t* dest, const void *addr, int len);
make_rtl(host_sm, void *addr, const rtlreg_t *src1, int len);

make_rtl(j, vaddr_t target);
make_rtl(jr, rtlreg_t *target);
make_rtl(jrelop, uint32_t relop, const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target);

#endif
