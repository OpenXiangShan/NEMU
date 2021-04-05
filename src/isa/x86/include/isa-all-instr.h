#include <cpu/decode.h>
#include "../local-include/rtl.h"
#include "../instr/rt/rt.h"

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap) \
  f(ret)

#define INSTR_UNARY(f) \
  f(call) \
  f(pushl_r) f(pushw_r) \
  f(pushl_I) f(pushw_I)

#define INSTR_BINARY(f) \
  f(movl_I2r) f(movl_G2E) f(movl_I2E) \
  f(movw_I2r) f(movw_G2E) f(movw_I2E) \
  f(subl_SI2E) \
  f(subw_SI2E) \
  f(xorl_G2E) \
  f(xorw_G2E)

#define INSTR_TERNARY(f)

def_all_EXEC_ID();
