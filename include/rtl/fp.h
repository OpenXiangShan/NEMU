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
  FPCALL_W16,
  FPCALL_W32,
  FPCALL_W64,
  FPCALL_W128,
  FPCALL_W80,
  FPCALL_W16_to_32,
  FPCALL_W32_to_64,
  FPCALL_SRC1_W16_to_32,
  FPCALL_SRC2_W16_to_32,
  FPCALL_SRC1_W32_to_64,
  FPCALL_SRC2_W32_to_64,
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
  FPCALL_RSQRT7,
  FPCALL_REC7,
  FPCALL_CLASS,
  
  FPCALL_MADD,
  FPCALL_NMADD,
  FPCALL_MSUB,
  FPCALL_NMSUB,
  FPCALL_MACC,
  FPCALL_NMACC,
  FPCALL_MSAC,
  FPCALL_NMSAC,

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

  FPCALL_NEED_RM,  // separator

  FPCALL_UADD,
  
  FPCALL_MAX,
  FPCALL_MIN,
  FPCALL_LE,
  FPCALL_EQ,
  FPCALL_LT,
  FPCALL_GE,
  FPCALL_NE,
  FPCALL_GT,

  FPCALL_SGNJ,
  FPCALL_SGNJN,
  FPCALL_SGNJX,

  FPCALL_FToU,
  FPCALL_FToS,
  FPCALL_FToUT,
  FPCALL_FToST,
  FPCALL_UToF,
  FPCALL_SToF,

  FPCALL_FToDU,
  FPCALL_FToDS,
  FPCALL_FToDUT,
  FPCALL_FToDST,
  FPCALL_UToDF,
  FPCALL_SToDF,
  FPCALL_FToDF,

  FPCALL_DFToU,
  FPCALL_DFToS,
  FPCALL_DFToUT,
  FPCALL_DFToST,
  FPCALL_DUToF,
  FPCALL_DSToF,
  FPCALL_DFToF,
  FPCALL_DFToFR
};

#define FPCALL_CMD(op, w) (((op) << 16) | (w))
#define FPCALL_OP(cmd) ((cmd) >> 16)
#define FPCALL_W(cmd)  ((cmd) & 0xf)

#endif
