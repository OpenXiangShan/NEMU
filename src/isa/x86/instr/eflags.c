#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#include <cpu/difftest.h>

#define EFLAGS_BIT_CF 0
#define EFLAGS_BIT_PF 2
#define EFLAGS_BIT_ZF 6
#define EFLAGS_BIT_SF 7
#define EFLAGS_BIT_IF 9
#define EFLAGS_BIT_DF 10
#define EFLAGS_BIT_OF 11

#define _EFLAGS(f) f(OF) f(IF) f(SF) f(ZF) f(CF) f(DF) f(PF)
#define __f(flag) concat(EFLAGS_MASK_, flag) = 1 << concat(EFLAGS_BIT_, flag),
enum {
  MAP(_EFLAGS, __f)
#undef __f
#define __f(flag) | concat(EFLAGS_MASK_, flag)
  EFLAGS_MASK_ALL = 0 MAP(_EFLAGS, __f)
#undef __f
};

#define RTL_ENCODE(flag) \
  rtl_shli(s, t0, &cpu.flag, concat(EFLAGS_BIT_, flag)); \
  rtl_or(s, dest, dest, t0);

#define RTL_DECODE(flag) \
  rtl_shri(s, &cpu.flag, src, concat(EFLAGS_BIT_, flag)); \
  rtl_andi(s, &cpu.flag, &cpu.flag, 0x1);

#define ENCODE(flag) | (cpu.flag << (concat(EFLAGS_BIT_, flag)))
#define DECODE(flag) cpu.flag = (val >> (concat(EFLAGS_BIT_, flag))) & 1;

void rtl_compute_eflags(Decode *s, rtlreg_t *dest) {
  rtl_mv(s, dest, rz);
  MAP(_EFLAGS, RTL_ENCODE)
  rtl_ori(s, dest, dest, 0x2);
}

void rtl_set_eflags(Decode *s, const rtlreg_t *src) {
  MAP(_EFLAGS, RTL_DECODE)
}

uint32_t compute_eflags() {
  return 0x2 MAP(_EFLAGS, ENCODE);
}

void set_eflags(uint32_t val) {
  MAP(_EFLAGS, DECODE);
}

void difftest_fix_eflags(void *arg) {
#if defined(CONFIG_ENGINE_INTERPRETER) && !defined(CONFIG_DIFFTEST_REF_KVM)
#define EFLAGS_MASK_ID (1 << 21)
#define EFLAGS_MASK_AC (1 << 18)
#define EFLAGS_MASK_AF (1 << 4)
#define EFLAGS_FIX_MASK (EFLAGS_MASK_ID | EFLAGS_MASK_AC | EFLAGS_MASK_AF)
  uint32_t esp = (uintptr_t)arg;
  if (cpu.cr0.paging) {
    paddr_t pg_base = isa_mmu_translate(esp, 4, MEM_TYPE_WRITE);
    assert((pg_base & PAGE_MASK) == MEM_RET_OK);
    esp = pg_base | (esp & PAGE_MASK);
  }
  uint32_t flags;
  ref_difftest_memcpy(esp, &flags, 4, DIFFTEST_TO_DUT);
  flags &= ~EFLAGS_FIX_MASK;
  ref_difftest_memcpy(esp, &flags, 4, DIFFTEST_TO_REF);
#endif
}
