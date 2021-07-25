#include <common.h>
#ifdef CONFIG_RVV_010

#ifndef __RISCV64_VLDST_IMPL_H__
#define  __RISCV64_VLDST_IMPL_H__

#include "cpu/exec.h"
#include "vreg.h"
#include "../local-include/csr.h"
#include "../local-include/rtl.h"
#include <stdio.h>
#include "../local-include/intr.h"
#include <setjmp.h>

#define id_src (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)

// vector load
#define MODE_UNIT    0
#define MODE_STRIDED 1
#define MODE_INDEXED 2

#define VLD(mode, is_signed, s, mmu_mode) vld(mode, is_signed, s, mmu_mode);

void vld(int mode, int is_signed, Decode *s, int mmu_mode);
// vector store
#define VST(mode, mmu_mode) vst(mode, s, mmu_mode);
void vst(int mode, Decode *s, int mmu_mode);

#endif // __RISCV64_VLDST_IMPL_H__

#endif // CONFIG_RVV_010