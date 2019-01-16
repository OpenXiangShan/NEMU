#include "cpu/rtl.h"
#include "cpu/exec.h"
#include "arch/intr.h"

CPU_state cpu;

rtlreg_t s0, s1, t0, t1, at;

/* shared by all helper functions */
DecodeInfo decinfo;

void decinfo_set_jmp(bool is_jmp) {
  decinfo.is_jmp = is_jmp;
}

make_EHelper(arch);

void exec_wrapper(bool print_flag) {
  vaddr_t ori_pc = cpu.pc;

#ifdef DEBUG
  decinfo.p = decinfo.asm_buf;
  decinfo.p += sprintf(decinfo.p, "%8x:   ", ori_pc);
#endif

  decinfo.seq_pc = ori_pc;
  exec_arch(&decinfo.seq_pc);

#ifdef DEBUG
  int instr_len = decinfo.seq_pc - ori_pc;
  sprintf(decinfo.p, "%*.s", 50 - (12 + 3 * instr_len), "");
  strcat(decinfo.asm_buf, decinfo.assembly);
  Log_write("%s\n", decinfo.asm_buf);
  if (print_flag) {
    puts(decinfo.asm_buf);
  }
#endif

  update_pc();

#define IRQ_TIMER 32
  void raise_intr(uint8_t, vaddr_t);
  if (cpu.INTR && arch_istatus()) {
    cpu.INTR = false;
    raise_intr(IRQ_TIMER, cpu.pc);
    update_pc();
  }

#if defined(DIFF_TEST)
  void difftest_step(vaddr_t pc);
  difftest_step(ori_pc);
#endif
}
