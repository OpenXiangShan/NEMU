#include <common.h>
#ifdef CONFIG_RVV_010

#ifndef __RISCV64_VCOMPUTE_IMPL_H__
#define __RISCV64_VCOMPUTE_IMPL_H__

#include "cpu/exec.h"
#include "vreg.h"
#include "../local-include/csr.h"
#include <stdio.h>
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include <setjmp.h>

#define id_src (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)



enum op_t {
  ADD, SUB, RSUB, MINU, MIN, MAXU, MAX, AND,
  OR, XOR, RGATHER, SLIDEUP, SLIDEDOWN, ADC, MADC, SBC,
  MSBC, MERGE, MSEQ, MSNE, MSLTU, MSLT, MSLEU, MSLE,
  MSGTU, MSGT, SADDU, SADD, SSUBU, SSUB, AADD, SLL,
  ASUB, SMUL, SRL, SRA, SSRL, SSRA, NSRL, NSRA,
  NCLIPU, NCLIP, WREDSUMU, WREDSUM, DOTU, DOT, WSMACCU, WSMACC,
  WSMAXXSU, WSMACCUS,

  REDSUM, REDAND, REDOR, REDXOR, REDMINU, REDMIN, REDMAXU, REDMAX,
  EXT_X_V, MV_S_X, SLIDE1UP, SLIDE1DOWN, MPOPC, VMFIRST, MUNARYO, COMPRESS,
  MANDNOT, MAND, MOR, MXOR, MORNOT, MNAND, MNOR, MXNOR,
  DIVU, DIV, REMU, REM, MULHU, MUL, MULHSU, MULH,
  MADD, NMSUB, MACC, NMSAC, WADDU, WADD, WSUBU, WSUB,
  WADDU_W, WADD_W, WSUBU_W, WSUB_W, WMULU, WMULSU, WMUL, WMACCU,
  WNMACC, WMACCSU, WMACCUS,
};
void vp_set_dirty();
void arthimetic_instr(int opcode, int is_signed, int dest_reg, Decode *s);
void mask_instr(int opcode, Decode *s);
void reduction_instr(int opcode, int is_signed, Decode *s);

#define ARTHI(opcode, is_signed) arthimetic_instr(opcode, is_signed, 0, s);
#define ARTHI_COMP(opcode, is_signed) arthimetic_instr(opcode, is_signed, 1, s);

#define MASKINSTR(opcode) mask_instr(opcode, s);

#define REDInstr(opcode, is_signed) reduction_instr(opcode, is_signed, s);

#endif // __RISCV64_VCOMPUTE_IMPL_H__

#endif // CONFIG_RVV_010