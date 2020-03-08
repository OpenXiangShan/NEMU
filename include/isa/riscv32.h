#ifndef __ISA_RISCV32_H__
#define __ISA_RISCV32_H__

#include <common.h>

// memory
#define IMAGE_START 0x100000
#define PMEM_BASE 0x80000000

// reg

typedef struct {
  union {
    rtlreg_t _32;
  } gpr[32];

  vaddr_t pc;
  vaddr_t stvec;
  vaddr_t scause;
  vaddr_t sepc;
  vaddr_t sscratch;
  union {
    struct {
      uint32_t uie : 1;
      uint32_t sie : 1;
      uint32_t pad0: 2;
      uint32_t upie: 1;
      uint32_t spie: 1;
      uint32_t pad1: 2;
      uint32_t spp : 1;
      uint32_t dontcare :21;
    };
    uint32_t val;
  } sstatus;
  union {
    struct {
      uint32_t ppn :22;
      uint32_t asid: 9;
      uint32_t mode: 1;
    };
    uint32_t val;
  } satp;

  bool INTR;
} CPU_state;

// decode
struct ISADecodeInfo {
  union {
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      uint32_t rs2       : 5;
      uint32_t funct7    : 7;
    } r;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      int32_t  simm11_0  :12;
    } i;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t imm4_0    : 5;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      uint32_t rs2       : 5;
      int32_t  simm11_5  : 7;
    } s;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t imm11     : 1;
      uint32_t imm4_1    : 4;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      uint32_t rs2       : 5;
      uint32_t imm10_5   : 6;
      int32_t  simm12    : 1;
    } b;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t imm31_12  :20;
    } u;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t imm19_12  : 8;
      uint32_t imm11     : 1;
      uint32_t imm10_1   :10;
      int32_t  simm20    : 1;
    } j;
    struct {
      uint32_t pad7      :20;
      uint32_t csr       :12;
    } csr;
    uint32_t val;
  } instr;
};

#endif
