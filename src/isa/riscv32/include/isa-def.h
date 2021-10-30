#ifndef __ISA_RISCV32_H__
#define __ISA_RISCV32_H__

#include <common.h>

typedef struct {
  rtlreg_t gpr[32];

  vaddr_t pc;
#ifndef __ICS_EXPORT
  vaddr_t mtvec;
  vaddr_t mcause;
  vaddr_t mepc;
  vaddr_t mscratch;
  int mode;
  union {
    struct {
      uint32_t pad0: 3;
      uint32_t mie : 1;
      uint32_t pad1: 3;
      uint32_t mpie: 1;
      uint32_t pad2: 3;
      uint32_t mpp : 2;
      uint32_t dontcare :19;
    };
    uint32_t val;
  } mstatus;
  union {
    struct {
      uint32_t ppn :22;
      uint32_t asid: 9;
      uint32_t mode: 1;
    };
    uint32_t val;
  } satp;

  bool INTR;
#endif
} riscv32_CPU_state;

// decode
typedef struct {
  union {
    uint32_t val;
  } instr;
} riscv32_ISADecodeInfo;

#ifdef __ICS_EXPORT
#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)
#else
#define isa_mmu_state() (cpu.satp.mode ? MMU_TRANSLATE : MMU_DIRECT)
#define isa_mmu_check(vaddr, len, type) isa_mmu_state()
#endif

#endif
