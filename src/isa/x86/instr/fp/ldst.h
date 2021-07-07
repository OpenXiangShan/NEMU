#include "fp-include.h"

def_EHelper(fld1) {
  ftop_push();
  *dfdest = 0x3ff0000000000000ull;
}

def_EHelper(flds) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_flm(s, dfdest, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  rtl_fcvt_f32_to_f64(s, dfdest, dfdest);
  ftop_push();
}

def_EHelper(fstps) {
  rt_decode_mem(s, id_dest, false, 0);
  rtl_fcvt_f64_to_f32(s, &s->isa.fptmp, dfdest);
  rtl_fsm(s, &s->isa.fptmp, &s->isa.mbr, s->isa.moff, 4, MMU_DYNAMIC);
  ftop_pop();
}
