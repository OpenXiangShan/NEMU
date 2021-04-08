def_DEWBWHelper(inl_dx2a, dx_src, in, r, l);
def_DEWBWHelper(inw_dx2a, dx_src, in, r, w);

def_DEWHelper(outb_a2dx, a2dx, out, b);

#if 0

void load_sreg(int idx, uint16_t val);

static inline def_EHelper(lidt) {
  rtl_addi(s, ddest, s->isa.mbase, s->isa.moff);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, ddest, CSR_IDTR);
  print_asm_template1(lidt);
}

static inline def_EHelper(lgdt) {
  rtl_addi(s, ddest, s->isa.mbase, s->isa.moff);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, ddest, CSR_GDTR);
  print_asm_template1(lgdt);
}

static inline def_EHelper(lldt) {
  rtl_hostcall(s, HOSTCALL_CSR, NULL, ddest, CSR_LDTR);
  print_asm_template1(lldt);
}

static inline def_EHelper(mov_r2cr) {
  rtl_hostcall(s, HOSTCALL_CSR, NULL, dsrc1, CSR_CR0 + id_dest->reg);
  print_asm("movl %%%s,%%cr%d", reg_name(id_src1->reg, 4), id_dest->reg);
}

static inline def_EHelper(mov_cr2r) {
  rtl_hostcall(s, HOSTCALL_CSR, ddest, NULL, CSR_CR0 + id_src1->reg);
  print_asm("movl %%cr%d,%%%s", id_src1->reg, reg_name(id_dest->reg, 4));
}

static inline def_EHelper(mov_rm2sreg) {
  rtl_hostcall(s, HOSTCALL_CSR, NULL, dsrc1, id_dest->reg);
  if (ISNDEF(CONFIG_DIFFTEST_REF_NEMU) && id_dest->reg == 2) { difftest_skip_dut(1, 2); } // SS
  print_asm("movw %s,%%%s", id_src1->str, sreg_name(id_dest->reg));
}

static inline def_EHelper(mov_sreg2rm) {
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, id_src1->reg);
  operand_write(s, id_dest, s0);
  print_asm("movw %%%s,%s", sreg_name(id_src1->reg), id_dest->str);
}

static inline def_EHelper(push_sreg_internal) {
  rtl_hostcall(s, HOSTCALL_CSR, s0, NULL, id_dest->reg);
  rtl_push(s, s0);
  print_asm("push %%%s", sreg_name(id_dest->reg));
}

static inline def_EHelper(pop_sreg_internal) {
  rtl_pop(s, s0);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, s0, id_dest->reg);
  print_asm("pop %%%s", sreg_name(id_dest->reg));
}

static inline def_EHelper(push_fs) {
  id_dest->reg = CSR_FS;
  exec_push_sreg_internal(s);
}

static inline def_EHelper(push_es) {
  id_dest->reg = CSR_ES;
  exec_push_sreg_internal(s);
}

static inline def_EHelper(push_ds) {
  id_dest->reg = CSR_DS;
  exec_push_sreg_internal(s);
}

static inline def_EHelper(pop_ds) {
  id_dest->reg = CSR_DS;
  exec_pop_sreg_internal(s);
}

static inline def_EHelper(pop_es) {
  id_dest->reg = CSR_ES;
  exec_pop_sreg_internal(s);
}

static inline def_EHelper(pop_fs) {
  id_dest->reg = CSR_FS;
  exec_pop_sreg_internal(s);
}

static inline def_EHelper(int) {
  rtl_trap(s, s->seq_pc, id_dest->imm);
  print_asm("int %s", id_dest->str);
}

static inline def_EHelper(iret) {
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, PRIV_IRET);
  rtl_jr(s, s0);
  print_asm("iret");
}

static inline def_EHelper(invlpg) {
}

static inline def_EHelper(ltr) {
  rtl_hostcall(s, HOSTCALL_CSR, NULL, ddest, CSR_TR);
  print_asm_template1(ltr);
}

static inline def_EHelper(mov_r2dr) {
}
#endif
