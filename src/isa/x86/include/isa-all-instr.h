#include <cpu/decode.h>
#include "../local-include/rtl.h"
#include "../instr/rt/rt.h"

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap) f(nop) \
  f(ret) f(leave) \
  f(cwtl) f(cltd) \
  f(movsb)

#define INSTR_UNARY(f) \
  f(call) f(jcc) f(jmp) f(setcc) f(call_E) f(jmp_E) \
  f(pushl_r) f(pushl_E) f(pushl_I) \
  f(pushw_r) f(pushw_E) f(pushw_I) \
  f(pushb_SI) \
  f(popl_r) \
  f(popw_r) \
  f(incl_r) f(incl_E) \
  f(incw_r) f(incw_E) \
  f(negl_E) \
  f(negw_E) \
  f(decl_r) f(decl_E) \
  f(decw_r) f(decw_E) \
  f(decb_E) \

#define INSTR_BINARY(f) \
  f(movl_I2r) f(movl_G2E) f(movl_E2G) f(movl_I2E) f(movl_O2a) f(movl_a2O) \
  f(movw_I2r) f(movw_G2E) f(movw_E2G) f(movw_I2E) f(movw_O2a) f(movw_a2O) \
  f(movb_I2r) f(movb_G2E) f(movb_E2G) f(movb_I2E) f(movb_O2a) f(movb_a2O) \
  f(addl_G2E) f(addl_E2G) f(addl_I2a) f(addl_SI2E) \
  f(addw_G2E) f(addw_E2G) f(addw_I2a) f(addw_SI2E) \
  f(addb_G2E) f(addb_E2G) \
  f(subl_G2E) f(subl_E2G) f(subl_I2E) f(subl_SI2E) \
  f(subw_G2E) f(subw_E2G) f(subw_I2E) f(subw_SI2E) \
  f(adcl_G2E) f(adcl_E2G) f(adcl_SI2E) \
  f(adcw_G2E) f(adcw_E2G) f(adcw_SI2E) \
  f(sbbl_G2E) f(sbbl_E2G) f(sbbl_SI2E) \
  f(sbbw_G2E) f(sbbw_E2G) f(sbbw_SI2E) \
  f(cmpl_G2E) f(cmpl_E2G) f(cmpl_I2a) f(cmpl_I2E) f(cmpl_SI2E) \
  f(cmpw_G2E) f(cmpw_E2G) f(cmpw_I2a) f(cmpw_I2E) f(cmpw_SI2E) \
  f(cmpb_G2E) f(cmpb_I2a) f(cmpb_I2E) \
  f(andl_G2E) f(andl_E2G) f(andl_I2a) f(andl_I2E) f(andl_SI2E) \
  f(andw_G2E) f(andw_E2G) f(andw_I2a) f(andw_I2E) f(andw_SI2E) \
  f(andb_E2G) \
  f(orl_G2E) f(orl_E2G) f(orl_I2a) f(orl_I2E) f(orl_SI2E) \
  f(orw_G2E) f(orw_E2G) f(orw_I2a) f(orw_I2E) f(orw_SI2E) \
  f(orb_E2G) f(orb_I2E) \
  f(testl_G2E) f(testl_I2E) \
  f(testw_G2E) f(testw_I2E) \
  f(testb_G2E) f(testb_I2E) f(testb_I2a) \
  f(xorl_G2E) f(xorl_E2G) f(xorl_I2a) f(xorl_SI2E) \
  f(xorw_G2E) f(xorw_E2G) f(xorw_I2a) f(xorw_SI2E) \
  f(xorb_G2E) f(xorb_E2G) \
  f(notl_E) \
  f(notw_E) \
  f(shll_Ib2E) f(shll_cl2E) \
  f(shlw_Ib2E) f(shlw_cl2E) \
  f(shrl_Ib2E) f(shrl_cl2E) f(shrl_1_E) \
  f(shrw_Ib2E) f(shrw_cl2E) f(shrw_1_E) \
  f(shrb_1_E) \
  f(sarl_Ib2E) f(sarl_cl2E) f(sarl_1_E) \
  f(sarw_Ib2E) f(sarw_cl2E) f(sarw_1_E) \
  f(roll_cl2E) \
  f(rolw_cl2E) \
  f(lea) \
  f(movzbw_Eb2G) f(movzbl_Eb2G) f(movzwl_Ew2G) \
  f(movsbw_Eb2G) f(movsbl_Eb2G) f(movswl_Ew2G) \
  f(mull_E) \
  f(mulw_E) \
  f(imull_E) f(imull_E2G) f(imull_I_E2G) \
  f(imulw_E) f(imulw_E2G) f(imulw_I_E2G) \
  f(divl_E) \
  f(divw_E) \
  f(idivl_E) \
  f(idivw_E) \
  f(bsrl_E2G) \
  f(bsrw_E2G) \
  f(inl_dx2a) \
  f(inw_dx2a) \
  f(outb_a2dx) \

#define INSTR_TERNARY(f)

def_all_EXEC_ID();
