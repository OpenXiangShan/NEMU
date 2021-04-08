def_REHelper(mov) {
  rtl_mv(s, ddest, dsrc1);
}

def_REHelper(push) {
  rtl_push(s, ddest);
}

def_REHelper(pop) {
  rtl_pop(s, ddest);
}

def_REHelper(lea) {
  rtl_addi(s, ddest, &s->isa.mbr, s->isa.moff);
}

def_REHelper(movsb) {
  rtl_sext(s, ddest, dsrc1, 1);
}

def_REHelper(movsw) {
  rtl_sext(s, ddest, dsrc1, 2);
}
