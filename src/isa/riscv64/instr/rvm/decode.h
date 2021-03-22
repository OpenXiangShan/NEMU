def_THelper(rvm) {
  int index = s->isa.instr.r.funct3;
  switch (index) {
    TAB(0, mul)   TAB(1, mulh)  TAB(2,mulhsu) TAB(3, mulhu)
    TAB(4, div)   TAB(5, divu)  TAB(6, rem)   TAB(7, remu)
  }
  return EXEC_ID_inv;
}

def_THelper(rvm32) {
  int index = s->isa.instr.r.funct3;
  switch (index) {
    TAB(0, mulw)
    TAB(4, divw) TAB(5, divuw) TAB(6, remw)  TAB(7, remuw)
  }
  return EXEC_ID_inv;
}
