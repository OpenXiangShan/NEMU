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
#ifdef ISA64
  rtl_andi(&id_src2->val, &id_src2->val, 0x3f);
#else
  rtl_andi(&id_src2->val, &id_src2->val, 0x1f);
#endif
  rtl_shl(&s0, &id_src->val, &id_src2->val);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sll);
}

make_EHelper(srl) {
#ifdef ISA64
  rtl_andi(&id_src2->val, &id_src2->val, 0x3f);
#else
  rtl_andi(&id_src2->val, &id_src2->val, 0x1f);
#endif

  if (decinfo.isa.instr.funct7 == 32) {
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
  rtl_add(&s0, &id_src->val, &cpu.pc);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template2(auipc);
}

make_EHelper(lui) {
  rtl_sr(id_dest->reg, &id_src->val, 4);

  print_asm_template2(lui);
}

make_EHelper(addw) {
  rtl_add(&s0, &id_src->val, &id_src2->val);
  rtl_li(&s1, 32);
  rtl_shl(&s0, &s0, &s1);
  rtl_sar64(&s0, &s0, &s1);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(addw);
}

make_EHelper(subw) {
  rtl_sub(&s0, &id_src->val, &id_src2->val);
  rtl_li(&s1, 32);
  rtl_shl(&s0, &s0, &s1);
  rtl_sar64(&s0, &s0, &s1);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(subw);
}

make_EHelper(sllw) {
  rtl_andi(&id_src2->val, &id_src2->val, 0x1f);
  rtl_shl(&s0, &id_src->val, &id_src2->val);
  rtl_li(&s1, 32);
  rtl_shl(&s0, &s0, &s1);
  rtl_sar64(&s0, &s0, &s1);
  rtl_sr(id_dest->reg, &s0, 4);

  print_asm_template3(sllw);
}

make_EHelper(srlw) {
  if (decinfo.isa.instr.funct7 == 32) {
    // sraw
    rtl_sar(&s0, &id_src->val, &id_src2->val);
    rtl_li(&s1, 32);
    rtl_shl(&s0, &s0, &s1);
    rtl_sar64(&s0, &s0, &s1);
    print_asm_template3(sraw);
  }
  else {
    // srlw
    rtl_andi(&id_src2->val, &id_src2->val, 0x1f);

    rtl_shr64(&s0, &id_src->val, &id_src2->val);
    rtl_li(&s1, 32);
    rtl_shl(&s0, &s0, &s1);
    rtl_sar64(&s0, &s0, &s1);
    print_asm_template3(srlw);
  }

  rtl_sr(id_dest->reg, &s0, 4);
}

make_EHelper(sraw) {
  exec_srlw(NULL);
}