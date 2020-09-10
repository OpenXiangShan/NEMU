rtlreg_t rocc_sdi(uint32_t funct7, uint32_t funct3, rtlreg_t rs1, rtlreg_t rs2);

static inline def_EHelper(rocc3) {
  *ddest = rocc_sdi(s->isa.instr.r.funct7, s->isa.instr.r.funct3, *dsrc1, *dsrc2);
  print_asm_template3(rocc3);
}
