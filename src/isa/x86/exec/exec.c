#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "all-instr.h"

static inline void set_width(DecodeExecState *s, int width) {
  if (width == 0) {
    width = s->isa.is_operand_size_16 ? 2 : 4;
  }
  s->src1.width = s->dest.width = s->src2.width = width;
}

#define decode_empty(s)

#define IDEXW(idx, id, ex, w) CASE_ENTRY(idx, concat(decode_, id), concat(exec_, ex), w)
#define IDEX(idx, id, ex)     IDEXW(idx, id, ex, 0)
#define EXW(idx, ex, w)       IDEXW(idx, empty, ex, w)
#define EX(idx, ex)           EXW(idx, ex, 0)
#define EMPTY(idx)            EX(idx, inv)

#define CASE_ENTRY(idx, id, ex, w) case idx: id(s); ex(s); break;

/* 0x80, 0x81, 0x83 */
static inline make_EHelper(gp1) {
  switch (s->isa.ext_opcode) {
    EX(0x00, add) EX(0x01, or)  EX(0x02, adc) EX(0x03, sbb)
    EX(0x04, and) EX(0x05, sub) EX(0x06, xor) EX(0x07, cmp)
  }
}

/* 0xc0, 0xc1, 0xd0, 0xd1, 0xd2, 0xd3 */
static inline make_EHelper(gp2) {
  switch (s->isa.ext_opcode) {
    EX(0x00, rol) EX(0x01, ror) EMPTY(0x02) EMPTY(0x03)
    EX(0x04, shl) EX(0x05, shr) EMPTY(0x06) EX   (0x07, sar)
  }
}

/* 0xf6, 0xf7 */
static inline make_EHelper(gp3) {
  switch (s->isa.ext_opcode) {
    IDEX(0x00, test_I, test) EMPTY(0x01)        EX(0x02, not) EX(0x03, neg)
    EX  (0x04, mul)          EX   (0x05, imul1) EX(0x06, div) EX(0x07, idiv)
  }
}

/* 0xfe */
static inline make_EHelper(gp4) {
  switch (s->isa.ext_opcode) {
    EX   (0x00, inc) EX   (0x01, dec) EMPTY(0x02) EMPTY(0x03)
    EMPTY(0x04)      EMPTY(0x05)      EMPTY(0x06) EMPTY(0x07)
  }
}

/* 0xff */
static inline make_EHelper(gp5) {
  switch (s->isa.ext_opcode) {
    EX(0x00, inc)    EX   (0x01, dec) EX(0x02, call_rm) EMPTY(0x03)
    EX(0x04, jmp_rm) EMPTY(0x05)      EX(0x06, push)    EMPTY(0x07)
  }
}

/* 0x0f 0x01*/
static inline make_EHelper(gp7) {
  switch (s->isa.ext_opcode) {
    EMPTY(0x00) EMPTY(0x01) EMPTY(0x02) EX   (0x03, lidt)
    EMPTY(0x04) EMPTY(0x05) EMPTY(0x06) EMPTY(0x07)
  }
}

#undef CASE_ENTRY
#define CASE_ENTRY(idx, id, ex, w) case idx: set_width(s, w); id(s); ex(s); break;

static inline make_EHelper(2byte_esc) {
  uint8_t opcode = instr_fetch(&s->seq_pc, 1);
  s->opcode = opcode;
  switch (opcode) {
  /* TODO: Add more instructions!!! */
    IDEX (0x01, gp7_E, gp7)
    IDEX (0x20, mov_G2E, mov_cr2r)
    IDEX (0x22, mov_E2G, mov_r2cr)
    IDEXW(0x80, J, jcc, 4) IDEXW(0x81, J, jcc, 4) IDEXW(0x82, J, jcc, 4) IDEXW(0x83, J, jcc, 4)
    IDEXW(0x84, J, jcc, 4) IDEXW(0x85, J, jcc, 4) IDEXW(0x86, J, jcc, 4) IDEXW(0x87, J, jcc, 4)
    IDEXW(0x88, J, jcc, 4) IDEXW(0x89, J, jcc, 4) IDEXW(0x8a, J, jcc, 4) IDEXW(0x8b, J, jcc, 4)
    IDEXW(0x8c, J, jcc, 4) IDEXW(0x8d, J, jcc, 4) IDEXW(0x8e, J, jcc, 4) IDEXW(0x8f, J, jcc, 4)
    IDEXW(0x90, setcc_E, setcc, 1) IDEXW(0x91, setcc_E, setcc, 1) IDEXW(0x92, setcc_E, setcc, 1) IDEXW(0x93, setcc_E, setcc, 1)
    IDEXW(0x94, setcc_E, setcc, 1) IDEXW(0x95, setcc_E, setcc, 1) IDEXW(0x96, setcc_E, setcc, 1) IDEXW(0x97, setcc_E, setcc, 1)
    IDEXW(0x98, setcc_E, setcc, 1) IDEXW(0x99, setcc_E, setcc, 1) IDEXW(0x9a, setcc_E, setcc, 1) IDEXW(0x9b, setcc_E, setcc, 1)
    IDEXW(0x9c, setcc_E, setcc, 1) IDEXW(0x9d, setcc_E, setcc, 1) IDEXW(0x9e, setcc_E, setcc, 1) IDEXW(0x9f, setcc_E, setcc, 1)
    IDEX (0xa4, Ib_G2E, shld)
    IDEX (0xa5, cl_G2E, shld)
    IDEX (0xac, Ib_G2E, shrd)
    IDEX (0xaf, E2G, imul2)
    IDEXW(0xb6, mov_E2G, movzx, 1)
    IDEXW(0xb7, mov_E2G, movzx, 2)
    IDEX (0xbd, mov_E2G, bsr)
    IDEXW(0xbe, mov_E2G, movsx, 1)
    IDEXW(0xbf, mov_E2G, movsx, 2)
    default: exec_inv(s);
  }
}

static inline void exec(DecodeExecState *s) {
  uint8_t opcode;
again:
  opcode = instr_fetch(&s->seq_pc, 1);
  s->opcode = opcode;
  switch (opcode) {
//       1         2         3         4         5         6         7         8         9
//34567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
IDEXW(0x00, G2E, add, 1)    IDEX (0x01, G2E, add)       IDEXW(0x02, E2G, add, 1)    IDEX (0x03, E2G, add)
EMPTY(0x04)                 IDEX (0x05, I2a, add)
IDEXW(0x08, G2E, or, 1)     IDEX (0x09, G2E, or)        IDEXW(0x0a, E2G, or, 1)     IDEX (0x0b, E2G, or)
IDEXW(0x0c, I2a, or, 1)     IDEX (0x0d, I2a, or)        EMPTY(0x0e)                 EX   (0x0f, 2byte_esc)
IDEXW(0x10, G2E, adc, 1)    IDEX (0x11, G2E, adc)       IDEXW(0x12, E2G, adc, 1)    IDEX (0x13, E2G, adc)

IDEXW(0x18, G2E, sbb, 1)    IDEX (0x19, G2E, sbb)       IDEXW(0x1a, E2G, sbb, 1)    IDEX (0x1b, E2G, sbb)

IDEXW(0x20, G2E, and, 1)    IDEX (0x21, G2E, and)       IDEXW(0x22, E2G, and, 1)    IDEX (0x23, E2G, and)
IDEXW(0x24, I2a, and, 1)    IDEX (0x25, I2a, and)
IDEXW(0x28, G2E, sub, 1)    IDEX (0x29, G2E, sub)       EMPTY(0x2a)                 IDEX (0x2b, E2G, sub)
EMPTY(0x2c)                 IDEX (0x2d, I2a, sub)
IDEXW(0x30, G2E, xor, 1)    IDEX (0x31, G2E, xor)       IDEXW(0x32, E2G, xor, 1)    IDEX (0x33, E2G, xor)
EMPTY(0x34)                 IDEX (0x35, I2a, xor)
IDEXW(0x38, G2E, cmp, 1)    IDEX (0x39, G2E, cmp)       IDEXW(0x3a, E2G, cmp, 1)    IDEX (0x3b, E2G, cmp)
IDEXW(0x3c, I2a, cmp, 1)    IDEX (0x3d, I2a, cmp)
IDEX (0x40, r, inc)         IDEX (0x41, r, inc)         IDEX (0x42, r, inc)         IDEX (0x43, r, inc)
IDEX (0x44, r, inc)         IDEX (0x45, r, inc)         IDEX (0x46, r, inc)         IDEX (0x47, r, inc)
IDEX (0x48, r, dec)         IDEX (0x49, r, dec)         IDEX (0x4a, r, dec)         IDEX (0x4b, r, dec)
IDEX (0x4c, r, dec)         IDEX (0x4d, r, dec)         IDEX (0x4e, r, dec)         IDEX (0x4f, r, dec)
IDEX (0x50, r, push)        IDEX (0x51, r, push)        IDEX (0x52, r, push)        IDEX (0x53, r, push)
IDEX (0x54, r, push)        IDEX (0x55, r, push)        IDEX (0x56, r, push)        IDEX (0x57, r, push)
IDEX (0x58, r, pop)         IDEX (0x59, r, pop)         IDEX (0x5a, r, pop)         IDEX (0x5b, r, pop)
IDEX (0x5c, r, pop)         IDEX (0x5d, r, pop)         IDEX (0x5e, r, pop)         IDEX (0x5f, r, pop)
EX   (0x60, pusha)          EX   (0x61, popa)
                                                        //EX   (0x66, operand_size)
IDEX (0x68, I, push)        IDEX (0x69, I_E2G, imul3)   IDEXW(0x6a, push_SI, push, 1)IDEX (0x6b, SI_E2G, imul3)

IDEXW(0x70, J, jcc, 1)      IDEXW(0x71, J, jcc, 1)      IDEXW(0x72, J, jcc, 1)      IDEXW(0x73, J, jcc, 1)
IDEXW(0x74, J, jcc, 1)      IDEXW(0x75, J, jcc, 1)      IDEXW(0x76, J, jcc, 1)      IDEXW(0x77, J, jcc, 1)
IDEXW(0x78, J, jcc, 1)      IDEXW(0x79, J, jcc, 1)      IDEXW(0x7a, J, jcc, 1)      IDEXW(0x7b, J, jcc, 1)
IDEXW(0x7c, J, jcc, 1)      IDEXW(0x7d, J, jcc, 1)      IDEXW(0x7e, J, jcc, 1)      IDEXW(0x7f, J, jcc, 1)
IDEXW(0x80, I2E, gp1, 1)    IDEX (0x81, I2E, gp1)       EMPTY(0x82)                 IDEX (0x83, SI2E, gp1)
IDEXW(0x84, G2E, test, 1)   IDEX (0x85, G2E, test)                                  IDEX (0x87, G2E, xchg)
IDEXW(0x88, mov_G2E, mov, 1)IDEX (0x89, mov_G2E, mov)   IDEXW(0x8a, mov_E2G, mov, 1)IDEX (0x8b, mov_E2G, mov)
EMPTY(0x8c)                 IDEX (0x8d, lea_M2G, lea)
EX   (0x90, nop)

EX   (0x98, cwtl)           EX   (0x99, cltd)

IDEXW(0xa0, O2a, mov, 1)    IDEX (0xa1, O2a, mov)       IDEXW(0xa2, a2O, mov, 1)    IDEX (0xa3, a2O, mov)
EXW  (0xa4, movs, 1)        EX   (0xa5, movs)
IDEXW(0xa8, I2a, test, 1)   IDEX (0xa9, I2a, test)

IDEXW(0xb0, mov_I2r, mov, 1)IDEXW(0xb1, mov_I2r, mov, 1)IDEXW(0xb2, mov_I2r, mov, 1)IDEXW(0xb3, mov_I2r, mov, 1)
IDEXW(0xb4, mov_I2r, mov, 1)IDEXW(0xb5, mov_I2r, mov, 1)IDEXW(0xb6, mov_I2r, mov, 1)IDEXW(0xb7, mov_I2r, mov, 1)
IDEX (0xb8, mov_I2r, mov)   IDEX (0xb9, mov_I2r, mov)   IDEX (0xba, mov_I2r, mov)   IDEX (0xbb, mov_I2r, mov)
IDEX (0xbc, mov_I2r, mov)   IDEX (0xbd, mov_I2r, mov)   IDEX (0xbe, mov_I2r, mov)   IDEX (0xbf, mov_I2r, mov)
IDEXW(0xc0, gp2_Ib2E, gp2, 1)IDEX (0xc1, gp2_Ib2E, gp2) IDEXW(0xc2, I, ret_imm, 2)  EX   (0xc3, ret)
                                                        IDEXW(0xc6, mov_I2E, mov, 1)IDEX (0xc7, mov_I2E, mov)
                            EX   (0xc9, leave)
                            IDEXW(0xcd, I, int, 1)                                  EX   (0xcf, iret)
IDEXW(0xd0, gp2_1_E, gp2, 1)IDEX (0xd1, gp2_1_E, gp2)   IDEXW(0xd2, gp2_cl2E, gp2, 1)IDEX (0xd3, gp2_cl2E, gp2)
                                                        EX   (0xd6, nemu_trap)




IDEX (0xe8, J, call)        IDEXW(0xe9, J, jmp, 4)      EMPTY(0xea)                 IDEXW(0xeb, J, jmp, 1)
IDEXW(0xec, in_dx2a, in, 1) IDEX (0xed, in_dx2a, in)    IDEXW(0xee, out_a2dx, out, 1)IDEX (0xef, out_a2dx, out)

                                                        IDEXW(0xf6, E, gp3, 1)      IDEX (0xf7, E, gp3)

                                                        IDEXW(0xfe, E, gp4, 1)      IDEX (0xff, E, gp5)

  case 0x66: s->isa.is_operand_size_16 = true; goto again;
  default: exec_inv(s);
  }
}

//#define USE_KVM
vaddr_t isa_exec_once() {
#ifdef USE_KVM
  extern void kvm_exec(void);
  kvm_exec();
  return 0;
#endif
  DecodeExecState s;
  s.is_jmp = 0;
  s.isa = (ISADecodeInfo) { 0 };
  s.seq_pc = cpu.pc;

  exec(&s);
  update_pc(&s);

#if !defined(DIFF_TEST) && !_SHARE
  void query_intr(DecodeExecState *s);
  query_intr(&s);
#endif
  return s.seq_pc;
}
