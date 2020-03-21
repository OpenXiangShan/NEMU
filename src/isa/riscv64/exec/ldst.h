#include "../local-include/intr.h"

static inline make_EHelper(ld) {
  rtl_lm(s, s0, &id_src1->addr, 0, s->width);
  check_mem_ex();
  rtl_sr(s, id_dest->reg, s0, 0);

  switch (s->width) {
    case 8: print_asm_template2(ld); break;
    case 4: print_asm_template2(lwu); break;
    case 2: print_asm_template2(lhu); break;
    case 1: print_asm_template2(lbu); break;
    default: assert(0);
  }
}

// load sign value
static inline make_EHelper(lds) {
  rtl_lm(s, s0, &id_src1->addr, 0, s->width);
  check_mem_ex();
  rtl_sext(s, s0, s0, s->width);
  rtl_sr(s, id_dest->reg, s0, 0);

  switch (s->width) {
    case 4: print_asm_template2(lw); break;
    case 2: print_asm_template2(lh); break;
    case 1: print_asm_template2(lb); break;
    default: assert(0);
  }
}

static inline make_EHelper(st) {
  rtl_sm(s, &id_src1->addr, 0, ddest, s->width);
  check_mem_ex();

  switch (s->width) {
    case 8: print_asm_template2(sd); break;
    case 4: print_asm_template2(sw); break;
    case 2: print_asm_template2(sh); break;
    case 1: print_asm_template2(sb); break;
    default: assert(0);
  }
}
