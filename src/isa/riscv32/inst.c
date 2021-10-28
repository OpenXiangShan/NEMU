#include "local-include/reg.h"
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <rtl/rtl.h>

enum {
  TYPE_R, TYPE_I, TYPE_J,
  TYPE_U, TYPE_S, TYPE_B,
  TYPE_N,
};

static void decode_imm(Decode *s, word_t *imm, int type) {
  uint32_t i = s->isa.instr.val;
  switch (type) {
    case TYPE_I: *imm = SEXT(BITS(i, 31, 20), 12); break;
    case TYPE_U: *imm = BITS(i, 31, 12) << 12; break;
    case TYPE_S: *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); break;
    case TYPE_B: *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | 
                        (BITS(i, 7, 7) << 11) |
                        (BITS(i, 30, 25) << 5) |
                        (BITS(i, 11, 6) << 1); break;
    case TYPE_J: *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | 
                        (BITS(i, 19, 12) << 12) |
                        (BITS(i, 20, 20) << 11) |
                        (BITS(i, 30, 21) << 1); break;
  }
}

#define INSTPAT_INST(s) ((s)->isa.instr.val)
#define INSTPAT_MATCH(s, name, type, ... /* body */ ) { \
  decode_imm(s, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
#define R(i) (cpu.gpr[i]._32)
#define Mr(addr, len)       vaddr_read(s, addr, len, MMU_DYNAMIC)
#define Mw(addr, len, data) vaddr_write(s, addr, len, data, MMU_DYNAMIC)
#define NEMUTRAP() rtl_hostcall(s, HOSTCALL_EXIT, NULL, &R(10), NULL, 0) // R(10) is $a0
#define INV() rtl_hostcall(s, HOSTCALL_INV, NULL, NULL, NULL, 0);

int isa_new_fetch_decode(Decode *ss) {
  Decode info, *s = &info;
  s->isa.instr.val = instr_fetch(&ss->snpc, 4);

  uint32_t instr = s->isa.instr.val;
  int rd  = BITS(instr, 11, 7);
  int rs1 = BITS(instr, 19, 15);
  int rs2 = BITS(instr, 24, 20);
  uint32_t imm = 0;

  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui      , U, R(rd) = imm);
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc    , U, R(rd) = imm + s->pc);
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw       , I, R(rd) = Mr(R(rs1) + imm, 4));
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw       , S, Mw(R(rs1) + imm, 4, R(rs2)));
  INSTPAT("??????? ????? ????? ??? ????? 11010 11", nemu_trap, N, NEMUTRAP());
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv      , N, INV());
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}
