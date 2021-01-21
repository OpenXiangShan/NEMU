#include <isa.h>

void dev_raise_intr() {
#ifndef __ICS_EXPORT
  cpu.INTR = true;
#endif
}
