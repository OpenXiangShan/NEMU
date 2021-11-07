#include <rtl/rtl.h>
#include <cpu/cpu.h>

__attribute__((cold))
def_rtl(amo_slow_path, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2) {
  uint32_t instr = s->isa.instr.val;
  uint32_t funct5 = BITS(instr, 31, 27);
  int width = BITS(instr, 12, 12) ? 8 : 4;

  if (funct5 == 0b00010) { // lr
    vaddr_t paddr = *src1;
    if (isa_mmu_check(*src1, width, MEM_TYPE_READ) == MMU_TRANSLATE) {
      paddr = isa_mmu_translate(*src1, width, MEM_TYPE_READ) | ((*src1) & PAGE_MASK);
      check_ex();
    }
    rtl_lms(s, dest, src1, 0, width, MMU_DYNAMIC);
    cpu.lr_addr = paddr;
    return;
  } else if (funct5 == 0b00011) { // sc
    vaddr_t paddr = *src1;
    if (isa_mmu_check(*src1, width, MEM_TYPE_WRITE) == MMU_TRANSLATE) {
      paddr = isa_mmu_translate(*src1, width, MEM_TYPE_WRITE) | ((*src1) & PAGE_MASK);
      check_ex();
    }
    // should check overlapping instead of equality
    int success = cpu.lr_addr == paddr;
    if (success) { rtl_sm(s, src2, src1, 0, width, MMU_DYNAMIC); }
    cpu.lr_addr = (word_t)-1;
    rtl_li(s, dest, !success);
    return;
  }

  cpu.amo = true;
  rtl_lms(s, s0, src1, 0, width, MMU_DYNAMIC);
  check_ex();
  switch (funct5) {
    case 0b00001: rtl_mv (s, s1, src2); break;
    case 0b00000: rtl_add(s, s1, s0, src2); break;
    case 0b01000: rtl_or (s, s1, s0, src2); break;
    case 0b01100: rtl_and(s, s1, s0, src2); break;
    case 0b00100: rtl_xor(s, s1, s0, src2); break;
    case 0b11100: *s1 = (*s0 > *src2 ? *s0 : *src2); break;
    case 0b10100: *s1 = ((sword_t)*s0 > (sword_t)*src2 ? *s0 : *src2); break;
    case 0b11000: *s1 = (*s0 < *src2 ? *s0 : *src2); break;
    case 0b10000: *s1 = ((sword_t)*s0 < (sword_t)*src2 ? *s0 : *src2); break;
    default: assert(0);
  }
  rtl_sm(s, s1, src1, 0, width, MMU_DYNAMIC);
  check_ex();
  rtl_mv(s, dest, s0);
  cpu.amo = false;
}
