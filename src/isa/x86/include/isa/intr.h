#include "reg.h"

static inline bool isa_istatus(void) {
  return cpu.IF;
}
