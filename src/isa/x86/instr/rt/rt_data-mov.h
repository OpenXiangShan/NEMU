def_REHelper(mov) {
  rtl_mv(s, ddest, dsrc1);
}

def_REHelper(push) {
  rtl_push(s, ddest);
}

def_REHelper(lea) {
  rtl_addi(s, ddest, s->isa.mbase, s->isa.moff);
}
