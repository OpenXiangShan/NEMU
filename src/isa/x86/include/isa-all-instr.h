#include <cpu/decode.h>
#include "../local-include/rtl.h"
#include "../instr/decode.h"
#include "../instr/rt_decode.h"
#include "../instr/rt_exec.h"
#include "../instr/rt_wb.h"

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap)

#define INSTR_UNARY(f)
#define INSTR_BINARY(f) \
  f(movl_I2r) f(movl_G2E) f(movl_I2E) \
  f(movw_I2r) f(movw_G2E) f(movw_I2E)
#define INSTR_TERNARY(f)

def_all_EXEC_ID();
