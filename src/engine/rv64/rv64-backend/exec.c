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
  }

  return r.gpr[30]._64;
}

void rv64_guest_getregs(void *cpu) {
  CPU_state r;
  rv64_getregs(&r);
  uint32_t *x86 = cpu;
  int i;
  for (i = 0; i < 8; i ++) {
    x86[i] = r.gpr[i + 0x10]._64;
  }
}

void rv64_guest_setregs(void *cpu) {
  CPU_state r;
  rv64_getregs(&r);
  uint32_t *x86 = cpu;
  int i;
  for (i = 0; i < 8; i ++) {
    r.gpr[i + 0x10]._64 = x86[i];
  }
  rv64_setregs(&r);
}

void init_rv64_reg() {
  CPU_state r;
  rv64_getregs(&r);
  r.gpr[24]._64 = 0x00000000fffffffful;
  r.gpr[25]._64 = 0x000000000000fffful;
  rv64_setregs(&r);
}
