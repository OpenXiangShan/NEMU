#include <common.h>
#include <monitor/monitor.h>
#include <stdlib.h>
#include <isa.h>
#include <monitor/difftest.h>

#define BUF_SIZE 13000 //8192

uint32_t trans_buffer[BUF_SIZE] = {};
int trans_buffer_index = 0;
int tran_is_jmp = false;

static void clear_trans_buffer() { trans_buffer_index = 0; }
void asm_print(vaddr_t ori_pc, int instr_len, bool print_flag);
vaddr_t rv64_exec_trans_buffer(void *buf, int nr_instr);
void rv64_guest_getregs(void *cpu);

void write_ins(uint32_t ins) {
  assert(trans_buffer_index < BUF_SIZE);
  trans_buffer[trans_buffer_index++]=ins;
}

void mainloop() {
  nemu_state.state = NEMU_RUNNING;
  while (1) {
    __attribute__((unused)) vaddr_t ori_pc = cpu.pc;
    __attribute__((unused)) vaddr_t seq_pc = isa_exec_once();

#ifdef DEBUG
    asm_print(ori_pc, seq_pc - ori_pc, true);
#endif

#ifndef DIFF_TEST
    if (tran_is_jmp) {
#endif
      vaddr_t next_pc = rv64_exec_trans_buffer(trans_buffer, trans_buffer_index);
      if (tran_is_jmp) cpu.pc = next_pc;
    //  Log("new basic block pc = %x", cpu.pc);
      clear_trans_buffer();
      tran_is_jmp = false;
#ifndef DIFF_TEST
    }
#endif

#ifdef DIFF_TEST
    rv64_guest_getregs(&cpu);
    difftest_step(ori_pc, cpu.pc);
#endif
    if (nemu_state.state != NEMU_RUNNING) break;
  }

  // get cpu.eax to determine whether we hit good trap
  rv64_guest_getregs(&cpu);
  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s\33[0m at pc = " FMT_WORD "\n\n",
          (nemu_state.state == NEMU_ABORT ? "\33[1;31mABORT" :
           (nemu_state.halt_ret == 0 ? "\33[1;32mHIT GOOD TRAP" : "\33[1;31mHIT BAD TRAP")),
          nemu_state.halt_pc);
      if (nemu_state.state == NEMU_ABORT) abort();
  }
}
