#include <math.h>
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
static inline def_EHelper(concat(f, x)) { \
    concat(rtl_f64_, x)(s, dfdest, dfdest, dfsrc1); \
    print_asm_fpu_template2(concat(f, x)); \
}

#define BUILD_EXEC_F_R(x) \
static inline def_EHelper(concat3(f, x, r)) { \
    concat(rtl_f64_, x)(s, dfdest, dfsrc1, dfdest); \
    print_asm_fpu_template2(concat3(f, x, r)); \
}

#define BUILD_EXEC_F_P(x) \
static inline def_EHelper(concat3(f, x, p)) { \
    concat(rtl_f64_, x)(s, dfdest, dfdest, dfsrc1); \
    rtl_popftop(); \
    print_asm_fpu_template2(concat3(f, x, p)); \
}

#define BUILD_EXEC_F_RP(x) \
static inline def_EHelper(concat3(f, x, rp)) { \
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
static inline def_EHelper(fcom){
  fucom_helper(s,dfdest,dfsrc1);
  print_asm_fpu_template2(fcom);
}
static inline def_EHelper(fcomp){
  fucom_helper(s,dfdest,dfsrc1);
  rtl_popftop();
  print_asm_fpu_template2(fcomp);
}
static inline def_EHelper(fcompp){
  fucom_helper(s,dfdest,dfsrc1);
  rtl_pop2ftop();
  print_asm_fpu_template2(fcompp);
}
static inline def_EHelper(fucom){
  fucom_helper(s,dfdest,dfsrc1);
  print_asm_fpu_template2(fucom);
}
static inline def_EHelper(fucomp){
  fucom_helper(s,dfdest,dfsrc1);
  rtl_popftop();
  print_asm_fpu_template2(fucomp);
}
static inline def_EHelper(fucompp){
  fucom_helper(s,dfdest,dfsrc1);
  rtl_pop2ftop();
  print_asm_fpu_template2(fucompp);
}

//compare dest & src, save flag in eflags
static inline void fcom_helper(DecodeExecState *s, uint64_t* fp_dest, uint64_t* fp_src){
  rtl_f64_lt(s, t0, fp_dest, fp_src);
  rtl_set_CF(s, t0);
  rtl_f64_eq(s, t0, fp_dest, fp_src);
  rtl_set_ZF(s, t0);
  rtl_set_PF(s, rz);
}

static inline def_EHelper(fcomip){
  fcom_helper(s, dfdest, dfsrc1);
  rtl_popftop();
  print_asm_fpu_template2(fcomip);
}

static inline def_EHelper(fcomi){
  fcom_helper(s, dfdest, dfsrc1);
  print_asm_fpu_template2(fcomi);
}

static inline def_EHelper(fucomi){
  fcom_helper(s, dfdest, dfsrc1);
  print_asm_fpu_template2(fucomi);
}

static inline def_EHelper(fucomip){
  fcom_helper(s, dfdest, dfsrc1);
  rtl_popftop();
  print_asm_fpu_template2(fucomi);
}

static inline def_EHelper(fld){
  operand_fwrite(s, id_dest, dfsrc1);
  print_asm_fpu_template2(fld);
}
static inline def_EHelper(fld1){
  rtl_fld_const(s, dfdest, fconst_1);
  print_asm_fpu_template(fld1);
}
static inline def_EHelper(fldl2t){
  rtl_fld_const(s, dfdest, fconst_l2t);
  print_asm_fpu_template(fldl2t);
}
static inline def_EHelper(fldl2e){
  rtl_fld_const(s, dfdest, fconst_l2e);
  print_asm_fpu_template(fldl2e);
}
static inline def_EHelper(fldpi){
  rtl_fld_const(s, dfdest, fconst_pi);
  print_asm_fpu_template(fldpi);
}
static inline def_EHelper(fldlg2){
  rtl_fld_const(s, dfdest, fconst_lg2);
  print_asm_fpu_template(fldlg2);
}
static inline def_EHelper(fldln2){
  rtl_fld_const(s, dfdest, fconst_ln2);
  print_asm_fpu_template(fldln2);
}
static inline def_EHelper(fldz){ 
  rtl_fld_const(s, dfdest, fconst_z);
  print_asm_fpu_template(fldz);
}

static inline def_EHelper(fst){
  operand_fwrite(s, id_dest, dfsrc1);
  print_asm_fpu_template2(fst);
}
static inline def_EHelper(fstp){
  operand_fwrite(s, id_dest, dfsrc1);
  rtl_popftop();
  print_asm_fpu_template2(fstp);
}
static inline def_EHelper(fxch){
  rtl_sfr(s, &id_src2->fval, dfdest);
  rtl_sfr(s, dfdest, dfsrc1);
  rtl_sfr(s, dfsrc1, &id_src2->fval);
  print_asm_fpu_template2(fxch);
}

static inline def_EHelper(fchs){
  rtl_f64_chs(s, dfdest);
  print_asm_fpu_template(fchs);
}
static inline def_EHelper(fabs){
  rtl_f64_abs(s, dfdest);
  print_asm_fpu_template(fabs);
}
static inline def_EHelper(ftst){
  rtl_fld_const(s, &id_src1->fval, fconst_z);
  fucom_helper(s,dfdest,&id_src1->fval);
  print_asm_fpu_template(ftst);
}
static inline def_EHelper(fxam){
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


static inline def_EHelper(finit){
  rtl_mv(s,&cpu.ftop, rz);
  rtl_mv(s,&cpu.fsw,rz);
  rtl_li(s,&cpu.fcw,0x37f);
  print_asm("finit");
}
static inline def_EHelper(fstsw){
  operand_write(s, id_dest, dsrc1);
  print_asm_template2(fnstsw);
#ifndef __DIFF_REF_NEMU__
  difftest_skip_ref();
#endif
}

static inline def_EHelper(fldcw){
  rtl_sr_fcw(s, dsrc1);
  print_asm_template2(fldcw);
}
static inline def_EHelper(fstcw){
  operand_write(s, id_dest, dsrc1);
  print_asm_template2(fnstcw);
}

static inline def_EHelper(fldenv){
  rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 0, 4);
  rtl_sr_fcw(s, s0);
  rtl_lm(s, s0, s->isa.mbase, s->isa.moff + 4, 4);
  rtl_sr_fsw(s, s0);
  cpu.ftop = (cpu.fsw >> 11) & 0x7;
  // others are not loaded
  print_asm_template2(fldenv);
}

static inline def_EHelper(fstenv){
  rtl_sm(s, s->isa.mbase, s->isa.moff + 0, &cpu.fcw, 4);
  cpu.fsw = (cpu.fsw & ~0x3800) | ((cpu.ftop & 0x7) << 11);
  rtl_sm(s, s->isa.mbase, s->isa.moff + 4, &cpu.fsw, 4);
  rtlreg_t ftag = 0;
  int i;
  for (i = 7; i >= 0; i --) {
    ftag <<= 2;
    if (i > cpu.ftop) { ftag |= 3; } // empty
    else {
      rtl_class387(s, s0, &cpu.fpr[i]);
      if (*s0 == 0x4200 || *s0 == 0x4000) { ftag |= 1; } // zero
      else if (*s0 == 0x700 || *s0 == 0x4600 || *s0 == 0x4400 || *s0 == 0x500 ||
          *s0 == 0x100 || *s0 == 0x300) { ftag |= 2; } // NaNs, INFs, denormal
    }
  }
  rtl_sm(s, s->isa.mbase, s->isa.moff + 8, &ftag, 4);
  rtl_sm(s, s->isa.mbase, s->isa.moff + 12, rz, 4);  // fpip
  rtl_sm(s, s->isa.mbase, s->isa.moff + 16, rz, 4);  // fpcs
  rtl_sm(s, s->isa.mbase, s->isa.moff + 20, rz, 4);  // fpoo
  rtl_sm(s, s->isa.mbase, s->isa.moff + 24, rz, 4);  // fpos
  print_asm_template2(fstenv);
}

static inline def_EHelper(fcmovbe){
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_BE);
#else
  rtl_setcc(s, s0, CC_BE);
#endif
  if (*s0) {
    *dfdest = *dfsrc1;
  }
  print_asm_fpu_template2(fcmovbe);
}

static inline def_EHelper(fcmovnbe){
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_BE);
#else
  rtl_setcc(s, s0, CC_BE);
#endif
  if (!*s0) {
    *dfdest = *dfsrc1;
  }
  print_asm_fpu_template2(fcmovnbe);
}

static inline def_EHelper(fcmove){
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_E);
#else
  rtl_setcc(s, s0, CC_E);
#endif
  if (*s0) {
    *dfdest = *dfsrc1;
  }
  print_asm_fpu_template2(fcmove);
}

static inline def_EHelper(fcmovne){
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_NE);
#else
  rtl_setcc(s, s0, CC_NE);
#endif
  if (*s0) {
    *dfdest = *dfsrc1;
  }
  print_asm_fpu_template2(fcmovne);
}

static inline def_EHelper(fcmovb){
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B);
#else
  rtl_setcc(s, s0, CC_B);
#endif
  if (*s0) {
    *dfdest = *dfsrc1;
  }
  print_asm_fpu_template2(fcmovb);
}

static inline def_EHelper(fcmovnb){
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_B);
#else
  rtl_setcc(s, s0, CC_B);
#endif
  if (!(*s0)) {
    *dfdest = *dfsrc1;
  }
  print_asm_fpu_template2(fcmovb);
}

static inline def_EHelper(fcmovu){
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_P);
#else
  rtl_setcc(s, s0, CC_P);
#endif
  if (*s0) {
    *dfdest = *dfsrc1;
  }
  print_asm_fpu_template2(fcmovu);
}

static inline def_EHelper(fcmovnu){
#ifdef LAZY_CC
  rtl_lazy_setcc(s, s0, CC_P);
#else
  rtl_setcc(s, s0, CC_P);
#endif
  if (!(*s0)) {
    *dfdest = *dfsrc1;
  }
  print_asm_fpu_template2(fcmovnu);
}

static inline def_EHelper(fsqrt) {
  *dfdest = f64_sqrt(fprToF64(*dfdest)).v;
  print_asm_fpu_template(fsqrt);
}

static inline def_EHelper(fyl2x) {
  union {
    double f;
    uint64_t i;
  } a;
  a.i = *dfdest;
  a.f = log2(a.f);
  *dfsrc1 = f64_mul(fprToF64(*dfsrc1), fprToF64(a.i)).v;
  rtl_popftop();
  print_asm_fpu_template(fyl2x);
}

static inline def_EHelper(frndint) {
  *dfdest = f64_roundToInt(fprToF64(*dfdest), softfloat_roundingMode, false).v;
  print_asm_fpu_template(frndint);
}

static inline def_EHelper(fyl2xp1) {
  union {
    double f;
    uint64_t i;
  } a;
  a.i = *dfdest;
  a.f = log2(a.f + 1.0);
  *dfsrc1 = f64_mul(fprToF64(*dfsrc1), fprToF64(a.i)).v;
  rtl_popftop();
  print_asm_fpu_template(fyl2xp1);
}

static inline def_EHelper(f2xm1) {
  union {
    double f;
    uint64_t i;
  } a;
  a.i = *dfdest;
  a.f = pow(2.0, a.f) - 1.0;
  *dfsrc1 = a.i;
  print_asm_fpu_template(f2xm1);
}

static inline def_EHelper(fscale) {
  union {
    double f;
    uint64_t i;
  } a;
  a.i = f64_roundToInt(fprToF64(*dfsrc1), softfloat_round_minMag, false).v;
  a.f = pow(2.0, a.f);
  *dfdest = f64_mul(fprToF64(*dfdest), fprToF64(a.i)).v;
  print_asm_fpu_template(fscale);
}

static inline def_EHelper(fpatan) {
  union {
    double f;
    uint64_t i;
  } dest, src;
  dest.i = *dfdest;
  src.i = *dfsrc1;

  double tmp = src.f / dest.f;
  dest.f = atan(tmp);
  if (src.f < 0 && dest.f > 0) { dest.f -= M_PI; }
  else if (src.f > 0 && dest.f < 0) { dest.f += M_PI; }
  assert(fabs(dest.f) < M_PI);

  *dfsrc1 = dest.i;
  rtl_popftop();
  print_asm_fpu_template(fpatan);
}

static inline def_EHelper(fprem) {
  union {
    double f;
    uint64_t i;
  } a, b;
  a.i = *dfdest;
  b.i = *dfsrc1;
  a.f = fmod(a.f, b.f);
  *dfdest = a.i;
  rtl_lr_fsw(s, s0);
  rtl_andi(s, s0, s0, ~0x4700);
  rtl_sr_fsw(s, s0);
  print_asm_fpu_template(fprem);
}
