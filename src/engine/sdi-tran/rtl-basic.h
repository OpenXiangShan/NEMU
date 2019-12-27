#ifndef __RTL_BASIC_H__
#define __RTL_BASIC_H__

#include <common.h>

/* RTL basic instructions */

make_rtl(li, rtlreg_t* dest, rtlreg_t imm);
make_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1);
make_rtl(add, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(sub, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(and, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(or, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(xor, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(shl, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(shr, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(sar, rtlreg_t* dest, const rtlreg_t *src1, const rtlreg_t *src2);
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
make_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, int len);
make_rtl(sm, const rtlreg_t* addr, const rtlreg_t* src1, int len);
make_rtl(host_lm, rtlreg_t* dest, const void *addr, int len);
make_rtl(host_sm, void *addr, const rtlreg_t *src1, int len);
make_rtl(setrelop, uint32_t relop, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2);
make_rtl(j, vaddr_t target);
make_rtl(jr, rtlreg_t *target);
make_rtl(jrelop, uint32_t relop, const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target);

#endif
