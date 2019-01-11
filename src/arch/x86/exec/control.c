#include "cpu/exec.h"
#include "cc.h"

make_EHelper(jmp) {
  // the target address is calculated at the decode stage
  rtl_j(decinfo.jmp_pc);

  print_asm("jmp %x", decinfo.jmp_pc);
}

make_EHelper(jcc) {
  // the target address is calculated at the decode stage
  uint32_t cc = decinfo.opcode & 0xf;
  rtl_setcc(&t0, cc);
  rtl_li(&t1, 0);
  rtl_jrelop(RELOP_NE, &t0, &t1, decinfo.jmp_pc);

  print_asm("j%s %x", get_cc_name(cc), decinfo.jmp_pc);
}

make_EHelper(jmp_rm) {
  rtl_jr(&id_dest->val);

  print_asm("jmp *%s", id_dest->str);
}

make_EHelper(call) {
  // the target address is calculated at the decode stage
//  TODO();
  rtl_li(&t2, decinfo.seq_pc);
  rtl_push(&t2);
  rtl_j(decinfo.jmp_pc);

  print_asm("call %x", decinfo.jmp_pc);
}

make_EHelper(ret) {
//  TODO();
  rtl_pop(&t0);
  rtl_jr(&t0);

  print_asm("ret");
}

make_EHelper(call_rm) {
//  TODO();
  rtl_li(&t2, decinfo.seq_pc);
  rtl_push(&t2);
  rtl_jr(&id_dest->val);

  print_asm("call *%s", id_dest->str);
}
