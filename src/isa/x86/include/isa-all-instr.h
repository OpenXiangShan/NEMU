#include <cpu/decode.h>
#include "../local-include/rtl.h"
#include "../instr/rt.h"

#define INSTR_NULLARY(f) \
  f(inv) f(nemu_trap) f(nop) \
  f(ret) f(leave) \
  f(cwtl) f(cltd) \
  f(movs)

#define INSTR_UNARY(f) \
  f(call) f(jcc) f(jmp) f(setcc) f(call_E) f(jmp_E) \
  f(push) f(pop) \
  f(inc) f(dec) f(neg) f(not) \
  f(mul) f(imul1) f(div) f(idiv)

#define INSTR_BINARY(f) \
  f(mov) f(add) f(sub) f(adc) f(sbb) f(cmp) \
  f(and) f(or) f(test) f(xor) f(shl) f(shr) f(sar) f(rol) f(ror) \
  f(lea) f(movzb) f(movzw) f(movsb) f(movsw) \
  f(imul2) \
  f(bsr) \
  f(in) f(out) \

#define INSTR_TERNARY(f) \
  f(imul3) f(shld) f(shrd)

def_all_EXEC_ID();
