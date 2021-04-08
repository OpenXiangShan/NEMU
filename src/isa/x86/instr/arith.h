def_DEWBWHelper(addl_G2E ,  G2E, add, E, l);
def_DEWBWHelper(addw_G2E ,  G2E, add, E, w);
def_DEWBWHelper(addb_G2E ,  G2E, add, E, b);
def_DEWBWHelper(addl_E2G ,  E2G, add, r, l);
def_DEWBWHelper(addw_E2G ,  E2G, add, r, w);
def_DEWBWHelper(addb_E2G ,  E2G, add, r, b);
def_DEWBWHelper(addl_I2a ,    r, add, r, l);
def_DEWBWHelper(addw_I2a ,    r, add, r, w);
def_DEWBWHelper(addl_SI2E,    E, add, E, l);
def_DEWBWHelper(addw_SI2E,    E, add, E, w);

def_DEWBWHelper(subl_G2E ,  G2E, sub, E, l);
def_DEWBWHelper(subw_G2E ,  G2E, sub, E, w);
def_DEWBWHelper(subl_E2G ,  E2G, sub, r, l);
def_DEWBWHelper(subw_E2G ,  E2G, sub, r, w);
def_DEWBWHelper(subl_I2E ,    E, sub, E, l);
def_DEWBWHelper(subw_I2E ,    E, sub, E, w);
def_DEWBWHelper(subl_SI2E,    E, sub, E, l);
def_DEWBWHelper(subw_SI2E,    E, sub, E, w);

def_DEWBWHelper(adcl_G2E ,  G2E, adc, E, l);
def_DEWBWHelper(adcw_G2E ,  G2E, adc, E, w);
def_DEWBWHelper(adcl_E2G ,  E2G, adc, r, l);
def_DEWBWHelper(adcw_E2G ,  E2G, adc, r, w);
def_DEWBWHelper(adcl_SI2E,    E, adc, E, l);
def_DEWBWHelper(adcw_SI2E,    E, adc, E, w);

def_DEWBWHelper(sbbl_G2E ,  G2E, sbb, E, l);
def_DEWBWHelper(sbbw_G2E ,  G2E, sbb, E, w);
def_DEWBWHelper(sbbl_E2G ,  E2G, sbb, r, l);
def_DEWBWHelper(sbbw_E2G ,  E2G, sbb, r, w);
def_DEWBWHelper(sbbl_SI2E,    E, sbb, E, l);
def_DEWBWHelper(sbbw_SI2E,    E, sbb, E, w);

def_DEWHelper(cmpl_G2E ,  G2E, cmp, l);
def_DEWHelper(cmpw_G2E ,  G2E, cmp, w);
def_DEWHelper(cmpb_G2E ,  G2E, cmp, b);
def_DEWHelper(cmpl_E2G ,  E2G, cmp, l);
def_DEWHelper(cmpw_E2G ,  E2G, cmp, w);
def_DEWHelper(cmpl_I2a ,    r, cmp, l);
def_DEWHelper(cmpw_I2a ,    r, cmp, w);
def_DEWHelper(cmpb_I2a ,    r, cmp, b);
def_DEWHelper(cmpl_I2E ,    E, cmp, l);
def_DEWHelper(cmpw_I2E ,    E, cmp, w);
def_DEWHelper(cmpb_I2E ,    E, cmp, b);
def_DEWHelper(cmpl_SI2E,    E, cmp, l);
def_DEWHelper(cmpw_SI2E,    E, cmp, w);

def_DEWBWHelper(incl_r, r, inc, r, l);
def_DEWBWHelper(incw_r, r, inc, r, w);
def_DEWBWHelper(incl_E, E, inc, E, l);
def_DEWBWHelper(incw_E, E, inc, E, w);

def_DEWBWHelper(decl_r, r, dec, r, l);
def_DEWBWHelper(decw_r, r, dec, r, w);
def_DEWBWHelper(decl_E, E, dec, E, l);
def_DEWBWHelper(decw_E, E, dec, E, w);
def_DEWBWHelper(decb_E, E, dec, E, b);

def_DEWHelper(negl_E, E, neg, l);
def_DEWHelper(negw_E, E, neg, w);

def_DEWHelper(mull_E, E, mul, l);
def_DEWHelper(mulw_E, E, mul, w);

def_DEWHelper(imull_E, E, imul1, l);
def_DEWHelper(imulw_E, E, imul1, w);
def_DEWBWHelper(imull_E2G, E2G, imul2, r, l);
def_DEWBWHelper(imulw_E2G, E2G, imul2, r, w);
def_DEWBWHelper(imull_I_E2G, E_src, imul3, r, l);
def_DEWBWHelper(imulw_I_E2G, E_src, imul3, r, w);

def_DEWHelper(divl_E, E, div, l);
def_DEWHelper(divw_E, E, div, w);

def_DEWHelper(idivl_E, E, idiv, l);
def_DEWHelper(idivw_E, E, idiv, w);

#if 0
static inline def_EHelper(xadd) {
  rtl_add(s, s0, ddest, dsrc1);
#ifdef LAZY_CC
  rtl_set_lazycc_src1(s, dsrc1);
  rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_ADD, id_dest->width);
#else
  rtl_update_ZFSF(s, s0, id_dest->width);
  if (id_dest->width != 4) {
    rtl_andi(s, s0, s0, 0xffffffffu >> ((4 - id_dest->width) * 8));
  }
  rtl_is_add_carry(s, s1, s0, ddest);
  rtl_set_CF(s, s1);
  rtl_is_add_overflow(s, s1, s0, ddest, dsrc1, id_dest->width);
  rtl_set_OF(s, s1);
#endif
  operand_write(s, id_src1, ddest);
  operand_write(s, id_dest, s0);
  print_asm_template2(xadd);
}
#endif
