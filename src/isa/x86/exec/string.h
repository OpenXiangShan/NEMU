static make_EHelper(movs) {
  rtl_lm(s, s0, &cpu.esi, 0, id_dest->width);
  rtl_sm(s, &cpu.edi, 0, s0, id_dest->width);

  rtl_addi(s, &cpu.esi, &cpu.esi, id_dest->width);
  rtl_addi(s, &cpu.edi, &cpu.edi, id_dest->width);

  print_asm("movs (%%esi), (%%edi)");
}
