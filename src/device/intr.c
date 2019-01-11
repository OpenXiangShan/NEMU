#include "nemu.h"

void dev_raise_intr() {
  cpu.INTR = true;
}
