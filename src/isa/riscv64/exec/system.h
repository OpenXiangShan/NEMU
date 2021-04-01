#include <monitor/difftest.h>
#include "../local-include/csr.h"

static inline bool csr_check(DecodeExecState *s, uint32_t addr) {
  switch (addr) {
    case 0xc01:  // time
    case 0x3b4:
    // case 0xb03:
      raise_intr(s, EX_II, cpu.pc);
      return false;
  }
  return true;
}

static inline make_EHelper(csrrw) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_read(s0, addr);
  csr_write(addr, dsrc1);
  rtl_mv(s, ddest, s0);

  print_asm_template3("csrrw");
}

static inline make_EHelper(csrrs) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_read(s0, addr);
  if (id_src1->reg != 0) {
    rtl_or(s, s1, s0, dsrc1);
    csr_write(addr, s1);
  }
  rtl_mv(s, ddest, s0);

  print_asm_template3("csrrs");
}

static inline make_EHelper(csrrc) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_read(s0, addr);
  if (id_src1->reg != 0) {
    rtl_not(s, s1, dsrc1);
    rtl_and(s, s1, s0, s1);
    csr_write(addr, s1);
  }
  rtl_mv(s, ddest, s0);

  print_asm_template3("csrrc");
}

static inline make_EHelper(csrrwi) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_read(s0, addr);
  rtl_li(s, s1, id_src1->imm);
  csr_write(addr, s1);
  rtl_mv(s, ddest, s0);

  print_asm_template3("csrrwi");
}

static inline make_EHelper(csrrsi) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_read(s0, addr);
  if (id_src1->reg != 0) {
    rtl_ori(s, s1, s0, id_src1->imm);
    csr_write(addr, s1);
  }
  rtl_mv(s, ddest, s0);

  print_asm_template3("csrrsi");
}

static inline make_EHelper(csrrci) {
  uint32_t addr = id_src2->imm;
  if (!csr_check(s, addr)) return;
  csr_read(s0, addr);
  if (id_src1->reg != 0) {
    rtl_andi(s, s1, s0, ~id_src1->imm);
    csr_write(addr, s1);
  }
  rtl_mv(s, ddest, s0);

  print_asm_template3("csrrci");
}

static inline make_EHelper(priv) {
  uint32_t type = s->isa.instr.csr.csr;
  switch (type) {
    case 0:
      raise_intr(s, 8 + cpu.mode, cpu.pc);
      print_asm("ecall");
      break;
    case 0x102:
      mstatus->sie = mstatus->spie;
#ifdef __DIFF_REF_QEMU__
      // this is bug of QEMU
      mstatus->spie = 0;
#else
      mstatus->spie = 1;
#endif
      change_mode(mstatus->spp);
      mstatus->spp = MODE_U;
      rtl_li(s, s0, sepc->val);
      rtl_jr(s, s0);
      print_asm("sret");
      break;
    case 0x120:
      rtl_sfence();
      print_asm("sfence.vma");
      break;
    case 0x105:
      //panic("Executing wfi now will exit NEMU\n"
      //    "TODO: how to let NEMU execute wfi as REF in DiffTest?");
      print_asm("wfi");

      // let the clock go quickly to reduce idle time in Linux
#if !_SHARE
      void clint_intr(void);
      clint_intr();
#endif
      break;
    case 0x302:
      mstatus->mie = mstatus->mpie;
#ifdef __DIFF_REF_QEMU__
      // this is bug of QEMU
      mstatus->mpie = 0;
#else
      mstatus->mpie = 1;
#endif
      change_mode(mstatus->mpp);
      mstatus->mpp = MODE_U;
      rtl_li(s, s0, mepc->val);
      rtl_jr(s, s0);
      print_asm("mret");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
}

static inline make_EHelper(fence) {
#ifndef __DIFF_REF_NEMU__
  difftest_skip_dut(1, 2);
#endif
}
