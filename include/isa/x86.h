#ifndef __ISA_X86_H__
#define __ISA_X86_H__

#include <common.h>

#ifndef __ICS_EXPORT
//#define LAZY_CC
#endif

// memory
#define x86_IMAGE_START 0x100000
#ifdef __ENGINE_rv64__
#define x86_PMEM_BASE 0x80000000
#else
#define x86_PMEM_BASE 0x0
#endif

// reg
#ifndef __ICS_EXPORT
/* the Control Register 0 */
typedef union CR0 {
  struct {
    uint32_t protect_enable      : 1;
    uint32_t dont_care           : 30;
    uint32_t paging              : 1;
  };
  uint32_t val;
} CR0;

/* the Control Register 3 (physical address of page directory) */
typedef union CR3 {
  struct {
    uint32_t pad : 12;
    uint32_t ppn : 20;
  };
  uint32_t val;
} CR3;
#endif

/* TODO: Re-organize the `CPU_state' structure to match the register
 * encoding scheme in i386 instruction format. For example, if we
 * access cpu.gpr[3]._16, we will get the `bx' register; if we access
 * cpu.gpr[1]._8[1], we will get the 'ch' register. Hint: Use `union'.
 * For more details about the register encoding scheme, see i386 manual.
 */

typedef struct {
#ifdef __ICS_EXPORT
  struct {
    uint32_t _32;
    uint16_t _16;
    uint8_t _8[2];
  } gpr[8];

  /* Do NOT change the order of the GPRs' definitions. */

  /* In NEMU, rtlreg_t is exactly uint32_t. This makes RTL instructions
   * in PA2 able to directly access these registers.
   */
  rtlreg_t eax, ecx, edx, ebx, esp, ebp, esi, edi;

  vaddr_t pc;
#else
  union {
    union {
      uint32_t _32;
      uint16_t _16;
      uint8_t _8[2];
    } gpr[8];

  /* Do NOT change the order of the GPRs' definitions. */

  /* In NEMU, rtlreg_t is exactly uint32_t. This makes RTL instructions
   * in PA2 able to directly access these registers.
   */
    struct {
      rtlreg_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    };
  };

  vaddr_t pc;
  uint32_t eflags;

  rtlreg_t OF, CF, SF, ZF, IF, DF, PF;

#ifdef LAZY_CC
  rtlreg_t cc_dest, cc_src1, cc_src2;
  uint32_t cc_width;
  uint32_t cc_op;
#endif

  struct {
    union {
      struct {
        uint32_t rpl : 2;
        uint32_t ti  : 1;
        uint32_t idx :13;
      };
      uint16_t val;
    };
    // hidden part
    rtlreg_t base;
  } sreg[8];

  struct {
    uint32_t limit :16;
    uint32_t base  :32;
  } idtr, gdtr;

  union {
    rtlreg_t cr[4];
    struct {
      CR0 cr0;
      rtlreg_t cr1;
      rtlreg_t cr2;
      CR3 cr3;
    };
  };

  int mem_exception;
  word_t error_code;

  bool INTR;
#endif
} x86_CPU_state;

// decode
typedef struct {
  bool is_operand_size_16;
#define PREFIX_REP   1
#define PREFIX_REPNZ 2
  int rep_flags;
  uint8_t ext_opcode;
  const rtlreg_t *sreg_base;
  const rtlreg_t *mbase;
  rtlreg_t mbr;
  word_t moff;
} x86_ISADecodeInfo;

#define suffix_char(width) ((width) == 4 ? 'l' : ((width) == 1 ? 'b' : ((width) == 2 ? 'w' : '?')))
#ifdef __ICS_EXPORT
#define isa_vaddr_check(vaddr, type, len) (MEM_RET_OK)
#else
#define isa_vaddr_check(vaddr, type, len) (cpu.cr0.paging ? MEM_RET_NEED_TRANSLATE : MEM_RET_OK)
#endif
#define x86_has_mem_exception() (false)

#endif
