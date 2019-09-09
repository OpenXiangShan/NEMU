#ifndef __RISCV64_REG_H__
#define __RISCV64_REG_H__

#include "common.h"
#include "memory.h"

#define PC_START (0x80000000u + IMAGE_START)

typedef struct {
  union {
    rtlreg_t _64;
    uint32_t _32[2];
    uint16_t _16[4];
    uint8_t _8[8];
  } gpr[32];

  vaddr_t pc;
  vaddr_t mtvec;
  vaddr_t mcause;
  vaddr_t mepc;
  union {
    struct {
      uint64_t uie : 1;
      uint64_t sie : 1;
      uint64_t pad0: 1;
      uint64_t mie : 1;
      uint64_t upie: 1;
      uint64_t spie: 1;
      uint64_t pad1: 1;
      uint64_t mpie: 1;
      uint64_t spp : 1;
      uint64_t pad2: 2;
      uint64_t mpp : 2;
    };
    uint64_t val;
  } mstatus;
  union {
    struct {
      uint64_t ppn :44;
      uint64_t asid:16;
      uint64_t mode: 4;
    };
    uint64_t val;
  } satp;

  bool INTR;
} CPU_state;

static inline int check_reg_index(int index) {
  assert(index >= 0 && index < 32);
  return index;
}

#define reg_l(index) (cpu.gpr[check_reg_index(index)]._64)

static inline const char* reg_name(int index, int width) {
  extern const char* regsl[];
  assert(index >= 0 && index < 32);
  return regsl[index];
}

#endif
