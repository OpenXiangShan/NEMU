#include "cc.h"
#define dfdest (id_dest->pfreg)
#define dfsrc1 (id_src1->pfreg)
#define dfsrc2 (id_src2->pfreg)

// ------------- dasm -------------

static inline const char* get_387_fomat(int subcode) {
  static const char *fomat[] = {
    "s", "i", "d", "wi", "li", " "
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
    print_asm_fpu_template2(concat(f, x)); \
}

#define BUILD_EXEC_F_R(x) \
static inline make_EHelper(concat3(f, x, r)) { \
    concat(rtl_f64_, x)(s, dfdest, dfsrc1, dfdest); \
    print_asm_fpu_template2(concat3(f, x, r)); \
}

#define BUILD_EXEC_F_P(x) \
static inline make_EHelper(concat3(f, x, p)) { \
    concat(rtl_f64_, x)(s, dfdest, dfdest, dfsrc1); \
    rtl_popftop(); \
    print_asm_fpu_template2(concat3(f, x, p)); \
}

#define BUILD_EXEC_F_RP(x) \
static inline make_EHelper(concat3(f, x, rp)) { \
    concat(rtl_f64_, x)(s, dfdest, dfsrc1, dfdest); \
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

//compare dest & src, save flag in fsw  (use t0, s0)
static inline void fucom_helper(DecodeExecState *s, uint64_t* fp_dest, uint64_t* fp_src){
  rtl_li(s, t0, 0x4500);
  rtl_lr_fsw(s,s0);
  rtl_or(s, s0, s0, t0);
  rtl_xor(s, s0, s0, t0);
  rtl_f64_lt(s, t0, fp_dest, fp_src);
  rtl_shli(s, t0, t0, 8);
  rtl_or(s, s0, s0, t0);
  rtl_f64_eq(s, t0, fp_dest, fp_src);
  rtl_shli(s, t0, t0, 14);
  rtl_or(s, s0, s0, t0);
  rtl_sr_fsw(s,s0);
}
static inline make_EHelper(fcom){
  fucom_helper(s,dfdest,dfsrc1);
  print_asm_fpu_template2(fcom);
}
static inline make_EHelper(fcomp){
  fucom_helper(s,dfdest,dfsrc1);
  rtl_popftop();
  print_asm_fpu_template2(fcomp);
}
static inline make_EHelper(fcompp){
  fucom_helper(s,dfdest,dfsrc1);
  rtl_pop2ftop();
  print_asm_fpu_template2(fcompp);
}
static inline make_EHelper(fucom){
  fucom_helper(s,dfdest,dfsrc1);
  print_asm_fpu_template2(fucom);
}
static inline make_EHelper(fucomp){
  fucom_helper(s,dfdest,dfsrc1);
  rtl_popftop();
  print_asm_fpu_template2(fucomp);
}
static inline make_EHelper(fucompp){
  fucom_helper(s,dfdest,dfsrc1);
  rtl_pop2ftop();
  print_asm_fpu_template2(fucompp);
}

static inline make_EHelper(fld){
  operand_fwrite(s, id_dest, dfsrc1);
  print_asm_fpu_template2(fld);
}
static inline make_EHelper(fld1){
  rtl_fld_const(s, dfdest, fconst_1);
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
  rtl_fld_const(s, dfdest, fconst_z);
  print_asm_fpu_template(fldz);
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
  rtl_sfr(s, &id_src2->fval, dfdest);
  rtl_sfr(s, dfdest, dfsrc1);
  rtl_sfr(s, dfsrc1, &id_src2->fval);
  print_asm_fpu_template2(fxch);
}

static inline make_EHelper(fchs){
  rtl_f64_chs(s, dfdest);
  print_asm_fpu_template(fchs);
}
static inline make_EHelper(fabs){
  rtl_f64_abs(s, dfdest);
  print_asm_fpu_template(fabs);
}
static inline make_EHelper(ftst){
  rtl_fld_const(s, &id_src1->fval, fconst_z);
  fucom_helper(s,dfdest,&id_src1->fval);
  print_asm_fpu_template(ftst);
}
static inline make_EHelper(fxam){
  //produce number in table as 80387 demand
  rtl_class387(s, s2, dfdest);
  rtl_li(s, t0, 0x4700);
  rtl_lr_fsw(s, s0);
  rtl_or(s, s0, s0, t0);
  rtl_xor(s, s0, s0, t0);
  rtl_or(s, s0, s0, s2);
  rtl_sr_fsw(s, s0);
  print_asm_fpu_template(fxam);
}


static inline make_EHelper(finit){
  rtl_mv(s,&cpu.ftop, rz);
  rtl_mv(s,&cpu.fsw,rz);
  rtl_li(s,&cpu.fcw,0x37f);
  print_asm("finit");
}
static inline make_EHelper(fstsw){
  operand_write(s, id_dest, dsrc1);
  print_asm_template2(fnstsw);
#ifndef __DIFF_REF_NEMU__
  difftest_skip_ref();
#endif
}

static inline make_EHelper(fldcw){
  rtl_sr_fcw(s, dsrc1);
  print_asm_template2(fldcw);
}
static inline make_EHelper(fstcw){
  operand_write(s, id_dest, dsrc1);
  print_asm_template2(fnstcw);
}

static inline make_EHelper(fldenv){
  TODO();
}
static inline make_EHelper(fstenv){
  TODO();
}