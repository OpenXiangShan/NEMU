#include "cpu/exec.h"

make_EHelper(mul) {
  rtl_mul_lo(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mul);
}

make_EHelper(mulh) {
  rtl_imul_hi(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mulh);
}

make_EHelper(mulhu) {
  rtl_mul_hi(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mulh);
}

make_EHelper(mulhsu) {
  //rtl_msb(&s0, &id_src->val, 8);
  //rtl_li(&s1, 0);
  //rtl_sub(&s1, &s1, &id_src->val); // s1 = -id_src->val
  //rtl_mux(&s1, &s0, &s1, &id_src->val); // s1 = |id_src->val|

  //rtl_mul_hi(&id_dest->val, &s1, &id_src2->val);

  //rtl_li(&s1, 0);
  //rtl_sub(&s1, &s1, &id_dest->val); // s1 = -id_dest->val
  //rtl_mux(&s0, &s0, &s1, &id_dest->val);

  s0 = ((__int128_t)(sword_t)id_src->val * id_src2->val) >> 64;

  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mulhsu);
}

make_EHelper(div) {
  if (id_src2->val == 0) {
    rtl_li(&s0, -1);
  } else if (id_src->val == 0x8000000000000000LL && id_src2->val == -1) {
    rtl_mv(&s0, &id_src->val);
  } else {
    rtl_idiv_q(&s0, &id_src->val, &id_src2->val);
  }
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(div);
}

make_EHelper(divu) {
  if (id_src2->val == 0) {
    rtl_li(&s0, -1);
  } else {
    rtl_div_q(&s0, &id_src->val, &id_src2->val);
  }
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(divu);
}

make_EHelper(rem) {
  if (id_src2->val == 0) {
    rtl_mv(&s0, &id_src->val);
  } else if (id_src->val == 0x8000000000000000LL && id_src2->val == -1) {
    rtl_li(&s0, 0);
  } else {
    rtl_idiv_r(&s0, &id_src->val, &id_src2->val);
  }
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(rem);
}

make_EHelper(remu) {
  if (id_src2->val == 0) {
    rtl_mv(&s0, &id_src->val);
  } else {
    rtl_div_r(&s0, &id_src->val, &id_src2->val);
  }
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(remu);
}

make_EHelper(mulw) {
  rtl_mul_lo(&s0, &id_src->val, &id_src2->val);
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(mulw);
}

make_EHelper(divw) {
  rtl_sext(&s0, &id_src->val, 4);
  rtl_sext(&s1, &id_src2->val, 4);
  if (s1 == 0) {
    rtl_li(&s0, -1);
  } else if (s0 == 0x80000000 && s1 == -1) {
    //rtl_mv(&s0, &s0);
  } else {
    rtl_idiv_q(&s0, &s0, &s1);
  }
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(divw);
}

make_EHelper(remw) {
  rtl_sext(&s0, &id_src->val, 4);
  rtl_sext(&s1, &id_src2->val, 4);
  if (s1 == 0) {
    //rtl_mv(&s0, &s0);
  } else if (s0 == 0x80000000 && s1 == -1) {
    rtl_li(&s0, 0);
  } else {
    rtl_idiv_r(&s0, &s0, &s1);
  }
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(remw);
}

make_EHelper(divuw) {
  rtl_andi(&s0, &id_src->val, 0xffffffffu);
  rtl_andi(&s1, &id_src2->val, 0xffffffffu);
  if (s1 == 0) {
    rtl_li(&s0, -1);
  } else {
    rtl_div_q(&s0, &s0, &s1);
  }
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(divuw);
}

make_EHelper(remuw) {
  rtl_andi(&s0, &id_src->val, 0xffffffffu);
  rtl_andi(&s1, &id_src2->val, 0xffffffffu);
  if (s1 == 0) {
    //rtl_mv(&s0, &s0);
  } else {
    rtl_div_r(&s0, &s0, &s1);
  }
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(remuw);
}
