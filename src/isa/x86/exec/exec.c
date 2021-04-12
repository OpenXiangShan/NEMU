#if 0
#include <cpu/exec.h>
#include "../local-include/decode.h"
#include "all-instr.h"

#undef CASE_ENTRY
#define CASE_ENTRY(idx, id, ex, w) case idx: set_width(s, w); id(s); return_on_mem_ex(); ex(s); break;

static inline void set_width(Decode *s, int width) {
  if (width == -1) return;
  if (width == 0) {
    width = s->isa.is_operand_size_16 ? 2 : 4;
  }
  s->src1.width = s->dest.width = s->src2.width = width;
}

/* 0x80, 0x81, 0x83 */
static inline def_EHelper(gp1) {
  switch (s->isa.ext_opcode) {
#ifdef __ICS_EXPORT
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(5) EMPTY(6) EMPTY(7)
#else
    EXW(0, add, -1) EXW(1, or, -1)  EXW(2, adc, -1) EXW(3, sbb, -1)
    EXW(4, and, -1) EXW(5, sub, -1) EXW(6, xor, -1) EXW(7, cmp, -1)
#endif
  }
}

/* 0xc0, 0xc1, 0xd0, 0xd1, 0xd2, 0xd3 */
static inline def_EHelper(gp2) {
  switch (s->isa.ext_opcode) {
#ifdef __ICS_EXPORT
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(5) EMPTY(6) EMPTY(7)
#else
    EXW(0, rol, -1) EXW(1, ror, -1) EXW  (2, rcl, -1) EXW  (3, rcr, -1)
    EXW(4, shl, -1) EXW(5, shr, -1) EMPTY(6)          EXW  (7, sar, -1)
#endif
  }
}

/* 0xf6, 0xf7 */
static inline def_EHelper(gp3) {
  switch (s->isa.ext_opcode) {
#ifdef __ICS_EXPORT
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(5) EMPTY(6) EMPTY(7)
#else
    IDEXW(0, test_I, test, -1) EMPTY(1)            EXW(2, not, -1) EXW(3, neg, -1)
    EXW  (4, mul, -1)          EXW  (5, imul1, -1) EXW(6, div, -1) EXW(7, idiv, -1)
#endif
  }
}

/* 0xfe */
static inline def_EHelper(gp4) {
  switch (s->isa.ext_opcode) {
#ifdef __ICS_EXPORT
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(5) EMPTY(6) EMPTY(7)
#else
    EXW  (0, inc, -1) EXW  (1, dec, -1) EMPTY(2) EMPTY(3)
    EMPTY(4)          EMPTY(5)          EMPTY(6) EMPTY(7)
#endif
  }
}

/* 0xff */
static inline def_EHelper(gp5) {
  switch (s->isa.ext_opcode) {
#ifdef __ICS_EXPORT
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(5) EMPTY(6) EMPTY(7)
#else
    EXW(0, inc, -1)    EXW  (1, dec, -1) EXW(2, call_rm, -1) EMPTY(3)
    EXW(4, jmp_rm, -1) EMPTY(5)          EXW(6, push, -1)    EMPTY(7)
#endif
  }
}

/* 0x0f 0x01*/
static inline def_EHelper(gp7) {
  switch (s->isa.ext_opcode) {
#ifdef __ICS_EXPORT
    EMPTY(0) EMPTY(1) EMPTY(2) EMPTY(3)
    EMPTY(4) EMPTY(5) EMPTY(6) EMPTY(7)
#else
    EMPTY(0) EMPTY(1) EXW  (2, lgdt, -1) EXW  (3, lidt, -1)
    EMPTY(4) EMPTY(5) EMPTY(6)           EXW  (7, invlpg, -1)
#endif
  }
}

#ifndef __ICS_EXPORT
/* 0x0f 0x00*/
static inline def_EHelper(gp6) {
  switch (s->isa.ext_opcode) {
    EMPTY(0) EMPTY(1) EXW  (2, lldt, -1) EXW  (3, ltr, -1)
    EMPTY(4) EMPTY(5) EMPTY(6)           EMPTY(7)
  }
}

/* 0x0f 0xba*/
static inline def_EHelper(gp8) {
  switch (s->isa.ext_opcode) {
    EMPTY(0)         EMPTY(1)          EMPTY(2)          EMPTY(3)
    EXW  (4, bt, -1) EXW  (5, bts, -1) EXW  (6, btr, -1) EMPTY(7)
  }
}

static inline def_EHelper(fp_gp1) {
  uint8_t opb = BITS(s->isa.ext_opcode,2,0);
  switch (opb) {
    EX(0x00, fchs) EX(0x01, fabs) EMPTY(0x02) EMPTY(0x03)
    EX(0x04, ftst) EX(0x05, fxam) EMPTY(0x06) EMPTY(0x07)
  }
}

static inline def_EHelper(fp_gp2) {
  uint8_t opb = BITS(s->isa.ext_opcode,2,0);
  switch (opb) {
    EX(0x00, fld1)   EX(0x01, fldl2t) EX(0x02, fldl2e) EX(0x03, fldpi)
    EX(0x04, fldlg2) EX(0x05, fldln2) EX(0x06, fldz)   EMPTY(0x07)
  }
}

static inline def_EHelper(fp_gp3) {
  uint8_t opb = BITS(s->isa.ext_opcode,2,0);
  switch (opb) {
    EMPTY(0x00) IDEX(0x01,St0_St1,fucompp) EMPTY(0x02) EMPTY(0x03)
    EMPTY(0x04) EMPTY(0x05) EMPTY(0x06) EMPTY(0x07)
  }
}

static inline def_EHelper(fp_gp4) {
  uint8_t opb = BITS(s->isa.ext_opcode,2,0);
  switch (opb) {
    EMPTY(0x00) EMPTY(0x01) EMPTY(0x02) EX   (0X03, finit)
    EMPTY(0x04) EMPTY(0x05) EMPTY(0x06) EMPTY(0x07)
  }
}

static inline def_EHelper(fp_gp5) {
  uint8_t opb = BITS(s->isa.ext_opcode,2,0);
  switch (opb) {
    EMPTY(0x00) IDEX(0x01,St0_St1,fcompp) EMPTY(0x02) EMPTY(0x03)
    EMPTY(0x04) EMPTY(0x05) EMPTY(0x06) EMPTY(0x07)
  }
}

static inline def_EHelper(fp_gp6) {
  uint8_t opb = BITS(s->isa.ext_opcode,2,0);
  switch (opb) {
    IDEXW(0x00, fsw2a, fstsw, 2) EMPTY(0x01) EMPTY(0x02) EMPTY(0x03)
    EMPTY(0x04) EMPTY(0x05) EMPTY(0x06) EMPTY(0x07)
  }
}

static inline def_EHelper(fp_gp7) {
  uint8_t opb = BITS(s->isa.ext_opcode,2,0);
  switch (opb) {
    IDEX(0x00, St0_St1, fprem) IDEX(0x01, St0_St1, fyl2xp1) EX(0x02, fsqrt) EMPTY(0x03)
    EX(0x04, frndint) IDEX(0x05, St0_St1, fscale) EMPTY(0x06) EMPTY(0x07)
  }
}

static inline def_EHelper(fp_gp8) {
  uint8_t opb = BITS(s->isa.ext_opcode,2,0);
  switch (opb) {
    EX(0x00, f2xm1) IDEX(0x01, St0_St1, fyl2x) EMPTY(0x02) IDEX(0x03, St0_St1, fpatan)
    EMPTY(0x04) EMPTY(0x05) EMPTY(0x06) EMPTY(0x07)
  }
}

static inline def_EHelper(fp) {
  uint8_t fp_opcode = BITS(s->opcode,2,0)<<4 | BITS(s->isa.ext_opcode,5,3);
  uint8_t fp_mod = BITS(s->isa.ext_opcode,7,6);
  if(fp_mod != 3){
    //if (cpu.pc == 0x82a46c9) Log("fp_opcode = 0x%x", fp_opcode);
    switch (fp_opcode)
    {
      IDEX(0x00, St0_M_32r, fadd)     IDEX(0x01, St0_M_32r, fmul)     IDEX(0x02, St0_M_32r, fcom)     IDEX(0x03, St0_M_32r, fcomp)
      IDEX(0x04, St0_M_32r, fsub)     IDEX(0x05, St0_M_32r, fsubr)    IDEX(0x06, St0_M_32r, fdiv)     IDEX(0x07, St0_M_32r, fdivr)
      IDEX(0x10, ld_St0_M_32r, fld)   EMPTY(0x11)                     IDEX(0x12, st_M_St0_32r, fst)   IDEX(0x13, st_M_St0_32r, fstp)
      IDEX(0x14, St0_Est, fldenv)     IDEXW(0x15, M2fcw, fldcw, 2)    IDEX(0x16, St0_Est, fstenv)       IDEXW(0x17, fcw2M, fstcw, 2)
      IDEX(0x20, St0_M_32i, fadd)     IDEX(0x21, St0_M_32i, fmul)     IDEX(0x22, St0_M_32i, fcom)     IDEX(0x23, St0_M_32i, fcomp)
      IDEX(0x24, St0_M_32i, fsub)     IDEX(0x25, St0_M_32i, fsubr)    IDEX(0x26, St0_M_32i, fdiv)     IDEX(0x27, St0_M_32i, fdivr)
      IDEX(0x30, ld_St0_M_32i, fld)   EMPTY(0x31)                     IDEX(0x32, st_M_St0_32i, fst)   IDEX(0x33, st_M_St0_32i, fstp)
      EMPTY(0x34)                     EMPTY(0x35) /*IDEX(0x35, ld_St0_M_80r, fld)*/   EMPTY(0x36)                     /*IDEX(0x37, st_M_St0_64r, fstp)*/ EMPTY(0x37)
      IDEX(0x40, St0_M_64r, fadd)     IDEX(0x41, St0_M_64r, fmul)     IDEX(0x42, St0_M_64r, fcom)     IDEX(0x43, St0_M_64r, fcomp)
      IDEX(0x44, St0_M_64r, fsub)     IDEX(0x45, St0_M_64r, fsubr)    IDEX(0x46, St0_M_64r, fdiv)     IDEX(0x47, St0_M_64r, fdivr)
      IDEX(0x50, ld_St0_M_64r, fld)   EMPTY(0x51)                     IDEX(0x52, st_M_St0_64r, fst)   IDEX(0x53, st_M_St0_64r, fstp)
      EMPTY(0x54)                     EMPTY(0x55)                     EMPTY(0x56)                     IDEXW(0x57, fsw2M, fstsw, 2)
      IDEX(0x60, St0_M_16i, fadd)     IDEX(0x61, St0_M_16i, fmul)     IDEX(0x62, St0_M_16i, fcom)     IDEX(0x63, St0_M_16i, fcomp)
      IDEX(0x64, St0_M_16i, fsub)     IDEX(0x65, St0_M_16i, fsubr)    IDEX(0x66, St0_M_16i, fdiv)     IDEX(0x67, St0_M_16i, fdivr)
      IDEX(0x70, ld_St0_M_16i, fld)   EMPTY(0x71)                     IDEX(0x72, st_M_St0_16i, fst)   IDEX(0x73, st_M_St0_16i, fstp)
      EMPTY(0x74)                     IDEX(0x75, ld_St0_M_64i, fld)   EMPTY(0x76)                     IDEX(0x77, st_M_St0_64i, fstp)
      default:
        exec_inv(s);
        break;
    }
  }
  else
  {
    //if (cpu.pc == 0x8094758) Log("fp_opcode = 0x%x", fp_opcode);
    switch (fp_opcode)
    {
      IDEX(0x00, St0_Est, fadd)       IDEX(0x01, St0_Est, fmul)       IDEX(0x02, St0_Est, fcom)       IDEX(0x03, St0_Est, fcomp)
      IDEX(0x04, St0_Est, fsub)       IDEX(0x05, St0_Est, fsubr)      IDEX(0x06, St0_Est, fdiv)       IDEX(0x07, St0_Est, fdivr)
      IDEX(0x10, ld_Est_St0, fld)     IDEX(0x11, St0_Est, fxch)       EX  (0x12, nop)                 EMPTY(0x13)
      IDEX(0x14, St0, fp_gp1)         IDEX(0x15, ld_St0, fp_gp2)      IDEX(0x16, St0_Est, fp_gp8)     IDEX(0x17, St0, fp_gp7)
      IDEX(0x20, St0_Est, fcmovb)     IDEX(0x21, St0_Est, fcmove)     IDEX(0x22, St0_Est, fcmovbe)    IDEX(0x23, St0_Est, fcmovu)
      EMPTY(0x24)                     EX  (0x25, fp_gp3)              EMPTY(0x26)                     EMPTY(0x27)
      IDEX(0x30, St0_Est, fcmovnb)    IDEX(0x31, St0_Est, fcmovne)    IDEX(0x32, St0_Est, fcmovnbe)   IDEX(0x33, St0_Est, fcmovnu)
      EX  (0x34, fp_gp4)              IDEX(0x35, St0_Est, fucomi)     IDEX(0x36, St0_Est, fcomi)      EMPTY(0x37)
      IDEX(0x40, Est_St0, fadd)       IDEX(0x41, Est_St0, fmul)       EMPTY(0x42)                     EMPTY(0x43)
      IDEX(0x44, Est_St0, fsubr)      IDEX(0x45, Est_St0, fsub)       IDEX(0x46, Est_St0, fdivr)      IDEX(0x47, Est_St0, fdiv)
      EMPTY(0x50)                     EMPTY(0x51)                     IDEX(0x52, st_Est_St0, fst)     IDEX(0x53, st_Est_St0, fstp)
      IDEX(0x54, St0_Est, fucom)      IDEX(0x55, St0_Est, fucomp)     EMPTY(0x56)                     EMPTY(0x57)
      IDEX(0x60, Est_St0, faddp)      IDEX(0x61, Est_St0, fmulp)      EMPTY(0x62)                     EX  (0x63, fp_gp5)
      IDEX(0x64, Est_St0, fsubrp)     IDEX(0x65, Est_St0, fsubp)      IDEX(0x66, Est_St0, fdivrp)     IDEX(0x67, Est_St0, fdivp)
      EMPTY(0x70)                     EMPTY(0x71)                     EMPTY(0x72)                     EMPTY(0x73)
      EX  (0x74, fp_gp6)              IDEX(0x75, St0_Est, fucomip)    IDEX (0x76, St0_Est, fcomip)    EMPTY(0x77)
      default:
        exec_inv(s);
        break;
    }
  }
  
}
#endif

static inline def_EHelper(2byte_esc) {
  uint8_t opcode = instr_fetch(&s->seq_pc, 1);
  s->opcode = opcode | 0x100;
  switch (opcode) {
  /* TODO: Add more instructions!!! */
    IDEX (0x01, gp7_E, gp7)
#ifndef __ICS_EXPORT
    IDEXW(0x00, gp6_E, gp6, 2)
    IDEX (0x20, mov_G2E, mov_cr2r)
    IDEX (0x22, mov_E2G, mov_r2cr)
    IDEX (0x23, mov_E2G, mov_r2dr)
    EX   (0x31, rdtsc)
    IDEXW(0x6f, E2xmm, movdqa_E2xmm, 4)
    IDEXW(0x73, Ib2xmm, psrlq, 4)
    IDEXW(0x7e, E2xmm, movq_E2xmm, 4)
    IDEXW(0x80, J, jcc, 4) IDEXW(0x81, J, jcc, 4) IDEXW(0x82, J, jcc, 4) IDEXW(0x83, J, jcc, 4)
    IDEXW(0x84, J, jcc, 4) IDEXW(0x85, J, jcc, 4) IDEXW(0x86, J, jcc, 4) IDEXW(0x87, J, jcc, 4)
    IDEXW(0x88, J, jcc, 4) IDEXW(0x89, J, jcc, 4) IDEXW(0x8a, J, jcc, 4) IDEXW(0x8b, J, jcc, 4)
    IDEXW(0x8c, J, jcc, 4) IDEXW(0x8d, J, jcc, 4) IDEXW(0x8e, J, jcc, 4) IDEXW(0x8f, J, jcc, 4)
    IDEX (0x40, E2G, cmovcc) IDEX (0x41, E2G, cmovcc) IDEX (0x42, E2G, cmovcc) IDEX (0x43, E2G, cmovcc)
    IDEX (0x44, E2G, cmovcc) IDEX (0x45, E2G, cmovcc) IDEX (0x46, E2G, cmovcc) IDEX (0x47, E2G, cmovcc)
    IDEX (0x48, E2G, cmovcc) IDEX (0x49, E2G, cmovcc) IDEX (0x4a, E2G, cmovcc) IDEX (0x4b, E2G, cmovcc)
    IDEX (0x4c, E2G, cmovcc) IDEX (0x4d, E2G, cmovcc) IDEX (0x4e, E2G, cmovcc) IDEX (0x4f, E2G, cmovcc)
    IDEXW(0x90, setcc_E, setcc, 1) IDEXW(0x91, setcc_E, setcc, 1) IDEXW(0x92, setcc_E, setcc, 1) IDEXW(0x93, setcc_E, setcc, 1)
    IDEXW(0x94, setcc_E, setcc, 1) IDEXW(0x95, setcc_E, setcc, 1) IDEXW(0x96, setcc_E, setcc, 1) IDEXW(0x97, setcc_E, setcc, 1)
    IDEXW(0x98, setcc_E, setcc, 1) IDEXW(0x99, setcc_E, setcc, 1) IDEXW(0x9a, setcc_E, setcc, 1) IDEXW(0x9b, setcc_E, setcc, 1)
    IDEXW(0x9c, setcc_E, setcc, 1) IDEXW(0x9d, setcc_E, setcc, 1) IDEXW(0x9e, setcc_E, setcc, 1) IDEXW(0x9f, setcc_E, setcc, 1)
    EX   (0xa0, push_fs)
    EX   (0xa1, pop_fs)
    EX   (0xa2, cpuid)
    IDEX (0xa3, bit_G2E, bt)
    IDEX (0xa4, Ib_G2E, shld)
    IDEX (0xa5, cl_G2E, shld)
    IDEX (0xab, bit_G2E, bts)
    IDEX (0xac, Ib_G2E, shrd)
    IDEX (0xad, cl_G2E, shrd)
    IDEX (0xaf, E2G, imul2)
    IDEX (0xb1, a_G2E, cmpxchg)
    IDEX (0xb3, bit_G2E, btr)
    IDEXW(0xb6, mov_E2G, movzx, 1)
    IDEXW(0xb7, mov_E2G, movzx, 2)
    IDEX (0xba, gp2_Ib2E, gp8)
    IDEX (0xbb, bit_G2E, btc)
    IDEX (0xbc, mov_E2G, bsf)
    IDEX (0xbd, mov_E2G, bsr)
    IDEXW(0xbe, mov_E2G, movsx, 1)
    IDEXW(0xbf, mov_E2G, movsx, 2)
    IDEX (0xc1, G2E, xadd)
    IDEXW(0xc7, E, cmpxchg8b, 4)
    IDEXW(0xc8, r, bswap, 4) IDEXW(0xc9, r, bswap, 4) IDEXW(0xca, r, bswap, 4) IDEXW(0xcb, r, bswap, 4)
    IDEXW(0xcc, r, bswap, 4) IDEXW(0xcd, r, bswap, 4) IDEXW(0xce, r, bswap, 4) IDEXW(0xcf, r, bswap, 4)
    IDEXW(0xd6, xmm2E, movq_xmm2E, 4)
    IDEXW(0xef, E2xmm, pxor, 4)
#endif
    default: exec_inv(s);
  }
}

static inline void fetch_decode_exec(Decode *s) {
  uint8_t opcode;
again:
  opcode = instr_fetch(&s->seq_pc, 1);
  return_on_mem_ex();
  s->opcode = opcode;
  switch (opcode) {
#ifdef __ICS_EXPORT
    EX   (0x0f, 2byte_esc)
    IDEXW(0x80, I2E, gp1, 1)
    IDEX (0x81, I2E, gp1)
    IDEX (0x83, SI2E, gp1)
    IDEXW(0x88, mov_G2E, mov, 1)
    IDEX (0x89, mov_G2E, mov)
    IDEXW(0x8a, mov_E2G, mov, 1)
    IDEX (0x8b, mov_E2G, mov)
    IDEXW(0xa0, O2a, mov, 1)
    IDEX (0xa1, O2a, mov)
    IDEXW(0xa2, a2O, mov, 1)
    IDEX (0xa3, a2O, mov)
    IDEXW(0xb0, mov_I2r, mov, 1)
    IDEXW(0xb1, mov_I2r, mov, 1)
    IDEXW(0xb2, mov_I2r, mov, 1)
    IDEXW(0xb3, mov_I2r, mov, 1)
    IDEXW(0xb4, mov_I2r, mov, 1)
    IDEXW(0xb5, mov_I2r, mov, 1)
    IDEXW(0xb6, mov_I2r, mov, 1)
    IDEXW(0xb7, mov_I2r, mov, 1)
    IDEX (0xb8, mov_I2r, mov)
    IDEX (0xb9, mov_I2r, mov)
    IDEX (0xba, mov_I2r, mov)
    IDEX (0xbb, mov_I2r, mov)
    IDEX (0xbc, mov_I2r, mov)
    IDEX (0xbd, mov_I2r, mov)
    IDEX (0xbe, mov_I2r, mov)
    IDEX (0xbf, mov_I2r, mov)
    IDEXW(0xc0, gp2_Ib2E, gp2, 1)
    IDEX (0xc1, gp2_Ib2E, gp2)
    IDEXW(0xc6, mov_I2E, mov, 1)
    IDEX (0xc7, mov_I2E, mov)
    IDEXW(0xd0, gp2_1_E, gp2, 1)
    IDEX (0xd1, gp2_1_E, gp2)
    IDEXW(0xd2, gp2_cl2E, gp2, 1)
    IDEX (0xd3, gp2_cl2E, gp2)
    EX   (0xd6, nemu_trap)
    IDEXW(0xf6, E, gp3, 1)
    IDEX (0xf7, E, gp3)
    IDEXW(0xfe, E, gp4, 1)
    IDEX (0xff, E, gp5)
#else
//       1         2         3         4         5         6         7         8         9
//34567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
IDEXW(0x00, G2E, add, 1)    IDEX (0x01, G2E, add)       IDEXW(0x02, E2G, add, 1)    IDEX (0x03, E2G, add)
IDEXW(0x04, I2a, add, 1)    IDEX (0x05, I2a, add)       EX   (0x06, push_es)        EX   (0x07, pop_es)
IDEXW(0x08, G2E, or, 1)     IDEX (0x09, G2E, or)        IDEXW(0x0a, E2G, or, 1)     IDEX (0x0b, E2G, or)
IDEXW(0x0c, I2a, or, 1)     IDEX (0x0d, I2a, or)        EMPTY(0x0e)                 EX   (0x0f, 2byte_esc)
IDEXW(0x10, G2E, adc, 1)    IDEX (0x11, G2E, adc)       IDEXW(0x12, E2G, adc, 1)    IDEX (0x13, E2G, adc)
IDEXW(0x14, I2a, adc, 1)    IDEX (0x15, I2a, adc)
IDEXW(0x18, G2E, sbb, 1)    IDEX (0x19, G2E, sbb)       IDEXW(0x1a, E2G, sbb, 1)    IDEX (0x1b, E2G, sbb)
IDEXW(0x1c, I2a, sbb, 1)                                EX   (0x1e, push_ds)        EX   (0x1f, pop_ds)
IDEXW(0x20, G2E, and, 1)    IDEX (0x21, G2E, and)       IDEXW(0x22, E2G, and, 1)    IDEX (0x23, E2G, and)
IDEXW(0x24, I2a, and, 1)    IDEX (0x25, I2a, and)
IDEXW(0x28, G2E, sub, 1)    IDEX (0x29, G2E, sub)       IDEXW(0x2a, E2G, sub, 1)    IDEX (0x2b, E2G, sub)
IDEXW(0x2c, I2a, sub, 1)    IDEX (0x2d, I2a, sub)
IDEXW(0x30, G2E, xor, 1)    IDEX (0x31, G2E, xor)       IDEXW(0x32, E2G, xor, 1)    IDEX (0x33, E2G, xor)
IDEXW(0x34, I2a, xor, 1)    IDEX (0x35, I2a, xor)
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
IDEXW(0x84, G2E, test, 1)   IDEX (0x85, G2E, test)      IDEXW(0x86, G2E, xchg, 1)   IDEX (0x87, G2E, xchg)
IDEXW(0x88, mov_G2E, mov, 1)IDEX (0x89, mov_G2E, mov)   IDEXW(0x8a, mov_E2G, mov, 1)IDEX (0x8b, mov_E2G, mov)
IDEX (0x8c, mov_G2E, mov_sreg2rm) IDEX (0x8d, lea_M2G, lea)   IDEX (0x8e, mov_E2G, mov_rm2sreg) IDEX (0x8f, E, pop)
EX   (0x90, nop)            IDEX (0x91, a2r, xchg)      IDEX (0x92, a2r, xchg)      IDEX (0x93, a2r, xchg)
IDEX (0x94, a2r, xchg)      IDEX (0x95, a2r, xchg)      IDEX (0x96, a2r, xchg)      IDEX (0x97, a2r, xchg)
EX   (0x98, cwtl)           EX   (0x99, cltd)                                       EX   (0x9b, fwait)
EX   (0x9c, pushf)          EX   (0x9d, popf)           EX   (0x9e, sahf)
IDEXW(0xa0, O2a, mov, 1)    IDEX (0xa1, O2a, mov)       IDEXW(0xa2, a2O, mov, 1)    IDEX (0xa3, a2O, mov)
EXW  (0xa4, movs, 1)        EX   (0xa5, movs)           EXW  (0xa6, cmps, 1)
IDEXW(0xa8, I2a, test, 1)   IDEX (0xa9, I2a, test)      IDEXW(0xaa, aSrc, stos, 1)  IDEX (0xab, aSrc, stos)
IDEXW(0xac, aDest, lods, 1)                             IDEXW(0xae, aSrc, scas, 1)
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
IDEX(0xd8, fp_ext, fp)      IDEX(0xd9, fp_ext, fp)      IDEX(0xda, fp_ext, fp)      IDEX(0xdb, fp_ext, fp)
IDEX(0xdc, fp_ext, fp)      IDEX(0xdd, fp_ext, fp)      IDEX(0xde, fp_ext, fp)      IDEX(0xdf, fp_ext, fp)
                                                                                    IDEXW(0xe3, J, jecxz, 1)
IDEXW(0xe4, in_I2a, in, 1)                              IDEXW(0xe6, out_a2I, out, 1)
IDEX (0xe8, J, call)        IDEXW(0xe9, J, jmp, 4)      IDEXW(0xea, LJ, ljmp, 4)    IDEXW(0xeb, J, jmp, 1)
IDEXW(0xec, in_dx2a, in, 1) IDEX (0xed, in_dx2a, in)    IDEXW(0xee, out_a2dx, out, 1)IDEX (0xef, out_a2dx, out)

EX   (0xf4, hlt)                                        IDEXW(0xf6, E, gp3, 1)      IDEX (0xf7, E, gp3)
EX   (0xf8, clc)            EX   (0xf9, stc)            EX   (0xfa, cli)            EX   (0xfb, sti)
EX   (0xfc, cld)            EX   (0xfd, std)            IDEXW(0xfe, E, gp4, 1)      IDEX (0xff, E, gp5)
  case 0x64: s->isa.sreg_base = &cpu.sreg[CSR_FS].base; goto again;
  case 0x65: s->isa.sreg_base = &cpu.sreg[CSR_GS].base; goto again;
  case 0xf0: cpu.lock = 1; goto again; // LOCK prefix
  case 0xf2: s->isa.rep_flags = PREFIX_REPNZ; goto again;
  case 0xf3: s->isa.rep_flags = PREFIX_REP; goto again;
#endif
  case 0x66: s->isa.is_operand_size_16 = true; goto again;
  default: exec_inv(s);
  }
}

#ifdef ENABLE_DIFFTEST_INSTR_QUEUE
static uint8_t instr_buf[20];
static uint8_t instr_len;

void add_instr(uint8_t *instr, int len) {
  int i;
  for (i = 0; i < len; i ++) {
    instr_buf[instr_len ++] = instr[i];
  }
}
#endif

vaddr_t isa_exec_once() {
#ifndef __ICS_EXPORT
//#define USE_KVM
#ifdef USE_KVM
  extern void kvm_exec();
  kvm_exec();
  return 0;
#endif
#endif
  Decode s;
  s.is_jmp = 0;
  s.isa = (ISADecodeInfo) { 0 };
  s.seq_pc = cpu.pc;

#ifdef ENABLE_DIFFTEST_INSTR_QUEUE
  instr_len = 0;
#endif

  fetch_decode_exec(&s);

#ifdef ENABLE_DIFFTEST_INSTR_QUEUE
  void commit_instr(vaddr_t thispc, uint8_t *instr_buf, uint8_t instr_len);
  commit_instr(cpu.pc, instr_buf, instr_len);
#endif

#ifndef CONFIG_PA
  if (cpu.mem_exception != 0) {
    cpu.pc = raise_intr(cpu.mem_exception, cpu.pc);
    cpu.mem_exception = 0;
  } else {
    update_pc(&s);
  }
  cpu.lock = 0;
#else
  update_pc(&s);
#endif

#ifndef __ICS_EXPORT
#if !defined(CONFIG_DIFFTEST) && !defined(CONFIG_SHARE)
  void query_intr();
  query_intr();
#endif
#endif
  return s.seq_pc;
}
#endif
