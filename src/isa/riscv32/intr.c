#include "cpu/rtl.h"

void raise_intr(uint8_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  TODO();
}

bool isa_query_intr(void) {
  return false;
}
