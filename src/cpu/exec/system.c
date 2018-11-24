#include "cpu/exec.h"

make_EHelper(lidt) {
  cpu.idtr.limit = vaddr_read(id_dest->addr, 2);
  cpu.idtr.base = vaddr_read(id_dest->addr + 2, 4);

  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  //TODO();
  rtl_lr(&cpu.cr[id_dest->reg], id_src->reg, 4);

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  //TODO();
  rtl_sr(id_dest->reg, &cpu.cr[id_src->reg], 4);

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#if defined(DIFF_TEST)
  difftest_skip_ref();
#endif
}

make_EHelper(int) {
  void raise_intr(uint8_t, vaddr_t);
  raise_intr(id_dest->val, *eip);

  print_asm("int %s", id_dest->str);

#if defined(DIFF_TEST) && defined(DIFF_TEST_QEMU)
  difftest_skip_dut();
#endif
}

make_EHelper(iret) {
  //TODO();
  rtl_pop(&t0);

  rtl_pop(&t1);
  cpu.cs = t1;

  void rtl_set_eflags(const rtlreg_t *src);
  rtl_pop(&t1);
  rtl_set_eflags(&t1);

  rtl_jr(&t0);

  difftest_skip_eflags(EFLAGS_MASK_ALL);

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
  difftest_skip_ref();
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
  difftest_skip_ref();
#endif

  print_asm_template2(out);
}
