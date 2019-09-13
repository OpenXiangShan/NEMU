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

  print_asm_template3(mulhu);
}

make_EHelper(mulhsu) {
  // Algorithm:
  // We want to obtain ans = mulhsu(a, b).
  // Consider mulhu(a, b).
  // If a >= 0, then ans = mulhu(a, b);
  // If a = -x < 0, then a = 2^64 - x in two's complement
  // Then
  //   mulhu(a, b) = mulhu(2^64 -x , b) = ((2^64 - x)b) >> 64
  //               = ((2^64b) >> 64) + ((-xb) >> 64)
  //               = b + mulhsu(a, b) = b + ans
  // Therefore, ans = mulhu(a, b) - b
  //
  // In the end, ans = (a < 0 ? mulhu(a, b) - b : mulhu(a, b))
  //                 = mulhu(a, b) - (a < 0 ? b : 0)

  rtl_sari(&s0, &id_src->val, 63);
  rtl_and(&s0, &id_src2->val, &s0); // s0 = (id_src->val < 0 ? id_src2->val : 0)
  rtl_mul_hi(&s1, &id_src->val, &id_src2->val);
  rtl_sub(&s0, &s1, &s0);

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
