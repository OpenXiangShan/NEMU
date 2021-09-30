#include "rvintrin.h"

def_EHelper(clz) {
  *ddest = _rv_clz(*dsrc1);
}

def_EHelper(ctz) {
  *ddest = _rv_ctz(*dsrc1);
}

def_EHelper(cpop) {
  *ddest = _rv_cpop(*dsrc1);
}

def_EHelper(sext_b) {
  *ddest = _rv_sext_b(*dsrc1);
}

def_EHelper(sext_h) {
  *ddest = _rv_sext_h(*dsrc1);
}

def_EHelper(zext_h) {
  *ddest = _rv_zext_h(*dsrc1);
}

def_EHelper(rev8) {
  *ddest = _rv_rev8(*dsrc1);
}

def_EHelper(revb) {
  *ddest = _rv_revb(*dsrc1);
}

def_EHelper(orc_b) {
  *ddest = _rv_orc_b(*dsrc1);
}

def_EHelper(clzw) {
  *ddest = _rv32_clz(*dsrc1);
}

def_EHelper(ctzw) {
  *ddest = _rv32_ctz(*dsrc1);
}

def_EHelper(cpopw) {
  *ddest = _rv32_cpop(*dsrc1);
}

def_EHelper(andn) {
  *ddest = _rv_andn(*dsrc1, *dsrc2);
}

def_EHelper(orn) {
  *ddest = _rv_orn(*dsrc1, *dsrc2);
}

def_EHelper(xnor) {
  *ddest = _rv_xnor(*dsrc1, *dsrc2);
}

def_EHelper(rol) {
  *ddest = _rv_rol(*dsrc1, *dsrc2);
}

def_EHelper(rolw) {
  *ddest = _rv32_rol(*dsrc1, *dsrc2);
}

def_EHelper(ror) {
  *ddest = _rv_ror(*dsrc1, *dsrc2);
}

def_EHelper(rori) {
  *ddest = _rv_ror(*dsrc1, id_src2->imm);
}

def_EHelper(rorw) {
  *ddest = _rv32_ror(*dsrc1, *dsrc2);
}

def_EHelper(roriw) {
  *ddest = _rv32_ror(*dsrc1, id_src2->imm);
}

def_EHelper(bclr) {
  *ddest = _rv_bclr(*dsrc1, *dsrc2);
}

def_EHelper(bclri) {
  *ddest = _rv_bclr(*dsrc1, id_src2->imm);
}

def_EHelper(bset) {
  *ddest = _rv_bset(*dsrc1, *dsrc2);
}

def_EHelper(bseti) {
  *ddest = _rv_bset(*dsrc1, id_src2->imm);
}

def_EHelper(binv) {
  *ddest = _rv_binv(*dsrc1, *dsrc2);
}

def_EHelper(binvi) {
  *ddest = _rv_binv(*dsrc1, id_src2->imm);
}

def_EHelper(bext) {
  *ddest = _rv_bext(*dsrc1, *dsrc2);
}

def_EHelper(bexti) {
  *ddest = _rv_bext(*dsrc1, id_src2->imm);
}

def_EHelper(clmul) {
  *ddest = _rv_clmul(*dsrc1, *dsrc2);
}

def_EHelper(clmulr) {
  *ddest = _rv_clmulr(*dsrc1, *dsrc2);
}

def_EHelper(clmulh) {
  *ddest = _rv_clmulh(*dsrc1, *dsrc2);
}

def_EHelper(min) {
  *ddest = _rv_min(*dsrc1, *dsrc2);
}

def_EHelper(minu) {
  *ddest = _rv_minu(*dsrc1, *dsrc2);
}

def_EHelper(max) {
  *ddest = _rv_max(*dsrc1, *dsrc2);
}

def_EHelper(maxu) {
  *ddest = _rv_maxu(*dsrc1, *dsrc2);
}

def_EHelper(pack) {
  *ddest = _rv_pack(*dsrc1, *dsrc2);
}

def_EHelper(packw) {
  *ddest = _rv32_pack(*dsrc1, *dsrc2);
}

def_EHelper(packh) {
  *ddest = _rv_packh(*dsrc1, *dsrc2);
}


def_EHelper(xpermn) {
  *ddest = _rv_xpermn(*dsrc1, *dsrc2);
}

def_EHelper(xpermb) {
  *ddest = _rv_xpermb(*dsrc1, *dsrc2);
}

def_EHelper(adduw) {
  *ddest = *dsrc2 + (uint32_t)*dsrc1;
}

def_EHelper(slliuw) {
  *ddest = (uint32_t)*dsrc1 << (id_src2->imm & 0x1f);
}

def_EHelper(sh1add) {
  *ddest = (*dsrc1 << 1) + *dsrc2;
}

def_EHelper(sh2add) {
  *ddest = (*dsrc1 << 2) + *dsrc2;
}

def_EHelper(sh3add) {
  *ddest = (*dsrc1 << 3) + *dsrc2;
}

def_EHelper(sh1adduw) {
  *ddest = ((uint32_t)*dsrc1 << 1) + *dsrc2;
}

def_EHelper(sh2adduw) {
  *ddest = ((uint32_t)*dsrc1 << 2) + *dsrc2;
}

def_EHelper(sh3adduw) {
  *ddest = ((uint32_t)*dsrc1 << 3) + *dsrc2;
}
