#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#include <common.h>

#ifndef __ICS_EXPORT
enum {
  NEMU_EXEC_RUNNING = 0, // unused by longjmp()
  NEMU_EXEC_END,
  NEMU_EXEC_AGAIN,
  NEMU_EXEC_EXCEPTION
};

void cpu_exec(uint64_t n);
IFNDEF(CONFIG_TARGET_AM, __attribute__((noreturn))) void longjmp_exec(int cause);
IFNDEF(CONFIG_TARGET_AM, __attribute__((noreturn))) void longjmp_exception(int ex_cause);

enum {
  SYS_STATE_UPDATE = 1,
  SYS_STATE_FLUSH_TCACHE = 2,
};
void set_sys_state_flag(int flag);
void mmu_tlb_flush(vaddr_t vaddr);

struct Decode;
void save_globals(struct Decode *s);
void fetch_decode(struct Decode *s, vaddr_t pc);
#else
void cpu_exec(uint64_t n);
#endif

#endif
