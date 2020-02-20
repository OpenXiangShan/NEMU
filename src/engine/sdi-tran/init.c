#include "sdi.h"

void init_tl();
void init_jmp();

void init_engine() {
  init_tl();
  init_jmp();
  assert(*tl_base==1);
  *tl_base=0;
}
