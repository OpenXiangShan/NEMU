#ifndef __ISA_MIPS32_H__
#define __ISA_MIPS32_H__

#include <common.h>

// memory
#define mips32_IMAGE_START 0x100000
#define mips32_PMEM_BASE 0x80000000

// reg
typedef struct {
  union {
    rtlreg_t _32;
  } gpr[32];

  union {
    struct {
      uint32_t ie:  1;
      uint32_t exl: 1;
      uint32_t dontcare: 30;
    };
    uint32_t val;
  } status;

  rtlreg_t lo, hi;
  uint32_t badvaddr;
  uint32_t cause;
  vaddr_t pc;
  uint32_t epc;

  union {
    struct {
      uint32_t ASID: 8;
      uint32_t pad : 5;
      uint32_t VPN2:19;
    };
    uint32_t val;
  } entryhi;
  uint32_t entrylo0, entrylo1;
  uint32_t index;

  int mem_exception;

  bool INTR;
} mips32_CPU_state;

// decode
typedef struct {
  union {
    struct {
      int32_t  simm   : 16;
      uint32_t rt     :  5;
      uint32_t rs     :  5;
      uint32_t opcode :  6;
    } i;
    struct {
      uint32_t imm    : 16;
      uint32_t rt     :  5;
      uint32_t rs     :  5;
      uint32_t opcode :  6;
    } iu;
    struct {
      uint32_t target : 26;
      uint32_t opcode :  6;
    } j;
    struct {
      uint32_t func   : 6;
      uint32_t sa     : 5;
      uint32_t rd     : 5;
      uint32_t rt     : 5;
      uint32_t rs     : 5;
      uint32_t opcode : 6;
    } r;
    uint32_t val;
  } instr;
} mips32_ISADecodeInfo;

#define isa_vaddr_check(vaddr, type, len) ((vaddr & 0x80000000u) == 0 ? MEM_RET_NEED_TRANSLATE : MEM_RET_OK)
#define mips32_has_mem_exception() (cpu.mem_exception != 0)

#endif
