#include <cpu/decode.h>
#include <rtl/rtl.h>

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap)

#define INSTR_UNARY(f)
#define INSTR_BINARY(f)
#define INSTR_TERNARY(f)

def_all_EXEC_ID();
