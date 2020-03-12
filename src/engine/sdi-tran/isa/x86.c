#include <cpu/decode.h>
#include <rtl/rtl.h>
#include <isa.h>

uint8_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest) {
  rtlreg_t* gpr_start = (rtlreg_t *)cpu.gpr;
  rtlreg_t* gpr_end = (void *)gpr_start + sizeof(cpu.gpr);

  if (dest >= gpr_start && dest < gpr_end)
    return (dest - gpr_start)|0x10;//from x16 to x23

#define CASE(ptr, idx) if (dest == ptr) return idx;
  CASE(s0, 2)
  CASE(s1, 3)
  CASE(t0, 4)
  CASE(t1, 5)
  CASE(ir, 6)
  CASE(dsrc1, 7)
  CASE(dsrc2, 8)
  CASE(ddest, 9)
  CASE(&id_src1->addr, 10)
  CASE(&id_src2->addr, 11)
  CASE(&id_dest->addr, 12)
  CASE(&cpu.CF, 13)
  CASE(&cpu.OF, 14)
  CASE(&cpu.ZF, 15)
  CASE(&cpu.SF, 1)
  panic("bad ptr = %p", dest);
}
