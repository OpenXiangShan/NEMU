#ifndef __RTL_FP_H__
#define __RTL_FP_H__

#include <common.h>

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
  FPCALL_ROUNDINT,
  FPCALL_POW2,
  FPCALL_LOG2,
  FPCALL_MOD,
  FPCALL_ATAN,
};

#endif
