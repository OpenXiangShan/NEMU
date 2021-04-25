def_EHelper(in) {
  rt_decode(s, id_dest, false, s->isa.width);
  rt_decode(s, id_src1, true, 2);
  rtl_hostcall(s, HOSTCALL_PIO, ddest, dsrc1, NULL, (1 << 4) | s->isa.width);
  rtl_wb_r(s, ddest);
}

def_EHelper(out) {
  rt_decode(s, id_dest, true, 2);
  rt_decode(s, id_src1, true, s->isa.width);
  rtl_hostcall(s, HOSTCALL_PIO, ddest, dsrc1, NULL, (0 << 4) | s->isa.width);
}

def_EHelper(mov_r2cr) {
  rtl_decode_binary(s, false, true);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, dsrc1, NULL, CSR_CR0 + id_dest->reg);
}

def_EHelper(mov_cr2r) {
  rtl_decode_binary(s, false, false);
  rtl_hostcall(s, HOSTCALL_CSR, ddest, NULL, NULL, CSR_CR0 + id_src1->reg);
  rtl_priv_next(s);
}

def_EHelper(lidt) {
  rtl_decode_unary(s, false);
  rtl_addi(s, ddest, &s->isa.mbr, s->isa.moff);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, ddest, NULL, CSR_IDTR);
}

def_EHelper(lgdt) {
  rtl_decode_unary(s, false);
  rtl_addi(s, ddest, &s->isa.mbr, s->isa.moff);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, ddest, NULL, CSR_GDTR);
}

def_EHelper(ltr) {
  rtl_decode_unary(s, true);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, ddest, NULL, CSR_TR);
}

def_EHelper(_int) {
  rtl_trap(s, s->snpc, id_dest->val);
  rtl_priv_jr(s, t0);
}

def_EHelper(iret) {
  rtl_hostcall(s, HOSTCALL_PRIV, s0, NULL, NULL, PRIV_IRET);
  rtl_priv_jr(s, s0);
}

def_EHelper(mov_rm2sreg) {
  rtl_decode_binary(s, false, true);
  rtl_hostcall(s, HOSTCALL_CSR, NULL, dsrc1, NULL, id_dest->reg);
  if (ISNDEF(CONFIG_DIFFTEST_REF_NEMU) && id_dest->reg == 2) { difftest_skip_dut(1, 2); } // SS
}

#if 0
static inline def_EHelper(lldt) {
  rtl_hostcall(s, HOSTCALL_CSR, NULL, ddest, CSR_LDTR);
  print_asm_template1(lldt);
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

static inline def_EHelper(invlpg) {
}

static inline def_EHelper(mov_r2dr) {
}
#endif
