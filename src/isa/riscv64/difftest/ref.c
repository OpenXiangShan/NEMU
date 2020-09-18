#include <isa.h>
#include <cpu/exec.h>
#include "../local-include/csr.h"
#include "difftest.h"

static void csr_prepare() {
  // return;
  cpu.mstatus = mstatus->val;
  cpu.mcause  = mcause->val;
  cpu.mepc    = mepc->val;

  rtlreg_t temp;
  csr_read(&temp, 0x100); // sstatus
  cpu.sstatus = temp;
  cpu.scause  = scause->val;
  cpu.sepc    = sepc->val;

  cpu.satp     = satp->val;
  cpu.mip      = mip->val;
  cpu.mie      = mie->val;
  cpu.mscratch = mscratch->val;
  cpu.sscratch = sscratch->val;
  cpu.mideleg  = mideleg->val;
  cpu.medeleg  = medeleg->val;
}

static void csr_writeback() {
  // return;
  mstatus->val = cpu.mstatus;
  mcause ->val = cpu.mcause ;
  mepc   ->val = cpu.mepc   ;
  //sstatus->val = cpu.sstatus;  // sstatus is a shadow of mstatus
  scause ->val = cpu.scause ;
  sepc   ->val = cpu.sepc   ;

  satp->val     = cpu.satp;
  mip->val      = cpu.mip;
  mie->val      = cpu.mie;
  mscratch->val = cpu.mscratch;
  sscratch->val = cpu.sscratch;
  mideleg->val  = cpu.mideleg;
  medeleg->val  = cpu.medeleg;
}

void isa_difftest_getregs(void *r) {
  csr_prepare();
  memcpy(r, &cpu, DIFFTEST_REG_SIZE);
}

void isa_difftest_setregs(const void *r) {
  memcpy(&cpu, r, DIFFTEST_REG_SIZE);
  csr_writeback();
}

void isa_difftest_sync(uint64_t *sync) {
  // sync[0] lrscValid
  uint64_t lrscValid = sync[0];
  // sync[1] lrscAddr
  // uint64_t lrscAddr = sync[1];
  cpu.lr_valid = lrscValid;
  // printf("sync valid %lx addr %lx  current valid %lx addr %lx\n", lrscValid, lrscAddr, cpu.lr_valid, cpu.lr_addr);
  // if(!lrscValid && cpu.lr_valid && isSC){
  //   cpu.lr_valid = 0;
  //   // printf("NEMU skipped a timeout sc\n");
  //   if(lrscAddr != cpu.lr_addr){
  //     // printf("[Warning] NEMU skipped a timeout sc, but lr_addr 0x%lx-0x%lx does not match\n", lrscAddr, cpu.lr_addr);
  //   }
  // }
}

void isa_difftest_raise_intr(word_t NO) {
  DecodeExecState s;
  s.is_jmp = 0;
  s.isa = (ISADecodeInfo) { 0 };

  void raise_intr(DecodeExecState *s, word_t NO, vaddr_t epc);
  raise_intr(&s, NO, cpu.pc);
  update_pc(&s);
}
