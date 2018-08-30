#include "cpu/exec.h"

void diff_test_skip_qemu();
void diff_test_skip_nemu();

make_EHelper(lidt) {
  TODO();

  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  TODO();

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  TODO();

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#if defined(DIFF_TEST)
  diff_test_skip_qemu();
#endif
}

make_EHelper(int) {
  TODO();

  print_asm("int %s", id_dest->str);

#if defined(DIFF_TEST)
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  TODO();

  print_asm("iret");
}

uint32_t pio_read_l(ioaddr_t);
uint32_t pio_read_w(ioaddr_t);
uint32_t pio_read_b(ioaddr_t);
void pio_write_l(ioaddr_t, uint32_t);
void pio_write_w(ioaddr_t, uint32_t);
void pio_write_b(ioaddr_t, uint32_t);

extern bool diff_test_fix;

make_EHelper(in) {
//  TODO();

  uint32_t val;
  switch (id_dest->width) {
    case 1: val = pio_read_b(id_src->val); break;
    case 2: val = pio_read_w(id_src->val); break;
    case 4: val = pio_read_l(id_src->val); break;
    default: assert(0);
  }

  rtl_li(&t0, val);
  operand_write(id_dest, &t0);

#if defined(DIFF_TEST)
  diff_test_skip_qemu();
#endif

  print_asm_template2(in);
}

make_EHelper(out) {
//  TODO();

  switch (id_dest->width) {
    case 1: pio_write_b(id_dest->val, id_src->val); break;
    case 2: pio_write_w(id_dest->val, id_src->val); break;
    case 4: pio_write_l(id_dest->val, id_src->val); break;
    default: assert(0);
  }

#if defined(DIFF_TEST)
  diff_test_skip_qemu();
#endif

  print_asm_template2(out);
}
