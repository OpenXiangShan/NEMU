static inline def_DHelper(csr) {
  decode_op_r(s, id_src1, s->isa.instr.i.rs1, true);
  decode_op_i(s, id_src2, s->isa.instr.csr.csr, true);
  decode_op_r(s, id_dest, s->isa.instr.i.rd, false);
}

#ifdef CONFIG_DEBUG
def_THelper(priv) {
  switch (s->isa.instr.csr.csr) {
    TAB(0, ecall)  TAB (0x102, sret)  TAB (0x105, wfi) TAB (0x120, sfence_vma)
    TAB(0x302, mret)
  }
  return EXEC_ID_inv;
};

def_THelper(system) {
  switch (s->isa.instr.i.funct3) {
    TAB(0, priv)  TAB(1, csrrw)  TAB(2, csrrs)  TAB(3, csrrc)
                  TAB(5, csrrwi) TAB(6, csrrsi) TAB(7, csrrci)
  }
  return EXEC_ID_inv;
};
#endif
