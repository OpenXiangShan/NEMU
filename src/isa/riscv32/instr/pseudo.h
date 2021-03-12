// standard pseudo instructions
// See Chapter 25 "RISC-V Assembly Programmer's Handbook" in the RISC-V ISA manual
//
// The following pseudo instructions are excluded
// (1) not present in RV32
//       negw   sext.w
// (2) seem not frequently present during execution
//       nop    not    neg
//       seqz   snez   sltz   sgtz
//       jalr(rs)
//       [[all CSR instructions]]
// (3) only expansion without optimization
//       la
//       l{b|h|w|d} symbol
//       s{b|h|w|d} symbol
//       bgt    ble    bgtu   bleu
//       call   tail
//       fence
// (4) still not considered
//       fmv.s  fabs.s fneg.s
//       fmv.d  fabs.d fneg.d
//       fl{w|d} symbol
//       fs{w|d} symbol
// (5) already provided in RVC
//       j      jal(ra)
//       beqz   bnez
//       li     mv

def_EHelper(p_blez) {
  rtl_jrelop(s, RELOP_GE, rz, dsrc2, id_dest->imm);
}

def_EHelper(p_bgtz) {
  rtl_jrelop(s, RELOP_LT, rz, dsrc2, id_dest->imm);
}

def_EHelper(p_bltz) {
  rtl_jrelop(s, RELOP_LT, dsrc1, rz, id_dest->imm);
}

def_EHelper(p_bgez) {
  rtl_jrelop(s, RELOP_GE, dsrc1, rz, id_dest->imm);
}

def_EHelper(p_ret) {
//  IFDEF(CONFIG_ENGINE_INTERPRETER, rtl_andi(s, s0, s0, ~0x1u));
  IFNDEF(CONFIG_DIFFTEST_REF_NEMU, difftest_skip_dut(1, 2));
  rtl_jr(s, &cpu.gpr[1]._32);
}

// non-standard pseudo instructions

def_EHelper(p_li_0) {
  rtl_li(s, ddest, 0);
}

def_EHelper(p_li_1) {
  rtl_li(s, ddest, 1);
}

def_EHelper(p_inc) {
  rtl_addi(s, ddest, ddest, 1);
}

def_EHelper(p_dec) {
  rtl_subi(s, ddest, ddest, 1);
}
