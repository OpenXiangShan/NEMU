#ifndef __RISCV64_FPU_H__
#define __RISCV64_FPU_H__

#include "rtl/rtl.h"
#include "softfloat/softfloat.h"
#include "softfloat/specialize.h"
#include "softfloat/internals.h"
#include "../csr.h"
#include "../intr.h"


inline void write_fflags(rtlreg_t v){
  // call csr_write to keep fflags and fcsr consistent
  csr_write(1, &v);
}

#define require(x) if (!(x)) longjmp_raise_intr(EX_II);
#define require_fp require(mstatus->fs != 0)
#define dirty_fp_state (mstatus->val |= MSTATUS_FS | MSTATUS64_SD)
#define set_fp_exceptions ({ if (softfloat_exceptionFlags) { \
                               dirty_fp_state; \
                               write_fflags(fflags->val | softfloat_exceptionFlags); \
                             } \
                             softfloat_exceptionFlags = 0; })
#define RM ({ int rm = decinfo.isa.instr.rm; \
              if(rm == 7) rm = frm->val; \
              if(rm > 4) assert(0); \
              rm; })
#define isBoxedF32(r) ((uint32_t)((r >> 32) + 1) == 0)
#define unboxF32(r) (isBoxedF32(r) ? (uint32_t)r : defaultNaNF32UI)
#define F32_SIGN ((uint32_t)1 << 31)
#define F64_SIGN ((uint64_t)1 << 63)
#define fsgnj32(a, b, n, x) \
  ((f32(a).v & ~F32_SIGN) | ((((x) ? f32(a).v : (n) ? F32_SIGN : 0) ^ f32(b).v) & F32_SIGN))
#define fsgnj64(a, b, n, x) \
  ((f64(a).v & ~F64_SIGN) | ((((x) ? f64(a).v : (n) ? F64_SIGN : 0) ^ f64(b).v) & F64_SIGN))


inline float32_t f32(rtlreg_t v) { float32_t f; f.v = unboxF32(v); return f; }

inline float64_t f64(rtlreg_t v) { float64_t f; f.v = v; return f; }

static inline void lfpr(rtlreg_t* dest, int r) {
  *dest = fpreg_l(r);
}

static inline void sfpr(int r, const rtlreg_t *src1, int width) {
  if(width == 4){
    fpreg_l(r) = ((uint64_t)-1 << 32) | (*src1);
  }else{
    assert(width==8);
    fpreg_l(r) = *src1;
  }
  dirty_fp_state;
}

float32_t f32_min(float32_t a, float32_t b);

float64_t f64_min(float64_t a, float64_t b);

float32_t f32_max(float32_t a, float32_t b);

float64_t f64_max(float64_t a, float64_t b);

// a macro to build exec_fadd/fsub/fmul/fmdiv/fmin/fmax
#define BUILD_EXEC_F(x) \
  make_EHelper(concat(f, x)) { \
    require_fp; \
    softfloat_roundingMode = RM; \
    switch (decinfo.width) \
    { \
    case 8: \
      s0 = concat(f64_, x)(f64(id_src->val), f64(id_src2->val)).v; \
      print_asm_fpu_template_3(concat(f, x), d); \
      break; \
    case 4: \
      s0 = concat(f32_, x)(f32(id_src->val), f32(id_src2->val)).v; \
      print_asm_fpu_template_3(concat(f, x), s); \
      break; \
    default: \
      assert(0); \
    } \
    sfpr(id_dest->reg, &s0, decinfo.width); \
    set_fp_exceptions; \
  }

// macros to build exec_fmadd/fnmadd/fmsub/fnmsub

inline rtlreg_t neg64(rtlreg_t a){
  return a ^ F64_SIGN;
}
inline rtlreg_t neg32(rtlreg_t a){
  return ((uint64_t)-1 << 32) | (a ^ F32_SIGN);
}

#define NEG(w, src) concat(neg, w)(src)

#define fnmsub(w, src1, src2, src3) \
  concat3(f, w, _mulAdd)( \
    concat(f, w)(NEG(w, src1)), \
    concat(f, w)(src2), \
    concat(f, w)(src3) \
  ).v
#define fmsub(w, src1, src2, src3) \
  concat3(f, w, _mulAdd)( \
    concat(f, w)(src1), \
    concat(f, w)(src2), \
    concat(f ,w)(NEG(w, src3)) \
  ).v
#define fmadd(w, src1, src2, src3) \
  concat3(f, w, _mulAdd)( \
    concat(f, w)(src1), \
    concat(f, w)(src2), \
    concat(f, w)(src3) \
  ).v
#define fnmadd(w, src1, src2, src3) \
  concat3(f, w, _mulAdd)( \
    concat(f, w)(NEG(w, src1)), \
    concat(f, w)(src2), \
    concat(f ,w)(NEG(w, src3)) \
  ).v

#define BUILD_EXEC_FM(func) \
  make_EHelper(func){ \
    require_fp; \
    softfloat_roundingMode = RM; \
    s1 = fpreg_l(decinfo.isa.instr.funct5); \
    switch (decinfo.width) \
    { \
    case 8: \
      s0 = func(64, id_src->val, id_src2->val, s1); \
      print_asm_fpu_template_4(func, d); \
      break; \
    case 4: \
      s0 = func(32, id_src->val, id_src2->val, s1); \
      print_asm_fpu_template_4(func, s); \
      break; \
    default: \
      assert(0); \
    } \
    sfpr(id_dest->reg, &s0, decinfo.width); \
    set_fp_exceptions; \
  }

// dasm
#define print_asm_fpu_template_3(inst, fmt) \
  print_asm(str(inst) "." str(fmt) "%c %s,%s,%s", \
    suffix_char(id_dest->width), id_src->str, id_src2->str, id_dest->str)

#define print_asm_fpu_template_4(inst, fmt) \
  print_asm(str(inst) "." str(fmt) "%c %s,%s,%s,%s", \
    suffix_char(id_dest->width), id_src->str, id_src2->str, fpreg_name(decinfo.isa.instr.funct5, 4), id_dest->str)

#endif