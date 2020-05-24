#include <isa/riscv64.h>
#include "../tran.h"

#define RV64_EXEC_PC (riscv64_PMEM_BASE + BBL_MAX_SIZE) // skip bbl

// set rv64.pc and execute
void backend_exec_code(uint64_t pc, int nr_instr) {
  riscv64_CPU_state r;
  backend_getregs(&r);
  r.pc = pc;
  backend_setregs(&r);
  backend_exec(nr_instr);
}

vaddr_t rv64_exec_trans_buffer(void *buf, int nr_instr, int npc_type) {
  // copy code to rv64 interpreter to execute it
  backend_memcpy_from_frontend(RV64_EXEC_PC, buf, sizeof(uint32_t) * nr_instr);

  // if the basic block is end with a branch instruction,
  // execute until the branch instruction
  // see rtl_jrelop() at rtl-basic.c
  int nr_exec = (npc_type == NEXT_PC_BRANCH ? nr_instr - 5 : nr_instr);
  backend_exec_code(RV64_EXEC_PC, nr_exec);

  riscv64_CPU_state r;
  backend_getregs(&r);
  uint64_t pc_end = RV64_EXEC_PC + sizeof(uint32_t) * nr_exec;
  while (r.pc != pc_end) {
    // if it is the case, we may trigger exception during the execution above
    backend_exec(1);
    backend_getregs(&r);
  }

  if (npc_type == NEXT_PC_BRANCH) {
    // execute the branch instruction and load x86.pc to x30
    backend_exec(3);
    backend_getregs(&r);
  }

  return r.gpr[tmp0]._64;
}
