#include "../bmanipulation/rvintrin.h"

#define IS_BWORD ((s->isa.instr.bi.opcode6_2 >> 1) & 0x1)

#define BUILD_EXEC_B1(x) \
  static inline make_EHelper(x) { \
    *ddest = concat(_rv_, x)(*dsrc1); \
    print_asm_template2(x); \
  }

BUILD_EXEC_B1(clz)
BUILD_EXEC_B1(ctz)
BUILD_EXEC_B1(cpop)
BUILD_EXEC_B1(sext_b)
BUILD_EXEC_B1(sext_h)
BUILD_EXEC_B1(zext_h)
BUILD_EXEC_B1(orc_b)
BUILD_EXEC_B1(rev8)
// BUILD_EXEC_B1(bmatflip)
// BUILD_EXEC_B1(crc32_b)
// BUILD_EXEC_B1(crc32_h)
// BUILD_EXEC_B1(crc32_w)
// BUILD_EXEC_B1(crc32_d)
// BUILD_EXEC_B1(crc32c_b)
// BUILD_EXEC_B1(crc32c_h)
// BUILD_EXEC_B1(crc32c_w)
// BUILD_EXEC_B1(crc32c_d)

#define BUILD_EXEC_B1W(x) \
  static inline make_EHelper(x ## w) { \
    *ddest = concat(_rv32_, x)(*dsrc1); \
    print_asm_template2(x ## w); \
  }

BUILD_EXEC_B1W(clz)
BUILD_EXEC_B1W(ctz)
BUILD_EXEC_B1W(cpop)


#define BUILD_EXEC_B2(x) \
  static inline make_EHelper(x) { \
    *ddest = concat(_rv_, x)(*dsrc1, *dsrc2); \
    print_asm_template3(x); \
  }

BUILD_EXEC_B2(andn)
BUILD_EXEC_B2(orn)
BUILD_EXEC_B2(xnor)
// BUILD_EXEC_B2(slo)
// BUILD_EXEC_B2(sro)
BUILD_EXEC_B2(rol)
BUILD_EXEC_B2(ror)
BUILD_EXEC_B2(bclr)
BUILD_EXEC_B2(bset)
BUILD_EXEC_B2(binv)
BUILD_EXEC_B2(bext)
// BUILD_EXEC_B2(sbclr)
// BUILD_EXEC_B2(sbset)
// BUILD_EXEC_B2(sbinv)
// BUILD_EXEC_B2(sbext)
// BUILD_EXEC_B2(gorc)
// BUILD_EXEC_B2(grev)
BUILD_EXEC_B2(clmul)
BUILD_EXEC_B2(clmulr)
BUILD_EXEC_B2(clmulh)
BUILD_EXEC_B2(min)
BUILD_EXEC_B2(max)
BUILD_EXEC_B2(minu)
BUILD_EXEC_B2(maxu)
// BUILD_EXEC_B2(shfl)
// BUILD_EXEC_B2(unshfl)
// BUILD_EXEC_B2(bdep)
// BUILD_EXEC_B2(bext)
// BUILD_EXEC_B2(pack)
// BUILD_EXEC_B2(packu)
// BUILD_EXEC_B2(bmator)
// BUILD_EXEC_B2(bmatxor)
// BUILD_EXEC_B2(packh)
// BUILD_EXEC_B2(bfp)


#define BUILD_EXEC_B2W(x) \
  static inline make_EHelper(x ## w) { \
    *ddest = concat(_rv32_, x)(*dsrc1, *dsrc2); \
    print_asm_template3(x ## w); \
  }

// BUILD_EXEC_B2W(slo)
// BUILD_EXEC_B2W(sro)
// BUILD_EXEC_B2W(rol)
BUILD_EXEC_B2W(ror)
// BUILD_EXEC_B2W(sbclr)
// BUILD_EXEC_B2W(sbset)
// BUILD_EXEC_B2W(sbinv)
// BUILD_EXEC_B2W(sbext)
// BUILD_EXEC_B2W(gorc)
// BUILD_EXEC_B2W(grev)
// BUILD_EXEC_B2W(clmul)
// BUILD_EXEC_B2W(clmulr)
// BUILD_EXEC_B2W(clmulh)
// BUILD_EXEC_B2W(shfl)
// BUILD_EXEC_B2W(unshfl)
// BUILD_EXEC_B2W(bdep)
// BUILD_EXEC_B2W(bext)
// BUILD_EXEC_B2W(pack)
// BUILD_EXEC_B2W(packu)
// BUILD_EXEC_B2W(bfp)

#define BUILD_EXEC_BI2(x) \
  static inline make_EHelper(x ## i) { \
    *ddest = concat(_rv_, x)(*dsrc1, id_src2->imm); \
    print_asm_template3(x); \
  }

// BUILD_EXEC_BI2(slo)
// BUILD_EXEC_BI2(sro)
BUILD_EXEC_BI2(ror)
BUILD_EXEC_BI2(bclr)
BUILD_EXEC_BI2(binv)
BUILD_EXEC_BI2(bext)
BUILD_EXEC_BI2(bset)
// BUILD_EXEC_BI2(sbclr)
// BUILD_EXEC_BI2(sbinv)
// BUILD_EXEC_BI2(sbext)
// BUILD_EXEC_BI2(sbset)
// BUILD_EXEC_BI2(orc_b) //orc_b
// BUILD_EXEC_BI2(rev8) //rev8
// BUILD_EXEC_BI2(shfl)
// BUILD_EXEC_BI2(unshfl)

#define BUILD_EXEC_BI2W(x) \
  static inline make_EHelper(x ## i ## w) { \
    *ddest = concat(_rv32_, x)(*dsrc1, id_src2->imm); \
    print_asm_template3(x); \
  }

// BUILD_EXEC_BI2W(slo)
// BUILD_EXEC_BI2W(sro)
BUILD_EXEC_BI2W(ror)
// BUILD_EXEC_BI2W(sbclr)
// BUILD_EXEC_BI2W(sbinv)
// BUILD_EXEC_BI2W(sbext)
// BUILD_EXEC_BI2W(sbset)
// BUILD_EXEC_BI2W(gorc)
// BUILD_EXEC_BI2W(grev)


// static inline make_EHelper(cmix) { 
//   *s0 = reg_l(s->isa.instr.fp.funct5); 
//   *ddest = _rv_cmix(*dsrc2, *dsrc1, *s0); 
//   print_asm_template3(cmix); 
// }

// static inline make_EHelper(cmov) { 
//   *s0 = reg_l(s->isa.instr.fp.funct5); 
//   *ddest = _rv_cmov(*dsrc2, *dsrc1, *s0); 
//   print_asm_template3(cmov); 
// }

// static inline make_EHelper(fsl) { 
//   *s0 = reg_l(s->isa.instr.fp.funct5);
//   if(IS_BWORD) *ddest = concat(_rv32_, fsl)(*dsrc1, *s0, *dsrc2); 
//   else *ddest = concat(_rv_, fsl)(*dsrc1, *s0, *dsrc2); 
//   print_asm_template3(fsl); 
// }

// static inline make_EHelper(fsr) { 
//   *s0 = reg_l(s->isa.instr.fp.funct5);
//   if(IS_BWORD) *ddest = concat(_rv32_, fsr)(*dsrc1, *s0, *dsrc2); 
//   else *ddest = concat(_rv_, fsr)(*dsrc1, *s0, *dsrc2); 
//   print_asm_template3(fsr); 
// }

// static inline make_EHelper(fsri) { 
//   *s0 = reg_l(s->isa.instr.fp.funct5);
//   if(IS_BWORD) *ddest = concat(_rv32_, fsr)(*dsrc1, *s0, id_src2->imm); 
//   else *ddest = concat(_rv_, fsr)(*dsrc1, *s0, id_src2->imm); 
//   print_asm_template3(fsri); 
// }

// make_EHelper(addwu) { *ddest = (uint32_t)*dsrc1 + (uint32_t)*dsrc2; print_asm_template3(addwu); }
// make_EHelper(subwu) { *ddest = (uint32_t)*dsrc1 - (uint32_t)*dsrc2; print_asm_template3(subwu); }
// make_EHelper(addiwu) { *ddest = (uint32_t)*dsrc1 + (uint32_t)id_src2->imm; print_asm_template3(addiwu); }
make_EHelper(adduw) { *ddest = *dsrc1 + (uint32_t)*dsrc2; print_asm_template3(adduw); }
// make_EHelper(subuw) { *ddest = *dsrc1 - (uint32_t)*dsrc2; print_asm_template3(subuw); }
make_EHelper(slliuw) { *ddest = (uint32_t)*dsrc1 << id_src2->imm; print_asm_template3(slliuw); }

make_EHelper(sh1add) { *ddest = (*dsrc1 << 1) + *dsrc2; print_asm_template3(sh1add); }
make_EHelper(sh2add) { *ddest = (*dsrc1 << 2) + *dsrc2; print_asm_template3(sh2add); }
make_EHelper(sh3add) { *ddest = (*dsrc1 << 3) + *dsrc2; print_asm_template3(sh3add); }
make_EHelper(sh1adduw) { *ddest = ((uint32_t)*dsrc1 << 1) + *dsrc2; print_asm_template3(sh1adduw); }
make_EHelper(sh2adduw) { *ddest = ((uint32_t)*dsrc1 << 2) + *dsrc2; print_asm_template3(sh2adduw); }
make_EHelper(sh3adduw) { *ddest = ((uint32_t)*dsrc1 << 3) + *dsrc2; print_asm_template3(sh3adduw); }

//printf("rs1=%lx, rs2=%lx, rs3=%lx, rd=%lx.", *dsrc1, id_src2->imm, *s0, *ddest);