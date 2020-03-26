#include <common.h>
#include <monitor/monitor.h>
#include <stdlib.h>
#include <isa.h>
#include <monitor/difftest.h>
#include "tran.h"
#include "spill.h"

#define BUF_SIZE 13000 //8192

uint32_t trans_buffer[BUF_SIZE] = {};
int trans_buffer_index = 0;
int tran_next_pc = NEXT_PC_SEQ;

static void clear_trans_buffer() { trans_buffer_index = 0; }
void asm_print(vaddr_t ori_pc, int instr_len, bool print_flag);
vaddr_t rv64_exec_trans_buffer(void *buf, int nr_instr);
void guest_getregs(CPU_state *cpu);

void write_ins(uint32_t ins) {
  assert(trans_buffer_index < BUF_SIZE);
  trans_buffer[trans_buffer_index++]=ins;
}

void mainloop() {
  tmp_regs_init();
  nemu_state.state = NEMU_RUNNING;
  uint64_t total_instr = 0;
  while (1) {
    __attribute__((unused)) vaddr_t ori_pc = cpu.pc;
    __attribute__((unused)) vaddr_t seq_pc = isa_exec_once();
    for (int i = 0; i < TMP_REG_NUM; i++) {
      tmp_regs[i].dirty = 0;
    }

    if (nemu_state.state != NEMU_RUNNING) tran_next_pc = NEXT_PC_END;

#ifdef DEBUG
    asm_print(ori_pc, seq_pc - ori_pc, true);
#endif

#ifndef DIFF_TEST
    if (tran_next_pc != NEXT_PC_SEQ) {
#endif
      vaddr_t next_pc = rv64_exec_trans_buffer(trans_buffer, trans_buffer_index);
      total_instr += trans_buffer_index;

      if (tran_next_pc == NEXT_PC_END) {
        // get cpu.eax and interpret `nemu_trap` again
        guest_getregs(&cpu);
        cpu.pc = ori_pc;
        isa_exec_once();
        break;
      }

      if (tran_next_pc != NEXT_PC_SEQ) cpu.pc = next_pc;
    //  Log("new basic block pc = %x", cpu.pc);
      clear_trans_buffer();
      tran_next_pc = NEXT_PC_SEQ;
#ifndef DIFF_TEST
    }
#endif

#ifdef DIFF_TEST
    guest_getregs(&cpu);
    difftest_step(ori_pc, cpu.pc);
    if (nemu_state.state == NEMU_ABORT) break;
#endif
  }

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;

    case NEMU_END: case NEMU_ABORT:
      Log("nemu: %s\33[0m at pc = " FMT_WORD "\n\n",
          (nemu_state.state == NEMU_ABORT ? "\33[1;31mABORT" :
           (nemu_state.halt_ret == 0 ? "\33[1;32mHIT GOOD TRAP" : "\33[1;31mHIT BAD TRAP")),
          nemu_state.halt_pc);
      if (nemu_state.state == NEMU_ABORT) abort();
  }

  Log("#(rv64 instr) = %ld\n", total_instr);
}
