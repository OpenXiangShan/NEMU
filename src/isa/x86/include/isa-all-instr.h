#include <cpu/decode.h>
#include "../local-include/rtl.h"
#include "../instr/rt/rt.h"

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap) f(nop) \
  f(ret) f(leave) \
  f(cltd)

#define INSTR_UNARY(f) \
  f(call) f(jcc) f(jmp) f(setcc) f(call_E) f(jmp_E) \
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
  f(movl_I2r) f(movl_G2E) f(movl_E2G) f(movl_I2E) f(movl_O2a) f(movl_a2O) \
  f(movw_I2r) f(movw_G2E) f(movw_E2G) f(movw_I2E) f(movw_O2a) f(movw_a2O) \
  f(movb_G2E) f(movb_E2G) f(movb_I2E) f(movb_O2a) f(movb_a2O) \
  f(addl_G2E) f(addl_E2G) f(addl_SI2E) \
  f(addw_G2E) f(addw_E2G) f(addw_SI2E) \
  f(subl_G2E) f(subl_E2G) f(subl_I2E) f(subl_SI2E) \
  f(subw_G2E) f(subw_E2G) f(subw_I2E) f(subw_SI2E) \
  f(adcl_E2G) \
  f(adcw_E2G) \
  f(sbbl_E2G) \
  f(sbbw_E2G) \
  f(cmpl_G2E) f(cmpl_E2G) f(cmpl_I2a) f(cmpl_I2E) f(cmpl_SI2E) \
  f(cmpw_G2E) f(cmpw_E2G) f(cmpw_I2a) f(cmpw_I2E) f(cmpw_SI2E) \
  f(cmpb_G2E) f(cmpb_I2a) f(cmpb_I2E) \
  f(andl_SI2E) \
  f(andw_SI2E) \
  f(orl_G2E) \
  f(orw_G2E) \
  f(testl_G2E) \
  f(testw_G2E) \
  f(testb_I2E) \
  f(xorl_G2E) \
  f(xorw_G2E) \
  f(notl_E) \
  f(notw_E) \
  f(shll_Ib2E) f(shll_cl2E) \
  f(shlw_Ib2E) f(shlw_cl2E) \
  f(shrl_Ib2E) f(shrl_cl2E) \
  f(shrw_Ib2E) f(shrw_cl2E) \
  f(sarl_Ib2E) f(sarl_cl2E) f(sarl_1_E)\
  f(sarw_Ib2E) f(sarw_cl2E) f(sarw_1_E)\
  f(lea) \
  f(movzbw_Eb2G) f(movzbl_Eb2G) f(movzwl_Ew2G) \
  f(movsbw_Eb2G) f(movsbl_Eb2G) f(movswl_Ew2G) \
  f(mull_E) \
  f(mulw_E) \
  f(imull_E) f(imull_E2G) \
  f(imulw_E) f(imulw_E2G) \
  f(idivl_E) \
  f(idivw_E) \

#define INSTR_TERNARY(f)

def_all_EXEC_ID();
