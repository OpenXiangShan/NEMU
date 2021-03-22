#ifndef __RTL_FP_H__
#define __RTL_FP_H__

enum {
  FPCALL_W32,
  FPCALL_W64,
  FPCALL_W128,
  FPCALL_W80,
};

enum {
  FPCALL_ADD,
  FPCALL_SUB,
  FPCALL_MUL,
  FPCALL_DIV,
  FPCALL_SQRT,

  FPCALL_LE,
  FPCALL_EQ,
  FPCALL_LT,

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
};

#define FPCALL_CMD(op, w) (((op) << 16) | (w))
#define FPCALL_OP(cmd) ((cmd) >> 16)
#define FPCALL_W(cmd)  ((cmd) & 0x3)

#endif
