#include <isa.h>

void dev_raise_intr() {
  cpu.INTR = true;
}
