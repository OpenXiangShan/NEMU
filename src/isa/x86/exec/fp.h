#include "cc.h"
#define dfdest (&id_dest->fval)
#define dfsrc1 (&id_src1->fval)
#define dfsrc2 (&id_src2->fval)

// ------------- dasm -------------

static inline const char* get_387_fomat(int subcode) {
  static const char *fomat[] = {
    "s", "i", "d", "wi", "li"
  };
  return fomat[subcode];
}

#define print_asm_fpu_template(instr) \
  print_asm(str(instr) "%s %s", get_387_fomat(s->isa.fpu_MF), id_dest->str)
#define print_asm_fpu_template2(instr) \
  print_asm(str(instr) "%s %s,%s", get_387_fomat(s->isa.fpu_MF), id_src1->str, id_dest->str)

// ------------- EHelpers -------------

// a macro to build exec_fadd/fsub/fmul/fmdiv/fmin/fmax
#define BUILD_EXEC_F(x) \
static inline make_EHelper(concat(f, x)) { \
    concat(rtl_f64_, x)(s, dfdest, dfdest, dfsrc1); \
    operand_fwrite(s, id_dest, dfdest); \
    print_asm_fpu_template2(concat(f, x)); \
}

#define BUILD_EXEC_F_R(x) \
static inline make_EHelper(concat3(f, x, r)) { \
    concat(rtl_f64_, x)(s, dfdest, dfsrc1, dfdest); \
    operand_fwrite(s, id_dest, dfdest); \
    print_asm_fpu_template2(concat3(f, x, r)); \
}

#define BUILD_EXEC_F_P(x) \
static inline make_EHelper(concat3(f, x, p)) { \
    concat(rtl_f64_, x)(s, dfdest, dfdest, dfsrc1); \
    operand_fwrite(s, id_dest, dfdest); \
    rtl_popftop(); \
    print_asm_fpu_template2(concat3(f, x, p)); \
}

#define BUILD_EXEC_F_RP(x) \
static inline make_EHelper(concat3(f, x, rp)) { \
    concat(rtl_f64_, x)(s, dfdest, dfsrc1, dfdest); \
    operand_fwrite(s, id_dest, dfdest); \
    rtl_popftop(); \
    print_asm_fpu_template2(concat3(f, x, rp)); \
}

BUILD_EXEC_F(add);
BUILD_EXEC_F(sub);
BUILD_EXEC_F(mul);
BUILD_EXEC_F(div);

BUILD_EXEC_F_R(sub);
BUILD_EXEC_F_R(div);

BUILD_EXEC_F_P(add);
BUILD_EXEC_F_P(sub);
BUILD_EXEC_F_P(mul);
BUILD_EXEC_F_P(div);

BUILD_EXEC_F_RP(sub);
BUILD_EXEC_F_RP(div);

static inline make_EHelper(fcom){
  rtl_f64_lt(s, &cpu.fCF, dfdest, dfsrc1);
  rtl_f64_eq(s, &cpu.fZF, dfdest, dfsrc1);
  rtl_mv(s, &cpu.fPF, rz);
  print_asm_fpu_template2(fcom);
}
static inline make_EHelper(fcomp){
  rtl_f64_lt(s, &cpu.fCF, dfdest, dfsrc1);
  rtl_f64_eq(s, &cpu.fZF, dfdest, dfsrc1);
  rtl_mv(s, &cpu.fPF, rz);
  rtl_popftop();
  print_asm_fpu_template2(fcomp);
}
static inline make_EHelper(fcompp){
  rtl_f64_lt(s, &cpu.fCF, dfdest, dfsrc1);
  rtl_f64_eq(s, &cpu.fZF, dfdest, dfsrc1);
  rtl_mv(s, &cpu.fPF, rz);
  rtl_popftop();
  rtl_popftop();
  print_asm_fpu_template2(fcompp);
}
static inline make_EHelper(fucom){
  rtl_f64_lt(s, &cpu.fCF, dfdest, dfsrc1);
  rtl_f64_eq(s, &cpu.fZF, dfdest, dfsrc1);
  rtl_mv(s, &cpu.fPF, rz);
  print_asm_fpu_template2(fucom);
}
static inline make_EHelper(fucomp){
  rtl_f64_lt(s, &cpu.fCF, dfdest, dfsrc1);
  rtl_f64_eq(s, &cpu.fZF, dfdest, dfsrc1);
  rtl_mv(s, &cpu.fPF, rz);
  rtl_popftop();
  print_asm_fpu_template2(fucomp);
}
static inline make_EHelper(fucompp){
  rtl_f64_lt(s, &cpu.fCF, dfdest, dfsrc1);
  rtl_f64_eq(s, &cpu.fZF, dfdest, dfsrc1);
  rtl_mv(s, &cpu.fPF, rz);
  rtl_popftop();
  rtl_popftop();
  print_asm_fpu_template2(fucompp);
}

static inline make_EHelper(fld){
  operand_fwrite(s, id_dest, dfsrc1);
  print_asm_fpu_template2(fld);
}
static inline make_EHelper(fld1){
  rtl_fld_const(s, dfdest, fconst_1);
  operand_fwrite(s, id_dest, dfdest);
  print_asm_fpu_template(fld1);
}
static inline make_EHelper(fldl2t){
  TODO();
}
static inline make_EHelper(fldl2e){
  TODO();
}
static inline make_EHelper(fldpi){
  TODO();
}
static inline make_EHelper(fldlg2){
  TODO();
}
static inline make_EHelper(fldln2){
  TODO();
}
static inline make_EHelper(fldz){ 
  TODO();
}

static inline make_EHelper(fst){
  operand_fwrite(s, id_dest, dfsrc1);
  print_asm_fpu_template2(fst);
}
static inline make_EHelper(fstp){
  operand_fwrite(s, id_dest, dfsrc1);
  rtl_popftop();
  print_asm_fpu_template2(fstp);
}
static inline make_EHelper(fxch){
  rtl_sfr(s, dfsrc2, id_dest->pfreg);
  rtl_sfr(s, id_dest->pfreg, dfsrc1);
  rtl_sfr(s, id_src1->pfreg, dfsrc2);
  print_asm_fpu_template2(fxch);
}

static inline make_EHelper(fchs){
  rtl_f64_chs(s, dfdest);
  operand_fwrite(s, id_dest, dfdest);
  print_asm_fpu_template(concat(f, chs));
}
static inline make_EHelper(fabs){
  rtl_f64_abs(s, dfdest);
  operand_fwrite(s, id_dest, dfdest);
  print_asm_fpu_template(concat(f, abs));
}
static inline make_EHelper(ftst){
  TODO();
}
static inline make_EHelper(fxam){
  TODO();
}


static inline make_EHelper(finit){
  rtl_mv(s,&cpu.ftop, rz);
  for(int i=0; i<4; i++)
    rtl_mv(s,&cpu.fc[i], rz);
}
static inline make_EHelper(fldenv){
  TODO();
}
static inline make_EHelper(fstenv){
  TODO();
}