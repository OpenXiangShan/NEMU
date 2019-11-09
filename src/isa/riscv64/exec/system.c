#include "cpu/exec.h"
#include "../csr.h"

make_EHelper(csrrw) {
  uint32_t addr = id_src2->val;
  csr_read(&s0, addr);
  rtl_sr(id_dest->reg, &s0, 8);
  csr_write(addr, &id_src->val);

  print_asm_template3("csrrw");
}

make_EHelper(csrrs) {
  uint32_t addr = id_src2->val;
  csr_read(&s0, addr);
  rtl_sr(id_dest->reg, &s0, 8);
  if (id_src->reg != 0) {
    rtl_or(&s0, &s0, &id_src->val);
    csr_write(addr, &s0);
  }

  print_asm_template3("csrrs");
}

make_EHelper(csrrc) {
  uint32_t addr = id_src2->val;
  csr_read(&s0, addr);
  rtl_sr(id_dest->reg, &s0, 8);
  if (id_src->reg != 0) {
    rtl_not(&s1, &id_src->val);
    rtl_and(&s0, &s0, &s1);
    csr_write(addr, &s0);
  }

  print_asm_template3("csrrc");
}

extern void raise_intr(word_t NO, vaddr_t epc);

make_EHelper(priv) {
  uint32_t type = decinfo.isa.instr.csr;
  switch (type) {
    case 0:
      raise_intr(8 + cpu.mode, cpu.pc);
      print_asm("ecall");
      break;
    case 0x102:
      mstatus->sie = mstatus->spie;
      mstatus->spie = 1;
      change_mode(mstatus->spp);
      mstatus->mpp = MODE_U;
      rtl_li(&s0, sepc->val);
      rtl_jr(&s0);
      cpu.lr = false;
      print_asm("sret");
      break;
    case 0x120:
      print_asm("sfence.vma");
      break;
    case 0x302:
      mstatus->mie = mstatus->mpie;
      mstatus->mpie = 1;
      change_mode(mstatus->mpp);
      mstatus->mpp = MODE_U;
      rtl_li(&s0, mepc->val);
      rtl_jr(&s0);
      cpu.lr = false;
      print_asm("mret");
      break;
    default: panic("unimplemented priv instruction type = 0x%x", type);
  }

  difftest_skip_dut(1, 2);
}
