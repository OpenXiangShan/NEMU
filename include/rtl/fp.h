/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#ifndef __RTL_FP_H__
#define __RTL_FP_H__

enum {
  FPCALL_W32,
  FPCALL_W64,
  FPCALL_W128,
  FPCALL_W80,
};

enum {
  FPCALL_RM_RNE,  // round to nearest, ties to even
  FPCALL_RM_RTZ,  // round towards zero
  FPCALL_RM_RDN,  // round down (towards -INF)
  FPCALL_RM_RUP,  // round up (towards +INF)
  FPCALL_RM_RMM,  // round to nearest, ties to max magnitude
};

enum {
  FPCALL_EX_NX = 0x01,  // inexact
  FPCALL_EX_UF = 0x02,  // underflow
  FPCALL_EX_OF = 0x04,  // overflow
  FPCALL_EX_DZ = 0x08,  // divide by zero
  FPCALL_EX_NV = 0x10,  // invalid operation
};

enum {
  FPCALL_ADD,
  FPCALL_SUB,
  FPCALL_MUL,
  FPCALL_DIV,
  FPCALL_SQRT,
  FPCALL_MADD,

  FPCALL_I32ToF,
  FPCALL_U32ToF,
  FPCALL_I64ToF,
  FPCALL_U64ToF,

  FPCALL_FToI32,
  FPCALL_FToU32,
  FPCALL_FToI64,
  FPCALL_FToU64,

  FPCALL_F32ToF64,
  FPCALL_F64ToF32,

  FPCALL_NEED_RM,  // seperator

  FPCALL_MAX,
  FPCALL_MIN,
  FPCALL_LE,
  FPCALL_EQ,
  FPCALL_LT,
};

#define FPCALL_CMD(op, w) (((op) << 16) | (w))
#define FPCALL_OP(cmd) ((cmd) >> 16)
#define FPCALL_W(cmd)  ((cmd) & 0x3)

#endif
