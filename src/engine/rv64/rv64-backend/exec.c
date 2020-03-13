#include <isa/riscv64.h>

extern void (*rv64_memcpy_from_frontend)(paddr_t dest, void *src, size_t n);
extern void (*rv64_getregs)(void *c);
extern void (*rv64_setregs)(const void *c);
extern void (*rv64_exec)(uint64_t n);

vaddr_t rv64_exec_code(void *code, int nr_instr) {
  const paddr_t rv64_start = 0;
  // copy code to rv64 interpreter to execute it
  rv64_memcpy_from_frontend(rv64_start, code, sizeof(uint32_t) * nr_instr);

  // set rv64.pc
  CPU_state r;
  rv64_getregs(&r);
  r.pc = rv64_start;
  rv64_setregs(&r);

  rv64_exec(nr_instr);

  rv64_getregs(&r);
  return r.gpr[30]._64;
}
