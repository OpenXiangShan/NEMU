#include "cpu/exec.h"

make_EHelper(add) {
  rtl_add(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(add);
}

make_EHelper(sub) {
  rtl_sub(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sub);
}

make_EHelper(sll) {
  rtl_andi(&id_src2->val, &id_src2->val, 0x3f);
  rtl_shl(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sll);
}

make_EHelper(srl) {
  rtl_andi(&id_src2->val, &id_src2->val, 0x3f);
  // the LSB of funct7 may be "1" due to the shift amount can be >= 32
  if ((decinfo.isa.instr.funct7 & ~0x1) == 32) {
    // sra
    rtl_sar(&s0, &id_src->val, &id_src2->val);
    print_asm_template3(sra);
  }
  else {
    rtl_shr(&s0, &id_src->val, &id_src2->val);
    print_asm_template3(srl);
  }
  rtl_sr(id_dest->reg, &s0, 4);
}

make_EHelper(sra) {
  exec_srl(NULL);
}

make_EHelper(slt) {
  rtl_setrelop(RELOP_LT, &s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(slt);
}

make_EHelper(sltu) {
  rtl_setrelop(RELOP_LTU, &s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sltu);
}

make_EHelper(xor) {
  rtl_xor(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(xor);
}

make_EHelper(or) {
  rtl_or(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(or);
}

make_EHelper(and) {
  rtl_and(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(and);
}

make_EHelper(auipc) {
  rtl_shli(&s0, &id_src2->val, 12);
  rtl_add(&s0, &s0, &cpu.pc);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template2(auipc);
}

make_EHelper(lui) {
  rtl_shli(&s0, &id_src2->val, 12);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template2(lui);
}

make_EHelper(addw) {
  rtl_add(&s0, &id_src->val, &id_src2->val);
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(addw);
}

make_EHelper(subw) {
  rtl_sub(&s0, &id_src->val, &id_src2->val);
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(subw);
}

make_EHelper(sllw) {
  rtl_andi(&id_src2->val, &id_src2->val, 0x1f);
  rtl_shl(&s0, &id_src->val, &id_src2->val);
  rtl_sext(&s0, &s0, 4);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sllw);
}

make_EHelper(srlw) {
  rtl_andi(&id_src2->val, &id_src2->val, 0x1f);
  assert((decinfo.isa.instr.funct7 & 0x1) == 0);
  if (decinfo.isa.instr.funct7 == 32) {
    // sraw
    rtl_sext(&id_src->val, &id_src->val, 4);
    rtl_sar(&s0, &id_src->val, &id_src2->val);
    rtl_sext(&s0, &s0, 4);
    print_asm_template3(sraw);
  }
  else {
    // srlw
    rtl_andi(&id_src->val, &id_src->val, 0xffffffffu);
    rtl_shr(&s0, &id_src->val, &id_src2->val);
    rtl_sext(&s0, &s0, 4);
    print_asm_template3(srlw);
  }

  rtl_sr(id_dest->reg, &s0, 4);
}

make_EHelper(sraw) {
  exec_srlw(NULL);
}
