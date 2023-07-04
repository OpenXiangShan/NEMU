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
  WNMACC, WMACCSU, WMACCUS, VEXT
};

void vp_set_dirty();
void arthimetic_instr(int opcode, int is_signed, int widening, int narrow, int dest_mask, Decode *s);
void mask_instr(int opcode, Decode *s);
void reduction_instr(int opcode, int is_signed, int wide, Decode *s);

#define ARTHI(opcode, is_signed) arthimetic_instr(opcode, is_signed, 0, 0, 0, s);
#define ARTHI_WIDE(opcode, is_signed) arthimetic_instr(opcode, is_signed, 1, 0, 0, s);
#define ARTHI_MASK(opcode, is_signed) arthimetic_instr(opcode, is_signed, 0, 0, 1, s);
#define ARTHI_NARROW(opcode, is_signed, narrow) arthimetic_instr(opcode, is_signed, 0, narrow, 0, s);

#define MASKINSTR(opcode) mask_instr(opcode, s);

#define REDInstr(opcode, is_signed) reduction_instr(opcode, is_signed, 0, s);
#define REDInstr_WIDE(opcode, is_signed) reduction_instr(opcode, is_signed, 1, s);

#endif // __RISCV64_VCOMPUTE_IMPL_H__

#endif // CONFIG_RVV