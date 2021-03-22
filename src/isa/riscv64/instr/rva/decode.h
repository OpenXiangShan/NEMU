#ifdef CONFIG_DEBUG
def_THelper(atomic) {
  uint32_t funct5 = s->isa.instr.r.funct7 >> 2;
  int funct3 = s->isa.instr.r.funct3;
  assert((funct3 == 2) || (funct3 == 3));
  int width = funct3 & 1;
#define pair(x, y) (((x) << 1) | (y))
  switch (pair(funct5, width)) {
    TAB(pair(0b00000, 0), amoadd_w)  TAB(pair(0b00001, 0), amoswap_w)
    TAB(pair(0b00010, 0), lr_w)      TAB(pair(0b00011, 0), sc_w)
    TAB(pair(0b00100, 0), amoxor_w)  TAB(pair(0b01000, 0), amoor_w)
    TAB(pair(0b01100, 0), amoand_w)
    TAB(pair(0b10000, 0), amomin_w)  TAB(pair(0b10100, 0), amomax_w)
    TAB(pair(0b11000, 0), amominu_w) TAB(pair(0b11100, 0), amomaxu_w)

    TAB(pair(0b00000, 1), amoadd_d)  TAB(pair(0b00001, 1), amoswap_d)
    TAB(pair(0b00010, 1), lr_d)      TAB(pair(0b00011, 1), sc_d)
    TAB(pair(0b00100, 1), amoxor_d)  TAB(pair(0b01000, 1), amoor_d)
    TAB(pair(0b01100, 1), amoand_d)
    TAB(pair(0b10000, 1), amomin_d)  TAB(pair(0b10100, 1), amomax_d)
    TAB(pair(0b11000, 1), amominu_d) TAB(pair(0b11100, 1), amomaxu_d)
  }
#undef pair
  return EXEC_ID_inv;
}
#endif
