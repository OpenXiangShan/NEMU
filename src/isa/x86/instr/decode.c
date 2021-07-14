#include "../local-include/rtl.h"
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <isa-all-instr.h>

def_all_THelper();

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

enum {
  F_CF = 0x1,
  F_PF = 0x2,
  F_ZF = 0x4,
  F_SF = 0x8,
  F_OF = 0x10,
  F_ALL = F_CF | F_PF | F_ZF | F_SF | F_OF,
};

#ifdef CONFIG_x86_CC_SKIP
static const uint8_t cc2flag [16] = {
  [CC_O] = F_OF, [CC_NO] = F_OF,
  [CC_B] = F_CF, [CC_NB] = F_CF,
  [CC_E] = F_ZF, [CC_NE] = F_ZF,
  [CC_BE] = F_ZF | F_CF, [CC_NBE] = F_ZF | F_CF,
  [CC_S] = F_SF, [CC_NS] = F_SF,
  [CC_P] = F_PF, [CC_NP] = F_PF,
  [CC_L] = F_SF | F_OF, [CC_NL] = F_SF | F_OF,
  [CC_LE] = F_SF | F_OF | F_ZF, [CC_NLE] = F_SF | F_OF | F_ZF,
};

static const struct {
  uint8_t def, use;
} flag_table[TOTAL_INSTR] = {
  [EXEC_ID_add] = { F_ALL, 0 },
  [EXEC_ID_adc] = { F_ALL, F_CF },
  [EXEC_ID_and] = { F_ALL, 0 },
  [EXEC_ID_bsr] = { F_ALL, 0 },
  [EXEC_ID_cmp] = { F_ALL, 0 },
  [EXEC_ID_dec] = { F_ALL & ~F_CF, 0 },
  [EXEC_ID_div] = { F_ALL, 0 },
  [EXEC_ID_idiv] = { F_ALL, 0 },
  [EXEC_ID_imul1] = { F_ALL, 0 },
  [EXEC_ID_imul2] = { F_ALL, 0 },
  [EXEC_ID_imul3] = { F_ALL, 0 },
  [EXEC_ID_inc] = { F_ALL & ~F_CF, 0 },
  [EXEC_ID_jcc] = { 0, F_ALL },  // update `use` at the end of `isa_fetch_decode()`
  [EXEC_ID_mul] = { F_ALL, 0 },
  [EXEC_ID_neg] = { F_ALL, 0 },
  [EXEC_ID_or] = { F_ALL, 0 },
  [EXEC_ID_sar] = { F_ALL, 0 },
  [EXEC_ID_shl] = { F_ALL, 0 },
  [EXEC_ID_shr] = { F_ALL, 0 },
  [EXEC_ID_sbb] = { F_ALL, F_CF },
  [EXEC_ID_setcc] = { 0, F_ALL },  // update `use` at the end of `isa_fetch_decode()`
  [EXEC_ID_sub] = { F_ALL, 0 },
  [EXEC_ID_test] = { F_ALL, 0 },
  [EXEC_ID_xor] = { F_ALL, 0 },
  [EXEC_ID_pushf] = { 0, F_ALL },
  [EXEC_ID_clc] = { F_CF, 0 },
  [EXEC_ID_stc] = { F_CF, 0 },
  [EXEC_ID_cmovcc] = { 0, F_ALL },  // update `use` at the end of `isa_fetch_decode()`
  [EXEC_ID_xadd] = { F_ALL, 0 },
  [EXEC_ID_bt] = { F_ALL, 0 },
};
#endif

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
  rm->type = OP_TYPE_MEM;

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
  op->reg = r;
  if (width == 4) { op->preg = &reg_l(r); }
  else {
    assert(width == 1 || width == 2);
    op->preg = &op->val;
  }
  op->type = OP_TYPE_REG;
  print_Dop(op->str, OP_STR_SIZE, "%%%s", reg_name(r, width));
}

static inline void operand_imm(Decode *s, Operand *op, word_t imm) {
  op->preg = &op->val;
  op->val = imm;
  op->type = OP_TYPE_IMM;
  print_Dop(op->str, OP_STR_SIZE, "$0x%x", imm);
}

// decode operand helper
#define def_DopHelper(name) \
  static inline void concat(decode_op_, name) (Decode *s, Operand *op, int width)

/* Refer to Appendix A in i386 manual for the explanations of these abbreviations */

/* Ib, Iv */
def_DopHelper(I) {
  /* pc here is pointing to the immediate */
  word_t imm = instr_fetch(&s->snpc, width);
  operand_imm(s, op, imm);
}

/* I386 manual does not contain this abbreviation, but it is different from
 * the one above from the view of implementation. So we use another helper
 * function to decode it.
 */
/* sign immediate */
def_DopHelper(SI) {
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
  int r = s->isa.opcode & 0x7;
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
  if (reg != NULL) operand_reg(s, reg, m.reg, width);
  if (m.mod == 3) operand_reg(s, rm, m.R_M, width);
  else { load_addr(s, &m, rm); }
  //s->isa.is_rm_memory = (m.mod != 3);
}

/* Ob, Ov */
def_DopHelper(O) {
  s->isa.moff = instr_fetch(&s->snpc, 4);
  s->isa.mbase = rz;
  s->isa.midx = rz;
  op->preg = &op->val;
  op->type = OP_TYPE_MEM;
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
      IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(CONFIG_PA, cpu.lock = 1));
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

/* Gv <- EvIb
 * Gv <- EvIv
 * use for imul */
static inline def_DHelper(I_E2G) {
  operand_rm(s, id_src1, id_dest, width);
  decode_op_SI(s, id_src2, width); // imul takes the imm as signed
}

/* Eb <- Ib
 * Ev <- Iv
 */

static inline def_DHelper(I2E) {
  IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(CONFIG_PA, cpu.lock = 1));
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
#endif

/* used by test in group3 */
static inline def_DHelper(test_I) {
  decode_op_I(s, id_src1, width);
}

static inline def_DHelper(SI2E) {
  assert(width == 2 || width == 4);
  operand_rm(s, id_dest, NULL, width);
  decode_op_SI(s, id_src1, 1);
  if (width == 2) { *dsrc1 &= 0xffff; }
}

static inline def_DHelper(SI_E2G) {
  assert(width == 2 || width == 4);
  operand_rm(s, id_src1, id_dest, width);
  decode_op_SI(s, id_src2, 1);
}

static inline def_DHelper(1_E) { // use by gp2
  operand_rm(s, id_dest, NULL, width);
  operand_imm(s, id_src1, 1);
}

static inline def_DHelper(cl2E) {  // use by gp2
  //IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(CONFIG_PA, cpu.lock = 1));
  operand_rm(s, id_dest, NULL, width);
  // shift instructions will eventually use the lower
  // 5 bits of %cl, therefore it is OK to load %ecx
  operand_reg(s, id_src1, R_ECX, 4);
}

static inline def_DHelper(Ib2E) { // use by gp2
  //IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(CONFIG_PA, cpu.lock = 1));
  operand_rm(s, id_dest, NULL, width);
  decode_op_I(s, id_src1, 1);
}

/* Ev <- GvIb
 * use for shld/shrd */
static inline def_DHelper(Ib_G2E) {
  //IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(CONFIG_PA, cpu.lock = 1));
  operand_rm(s, id_dest, id_src1, width);
  decode_op_I(s, id_src2, 1);
}

/* Ev <- GvCL
 * use for shld/shrd */
static inline def_DHelper(cl_G2E) {
  //IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(CONFIG_PA, cpu.lock = 1));
  operand_rm(s, id_dest, id_src1, width);
  // shift instructions will eventually use the lower
  // 5 bits of %cl, therefore it is OK to load %ecx
  operand_reg(s, id_src2, R_ECX, 4);
}

#if 0
// for cmpxchg
static inline def_DHelper(a_G2E) {
  IFDEF(CONFIG_DIFFTEST_REF_KVM, IFNDEF(CONFIG_PA, cpu.lock = 1));
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
#endif

// for xchg
static inline def_DHelper(a2r) {
  decode_op_a(s, id_src1, width);
  decode_op_r(s, id_dest, width);
}

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
static inline def_DHelper(Ib2xmm) {
  operand_rm(s, id_dest, false, NULL, false);
  id_src1->width = 1;
  decode_op_I(s, id_src1, true);
}
#endif
#endif


static inline int SSEprefix(Decode *s) {
  assert(!(s->isa.rep_flags != 0 && s->isa.is_operand_size_16));
  if (s->isa.is_operand_size_16) return 1;
  else if (s->isa.rep_flags == PREFIX_REP) return 2;
  else if (s->isa.rep_flags == PREFIX_REPNZ) return 3;
  else return 0;
}

def_THelper(sse_0x6f) {
  int pfx = SSEprefix(s);
  switch (pfx) {
    case 1: decode_E2G(s, s->isa.width); return table_movdqa_E2xmm(s);
  }
  return EXEC_ID_inv;
}

def_THelper(sse_0x73) {
  int pfx = SSEprefix(s);
  assert(pfx == 1);
  def_INSTR_TABW("?? 010 ???", psrlq, -1);
  return EXEC_ID_inv;
}

def_THelper(sse_0x7e) {
  int pfx = SSEprefix(s);
  switch (pfx) {
    case 1: s->isa.width = 4; decode_G2E(s, s->isa.width); return table_movd_xmm2E(s);
    case 2: decode_E2G(s, s->isa.width); return table_movq_E2xmm(s);
  }
  return EXEC_ID_inv;
}

def_THelper(sse_0xd6) {
  int pfx = SSEprefix(s);
  switch (pfx) {
    case 1: decode_G2E(s, s->isa.width); return table_movq_xmm2E(s);
  }
  return EXEC_ID_inv;
}

def_THelper(sse_0xef) {
  int pfx = SSEprefix(s);
  switch (pfx) {
    case 1: decode_E2G(s, s->isa.width); return table_pxor(s);
  }
  return EXEC_ID_inv;
}

def_THelper(main);

def_THelper(operand_size) {
  s->isa.is_operand_size_16 = true;
  return table_main(s);
}

def_THelper(rep) {
#ifndef CONFIG_ENGINE_INTERPRETER
  panic("not support REP in engines other than interpreter");
#endif
  s->isa.rep_flags = PREFIX_REP;
  return table_main(s);
}

def_THelper(lock) {
  return table_main(s);
}

def_THelper(gs) {
  s->isa.sreg_base = &cpu.sreg[CSR_GS].base;
  return table_main(s);
}

#define def_x86_INSTR_IDTABW(decode_fun, pattern, id, tab, w) \
  def_INSTR_raw(decode_fun, pattern, { \
      if (w != -1) s->isa.width = (w == 0 ? (s->isa.is_operand_size_16 ? 2 : 4) : w); \
      concat(decode_, id)(s, s->isa.width); \
      return concat(table_, tab)(s); \
    })

#undef def_INSTR_IDTABW
#define def_INSTR_IDTABW(pattern, id, tab, w) \
  def_x86_INSTR_IDTABW(pattern_decode, pattern, id, tab, w)

#undef def_hex_INSTR_IDTABW
#define def_hex_INSTR_IDTABW(pattern, id, tab, w) \
  def_x86_INSTR_IDTABW(pattern_decode_hex, pattern, id, tab, w)

def_THelper(gp1) {
  def_INSTR_TABW("?? 000 ???", add, -1);
  def_INSTR_TABW("?? 001 ???", or , -1);
  def_INSTR_TABW("?? 010 ???", adc, -1);
  def_INSTR_TABW("?? 011 ???", sbb, -1);
  def_INSTR_TABW("?? 100 ???", and, -1);
  def_INSTR_TABW("?? 101 ???", sub, -1);
  def_INSTR_TABW("?? 110 ???", xor, -1);
  def_INSTR_TABW("?? 111 ???", cmp, -1);
  return EXEC_ID_inv;
}

def_THelper(gp2) {
  def_INSTR_TABW("?? 000 ???", rol, -1);
  def_INSTR_TABW("?? 001 ???", ror, -1);
  def_INSTR_TABW("?? 100 ???", shl, -1);
  def_INSTR_TABW("?? 101 ???", shr, -1);
  def_INSTR_TABW("?? 111 ???", sar, -1);
  return EXEC_ID_inv;
}

def_THelper(gp3) {
  def_INSTR_IDTABW("?? 000 ???", test_I, test, s->isa.width);
  def_INSTR_TABW  ("?? 010 ???", not, -1);
  def_INSTR_TABW  ("?? 011 ???", neg, -1);
  def_INSTR_TABW  ("?? 100 ???", mul, -1);
  def_INSTR_TABW  ("?? 101 ???", imul1, -1);
  def_INSTR_TABW  ("?? 110 ???", div, -1);
  def_INSTR_TABW  ("?? 111 ???", idiv, -1);
  return EXEC_ID_inv;
}

def_THelper(gp4) {
  def_INSTR_TABW("?? 000 ???", inc, -1);
  def_INSTR_TABW("?? 001 ???", dec, -1);
  return EXEC_ID_inv;
}

def_THelper(gp5) {
  def_INSTR_TABW("?? 000 ???", inc, -1);
  def_INSTR_TABW("?? 001 ???", dec, -1);
  def_INSTR_TABW("?? 010 ???", call_E, -1);
  def_INSTR_TABW("?? 100 ???", jmp_E, -1);
  def_INSTR_TABW("?? 110 ???", push, -1);
  return EXEC_ID_inv;
}

def_THelper(gp6) {
  def_INSTR_TABW("?? 011 ???", ltr, -1);
  return EXEC_ID_inv;
}

def_THelper(gp7) {
  def_INSTR_TABW("?? 010 ???", lgdt, -1);
  def_INSTR_TABW("?? 011 ???", lidt, -1);
  return EXEC_ID_inv;
}

def_THelper(_2byte_esc) {
  x86_instr_fetch(s, 1);
  s->isa.opcode = get_instr(s) | 0x100;

  def_hex_INSTR_IDTABW("00",    E, gp6, 2);
  def_hex_INSTR_IDTABW("01",    E, gp7, 4);
  def_hex_INSTR_IDTABW("20",  G2E, mov_cr2r, 4);
  def_hex_INSTR_IDTABW("22",  E2G, mov_r2cr, 4);
  def_hex_INSTR_TAB   ("31",       rdtsc);
  def_INSTR_IDTAB ("0100 ????",  E2G, cmovcc);
  def_hex_INSTR_TAB   ("6f",       sse_0x6f);
  def_hex_INSTR_IDTAB ("73", Ib2E, sse_0x73);
  def_hex_INSTR_TAB   ("7e",       sse_0x7e);
  def_INSTR_IDTABW("1000 ????",    J, jcc, 4);
  def_INSTR_IDTABW("1001 ????",    E, setcc, 1);
  def_hex_INSTR_TAB   ("a2",       cpuid);
  def_hex_INSTR_IDTAB ("a3",  G2E, bt);
  def_hex_INSTR_IDTAB ("a4",Ib_G2E,shld);
  def_hex_INSTR_IDTAB ("a5",cl_G2E,shld);
  def_hex_INSTR_IDTAB ("ab",  G2E, bts);
  def_hex_INSTR_IDTAB ("ac",Ib_G2E,shrd);
  def_hex_INSTR_IDTAB ("ad",cl_G2E,shrd);
  def_hex_INSTR_IDTAB ("af",  E2G, imul2);
  def_hex_INSTR_IDTAB ("b1",  G2E, cmpxchg);
  def_hex_INSTR_IDTAB ("b6", Eb2G, movzb);
  def_hex_INSTR_IDTABW("b7", Ew2G, movzw, 4);
  def_hex_INSTR_IDTAB ("bc",  E2G, bsf);
  def_hex_INSTR_IDTAB ("bd",  E2G, bsr);
  def_hex_INSTR_IDTAB ("be", Eb2G, movsb);
  def_hex_INSTR_IDTABW("bf", Ew2G, movsw, 4);
  def_hex_INSTR_IDTAB ("c1",  G2E, xadd);
  def_hex_INSTR_TAB   ("d6",       sse_0xd6);
  def_hex_INSTR_TAB   ("ef",       sse_0xef);
  return EXEC_ID_inv;
}

#include "fp/decode.h"

def_THelper(main) {
  x86_instr_fetch(s, 1);
  s->isa.opcode = get_instr(s);

  def_hex_INSTR_IDTABW("00",  G2E, add, 1);
  def_hex_INSTR_IDTAB ("01",  G2E, add);
  def_hex_INSTR_IDTABW("02",  E2G, add, 1);
  def_hex_INSTR_IDTAB ("03",  E2G, add);
  def_hex_INSTR_IDTAB ("05",  I2a, add);
  def_hex_INSTR_IDTABW("08",  G2E, or, 1);
  def_hex_INSTR_IDTAB ("09",  G2E, or);
  def_hex_INSTR_IDTABW("0a",  E2G, or, 1);
  def_hex_INSTR_IDTAB ("0b",  E2G, or);
  def_hex_INSTR_IDTABW("0c",  I2a, or, 1);
  def_hex_INSTR_IDTAB ("0d",  I2a, or);
  def_hex_INSTR_TAB   ("0f",       _2byte_esc);
  def_hex_INSTR_IDTABW("10",  G2E, adc, 1);
  def_hex_INSTR_IDTAB ("11",  G2E, adc);
  def_hex_INSTR_IDTAB ("13",  E2G, adc);
  def_hex_INSTR_IDTABW("18",  G2E, sbb, 1);
  def_hex_INSTR_IDTAB ("19",  G2E, sbb);
  def_hex_INSTR_IDTAB ("1b",  E2G, sbb);
  def_hex_INSTR_IDTABW("1c",  I2a, sbb, 1);
  def_hex_INSTR_IDTABW("20",  G2E, and, 1);
  def_hex_INSTR_IDTAB ("21",  G2E, and);
  def_hex_INSTR_IDTABW("22",  E2G, and, 1);
  def_hex_INSTR_IDTAB ("23",  E2G, and);
  def_hex_INSTR_IDTABW("24",  I2a, and, 1);
  def_hex_INSTR_IDTAB ("25",  I2a, and);
  def_hex_INSTR_IDTABW("28",  G2E, sub, 1);
  def_hex_INSTR_IDTAB ("29",  G2E, sub);
  def_hex_INSTR_IDTABW("2a",  E2G, sub, 1);
  def_hex_INSTR_IDTAB ("2b",  E2G, sub);
  def_hex_INSTR_IDTABW("2c",  I2a, sub, 1);
  def_hex_INSTR_IDTAB ("2d",  I2a, sub);
  def_hex_INSTR_IDTABW("30",  G2E, xor, 1);
  def_hex_INSTR_IDTAB ("31",  G2E, xor);
  def_hex_INSTR_IDTABW("32",  E2G, xor, 1);
  def_hex_INSTR_IDTAB ("33",  E2G, xor);
  def_hex_INSTR_IDTABW("34",  I2a, xor, 1);
  def_hex_INSTR_IDTAB ("35",  I2a, xor);
  def_hex_INSTR_IDTABW("38",  G2E, cmp, 1);
  def_hex_INSTR_IDTAB ("39",  G2E, cmp);
  def_hex_INSTR_IDTABW("3a",  E2G, cmp, 1);
  def_hex_INSTR_IDTAB ("3b",  E2G, cmp);
  def_hex_INSTR_IDTABW("3c",  I2a, cmp, 1);
  def_hex_INSTR_IDTAB ("3d",  I2a, cmp);
  def_INSTR_IDTAB ("0100 0???",    r, inc);
  def_INSTR_IDTAB ("0100 1???",    r, dec);
  def_INSTR_IDTAB ("0101 0???",    r, push);
  def_INSTR_IDTAB ("0101 1???",    r, pop);
  def_hex_INSTR_TAB   ("60",       pusha);
  def_hex_INSTR_TAB   ("61",       popa);
  def_hex_INSTR_TAB   ("65",       gs);
  def_hex_INSTR_TAB   ("66",       operand_size);
  def_hex_INSTR_IDTAB ("68",    I, push);
  def_hex_INSTR_IDTAB ("69",I_E2G, imul3);
  def_hex_INSTR_IDTABW("6a",   SI, push, 1);
  def_hex_INSTR_IDTAB ("6b",SI_E2G,imul3);
  def_INSTR_IDTABW("0111 ????",    J, jcc, 1);
  def_hex_INSTR_IDTABW("80",  I2E, gp1, 1);
  def_hex_INSTR_IDTAB ("81",  I2E, gp1);
  def_hex_INSTR_IDTAB ("83", SI2E, gp1);
  def_hex_INSTR_IDTABW("84",  G2E, test, 1);
  def_hex_INSTR_IDTAB ("85",  G2E, test);
  def_hex_INSTR_IDTABW("86",  G2E, xchg, 1);
  def_hex_INSTR_IDTAB ("87",  G2E, xchg);
  def_hex_INSTR_IDTABW("88",  G2E, mov, 1);
  def_hex_INSTR_IDTAB ("89",  G2E, mov);
  def_hex_INSTR_IDTABW("8a",  E2G, mov, 1);
  def_hex_INSTR_IDTAB ("8b",  E2G, mov);
  def_hex_INSTR_IDTABW("8d",  E2G, lea, 4);
  def_hex_INSTR_IDTABW("8e",  E2G, mov_rm2sreg, 2);
  def_hex_INSTR_TAB   ("90",       nop);
  def_INSTR_IDTAB ("1001 0???",  a2r, xchg);
  def_hex_INSTR_TAB   ("98",       cwtl);
  def_hex_INSTR_TAB   ("99",       cltd);
  def_hex_INSTR_TAB   ("9b",       fwait);
  def_hex_INSTR_TAB   ("9c",       pushf);
  def_hex_INSTR_TAB   ("9e",       sahf);
  def_hex_INSTR_IDTABW("a0",  O2a, mov, 1);
  def_hex_INSTR_IDTAB ("a1",  O2a, mov);
  def_hex_INSTR_IDTABW("a2",  a2O, mov, 1);
  def_hex_INSTR_IDTAB ("a3",  a2O, mov);

  if (s->isa.rep_flags == PREFIX_REP) {
    def_hex_INSTR_TABW  ("a4", rep_movs, 1);
    def_hex_INSTR_TAB   ("a5", rep_movs);
    def_hex_INSTR_TABW  ("a6", repz_cmps, 1);
    def_hex_INSTR_TABW  ("aa", rep_stos, 1);
    def_hex_INSTR_TAB   ("ab", rep_stos);
  }

  def_hex_INSTR_TABW  ("a4",       movs, 1);
  def_hex_INSTR_TAB   ("a5",       movs);
  def_hex_INSTR_IDTABW("a8",  I2a, test, 1);
  def_hex_INSTR_IDTAB ("a9",  I2a, test);
  def_hex_INSTR_TABW  ("aa", stos, 1);
  def_INSTR_IDTABW("1011 0???",  I2r, mov, 1);
  def_INSTR_IDTAB ("1011 1???",  I2r, mov);
  def_hex_INSTR_IDTABW("c0", Ib2E, gp2, 1);
  def_hex_INSTR_IDTAB ("c1", Ib2E, gp2);
  def_hex_INSTR_IDTABW("c2",    I, ret_imm, 2);
  def_hex_INSTR_TAB   ("c3",       ret);
  def_hex_INSTR_IDTABW("c6",  I2E, mov, 1);
  def_hex_INSTR_IDTAB ("c7",  I2E, mov);
  def_hex_INSTR_TAB   ("c9",       leave);
  def_hex_INSTR_IDTABW("cd",    I, _int, 1);
  def_hex_INSTR_TAB   ("cf",       iret);
  def_hex_INSTR_IDTABW("d0",  1_E, gp2, 1);
  def_hex_INSTR_IDTAB ("d1",  1_E, gp2);
  def_hex_INSTR_IDTABW("d2", cl2E, gp2, 1);
  def_hex_INSTR_IDTAB ("d3", cl2E, gp2);
  def_hex_INSTR_TAB   ("d6",       nemu_trap);
  def_hex_INSTR_TAB   ("d8",       fpu_d8);
  def_hex_INSTR_TAB   ("d9",       fpu_d9);
  def_hex_INSTR_TAB   ("da",       fpu_da);
  def_hex_INSTR_TAB   ("db",       fpu_db);
  def_hex_INSTR_TAB   ("dc",       fpu_dc);
  def_hex_INSTR_TAB   ("dd",       fpu_dd);
  def_hex_INSTR_TAB   ("de",       fpu_de);
  def_hex_INSTR_TAB   ("df",       fpu_df);
  def_hex_INSTR_IDTABW("e3",    J, jecxz, 1);
  def_hex_INSTR_IDTABW("e8",    J, call, 4);
  def_hex_INSTR_IDTABW("e9",    J,  jmp, 4);
  def_hex_INSTR_IDTABW("eb",    J,  jmp, 1);
  def_hex_INSTR_IDTAB ("ed", dx2a, in);
  def_hex_INSTR_IDTABW("ee", a2dx, out, 1);
  def_hex_INSTR_IDTAB ("ef", a2dx, out);
  def_hex_INSTR_TAB   ("f0",       lock);
  def_hex_INSTR_IDTABW("f6",    E, gp3, 1);
  def_hex_INSTR_IDTAB ("f7",    E, gp3);
  //def_hex_INSTR_TAB   ("f2",       repnz);
  def_hex_INSTR_TAB   ("f3",       rep);
  def_hex_INSTR_TAB   ("f8",       clc);
  def_hex_INSTR_TAB   ("f9",       stc);
  def_hex_INSTR_TAB   ("fc",       cld);
  def_hex_INSTR_TAB   ("fd",       std);
  def_hex_INSTR_IDTABW("fe",    E, gp4, 1);
  def_hex_INSTR_IDTAB ("ff",    E, gp5);
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

    case EXEC_ID_jcc: case EXEC_ID_jecxz:
      s->jnpc = id_dest->imm; s->type = INSTR_TYPE_B; break;
    case EXEC_ID_rep_movs:
    case EXEC_ID_rep_stos:
    case EXEC_ID_repz_cmps:
      s->jnpc = s->pc; s->type = INSTR_TYPE_B; break;

    case EXEC_ID_ret: case EXEC_ID_call_E: case EXEC_ID_jmp_E: case EXEC_ID_ret_imm:
    case EXEC_ID__int: case EXEC_ID_iret:
      s->type = INSTR_TYPE_I; break;
  }

#ifdef CONFIG_x86_CC_SKIP
  s->isa.flag_def = flag_table[idx].def;
  s->isa.flag_use = flag_table[idx].use;
  if (idx == EXEC_ID_jcc || idx == EXEC_ID_setcc || idx == EXEC_ID_cmovcc) {
    s->isa.flag_use = cc2flag[s->isa.opcode & 0xf];
  }

  static Decode *bb_start = NULL;
  static int bb_idx = 0;

  if (bb_idx == 0) bb_start = s;

  if (s->type != INSTR_TYPE_N) { // the end of a basic block
    if (s - bb_start == bb_idx) {
      // now scan and update `flag_def`
      Decode *p;
      //uint32_t use = s->isa.flag_use;
      uint32_t use = F_ALL; //s->isa.flag_use;
      for (p = s - 1; p >= bb_start; p --) {
        uint32_t real_def = p->isa.flag_def & use;
        use &= ~p->isa.flag_def;
        use |=  p->isa.flag_use;
        p->isa.flag_def = real_def;
      }
    }
    bb_idx = 0;
  } else {
    bb_idx ++;
  }
#endif

  return idx;
}
