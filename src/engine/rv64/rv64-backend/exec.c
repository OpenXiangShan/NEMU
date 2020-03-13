#include <isa/riscv64.h>

extern void (*rv64_memcpy_from_frontend)(paddr_t dest, void *src, size_t n);
extern void (*rv64_getregs)(void *c);
extern void (*rv64_setregs)(const void *c);
extern void (*rv64_exec)(uint64_t n);

#define RV64_EXEC_PC (16 * 1024) // skip bbl

// set rv64.pc and execute
void rv64_exec_code(uint64_t pc, int nr_instr) {
  CPU_state r;
  rv64_getregs(&r);
  r.pc = pc;
  rv64_setregs(&r);
  rv64_exec(nr_instr);
}

vaddr_t rv64_exec_trans_buffer(void *buf, int nr_instr) {
  // copy code to rv64 interpreter to execute it
  rv64_memcpy_from_frontend(RV64_EXEC_PC, buf, sizeof(uint32_t) * nr_instr);

  rv64_exec_code(RV64_EXEC_PC, nr_instr);

  CPU_state r;
  rv64_getregs(&r);
  uint64_t pc_end = RV64_EXEC_PC + sizeof(uint32_t) * nr_instr;
  while (r.pc != pc_end) {
    // if it is the case, we may trigger exception during the execution above
    rv64_exec(1);
    rv64_getregs(&r);
    assert(r.pc != 0x28c);
  }

  return r.gpr[30]._64;
}
