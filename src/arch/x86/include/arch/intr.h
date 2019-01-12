#include "reg.h"

static inline bool arch_istatus(void) {
  return cpu.IF;
}
