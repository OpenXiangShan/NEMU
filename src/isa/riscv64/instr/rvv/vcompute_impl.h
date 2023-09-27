/***************************************************************************************
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#ifdef CONFIG_RVV

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
  OR, XOR, RGATHER, RGATHEREI16, SLIDEUP, SLIDEDOWN, ADC, MADC, SBC,
  MSBC, MERGE, MSEQ, MSNE, MSLTU, MSLT, MSLEU, MSLE,
  MSGTU, MSGT, SADDU, SADD, SSUBU, SSUB, AADD, SLL,
  ASUB, SMUL, SRL, SRA, SSRL, SSRA, NSRL, NSRA,
  NCLIPU, NCLIP, WREDSUMU, WREDSUM, DOTU, DOT, WSMACCU, WSMACC,
  WSMAXXSU, WSMACCUS,

  REDSUM, REDAND, REDOR, REDXOR, REDMINU, REDMIN, REDMAXU, REDMAX,
  EXT_X_V, MV_S_X, SLIDE1UP, SLIDE1DOWN, POPC, FIRST, MUNARYO, COMPRESS,
  MANDNOT, MAND, MOR, MXOR, MORNOT, MNAND, MNOR, MXNOR,
  DIVU, DIV, REMU, REM, MULHU, MUL, MULHSU, MULH, MULSU,
  MADD, NMSUB, MACC, NMSAC, MACCSU, MACCUS, WADDU, WADD, WSUBU, WSUB,
  WADDU_W, WADD_W, WSUBU_W, WSUB_W, WMULU, WMULSU, WMUL, WMACCU,
  WNMACC, WMACCSU, WMACCUS, VEXT,

  FADD, FREDUSUM, FSUB, FREDOSUM, FMIN, FREDMIN, FMAX, FREDMAX, FSGNJ,
  FSGNJN, FSGNJX, FSLIDE1UP, FSLIDE1DOWN, FMV_F_S, FMV_S_F, FCVT_XUF,
  FCVT_XF, FCVT_FXU, FCVT_FX, FCVT_RTZ_XUF, FCVT_RTZ_XF,
  FWCVT_XUF, FWCVT_XF, FWCVT_FXU, FWCVT_FX, FWCVT_FF, FWCVT_RTZ_XUF,
  FWCVT_RTZ_XF, FNCVT_XUF, FNCVT_XF, FNCVT_FXU, FNCVT_FX, FNCVT_FF,
  FNCVT_ROD_FF, FNCVT_RTZ_XUF, FNCVT_RTZ_XF, FSQRT, FRSQRT7,
  FREC7, FCLASS, FMERGE, MFEQ, MFLE, MFLT, MFNE, MFGT, MFGE,
  FDIV, FRDIV, FMUL, FRSUB, FMADD, FNMADD, FMSUB, FNMSUB, FMACC, FNMACC,
  FMSAC, FNMSAC, FWREDUSUM, FWREDOSUM, FWADD_W, FWSUB_W,
  FWMUL, FWMACC, FWNMACC, FWMSAC, FWNMSAC
};

enum fp_wop_t {
  noWidening,
  vsdWidening,
  vsWidening,
  vdWidening,
  vdNarrow
};

void vp_set_dirty();
void arthimetic_instr(int opcode, int is_signed, int widening, int narrow, int dest_mask, Decode *s);
void floating_arthimetic_instr(int opcode, int is_signed, int widening, int dest_mask, Decode *s);
void mask_instr(int opcode, Decode *s);
void reduction_instr(int opcode, int is_signed, int wide, Decode *s);
void float_reduction_instr(int opcode, int widening, Decode *s);
void float_reduction_step1(uint64_t src1, uint64_t src2, Decode *s);
void float_reduction_step2(uint64_t src, Decode *s);
void float_reduction_computing(Decode *s);

#define ARTHI(opcode, is_signed) arthimetic_instr(opcode, is_signed, 0, 0, 0, s);
#define ARTHI_WIDE(opcode, is_signed) arthimetic_instr(opcode, is_signed, 1, 0, 0, s);
#define ARTHI_MASK(opcode, is_signed) arthimetic_instr(opcode, is_signed, 0, 0, 1, s);
#define ARTHI_NARROW(opcode, is_signed, narrow) arthimetic_instr(opcode, is_signed, 0, narrow, 0, s);

#define FLOAT_ARTHI(opcode, is_signed) floating_arthimetic_instr(opcode, is_signed, noWidening, 0, s);
#define FLOAT_ARTHI_DWIDE(opcode, is_signed) floating_arthimetic_instr(opcode, is_signed, vdWidening, 0, s);
#define FLOAT_ARTHI_SDWIDE(opcode) floating_arthimetic_instr(opcode, 0, vsdWidening, 0, s);
#define FLOAT_ARTHI_SWIDE(opcode) floating_arthimetic_instr(opcode, 0, vsWidening, 0, s);
#define FLOAT_ARTHI_DNARROW(opcode, is_signed) floating_arthimetic_instr(opcode, is_signed, vdNarrow, 0, s);
#define FLOAT_ARTHI_MASK(opcode) floating_arthimetic_instr(opcode, 0, noWidening, 1, s);

#define MASKINSTR(opcode) mask_instr(opcode, s);

#define REDUCTION(opcode, is_signed) reduction_instr(opcode, is_signed, 0, s);
#define WREDUCTION(opcode, is_signed) reduction_instr(opcode, is_signed, 1, s);

#define FREDUCTION(opcode) float_reduction_instr(opcode, 0, s);
#define FWREDUCTION(opcode) float_reduction_instr(opcode, vsWidening, s);

#endif // __RISCV64_VCOMPUTE_IMPL_H__

#endif // CONFIG_RVV