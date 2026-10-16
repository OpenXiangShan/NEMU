#ifndef __RISCV64_VFNCVTXX8_H__
#define __RISCV64_VFNCVTXX8_H__

#include "vcompute_impl.h"

enum {
  VFNCVTXX8_FORMAT_INT8,
  VFNCVTXX8_FORMAT_E4M3,
  VFNCVTXX8_FORMAT_E5M2,
};

void vfncvtxx8_execute(Decode *s, int format);

#endif // __RISCV64_VFNCVTXX8_H__
