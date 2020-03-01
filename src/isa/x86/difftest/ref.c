#include <isa.h>
#include "difftest.h"

void isa_difftest_getregs(void *r) {
  memcpy(r, &cpu, DIFFTEST_REG_SIZE);
}

void isa_difftest_setregs(const void *r) {
  memcpy(&cpu, r, DIFFTEST_REG_SIZE);
}
