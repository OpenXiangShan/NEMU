#ifndef __ISA_RISCV64_H__
#define __ISA_RISCV64_H__

#include <common.h>

typedef struct {
  union {
    uint64_t _64;
  } gpr[32];

  vaddr_t pc;
#ifndef CONFIG_PA
  uint64_t mstatus, mepc, mtval, mcause;
  uint64_t          sepc, stval, scause;
#endif

#ifndef __ICS_EXPORT
  union {
    uint64_t _64;
  } fpr[32];


  uint8_t mode;

  bool amo;
  int mem_exception;

  // for LR/SC
  uint64_t lr_addr;

  bool INTR;
#endif
} riscv64_CPU_state;

// decode
typedef struct {
  union {
    uint32_t val;
  } instr;
} riscv64_ISADecodeInfo;

#ifndef __ICS_EXPORT
enum { MODE_U = 0, MODE_S, MODE_H, MODE_M };

int get_data_mmu_state();
#define isa_mmu_state() get_data_mmu_state()
#else
#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)
#endif

#endif
