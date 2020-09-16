#ifndef __X86_MMU_H__
#define __X86_MMU_H__

#include <common.h>

typedef union GateDescriptor {
  struct {
    uint32_t offset_15_0      : 16;
    uint32_t dont_care0       : 16;
    uint32_t dont_care1       : 15;
    uint32_t present          : 1;
    uint32_t offset_31_16     : 16;
  };
  uint32_t val;
} GateDesc;

enum { MODE_R0, MODE_R1, MODE_R2, MODE_R3 };

#define return_on_mem_ex() do { if (cpu.mem_exception != 0) return; } while (0)

#endif
