#ifndef __ISA_RISCV64_H__
#define __ISA_RISCV64_H__

#include <common.h>

// memory
#ifdef __ENGINE_rv64__
#define riscv64_IMAGE_START 0x100000
#else
#define riscv64_IMAGE_START 0x0
#endif
#define riscv64_PMEM_BASE 0x80000000

// #define XIANGSHAN_DEBUG
// #define ENABLE_DISAMBIGUATE
#define FORCE_RAISE_PF

// reg
struct DisambiguationState {
  uint64_t exceptionNo;
  uint64_t mtval;
  uint64_t stval;
};

typedef struct {
  union {
    uint64_t _64;
  } gpr[32];

  union {
    uint64_t _64;
  } fpr[32];

  uint64_t pc;
  uint64_t mstatus, mcause, mepc;
  uint64_t sstatus, scause, sepc;
  
  uint64_t satp, mip, mie, mscratch, sscratch, mideleg, medeleg;

  uint64_t mtval, stval, mtvec, stvec;
  uint64_t mode;

  bool amo;
  int mem_exception;

  // for LR/SC
  uint64_t lr_addr;
  uint64_t lr_valid;

  bool INTR;

  // Disambiguation
  bool need_disambiguate;
  struct DisambiguationState disambiguation_state;
} riscv64_CPU_state;

// decode
typedef struct {
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
      int32_t  simm31_12 :20;
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
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t rm        : 3;
      uint32_t rs1       : 5;
      uint32_t rs2       : 5;
      uint32_t fmt       : 2;
      uint32_t funct5    : 5;
    } fp;
    uint32_t val;
  } instr;
} riscv64_ISADecodeInfo;

#define riscv64_has_mem_exception() (cpu.mem_exception != 0)

#endif
