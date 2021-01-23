#ifndef __TRAN_H__
#define __TRAN_H__

#include <common.h>

extern int tran_next_pc;
enum { NEXT_PC_SEQ, NEXT_PC_JMP, NEXT_PC_BRANCH, NEXT_PC_END };

extern void (*backend_memcpy)(paddr_t dest, void *src, size_t n, bool to_backend);
extern void (*backend_regcpy)(void *c, bool to_backend);
extern void (*backend_exec)(uint64_t n);

#define BBL_MAX_SIZE (16 * 1024)

// There are 4 types of indices.
// * rvidx -  the index of rv64 register, can be zero.
//            used to construct rv instructions
// * spmidx - the index of variable in SPM masked with SPM_IDX_MASK, which is non-zero.
//            used to allocate rtl registers which can not be mapped to rv64 registers
// * varidx - (mapped_to_spm ? smpidx : rvidx)
// * tmpidx - the index of record of temporal registers in spill.c

#define SPMIDX_MASK 0x20

#include "../../isa/riscv64/rocc/sdi.h"

#define spm_read(rd, idx) rv64_rocc3(SDI_ROCC_SPM_READ, idx, rd, 0, 0)
#define spm_write(rs, idx) rv64_rocc3(SDI_ROCC_SPM_WRITE, idx, 0, rs, 0)

// scratch pad memory, whose address space is [riscv64_PMEM_BASE, riscv64_PMEM_BASE + 64)
#define spm(op, reg, offset) concat(rv64_, op)(reg, spm_base, offset)
#define SPM_X86_REG 0    // x86 byte/word register write

enum { x0 = 0 };

// static register allocation
#if defined(__ISA_x86__)
enum { tmp0 = 30, mask32 = 24, mask16 = 25, spm_base = 26, tmp_reg1 = 0, tmp_reg2 = 0 };
#elif defined(__ISA_mips32__)
enum { tmp0 = 2, mask32 = 28, mask16 = 0, spm_base = 0, tmp_reg1 = 26, tmp_reg2 = 27 };
#define REG_SPILLING
#elif defined(__ISA_riscv32__)
enum { tmp0 = 26, mask32 = 27, mask16 = 0, spm_base = 0, tmp_reg1 = 3, tmp_reg2 = 4 };
#define REG_SPILLING
#elif defined(__ISA_riscv64__)
enum { tmp0 = 27, mask32 = 0, mask16 = 0, spm_base = 0, tmp_reg1 = 3, tmp_reg2 = 4 };
#define REG_SPILLING
#endif

#endif
