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
  FPCALL_I32ToF,
};

#define FPCALL_CMD(op, w) (((op) << 16) | (w))
#define FPCALL_OP(cmd) ((cmd) >> 16)
#define FPCALL_W(cmd)  ((cmd) & 0x3)

#endif
