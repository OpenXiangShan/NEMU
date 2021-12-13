#ifndef __INTR_H__
#define __INTR_H__

#include <cpu/decode.h>

enum {
  EX_IAM, // instruction address misaligned
  EX_IAF, // instruction address fault
  EX_II,  // illegal instruction
  EX_BP,  // breakpoint
  EX_LAM, // load address misaligned
  EX_LAF, // load address fault
  EX_SAM, // store/amo address misaligned
  EX_SAF, // store/amo address fault
  EX_ECU, // ecall from U-mode
  EX_ECS, // ecall from S-mode
  EX_RS0, // reserved
  EX_ECM, // ecall from M-mode
  EX_IPF, // instruction page fault
  EX_LPF, // load page fault
  EX_RS1, // reserved
  EX_SPF, // store/amo page fault
};

// now NEMU does not support EX_IAM,
// so it may ok to use EX_IAM to indicate a successful memory access
#define MEM_OK 0

word_t raise_intr(word_t NO, vaddr_t epc);
#define return_on_mem_ex() do { if (cpu.mem_exception != MEM_OK) return; } while (0)
bool intr_deleg_S(word_t exceptionNO);
#define INTR_TVAL_REG(ex) (*((intr_deleg_S(ex)) ? (word_t *)stval : (word_t *)mtval))

#endif
