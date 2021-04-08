#define def_RDHelper(name) static inline void concat(rt_decode_, name) (Decode *s, int width)

static inline void rt_operand_reg(Decode *s, Operand *op, int width) {
  if (width == 1 || width == 2) { rtl_lr(s, op->preg, op->reg, width); }
}

static inline void rt_operand_rm(Decode *s,
    Operand *rm, bool load_rm_val, Operand *reg, bool load_reg_val, int width) {
  if (reg != NULL && load_reg_val) rt_operand_reg(s, reg, width);
  if (!s->isa.is_rm_memory) {
    if (load_rm_val) rt_operand_reg(s, rm, width);
  } else {
#if 0
    if (((s->opcode == 0x80 || s->opcode == 0x81 || s->opcode == 0x83) && s->isa.ext_opcode == 7) ||
        (s->opcode == 0x1ba && s->isa.ext_opcode == 4)) {
      // fix with cmp and bt, since they do not write memory
      IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 0));
    }
#endif

    rtl_mv(s, &s->isa.mbr, s->isa.mbase);
    if (s->isa.midx != rz) {
      rtl_shli(s, s1, s->isa.midx, s->isa.mscale);
      rtl_add(s, &s->isa.mbr, &s->isa.mbr, s1);
    }
    if (ISNDEF(__PA__) && s->isa.sreg_base != NULL) {
      rtl_add(s, &s->isa.mbr, &s->isa.mbr, s->isa.sreg_base);
    }

    if (load_rm_val) rtl_lm(s, &rm->val, &s->isa.mbr, s->isa.moff, width, MMU_DYNAMIC);
  }
}

def_RDHelper(mov_G2E) {
  rt_operand_rm(s, id_dest, false, id_src1, true, width);
}

def_RDHelper(mov_E2G) {
  rt_operand_rm(s, id_src1, true, id_dest, false, width);
}

def_RDHelper(mov_Eb2G) {
  rt_operand_rm(s, id_src1, true, NULL, false, 1);
}

def_RDHelper(mov_Ew2G) {
  rt_operand_rm(s, id_src1, true, NULL, false, 2);
}

def_RDHelper(mov_I2E) {
  rt_operand_rm(s, id_dest, false, NULL, false, width);
}

def_RDHelper(E2G) {
  rt_operand_rm(s, id_src1, true, id_dest, true, width);
}

def_RDHelper(G2E) {
#if 0
  if (s->opcode != 0x38 && s->opcode != 0x39 && // cmp
      s->opcode != 0x84 && s->opcode != 0x85) { // test
    IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  }
#endif
  rt_operand_rm(s, id_dest, true, id_src1, true, width);
}

def_RDHelper(M2G) {
  rt_operand_rm(s, id_src1, false, id_dest, false, width);
}

def_RDHelper(a2dx) {
  rt_operand_reg(s, id_src1, width);
  rt_operand_reg(s, id_dest, 2);
}

def_RDHelper(r) {
  rt_operand_reg(s, id_dest, width);
}

def_RDHelper(E) {
  rt_operand_rm(s, id_dest, true, NULL, false, width);
}

def_RDHelper(O) {
  rtl_lm(s, dsrc1, s->isa.mbase, s->isa.moff, width, MMU_DYNAMIC);
}

def_RDHelper(a_src) {
  rt_operand_reg(s, id_src1, width);
}
