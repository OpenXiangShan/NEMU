#include "reg.h"

#define EX_SYSCALL 8
#define EX_TLB_LD 2
#define EX_TLB_ST 3
#define TLB_REFILL 0x80

static inline bool isa_istatus(void) {
  return false;
}
