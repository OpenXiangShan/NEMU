#include <cpu/decode.h>
#include <rtl/rtl.h>
#include <isa.h>

uint8_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest) {
  rtlreg_t* gpr_start = (rtlreg_t *)cpu.gpr;
  rtlreg_t* gpr_end = (void *)gpr_start + sizeof(cpu.gpr);

  if (dest >= gpr_start && dest < gpr_end)
    return (dest - gpr_start)|0x10;//from x16 to x23

#define CASE(ptr, idx) if (dest == ptr) return idx;
  CASE(s0, 1)
  CASE(s1, 2)
  CASE(t0, 3)
  CASE(t1, 4)
  CASE(ir, 5)
  CASE(dsrc1, 6)
  CASE(dsrc2, 7)
  CASE(ddest, 8)
  CASE(&id_src1->addr, 9)
  CASE(&id_src2->addr, 10)
  CASE(&id_dest->addr, 11)
  CASE(&cpu.CF, 12)
  CASE(&cpu.OF, 13)
  CASE(&cpu.ZF, 14)
  CASE(&cpu.SF, 15)
  panic("bad ptr = %p", dest);
}
