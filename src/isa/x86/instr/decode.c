#include "../local-include/rtl.h"
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include "decode.h"
#include <isa-all-instr.h>

def_all_THelper();

static inline int x86_width_decode(Decode *s, int width) {
  if (width == WIDTH_dynamic) return (s->isa.is_operand_size_16 ? 2 : 4);
  return width;
}

static inline word_t x86_instr_fetch(Decode *s, int len) {
  word_t ret = instr_fetch(&s->snpc, len);
  word_t ret_save = ret;
  int i;
  for (i = 0; i < len; i ++) {
    *(s->isa.p_instr) = ret & 0xff;
    ret >>= 8;
    s->isa.p_instr ++;
  }
  return ret_save;
}

static inline word_t get_instr(Decode *s) {
  return *(s->isa.p_instr - 1);
}

typedef union {
  struct {
    uint8_t R_M		:3;
    uint8_t reg		:3;
    uint8_t mod		:2;
  };
  struct {
    uint8_t dont_care	:3;
    uint8_t opcode		:3;
  };
  uint8_t val;
} ModR_M;

typedef union {
  struct {
    uint8_t base	:3;
    uint8_t index	:3;
    uint8_t ss		:2;
  };
  uint8_t val;
} SIB;

#if 0
static inline void load_addr(Decode *s, ModR_M *m, Operand *rm) {
  assert(m->mod != 3);

  sword_t disp = 0;
  int disp_size = 4;
  int base_reg = -1, index_reg = -1, scale = 0;

  if (m->R_M == R_ESP) {
    SIB sib;
    sib.val = instr_fetch(&s->seq_pc, 1);
    base_reg = sib.base;
    scale = sib.ss;

    if (sib.index != R_ESP) { index_reg = sib.index; }
  }
  else {
    /* no SIB */
    base_reg = m->R_M;
  }

  if (m->mod == 0) {
    if (base_reg == R_EBP) { base_reg = -1; }
    else { disp_size = 0; }
  }
  else if (m->mod == 1) { disp_size = 1; }

  if (disp_size != 0) {
    /* has disp */
    disp = instr_fetch(&s->seq_pc, disp_size);
    if (disp_size == 1) { disp = (int8_t)disp; }
  }

  s->isa.mbase = (base_reg != -1 ? &reg_l(base_reg) : rz);
  if (index_reg != -1) {
    rtl_shli(s, s1, &reg_l(index_reg), scale);
    rtl_add(s, &s->isa.mbr, s->isa.mbase, s1);
    s->isa.mbase = &s->isa.mbr;
  }
  if (ISNDEF(__PA__) && s->isa.sreg_base != NULL) {
    rtl_add(s, &s->isa.mbr, s->isa.mbase, s->isa.sreg_base);
    s->isa.mbase = &s->isa.mbr;
  }
  s->isa.moff = disp;

#ifdef CONFIG_DEBUG
  char disp_buf[16];
  char base_buf[8];
  char index_buf[8];

  if (disp_size != 0) {
    /* has disp */
    sprintf(disp_buf, "%s%#x", (disp < 0 ? "-" : ""), (disp < 0 ? -disp : disp));
  }
  else { disp_buf[0] = '\0'; }

  if (base_reg == -1) { base_buf[0] = '\0'; }
  else {
    sprintf(base_buf, "%%%s", reg_name(base_reg, 4));
  }

  if (index_reg == -1) { index_buf[0] = '\0'; }
  else {
    sprintf(index_buf, ",%%%s,%d", reg_name(index_reg, 4), 1 << scale);
  }

  if (base_reg == -1 && index_reg == -1) {
    sprintf(rm->str, "%s", disp_buf);
  }
  else {
    sprintf(rm->str, "%s(%s%s)", disp_buf, base_buf, index_buf);
  }
#endif

  rm->type = OP_TYPE_MEM;
}

void read_ModR_M(Decode *s, Operand *rm, bool load_rm_val, Operand *reg, bool load_reg_val) {
  ModR_M m;
  m.val = instr_fetch(&s->seq_pc, 1);
  s->isa.ext_opcode = m.opcode;
  if (reg != NULL) operand_reg(s, reg, load_reg_val, m.reg, reg->width);
  if (m.mod == 3) operand_reg(s, rm, load_rm_val, m.R_M, rm->width);
  else {
    if (((s->opcode == 0x80 || s->opcode == 0x81 || s->opcode == 0x83) && s->isa.ext_opcode == 7) ||
        (s->opcode == 0x1ba && s->isa.ext_opcode == 4)) {
      // fix with cmp and bt, since they do not write memory
      IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 0));
    }
    load_addr(s, &m, rm);
    if (load_rm_val) rtl_lm(s, &rm->val, s->isa.mbase, s->isa.moff, rm->width);
    rm->preg = &rm->val;
  }
}
#endif

static inline void operand_reg(Decode *s, Operand *op, int r, int width) {
  width = x86_width_decode(s, width);
  op->reg = r;
  if (width == 4) { op->preg = &reg_l(r); }
  else {
    assert(width == 1 || width == 2);
    op->preg = &op->val;
  }
  print_Dop(op->str, OP_STR_SIZE, "%%%s", reg_name(r, width));
}

static inline void operand_imm(Decode *s, Operand *op, word_t imm) {
  op->preg = &op->val;
  op->val = imm;
  print_Dop(op->str, OP_STR_SIZE, "$0x%x", imm);
}

// decode operand helper
#define def_DopHelper(name) \
  static inline void concat(decode_op_, name) (Decode *s, Operand *op, int width)

/* Refer to Appendix A in i386 manual for the explanations of these abbreviations */

/* Ib, Iv */
def_DopHelper(I) {
  /* pc here is pointing to the immediate */
  width = x86_width_decode(s, width);
  word_t imm = x86_instr_fetch(s, width);
  operand_imm(s, op, imm);
}

#if 0
/* I386 manual does not contain this abbreviation, but it is different from
 * the one above from the view of implementation. So we use another helper
 * function to decode it.
 */
/* sign immediate */
def_DopHelper(SI) {
  width = x86_width_decode(s, width);
#ifdef __ICS_EXPORT
  /* TODO: Use x86_instr_fetch() to read `op->width' bytes of memory
   * pointed by 's->seq_pc'. Interpret the result as a signed immediate,
   * and call `operand_imm()` as following.
   *
   operand_imm(s, op, ???);
   */
  TODO();
#else
  word_t imm = x86_instr_fetch(s, width);
  if (width == 1) imm = (int8_t)imm;
  else if (width == 2) imm = (int16_t)imm;
  operand_imm(s, op, imm);
#endif
}
#endif

#if 0
/* I386 manual does not contain this abbreviation.
 * It is convenient to merge them into a single helper function.
 */
/* AL/eAX */
def_DopHelper(a) {
  operand_reg(s, op, R_EAX, width);
}
#endif

/* This helper function is use to decode register encoded in the opcode. */
/* XX: AL, AH, BL, BH, CL, CH, DL, DH
 * eXX: eAX, eCX, eDX, eBX, eSP, eBP, eSI, eDI
 */
def_DopHelper(r) {
  int r = get_instr(s) & 0x7;
  operand_reg(s, op, r, width);
}

#if 0
/* I386 manual does not contain this abbreviation.
 * We decode everything of modR/M byte in one time.
 */
/* Eb, Ew, Ev
 * Gb, Gv
 * Cd,
 * M
 * Rd
 * Sw
 */
static inline void operand_rm(Decode *s, Operand *rm, bool load_rm_val, Operand *reg, bool load_reg_val) {
  read_ModR_M(s, rm, load_rm_val, reg, load_reg_val);
}

/* Ob, Ov */
def_DopHelper(O) {
  op->type = OP_TYPE_MEM;
  s->isa.moff = x86_instr_fetch(s, 4);
  s->isa.mbase = s->isa.sreg_base ? s->isa.sreg_base : rz;
  if (load_val) {
    rtl_lm(s, &op->val, s->isa.mbase, s->isa.moff, op->width);
    op->preg = &op->val;
  }

  print_Dop(op->str, OP_STR_SIZE, "0x%x", s->isa.moff);
}

/* Eb <- Gb
 * Ev <- Gv
 */
static inline def_DHelper(G2E) {
  if (s->opcode != 0x38 && s->opcode != 0x39 && // cmp
      s->opcode != 0x84 && s->opcode != 0x85) { // test
    IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  }
  operand_rm(s, id_dest, true, id_src1, true);
}

static inline def_DHelper(mov_G2E) {
  operand_rm(s, id_dest, false, id_src1, true);
}

// for bts and btr
static inline def_DHelper(bit_G2E) {
  operand_rm(s, id_dest, false, id_src1, true);
  if (s->isa.mbase) {
    rtl_shri(s, s0, dsrc1, 5);
    rtl_shli(s, s0, s0, 2);
    rtl_add(s, &s->isa.mbr, s->isa.mbase, s0);
    s->isa.mbase = &s->isa.mbr;
    if (s->opcode != 0x1a3) { // bt
      IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
    }
    rtl_lm(s, &id_dest->val, s->isa.mbase, s->isa.moff, id_dest->width);
  }
  rtl_andi(s, &id_src1->val, dsrc1, 0x1f);
  id_src1->preg = &id_src1->val;
}

/* Gb <- Eb
 * Gv <- Ev
 */
static inline def_DHelper(E2G) {
  operand_rm(s, id_src1, true, id_dest, true);
}

static inline def_DHelper(mov_E2G) {
  operand_rm(s, id_src1, true, id_dest, false);
}

static inline def_DHelper(lea_M2G) {
  operand_rm(s, id_src1, false, id_dest, false);
}

/* AL <- Ib
 * eAX <- Iv
 */
static inline def_DHelper(I2a) {
  decode_op_a(s, id_dest, true);
  decode_op_I(s, id_src1, true);
}

/* Gv <- EvIb
 * Gv <- EvIv
 * use for imul */
static inline def_DHelper(I_E2G) {
  operand_rm(s, id_src2, true, id_dest, false);
  decode_op_SI(s, id_src1, true); // imul takes the imm as signed
}

/* Eb <- Ib
 * Ev <- Iv
 */
static inline def_DHelper(I2E) {
  IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  operand_rm(s, id_dest, true, NULL, false);
  decode_op_I(s, id_src1, true);
}

static inline def_DHelper(mov_I2E) {
  operand_rm(s, id_dest, false, NULL, false);
  decode_op_I(s, id_src1, true);
}
#endif

/* XX <- Ib
 * eXX <- Iv
 */
static inline def_DHelper(I2r) {
  decode_op_r(s, id_dest, width);
  decode_op_I(s, id_src1, width);
}

#if 0
static inline def_DHelper(mov_I2r) {
  decode_op_r(s, id_dest, false);
  decode_op_I(s, id_src1, true);
}

/* used by unary operations */
static inline def_DHelper(I) {
  decode_op_I(s, id_dest, true);
}

static inline def_DHelper(r) {
  decode_op_r(s, id_dest, true);
}

static inline def_DHelper(E) {
  operand_rm(s, id_dest, true, NULL, false);
}

static inline def_DHelper(setcc_E) {
  operand_rm(s, id_dest, false, NULL, false);
}

static inline def_DHelper(gp6_E) {
  operand_rm(s, id_dest, true, NULL, false);
}

static inline def_DHelper(gp7_E) {
  operand_rm(s, id_dest, false, NULL, false);
}

/* used by test in group3 */
static inline def_DHelper(test_I) {
  decode_op_I(s, id_src1, true);
}

static inline def_DHelper(SI2E) {
  assert(id_dest->width == 2 || id_dest->width == 4);
  IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  operand_rm(s, id_dest, true, NULL, false);
  id_src1->width = 1;
  decode_op_SI(s, id_src1, true);
  if (id_dest->width == 2) {
    *dsrc1 &= 0xffff;
  }
}

static inline def_DHelper(SI_E2G) {
  assert(id_dest->width == 2 || id_dest->width == 4);
  operand_rm(s, id_src2, true, id_dest, false);
  id_src1->width = 1;
  decode_op_SI(s, id_src1, true);
}

static inline def_DHelper(gp2_1_E) {
  operand_rm(s, id_dest, true, NULL, false);
  operand_imm(s, id_src1, true, 1, 1);
}

static inline def_DHelper(gp2_cl2E) {
  IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  operand_rm(s, id_dest, true, NULL, false);
  // shift instructions will eventually use the lower
  // 5 bits of %cl, therefore it is OK to load %ecx
  operand_reg(s, id_src1, true, R_ECX, 4);
}

static inline def_DHelper(gp2_Ib2E) {
  IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  operand_rm(s, id_dest, true, NULL, false);
  id_src1->width = 1;
  decode_op_I(s, id_src1, true);
}

/* Ev <- GvIb
 * use for shld/shrd */
static inline def_DHelper(Ib_G2E) {
  IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  operand_rm(s, id_dest, true, id_src2, true);
  id_src1->width = 1;
  decode_op_I(s, id_src1, true);
}

/* Ev <- GvCL
 * use for shld/shrd */
static inline def_DHelper(cl_G2E) {
  IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  operand_rm(s, id_dest, true, id_src2, true);
  // shift instructions will eventually use the lower
  // 5 bits of %cl, therefore it is OK to load %ecx
  operand_reg(s, id_src1, true, R_ECX, 4);
}

// for cmpxchg
static inline def_DHelper(a_G2E) {
  IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  operand_rm(s, id_dest, true, id_src2, true);
  operand_reg(s, id_src1, true, R_EAX, 4);
}


static inline def_DHelper(O2a) {
  decode_op_O(s, id_src1, true);
  decode_op_a(s, id_dest, false);
}

static inline def_DHelper(a2O) {
  decode_op_a(s, id_src1, true);
  decode_op_O(s, id_dest, false);
}

// for scas and stos
static inline def_DHelper(aSrc) {
  decode_op_a(s, id_src1, true);
}

// for lods
static inline def_DHelper(aDest) {
  decode_op_a(s, id_dest, false);
}

// for xchg
static inline def_DHelper(a2r) {
  decode_op_a(s, id_src1, true);
  decode_op_r(s, id_dest, true);
}

static inline def_DHelper(J) {
  decode_op_SI(s, id_dest, false);
  // the target address can be computed in the decode stage
  s->jmp_pc = id_dest->simm + s->seq_pc;
}
#ifndef __ICS_EXPORT

// for long jump
static inline def_DHelper(LJ) {
  decode_op_I(s, id_dest, false); // offset
  id_src1->width = 2;
  decode_op_I(s, id_src1, false); // CS
  // the target address can be computed in the decode stage
  s->jmp_pc = id_dest->imm;
}
#endif

static inline def_DHelper(push_SI) {
  decode_op_SI(s, id_dest, true);
}

static inline def_DHelper(in_I2a) {
  id_src1->width = 1;
  decode_op_I(s, id_src1, true);
  decode_op_a(s, id_dest, false);
}

static inline def_DHelper(in_dx2a) {
  operand_reg(s, id_src1, true, R_DX, 2);
  decode_op_a(s, id_dest, false);
}

static inline def_DHelper(out_a2I) {
  decode_op_a(s, id_src1, true);
  id_dest->width = 1;
  decode_op_I(s, id_dest, true);
}

static inline def_DHelper(out_a2dx) {
  decode_op_a(s, id_src1, true);
  operand_reg(s, id_dest, true, R_DX, 2);
}

#ifndef __ICS_EXPORT
static inline def_DHelper(E2xmm) {
  operand_rm(s, id_src1, false, id_dest, false);
}

static inline def_DHelper(xmm2E) {
  operand_rm(s, id_dest, false, id_src1, false);
}

static inline def_DHelper(Ib2xmm) {
  operand_rm(s, id_dest, false, NULL, false);
  id_src1->width = 1;
  decode_op_I(s, id_src1, true);
}
#endif


void operand_write(Decode *s, Operand *op, rtlreg_t* src) {
  if (op->type == OP_TYPE_REG) { rtl_sr(s, op->reg, src, op->width); }
  else if (op->type == OP_TYPE_MEM) { rtl_sm(s, s->isa.mbase, s->isa.moff, src, op->width); }
  else { assert(0); }
}
#endif

def_THelper(main) {
  def_INSTR_IDTAB("1011 1???", I2r, movl_I2r);
  def_INSTR_TAB  ("1101 0110",      nemu_trap);
  return table_inv(s);
}

int isa_fetch_decode(Decode *s) {
  int idx = EXEC_ID_inv;
  s->isa.p_instr = s->isa.instr;
  s->isa.is_operand_size_16 = 0;
  s->isa.rep_flags = 0;

  x86_instr_fetch(s, 1);
  idx = table_main(s);

  return idx;
}
