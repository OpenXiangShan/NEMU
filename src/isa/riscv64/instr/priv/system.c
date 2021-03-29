#include <rtl/rtl.h>
#include <cpu/difftest.h>
#include <cpu/cpu.h>

__attribute__((cold))
int rtl_sys_slow_path(Decode *s, rtlreg_t *dest, const rtlreg_t *src1, uint32_t id, rtlreg_t *jpc) {
  uint32_t funct3 = s->isa.instr.i.funct3;
  if (funct3 == 0) {
    // priv
    IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
    if (id == 0) { // ecall
      rtl_trap(s, s->pc, 8 + cpu.mode);
      rtl_mv(s, jpc, t0);
    } else {
      rtl_hostcall(s, HOSTCALL_PRIV, jpc, src1, NULL, id);
    }
    int is_jmp = (id != 0x120) && (id != 0x105); // sfence.vma and wfi
    return is_jmp;
  }

  save_globals(s);
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 3));

  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, NULL, id);
  int imm = funct3 & 0x4;
  int op  = funct3 & 0x3;
  if (imm) rtl_li(s, s1, s->isa.instr.i.rs1);
  else rtl_mv(s, s1, src1);
  switch (op) {
    case 2: rtl_or(s, s1, s0, s1); break;
    case 3: rtl_not(s, s1, s1); rtl_and(s, s1, s0, s1); break;
  }
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, NULL, id);
  rtl_mv(s, dest, s0);
  return 0;
}
