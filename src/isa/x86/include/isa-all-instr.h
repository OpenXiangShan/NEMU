#include <cpu/decode.h>
#include "../local-include/rtl.h"
#include "../instr/rt/rt.h"

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap) f(nop) \
  f(ret) f(leave) \
  f(cltd)

#define INSTR_UNARY(f) \
  f(call) f(jcc) f(jmp) f(setcc) \
  f(pushl_r) f(pushl_E) f(pushl_I) \
  f(pushw_r) f(pushw_E) f(pushw_I) \
  f(pushb_SI) \
  f(popl_r) \
  f(popw_r) \
  f(incl_r) f(incl_E) \
  f(incw_r) f(incw_E) \
  f(decl_r) \
  f(decw_r) \

#define INSTR_BINARY(f) \
  f(movl_I2r) f(movl_G2E) f(movl_E2G) f(movl_I2E) \
  f(movw_I2r) f(movw_G2E) f(movw_E2G) f(movw_I2E) \
  f(movb_G2E) f(movb_E2G) \
  f(addl_G2E) f(addl_E2G) f(addl_SI2E) \
  f(addw_G2E) f(addw_E2G) f(addw_SI2E) \
  f(subl_E2G) f(subl_SI2E) \
  f(subw_E2G) f(subw_SI2E) \
  f(adcl_E2G) \
  f(adcw_E2G) \
  f(sbbl_E2G) \
  f(sbbw_E2G) \
  f(cmpl_G2E) f(cmpl_E2G) f(cmpl_I2a) f(cmpl_I2E) f(cmpl_SI2E) \
  f(cmpw_G2E) f(cmpw_E2G) f(cmpw_I2a) f(cmpw_I2E) f(cmpw_SI2E) \
  f(cmpb_G2E) f(cmpb_I2a) f(cmpb_I2E) \
  f(andl_SI2E) \
  f(andw_SI2E) \
  f(testl_G2E) \
  f(testw_G2E) \
  f(testb_I2E) \
  f(xorl_G2E) \
  f(xorw_G2E) \
  f(orl_G2E) \
  f(orw_G2E) \
  f(lea) \
  f(movzbw_Eb2G) f(movzbl_Eb2G) \
  f(imull_E2G) \
  f(imulw_E2G) \
  f(idivl_E) \
  f(idivw_E) \

#define INSTR_TERNARY(f)

def_all_EXEC_ID();
