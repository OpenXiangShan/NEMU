#include "../local-include/rtl.h"
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <isa-all-instr.h>

def_all_THelper();

static inline int x86_width_decode(Decode *s, int width) {
  if (width == 0) return (s->isa.is_operand_size_16 ? 2 : 4);
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

static inline void load_addr(Decode *s, ModR_M *m, Operand *rm) {
  assert(m->mod != 3);

  sword_t disp = 0;
  int disp_size = 4;
  int base_reg = -1, index_reg = -1, scale = 0;

  if (m->R_M == R_ESP) {
    SIB sib;
    sib.val = instr_fetch(&s->snpc, 1);
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
    disp = instr_fetch(&s->snpc, disp_size);
    if (disp_size == 1) { disp = (int8_t)disp; }
  }

  s->isa.mbase = (base_reg != -1 ? &reg_l(base_reg) : rz);
  s->isa.midx = (index_reg != -1 ? &reg_l(index_reg) : rz);
  s->isa.mscale = scale;
  s->isa.moff = disp;
  rm->preg = &rm->val;

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
  else { sprintf(base_buf, "%%%s", reg_name(base_reg, 4)); }

  if (index_reg == -1) { index_buf[0] = '\0'; }
  else { sprintf(index_buf, ",%%%s,%d", reg_name(index_reg, 4), 1 << scale); }

  if (base_reg == -1 && index_reg == -1) { sprintf(rm->str, "%s", disp_buf); }
  else { sprintf(rm->str, "%s(%s%s)", disp_buf, base_buf, index_buf); }
#endif
}

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
  word_t imm = instr_fetch(&s->snpc, width);
  operand_imm(s, op, imm);
}

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
  word_t imm = instr_fetch(&s->snpc, width);
  if (width == 1) imm = (int8_t)imm;
  else if (width == 2) imm = (int16_t)imm;
  operand_imm(s, op, imm);
#endif
}

/* I386 manual does not contain this abbreviation.
 * It is convenient to merge them into a single helper function.
 */
/* AL/eAX */
def_DopHelper(a) {
  operand_reg(s, op, R_EAX, width);
}

/* This helper function is use to decode register encoded in the opcode. */
/* XX: AL, AH, BL, BH, CL, CH, DL, DH
 * eXX: eAX, eCX, eDX, eBX, eSP, eBP, eSI, eDI
 */
def_DopHelper(r) {
  int r = get_instr(s) & 0x7;
  operand_reg(s, op, r, width);
}

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
static inline void operand_rm(Decode *s, Operand *rm, Operand *reg, int width) {
  ModR_M m;
  m.val = x86_instr_fetch(s, 1);
  s->isa.ext_opcode = m.opcode;
  if (reg != NULL) operand_reg(s, reg, m.reg, width);
  if (m.mod == 3) operand_reg(s, rm, m.R_M, width);
  else { load_addr(s, &m, rm); }
  s->isa.is_rm_memory = (m.mod != 3);
}

/* Ob, Ov */
def_DopHelper(O) {
  s->isa.moff = instr_fetch(&s->snpc, 4);
  s->isa.mbase = s->isa.sreg_base ? s->isa.sreg_base : rz;
  op->preg = &op->val;
  print_Dop(op->str, OP_STR_SIZE, "0x%x", s->isa.moff);
}

/* Eb <- Gb
 * Ev <- Gv
 */
static inline def_DHelper(G2E) {
  operand_rm(s, id_dest, id_src1, width);
}

#if 0
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
#endif

/* Gb <- Eb
 * Gv <- Ev
 */
static inline def_DHelper(E2G) {
  operand_rm(s, id_src1, id_dest, width);
}

static inline def_DHelper(Eb2G) {
  operand_rm(s, id_src1, id_dest, 1);
  // overwrite the wrong decode result by `operand_rm()` with the correct width
  operand_reg(s, id_dest, id_dest->reg, width);
}

static inline def_DHelper(Ew2G) {
  operand_rm(s, id_src1, id_dest, 2);
  // overwrite the wrong decode result by `operand_rm()` with the correct width
  operand_reg(s, id_dest, id_dest->reg, width);
}

/* AL <- Ib
 * eAX <- Iv
 */
static inline def_DHelper(I2a) {
  decode_op_a(s, id_dest, width);
  decode_op_I(s, id_src1, width);
}

#if 0

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

#endif

static inline def_DHelper(I2E) {
  operand_rm(s, id_dest, NULL, width);
  decode_op_I(s, id_src1, width);
}

/* XX <- Ib
 * eXX <- Iv
 */
static inline def_DHelper(I2r) {
  decode_op_r(s, id_dest, width);
  decode_op_I(s, id_src1, width);
}

/* used by unary operations */
static inline def_DHelper(I) {
  decode_op_I(s, id_dest, width);
}

static inline def_DHelper(SI) {
  decode_op_SI(s, id_dest, width);
}

static inline def_DHelper(r) {
  decode_op_r(s, id_dest, width);
}

static inline def_DHelper(E) {
  operand_rm(s, id_dest, NULL, width);
}

#if 0
static inline def_DHelper(gp6_E) {
  operand_rm(s, id_dest, true, NULL, false);
}

static inline def_DHelper(gp7_E) {
  operand_rm(s, id_dest, false, NULL, false);
}
#endif

/* used by test in group3 */
static inline def_DHelper(test_I) {
  decode_op_I(s, id_src1, width);
}

static inline def_DHelper(SI2E) {
  width = x86_width_decode(s, width);
  assert(width == 2 || width == 4);
  operand_rm(s, id_dest, NULL, width);
  decode_op_SI(s, id_src1, 1);
  if (width == 2) { *dsrc1 &= 0xffff; }
}

#if 0
static inline def_DHelper(SI_E2G) {
  assert(id_dest->width == 2 || id_dest->width == 4);
  operand_rm(s, id_src2, true, id_dest, false);
  id_src1->width = 1;
  decode_op_SI(s, id_src1, true);
}
#endif

static inline def_DHelper(1_E) { // use by gp2
  operand_rm(s, id_dest, NULL, width);
  operand_imm(s, id_src1, 1);
}

static inline def_DHelper(cl2E) {  // use by gp2
  //IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  operand_rm(s, id_dest, NULL, width);
  // shift instructions will eventually use the lower
  // 5 bits of %cl, therefore it is OK to load %ecx
  operand_reg(s, id_src1, R_ECX, 4);
}

static inline def_DHelper(Ib2E) { // use by gp2
  //IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(__PA__, cpu.lock = 1));
  operand_rm(s, id_dest, NULL, width);
  decode_op_I(s, id_src1, 1);
}

#if 0
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
#endif

static inline def_DHelper(O2a) {
  decode_op_O(s, id_src1, 0);
  decode_op_a(s, id_dest, width);
}

static inline def_DHelper(a2O) {
  decode_op_a(s, id_src1, width);
  decode_op_O(s, id_dest, 0);
}

#if 0
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
#endif

static inline def_DHelper(J) {
  // the target address can be computed in the decode stage
  decode_op_SI(s, id_dest, width);
  id_dest->imm = id_dest->val + s->snpc;
  operand_imm(s, id_src1, s->snpc);  // for call
}
#if 0
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

static inline def_DHelper(in_I2a) {
  id_src1->width = 1;
  decode_op_I(s, id_src1, true);
  decode_op_a(s, id_dest, false);
}
#endif

static inline def_DHelper(dx2a) {
  operand_reg(s, id_src1, R_DX, 2);
  decode_op_a(s, id_dest, width);
}

#if 0
static inline def_DHelper(out_a2I) {
  decode_op_a(s, id_src1, true);
  id_dest->width = 1;
  decode_op_I(s, id_dest, true);
}
#endif

static inline def_DHelper(a2dx) {
  decode_op_a(s, id_src1, width);
  operand_reg(s, id_dest, R_DX, 2);
}

#if 0
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

def_THelper(main);

def_THelper(operand_size) {
  s->isa.is_operand_size_16 = true;
  return table_main(s);
}

#define x86_def_INSTR_IDTABV(pattern, id, tab) \
  def_INSTR_raw(pattern, { concat(decode_, id)(s, 0); \
    if (s->isa.is_operand_size_16) return concat5(table_, tab, w, _, id)(s); \
    else return concat5(table_, tab, l, _, id)(s); \
  })
#define x86_def_INSTR_TABV(pattern, id, tab) \
  def_INSTR_raw(pattern, { \
    if (s->isa.is_operand_size_16) return concat5(table_, tab, w, _, id)(s); \
    else return concat5(table_, tab, l, _, id)(s); \
  })
#define x86_def_INSTR_IDTABW def_INSTR_IDTABW
#define x86_def_INSTR_IDTAB  def_INSTR_IDTAB
#define x86_def_INSTR_TABW   def_INSTR_TABW
#define x86_def_INSTR_TAB    def_INSTR_TAB

def_THelper(gp1_I2E_b) {
//  x86_def_INSTR_TAB("?? 100 ???", andb_I2E);
//  x86_def_INSTR_TAB("?? 101 ???", subb_I2E);
  x86_def_INSTR_TAB("?? 111 ???", cmpb_I2E);
  return EXEC_ID_inv;
}

def_THelper(gp1_I2E) {
//  x86_def_INSTR_TABV("?? 000 ???", I2E, add);
  x86_def_INSTR_TABV("?? 100 ???", I2E, and);
  x86_def_INSTR_TABV("?? 101 ???", I2E, sub);
  x86_def_INSTR_TABV("?? 111 ???", I2E, cmp);
  return EXEC_ID_inv;
}

def_THelper(gp1_SI2E) {
  x86_def_INSTR_TABV("?? 000 ???", SI2E, add);
  x86_def_INSTR_TABV("?? 001 ???", SI2E, or);
  x86_def_INSTR_TABV("?? 010 ???", SI2E, adc);
  x86_def_INSTR_TABV("?? 100 ???", SI2E, and);
  x86_def_INSTR_TABV("?? 101 ???", SI2E, sub);
  x86_def_INSTR_TABV("?? 110 ???", SI2E, xor);
  x86_def_INSTR_TABV("?? 111 ???", SI2E, cmp);
  return EXEC_ID_inv;
}

def_THelper(gp2_1_E) {
//  x86_def_INSTR_TABV("?? 100 ???", cl2E, shl);
  x86_def_INSTR_TABV("?? 101 ???", 1_E, shr);
  x86_def_INSTR_TABV("?? 111 ???", 1_E, sar);
  return EXEC_ID_inv;
}

def_THelper(gp2_cl2E) {
  x86_def_INSTR_TABV("?? 000 ???", cl2E, rol);
  x86_def_INSTR_TABV("?? 100 ???", cl2E, shl);
  x86_def_INSTR_TABV("?? 101 ???", cl2E, shr);
  x86_def_INSTR_TABV("?? 111 ???", cl2E, sar);
  return EXEC_ID_inv;
}

def_THelper(gp2_Ib2E) {
  x86_def_INSTR_TABV("?? 100 ???", Ib2E, shl);
  x86_def_INSTR_TABV("?? 101 ???", Ib2E, shr);
  x86_def_INSTR_TABV("?? 111 ???", Ib2E, sar);
  return EXEC_ID_inv;
}

def_THelper(gp3_b) {
  x86_def_INSTR_IDTABW("?? 000 ???", test_I, testb_I2E, 1);
  return EXEC_ID_inv;
}

def_THelper(test_gp3) {
  if (s->isa.is_operand_size_16) return table_testw_I2E(s);
  else return table_testl_I2E(s);
}

def_THelper(gp3) {
  x86_def_INSTR_IDTAB("?? 000 ???", test_I, test_gp3);
  x86_def_INSTR_TABV("?? 010 ???", E, not);
  x86_def_INSTR_TABV("?? 011 ???", E, neg);
  x86_def_INSTR_TABV("?? 100 ???", E, mul);
  x86_def_INSTR_TABV("?? 101 ???", E, imul);
  x86_def_INSTR_TABV("?? 110 ???", E, div);
  x86_def_INSTR_TABV("?? 111 ???", E, idiv);
  return EXEC_ID_inv;
}

def_THelper(gp5) {
  x86_def_INSTR_TABV("?? 000 ???", E, inc);
  x86_def_INSTR_TABV("?? 001 ???", E, dec);
  x86_def_INSTR_TAB ("?? 010 ???", call_E);
  x86_def_INSTR_TAB ("?? 100 ???", jmp_E);
  x86_def_INSTR_TABV("?? 110 ???", E, push);
  return EXEC_ID_inv;
}

def_THelper(_2byte_esc) {
  x86_instr_fetch(s, 1);
  s->isa.opcode = get_instr(s) | 0x100;

  x86_def_INSTR_IDTABW("1000 ????",    J, jcc, 4);
  x86_def_INSTR_IDTABW("1001 ????",    E, setcc, 1);
  x86_def_INSTR_IDTABV("1010 1111",  E2G, imul);
  x86_def_INSTR_IDTABV("1011 0110", Eb2G, movzb);
  x86_def_INSTR_IDTABW("1011 0111", Ew2G, movzwl_Ew2G, 4);
  x86_def_INSTR_IDTABV("1011 1101",  E2G, bsr);
  x86_def_INSTR_IDTABV("1011 1110", Eb2G, movsb);
  x86_def_INSTR_IDTABW("1011 1111", Ew2G, movswl_Ew2G, 4);
  return EXEC_ID_inv;
}

def_THelper(main) {
  x86_instr_fetch(s, 1);
  s->isa.opcode = get_instr(s);

  x86_def_INSTR_IDTABW("0000 0000",  G2E, addb_G2E, 1);
  x86_def_INSTR_IDTABV("0000 0001",  G2E, add);
  x86_def_INSTR_IDTABW("0000 0010",  E2G, addb_E2G, 1);
  x86_def_INSTR_IDTABV("0000 0011",  E2G, add);
  x86_def_INSTR_IDTABV("0000 0101",  I2a, add);
  x86_def_INSTR_IDTABV("0000 1001",  G2E, or);
  x86_def_INSTR_IDTABV("0000 1011",  E2G, or);
  x86_def_INSTR_IDTABV("0000 1101",  I2a, or);
  x86_def_INSTR_IDTABW("0000 1010",  E2G, orb_E2G, 1);
  x86_def_INSTR_TAB   ("0000 1111",       _2byte_esc);
  x86_def_INSTR_IDTABV("0001 0001",  G2E, adc);
  x86_def_INSTR_IDTABV("0001 0011",  E2G, adc);
  x86_def_INSTR_IDTABV("0001 1001",  G2E, sbb);
  x86_def_INSTR_IDTABV("0001 1011",  E2G, sbb);
  x86_def_INSTR_IDTABV("0010 0001",  G2E, and);
  x86_def_INSTR_IDTABW("0010 0010",  E2G, andb_E2G, 1);
  x86_def_INSTR_IDTABV("0010 0011",  E2G, and);
  x86_def_INSTR_IDTABV("0010 0101",  I2a, and);
  x86_def_INSTR_IDTABV("0010 1001",  G2E, sub);
  x86_def_INSTR_IDTABV("0010 1011",  E2G, sub);
  x86_def_INSTR_IDTABV("0011 0001",  G2E, xor);
  x86_def_INSTR_IDTABV("0011 0011",  E2G, xor);
  x86_def_INSTR_IDTABW("0011 1000",  G2E, cmpb_G2E, 1);
  x86_def_INSTR_IDTABV("0011 1001",  G2E, cmp);
  x86_def_INSTR_IDTABV("0011 1011",  E2G, cmp);
  x86_def_INSTR_IDTABW("0011 1100",  I2a, cmpb_I2a, 1);
  x86_def_INSTR_IDTABV("0011 1101",  I2a, cmp);
  x86_def_INSTR_IDTABV("0100 0???",    r, inc);
  x86_def_INSTR_IDTABV("0100 1???",    r, dec);
  x86_def_INSTR_IDTABV("0101 0???",    r, push);
  x86_def_INSTR_IDTABV("0101 1???",    r, pop);
  x86_def_INSTR_TAB   ("0110 0110",       operand_size);
  x86_def_INSTR_IDTABV("0110 1000",    I, push);
  x86_def_INSTR_IDTABW("0110 1010",   SI, pushb_SI, 1);
  x86_def_INSTR_IDTABW("0111 ????",    J, jcc, 1);
  x86_def_INSTR_IDTABW("1000 0000",  I2E, gp1_I2E_b, 1);
  x86_def_INSTR_IDTAB ("1000 0001",  I2E, gp1_I2E);
  x86_def_INSTR_IDTAB ("1000 0011", SI2E, gp1_SI2E);
  x86_def_INSTR_IDTABW("1000 0100",  G2E, testb_G2E, 1);
  x86_def_INSTR_IDTABV("1000 0101",  G2E, test);
  x86_def_INSTR_IDTABW("1000 1000",  G2E, movb_G2E, 1);
  x86_def_INSTR_IDTABV("1000 1001",  G2E, mov);
  x86_def_INSTR_IDTABW("1000 1010",  E2G, movb_E2G, 1);
  x86_def_INSTR_IDTABV("1000 1011",  E2G, mov);
  x86_def_INSTR_IDTABW("1000 1101",  E2G, lea, 4);
  x86_def_INSTR_TAB   ("1001 0000",       nop);
  x86_def_INSTR_TAB   ("1001 1001",       cltd);
  x86_def_INSTR_IDTABW("1010 0000",  O2a, movb_O2a, 1);
  x86_def_INSTR_IDTABV("1010 0001",  O2a, mov);
  x86_def_INSTR_IDTABW("1010 0010",  a2O, movb_a2O, 1);
  x86_def_INSTR_IDTABV("1010 0011",  a2O, mov);
  x86_def_INSTR_TABW  ("1010 0100",       movsb, 1);
  x86_def_INSTR_IDTABW("1010 1000",  I2a, testb_I2a, 1);
  x86_def_INSTR_IDTABW("1011 0???",  I2r, movb_I2r, 1);
  x86_def_INSTR_IDTABV("1011 1???",  I2r, mov);
  x86_def_INSTR_IDTAB ("1100 0001", Ib2E, gp2_Ib2E);
  x86_def_INSTR_TAB   ("1100 0011",       ret);
  x86_def_INSTR_IDTABW("1100 0110",  I2E, movb_I2E, 1);
  x86_def_INSTR_IDTABV("1100 0111",  I2E, mov);
  x86_def_INSTR_TAB   ("1100 1001",       leave);
  x86_def_INSTR_IDTAB ("1101 0001",  1_E, gp2_1_E);
  x86_def_INSTR_IDTAB ("1101 0011", cl2E, gp2_cl2E);
  x86_def_INSTR_TAB   ("1101 0110",       nemu_trap);
  x86_def_INSTR_IDTABW("1110 1000",    J, call, 4);
  x86_def_INSTR_IDTABW("1110 1001",    J,  jmp, 4);
  x86_def_INSTR_IDTABW("1110 1011",    J,  jmp, 1);
  x86_def_INSTR_IDTABV("1110 1101", dx2a, in);
  x86_def_INSTR_IDTABW("1110 1110", a2dx, outb_a2dx, 1);
  x86_def_INSTR_IDTABW("1111 0110",    E, gp3_b, 1);
  x86_def_INSTR_IDTAB ("1111 0111",    E, gp3);
  x86_def_INSTR_IDTAB ("1111 1111",    E, gp5);
  return table_inv(s);
}

int isa_fetch_decode(Decode *s) {
  int idx = EXEC_ID_inv;
  s->isa.p_instr = s->isa.instr;
  s->isa.is_operand_size_16 = 0;
  s->isa.rep_flags = 0;
  s->isa.sreg_base = NULL;

  idx = table_main(s);

  s->type = INSTR_TYPE_N;
  switch (idx) {
    case EXEC_ID_call: case EXEC_ID_jmp:
      s->jnpc = id_dest->imm; s->type = INSTR_TYPE_J; break;

    case EXEC_ID_jcc:
      s->jnpc = id_dest->imm; s->type = INSTR_TYPE_B; break;

    case EXEC_ID_ret: case EXEC_ID_call_E: case EXEC_ID_jmp_E:
      s->type = INSTR_TYPE_I; break;
  }

  return idx;
}
