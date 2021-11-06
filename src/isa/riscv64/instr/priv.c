#include <rtl/rtl.h>
#include <cpu/difftest.h>
#include <cpu/cpu.h>
#include "../local-include/csr.h"

extern bool csr_is_exist(uint32_t csrid);

__attribute__((cold))
int rtl_sys_slow_path(Decode *s, rtlreg_t *dest, const rtlreg_t *src1, uint32_t id, rtlreg_t *jpc) {
  uint32_t instr = s->isa.instr.val;
  uint32_t funct3 = BITS(instr, 14, 12);
  id &= 0xfff;
  if (funct3 == 0) {
    // priv
    IFNDEF(CONFIG_PERF_OPT, word_t dnpc = 0; jpc = &dnpc);
    IFDEF(CONFIG_DIFFTEST_REF_QEMU, difftest_skip_dut(1, 2));
    if (id == 0) { // ecall
      rtl_hostcall(s, HOSTCALL_TRAP_THIS, jpc, NULL, NULL, 8 + cpu.mode);
    } else {
      rtl_hostcall(s, HOSTCALL_PRIV, jpc, src1, NULL, id);
    }
    int is_jmp = (id != 0x120) && (id != 0x105); // sfence.vma and wfi
    IFNDEF(CONFIG_PERF_OPT, if (is_jmp) cpu.pc = dnpc);
    return is_jmp;
  }

  if (!csr_is_exist(id)) {
    extern void rt_inv(Decode *s);
    rt_inv(s);
    return 0;
  }
  IFDEF(CONFIG_DIFFTEST_REF_QEMU, difftest_skip_dut(1, 3));

  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, NULL, id);
  int imm = funct3 & 0x4;
  int op  = funct3 & 0x3;
  int rs1 = BITS(instr, 19, 15);
  if (imm) rtl_li(s, s1, rs1);
  else rtl_mv(s, s1, src1);
  switch (op) {
    case 2: rtl_or(s, s1, s0, s1); break;
    case 3: rtl_not(s, s1, s1); rtl_and(s, s1, s0, s1); break;
  }
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, NULL, id);
  rtl_mv(s, dest, s0);
  return 0;
}
