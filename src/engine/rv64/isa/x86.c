#ifdef __ISA_x86__

#include <cpu/decode.h>
#include <rtl/rtl.h>
#include <isa.h>

uint32_t reg_ptr2idx(DecodeExecState *s, const rtlreg_t* dest) {
  rtlreg_t* gpr_start = (rtlreg_t *)cpu.gpr;
  rtlreg_t* gpr_end = (void *)gpr_start + sizeof(cpu.gpr);

  if (dest >= gpr_start && dest < gpr_end)
    return (dest - gpr_start)|0x10;//from x16 to x23

#define CASE(ptr, idx) if (dest == ptr) return idx;
  CASE(rz, 0)
  CASE(s0, 2)
  CASE(s1, 3)
  CASE(t0, 4)
  CASE(t1, 5)
  CASE(&id_src1->val, 7)
  CASE(&id_src2->val, 8)
  CASE(&id_dest->val, 9)
  CASE(&s->isa.mbr, 10)
  CASE(&cpu.CF, 13)
  CASE(&cpu.OF, 14)
  CASE(&cpu.ZF, 15)
  CASE(&cpu.SF, 1)
  panic("bad ptr = %p", dest);
}

#endif
