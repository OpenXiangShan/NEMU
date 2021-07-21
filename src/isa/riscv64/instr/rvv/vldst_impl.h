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

#define VLD(mode, is_signed, s) vld(mode, is_signed, s);

void vld(int mode, int is_signed, Decode *s);
// vector store
#define VST(mode) vst(mode, s);
void vst(int mode, Decode *s);