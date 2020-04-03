#ifndef __TRAN_H__
#define __TRAN_H__

#include <common.h>

extern int tran_next_pc;
enum { NEXT_PC_SEQ, NEXT_PC_JMP, NEXT_PC_BRANCH, NEXT_PC_END };

extern void (*backend_memcpy_from_frontend)(paddr_t dest, void *src, size_t n);
extern void (*backend_getregs)(void *c);
extern void (*backend_setregs)(const void *c);
extern void (*backend_exec)(uint64_t n);

#define BBL_MAX_SIZE (16 * 1024)

// scratch pad memory, whose address space is [0, 64)
#define spm(op, reg, offset) concat(rv64_, op)(reg, x0, offset)
#define SPM_X86_REG 0    // x86 byte/word register write

enum { x0 = 0 };

// static register allocation
#if defined(__ISA_x86__)
enum { tmp0 = 30, mask32 = 24, mask16 = 25 };
#elif defined(__ISA_mips32__)
enum { tmp0 = 1, mask32 = 28, mask16 = 0 };
#define REG_SPILLING
#elif defined(__ISA_riscv32__)
enum { tmp0 = 3, mask32 = 4, mask16 = 0 };
//#define REG_SPILLING
#endif

#endif
