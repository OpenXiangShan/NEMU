#include <monitor/difftest.h>
#include "../local-include/csr.h"

static inline void csr_difftest() {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 3);
#endif
}

static inline bool csr_check(DecodeExecState *s, uint32_t addr) {
  switch (addr) {
    case 0xc01:  // time
    case 0x001:  // fflags
    case 0x002:  // frm
    case 0x003:  // fcsr
      rtl_li(s, s0, cpu.pc);
      rtl_hostcall(s, HOSTCALL_TRAP, s0, s0, EX_II);
      rtl_jr(s, s0);
      return false;
  }
  return true;
}

static inline def_EHelper(csrrw) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_difftest();
  rtl_hostcall(s, HOSTCALL_CSR, (id_dest->reg == 0) ? NULL : ddest, dsrc1, addr);
  print_asm_template3("csrrw");
}

static inline def_EHelper(csrrs) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_difftest();
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, addr);
  if (id_src1->reg != 0) {
    rtl_or(s, s1, s0, dsrc1);
    rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, addr);
  }
  if (ddest != NULL) rtl_mv(s, ddest, s0);
  print_asm_template3("csrrs");
}

static inline def_EHelper(csrrc) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_difftest();
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, addr);
  if (id_src1->reg != 0) {
    rtl_not(s, s1, dsrc1);
    rtl_and(s, s1, s0, s1);
    rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, addr);
  }
  if (ddest != NULL) rtl_mv(s, ddest, s0);
  print_asm_template3("csrrc");
}

static inline def_EHelper(csrrwi) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_difftest();
  rtl_li(s, s0, id_src1->imm);
  rtl_hostcall(s, HOSTCALL_CSR, (id_dest->reg == 0) ? NULL : ddest, s0, addr);
  print_asm_template3("csrrwi");
}

static inline def_EHelper(csrrsi) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_difftest();
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, addr);
  if (id_src1->imm != 0) {
    rtl_ori(s, s1, s0, id_src1->imm);
    rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, addr);
  }
  if (ddest != NULL) rtl_mv(s, ddest, s0);
  print_asm_template3("csrrsi");
}

static inline def_EHelper(csrrci) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_difftest();
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, addr);
  if (id_src1->imm != 0) {
    rtl_andi(s, s1, s0, ~id_src1->imm);
    rtl_hostcall(s, HOSTCALL_CSR, NULL, s1, addr);
  }
  if (ddest != NULL) rtl_mv(s, ddest, s0);
  print_asm_template3("csrrci");
}

static inline def_EHelper(priv) {
  uint32_t type = s->isa.instr.csr.csr;
  switch (type) {
    case 0:
      rtl_li(s, s0, cpu.pc);
      rtl_hostcall(s, HOSTCALL_TRAP, s0, s0, 8 + cpu.mode);
      rtl_jr(s, s0);
      print_asm("ecall");
      break;
    case 0x120:
      print_asm("sfence.vma");
      break;
    case 0x105:
      //panic("Executing wfi now will exit NEMU\n"
      //    "TODO: how to let NEMU execute wfi as REF in DiffTest?");
      print_asm("wfi");

      // let the clock go quickly to reduce idle time in Linux
#if !_SHARE
      void clint_intr();
      clint_intr();
#endif
      break;
    case 0x102:
      rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, type);
      rtl_jr(s, s0);
      print_asm("sret");
      break;
    case 0x302:
      rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, type);
      rtl_jr(s, s0);
      print_asm("mret");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
}

static inline def_EHelper(fence) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
}
