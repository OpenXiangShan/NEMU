#ifndef __ISA_X86_H__
#define __ISA_X86_H__

#include <common.h>

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

  uint64_t fpr[8];
  rtlreg_t ftop;
  rtlreg_t fsw,fcw;
  
#ifdef CONFIG_x86_CC_LAZY
  rtlreg_t cc_dest, cc_src1, cc_src2;
  uint32_t cc_width, cc_op, cc_dirty, cc_dynamic;
#endif

  union {
    __uint128_t _128;
    uint64_t _64[2];
    uint32_t _32[4];
  } xmm[8];

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
    uint32_t cr[4];
    struct {
      CR0 cr0;
      uint32_t cr1;
      uint32_t cr2;
      CR3 cr3;
    };
  };

  int mem_exception;
  word_t error_code;
  IFNDEF(CONFIG_PA, int lock);

  bool INTR;
#endif
} x86_CPU_state;

// decode
typedef struct {
  uint8_t instr[16];
  uint8_t *p_instr;
  uint16_t opcode;
#define PREFIX_REP   1
#define PREFIX_REPNZ 2
  int8_t width;
  uint8_t rep_flags;
  uint8_t flag_def;
  uint8_t flag_use;
  bool is_operand_size_16;
  const rtlreg_t *sreg_base;
  const rtlreg_t *mbase;
  const rtlreg_t *midx;
  word_t moff;
  word_t mscale;
  rtlreg_t mbr;
} x86_ISADecodeInfo;

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { R_AX, R_CX, R_DX, R_BX, R_SP, R_BP, R_SI, R_DI };
enum { R_AL, R_CL, R_DL, R_BL, R_AH, R_CH, R_DH, R_BH };

enum { MODE_R0, MODE_R1, MODE_R2, MODE_R3 };

enum {
  // selectors
  CSR_ES, CSR_CS, CSR_SS, CSR_DS,
  CSR_FS, CSR_GS, CSR_TR, CSR_LDTR,
  // table registers
  CSR_IDTR, CSR_GDTR,
  // control registers
  CSR_CR0, CSR_CR1, CSR_CR2, CSR_CR3, CSR_CR4
};


enum { OP_TYPE_IMM, OP_TYPE_REG, OP_TYPE_MEM };

//#define suffix_char(width) ((width) == 4 ? 'l' : ((width) == 1 ? 'b' : ((width) == 2 ? 'w' : '?')))
#ifdef __ICS_EXPORT
#define isa_mmu_state() (MMU_DIRECT)
#else
#define isa_mmu_state() (cpu.cr0.paging ? MMU_TRANSLATE : MMU_DIRECT)
#endif
#define isa_mmu_check(vaddr, len, type) isa_mmu_state()

#endif
