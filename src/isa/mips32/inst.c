#include "local-include/reg.h"
#include "local-include/intr.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>

#define R(i) gpr(i)
#define Mr(addr, len)       ({ word_t tmp = vaddr_read(s, addr, len, MMU_DYNAMIC); check_ex(); tmp; })
#define Mw(addr, len, data) vaddr_write(s, addr, len, data, MMU_DYNAMIC); check_ex()
#define check_ex() do { \
  if (g_ex_cause != NEMU_EXEC_RUNNING) { \
    cpu.pc = isa_raise_intr(g_ex_cause, s->pc); \
    g_ex_cause = NEMU_EXEC_RUNNING; \
    return 0; \
  } \
} while (0)

enum {
  TYPE_U, TYPE_R, TYPE_I,
  TYPE_J, TYPE_S, TYPE_B,
  TYPE_jalr, TYPE_N, // none
};

void csrrw(word_t *dest, const word_t *src, uint32_t csrid);
word_t priv_instr(uint32_t op, const word_t *src);
uint32_t clz(uint32_t x);

static word_t immU(uint32_t i) { return BITS(i, 15, 0); }
static word_t immI(uint32_t i) { return SEXT(BITS(i, 15, 0), 16); }
static word_t immS(uint32_t i) { return BITS(i, 10, 6); }
static word_t immJ(uint32_t i) { return BITS(i, 25, 0) << 2; }
static word_t immB(uint32_t i) { return immI(i) << 2; }

#ifdef CONFIG_PERF_OPT
#include <isa-all-instr.h>
static void decode_i(Operand *op, word_t val)  { op->imm = val; }
static void decode_r(Operand *op, word_t *reg) { op->preg = reg; }

static void decode_operand(Decode *s, word_t *src1, word_t *src2, word_t *imm, int type) {
  static word_t zero_null = 0;
  uint32_t i = s->isa.instr.val;
  int rd = BITS(i, 15, 11);
  int rt = BITS(i, 20, 16);
  int rs = BITS(i, 25, 21);
#define Rr(n) &R(n)
#define Rw(n) (n == 0 ? &zero_null : &R(n))
  decode_r(id_dest, Rw((type == TYPE_U || type == TYPE_I ? rt : rd)));
  switch (type) {
    case TYPE_U: decode_r(id_src1, Rr(rs)); decode_i(id_src2, immU(i)); break;
    case TYPE_I: decode_r(id_src1, Rr(rs)); decode_i(id_src2, immI(i)); break;
    case TYPE_J: decode_i(id_dest, (s->pc & 0xf0000000) | immJ(i)); decode_i(id_src2, s->pc + 8); break;
    case TYPE_S: decode_i(id_src1, immS(i)); decode_r(id_src2, Rr(rt)); break;
    case TYPE_B: decode_i(id_dest, s->pc + immB(i) + 4); // fall through
    case TYPE_R: decode_r(id_src1, Rr(rs)); decode_r(id_src2, Rr(rt)); break;
    case TYPE_jalr: decode_r(id_src1, Rr(rs)); decode_i(id_src2, s->pc + 8); break;
  }
}
#else
static void decode_operand(Decode *s, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.instr.val;
  int rt = BITS(i, 20, 16);
  int rs = BITS(i, 25, 21);
  switch (type) {
    case TYPE_U: *src1 = R(rs); *src2 = immU(i); break;
    case TYPE_I: *src1 = R(rs); *src2 = immI(i); break;
    case TYPE_J: *imm = (s->pc & 0xf0000000) | immJ(i); break;
    case TYPE_S: *src1 = immS(i); *src2 = R(rt); break;
    case TYPE_B: *imm = s->pc + immB(i) + 4; // fall through
    case TYPE_R: *src1 = R(rs); *src2 = R(rt); break;
    case TYPE_jalr: *src1 = R(rs); break;
  }
}
#endif

static void jcond(bool cond, vaddr_t target) {
  difftest_skip_ref();
  if (cond) cpu.pc = target;
}

static word_t lwl(Decode *s, uint32_t rt, vaddr_t addr) {
  // mem.shmat2
  uint32_t mem_shmat = 24 - ((addr & 0x3) * 8);
  // load the aligned memory word and prepare memory data
  word_t mem_data = Mr((addr & ~0x3u), 4) << mem_shmat;
  // reg.shmat = 24 - mem.shmat2
  uint32_t reg_shmat = 24 - mem_shmat;
  // prepare register data
  word_t reg_data = (R(rt) << (8 + reg_shmat)) >> (8 + reg_shmat);
  // merge the word
  R(rt) = reg_data | mem_data;
  return 0;
}

static word_t lwr(Decode *s, uint32_t rt, vaddr_t addr) {
  // mem.shmat2
  uint32_t mem_shmat = (addr & 0x3) * 8;
  // load the aligned memory word and prepare memory data
  word_t mem_data = Mr((addr & ~0x3u), 4) >> mem_shmat;
  // reg.shmat = 24 - mem.shmat2
  uint32_t reg_shmat = 24 - mem_shmat;
  // prepare register data
  word_t reg_data = (R(rt) >> (8 + reg_shmat)) << (8 + reg_shmat);
  // merge the word
  R(rt) = reg_data | mem_data;
  return 0;
}

static word_t swl(Decode *s, uint32_t rt, vaddr_t addr) {
  // mem.shmat2
  uint32_t mem_shmat = (addr & 0x3) * 8;
  // load the aligned memory word and prepare memory data
  word_t mem_data = Mr((addr & ~0x3u), 4);
  mem_data = (mem_data >> (8 + mem_shmat)) << (8 + mem_shmat);
  // reg.shmat = 24 - mem.shmat2
  uint32_t reg_shmat = 24 - mem_shmat;
  // prepare register data
  word_t reg_data = R(rt) >> reg_shmat;
  // merge the word
  mem_data = reg_data | mem_data;
  // write back
  Mw((addr & ~0x3u), 4, mem_data);
  return 0;
}

static word_t swr(Decode *s, uint32_t rt, vaddr_t addr) {
  // mem.shmat2
  uint32_t mem_shmat = 24 - (addr & 0x3) * 8;
  // load the aligned memory word and prepare memory data
  word_t mem_data = Mr((addr & ~0x3u), 4);
  mem_data = (mem_data << (8 + mem_shmat)) >> (8 + mem_shmat);
  // reg.shmat = 24 - mem.shmat2
  uint32_t reg_shmat = 24 - mem_shmat;
  // prepare register data
  word_t reg_data = R(rt) << reg_shmat;
  // merge the word
  mem_data = reg_data | mem_data;
  // write back
  Mw((addr & ~0x3u), 4, mem_data);
  return 0;
}

static int decode_exec(Decode *s) {
  int rd = BITS(s->isa.instr.val, 15, 11);
  int rt = BITS(s->isa.instr.val, 20, 16);
  word_t imm = 0, src1 = 0, src2 = 0;
  cpu.pc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.instr.val)
#define INSTPAT_MATCH(s, name, type, ... /* body */ ) { \
  decode_operand(s, &src1, &src2, &imm, concat(TYPE_, type)); \
  IFDEF(CONFIG_PERF_OPT, return concat(EXEC_ID_, name)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();
  INSTPAT("000000 00000 ????? ????? ????? 000000", sll    , S, R(rd) = src2 << src1);
  INSTPAT("000000 00000 ????? ????? ????? 000010", srl    , S, R(rd) = src2 >> src1);
  INSTPAT("000000 00000 ????? ????? ????? 000011", sra    , S, R(rd) = (sword_t)src2 >> src1);
  INSTPAT("000000 ????? ????? ????? 00000 000100", sllv   , R, R(rd) = src2 << src1);
  INSTPAT("000000 ????? ????? ????? 00000 000110", srlv   , R, R(rd) = src2 >> src1);
  INSTPAT("000000 ????? ????? ????? 00000 000111", srav   , R, R(rd) = (sword_t)src2 >> src1);
  INSTPAT("000000 ????? 00000 00000 ????? 001000", jr     , R, jcond(true, src1));
  INSTPAT("000000 ????? 00000 ????? ????? 001001", jalr   , jalr, R(rd) = s->pc + 8; jcond(true, src1));
  INSTPAT("000000 ????? ????? ????? 00000 001010", movz   , R, if (src2 == 0) R(rd) = src1);
  INSTPAT("000000 ????? ????? ????? 00000 001011", movn   , R, if (src2 != 0) R(rd) = src1);
  INSTPAT("000000 ????? ????? ????? ????? 001100", syscall, N, cpu.pc = isa_raise_intr(EX_SYSCALL, s->pc));
  INSTPAT("000000 00000 00000 ????? 00000 010000", mfhi   , R, R(rd) = cpu.hi);
  INSTPAT("000000 ????? 00000 00000 00000 010001", mthi   , R, cpu.hi = src1);
  INSTPAT("000000 00000 00000 ????? 00000 010010", mflo   , R, R(rd) = cpu.lo);
  INSTPAT("000000 ????? 00000 00000 00000 010011", mtlo   , R, cpu.lo = src1);
  INSTPAT("000000 ????? ????? 00000 00000 011000", mult   , R,
      uint64_t res = (int64_t)(int32_t)src1 * (int64_t)(int32_t)src2;
      cpu.lo = (uint32_t)res, cpu.hi = res >> 32);
  INSTPAT("000000 ????? ????? 00000 00000 011001", multu  , R,
      uint64_t res = (uint64_t)src1 * src2;
      cpu.lo = (uint32_t)res, cpu.hi = res >> 32);
  INSTPAT("000000 ????? ????? 00000 00000 011010", div    , R, cpu.lo = (int32_t)src1 / (int32_t)src2,
                                                               cpu.hi = (int32_t)src1 % (int32_t)src2);
  INSTPAT("000000 ????? ????? 00000 00000 011011", divu   , R, cpu.lo = src1 / src2, cpu.hi = src1 % src2);
  INSTPAT("000000 ????? ????? ????? 00000 100001", add    , R, R(rd) = src1 + src2);
  INSTPAT("000000 ????? ????? ????? 00000 100011", sub    , R, R(rd) = src1 - src2);
  INSTPAT("000000 ????? ????? ????? 00000 100100", and    , R, R(rd) = src1 & src2);
  INSTPAT("000000 ????? ????? ????? 00000 100101", or     , R, R(rd) = src1 | src2);
  INSTPAT("000000 ????? ????? ????? 00000 100110", xor    , R, R(rd) = src1 ^ src2);
  INSTPAT("000000 ????? ????? ????? 00000 100111", nor    , R, R(rd) = ~(src1 | src2));
  INSTPAT("000000 ????? ????? ????? 00000 101010", slt    , R, R(rd) = (int32_t)src1 < (int32_t)src2);
  INSTPAT("000000 ????? ????? ????? 00000 101011", sltu   , R, R(rd) = src1 < src2);
  INSTPAT("011100 ????? ????? ????? 00000 000010", mul    , R, R(rd) = (int32_t)src1 * (int32_t)src2);
  INSTPAT("011100 ????? ????? ????? 00000 100000", clz    , R, R(rd) = clz(src1));
  INSTPAT("000001 ????? 00000 ????? ????? ??????", bltz   , B, jcond((int32_t)src1 <  0, imm));
  INSTPAT("000001 ????? 00001 ????? ????? ??????", bgez   , B, jcond((int32_t)src1 >= 0, imm));
  INSTPAT("000010 ????? ????? ????? ????? ??????", j      , J, jcond(true, imm));
  INSTPAT("000011 ????? ????? ????? ????? ??????", jal    , J, jcond(true, imm); R(31) = s->pc + 8);
  INSTPAT("000100 ????? ????? ????? ????? ??????", beq    , B, jcond(src1 == src2, imm));
  INSTPAT("000101 ????? ????? ????? ????? ??????", bne    , B, jcond(src1 != src2, imm));
  INSTPAT("000110 ????? ????? ????? ????? ??????", blez   , B, jcond((int32_t)src1 <= 0, imm));
  INSTPAT("000111 ????? ????? ????? ????? ??????", bgtz   , B, jcond((int32_t)src1 >  0, imm));
  INSTPAT("001001 ????? ????? ????? ????? ??????", addi   , I, R(rt) = src1 + src2);
  INSTPAT("001010 ????? ????? ????? ????? ??????", slti   , I, R(rt) = (int32_t)src1 < (int32_t)src2);
  INSTPAT("001011 ????? ????? ????? ????? ??????", sltui  , I, R(rt) = src1 < src2);
  INSTPAT("001100 ????? ????? ????? ????? ??????", andi   , U, R(rt) = src1 & src2);
  INSTPAT("001101 ????? ????? ????? ????? ??????", ori    , U, R(rt) = src1 | src2);
  INSTPAT("001110 ????? ????? ????? ????? ??????", xori   , U, R(rt) = src1 ^ src2);
  INSTPAT("001111 ????? ????? ????? ????? ??????", lui    , U, R(rt) = src2 << 16);
  INSTPAT("100000 ????? ????? ????? ????? ??????", lb     , I, R(rt) = SEXT(Mr(src1 + src2, 1), 8));
  INSTPAT("100001 ????? ????? ????? ????? ??????", lh     , I, R(rt) = SEXT(Mr(src1 + src2, 2), 16));
  INSTPAT("100010 ????? ????? ????? ????? ??????", lwl    , I, lwl(s, rt, src1 + src2));
  INSTPAT("100011 ????? ????? ????? ????? ??????", lw     , I, R(rt) = Mr(src1 + src2, 4));
  INSTPAT("100100 ????? ????? ????? ????? ??????", lbu    , I, R(rt) = Mr(src1 + src2, 1));
  INSTPAT("100101 ????? ????? ????? ????? ??????", lhu    , I, R(rt) = Mr(src1 + src2, 2));
  INSTPAT("100110 ????? ????? ????? ????? ??????", lwr    , I, lwr(s, rt, src1 + src2));
  INSTPAT("101000 ????? ????? ????? ????? ??????", sb     , I, Mw(src1 + src2, 1, R(rt)));
  INSTPAT("101001 ????? ????? ????? ????? ??????", sh     , I, Mw(src1 + src2, 2, R(rt)));
  INSTPAT("101010 ????? ????? ????? ????? ??????", swl    , I, swl(s, rt, src1 + src2));
  INSTPAT("101011 ????? ????? ????? ????? ??????", sw     , I, Mw(src1 + src2, 4, R(rt)));
  INSTPAT("101110 ????? ????? ????? ????? ??????", swr    , I, swr(s, rt, src1 + src2));

  INSTPAT("010000 00000 ????? ????????????????"  , mfc0   , R, csrrw(&R(rt), NULL, rd));
  INSTPAT("010000 00100 ????? ????????????????"  , mtc0   , R, csrrw(NULL, &R(rt), rd));
  INSTPAT("010000 1 0000000000000000000 001000"  , tlbp   , N, priv_instr(PRIV_TLBP, NULL));
  INSTPAT("010000 1 0000000000000000000 000010"  , tlbwi  , N, priv_instr(PRIV_TLBWI, NULL));
  INSTPAT("010000 1 0000000000000000000 000110"  , tlbwr  , N, priv_instr(PRIV_TLBWR, NULL));
  INSTPAT("010000 1 0000000000000000000 011000"  , eret   , N, cpu.pc = priv_instr(PRIV_ERET, NULL));

  INSTPAT("111100 ????? ????? ????? ????? ??????", nemu_trap, N, NEMUTRAP(s->pc, R(2))); // R(2) is $v0;
  INSTPAT("?????? ????? ????? ????? ????? ??????", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_fetch_decode(Decode *s) {
  s->isa.instr.val = instr_fetch(&s->snpc, 4);
  check_ex();
  int idx = decode_exec(s);
#ifdef CONFIG_PERF_OPT
  s->type = INSTR_TYPE_N;
  switch (idx) {
    case EXEC_ID_j: case EXEC_ID_jal:
      s->jnpc = id_dest->imm; s->type = INSTR_TYPE_J; break;

    case EXEC_ID_beq: case EXEC_ID_bne: case EXEC_ID_blez:
    case EXEC_ID_bltz: case EXEC_ID_bgez: case EXEC_ID_bgtz:
      s->jnpc = id_dest->imm; s->type = INSTR_TYPE_B; break;

    case EXEC_ID_ret: case EXEC_ID_jr: case EXEC_ID_jalr:
    case EXEC_ID_eret: case EXEC_ID_syscall:
      s->type = INSTR_TYPE_I;
  }
#endif
  return idx;
}
