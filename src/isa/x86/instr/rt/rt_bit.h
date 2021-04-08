def_REHelper(bsr) {
#ifndef CONFIG_ENGINE_INTERPRETER
  panic("not support in engines other than interpreter");
#endif

  rtl_setrelopi(s, RELOP_EQ, s0, dsrc1, 0);
  rtl_set_ZF(s, s0);

  int bit = 31;
  if (*dsrc1 != 0) {
    while ((*dsrc1 & (1u << bit)) == 0) bit--;
    *ddest = bit;
    rtl_sr(s, id_dest->reg, ddest, width);
  }
}
