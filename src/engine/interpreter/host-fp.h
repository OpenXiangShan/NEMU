#ifndef __HOSTFP_H__
#include <rtl/fp.h>
#include <math.h>
#include <fenv.h>

#define defaultNaNF32UI 0x7FC00000

typedef union { uint32_t v; float  f; } float32_t;
static inline float32_t float32(float f) { float32_t r = { .f = f }; return r; }

static inline float32_t f32_add(float32_t a, float32_t b) { return float32(a.f + b.f); }
static inline float32_t f32_sub(float32_t a, float32_t b) { return float32(a.f - b.f); }
static inline float32_t f32_mul(float32_t a, float32_t b) { return float32(a.f * b.f); }
static inline float32_t f32_div(float32_t a, float32_t b) { return float32(a.f / b.f); }
static inline float32_t f32_sqrt(float32_t a) { return float32(sqrtf(a.f)); }
static inline float32_t f32_mulAdd(float32_t a, float32_t b,
    float32_t c) { return float32(fmaf(a.f, b.f, c.f)); }
static inline float32_t f32_min(float32_t a, float32_t b) { return float32(a.f < b.f ? a.f : b.f); }
static inline float32_t f32_max(float32_t a, float32_t b) { return float32(a.f > b.f ? a.f : b.f); }
static inline bool f32_le(float32_t a, float32_t b) { return a.f <= b.f; }
static inline bool f32_lt(float32_t a, float32_t b) { return a.f <  b.f; }
static inline bool f32_eq(float32_t a, float32_t b) { return a.f == b.f; }
static inline float32_t i32_to_f32 (rtlreg_t a) { return float32((int32_t)a); }
static inline float32_t ui32_to_f32(rtlreg_t a) { return float32((uint32_t)a); }
static inline float32_t i64_to_f32 (rtlreg_t a) { return float32((int64_t)a); }
static inline float32_t ui64_to_f32(rtlreg_t a) { return float32((uint64_t)a); }
static inline int32_t  my_f32_to_i32 (float32_t a) { return llrintf(a.f); }
static inline uint32_t my_f32_to_ui32(float32_t a) { return llrintf(a.f); }
static inline int64_t  my_f32_to_i64 (float32_t a) { return llrintf(a.f); }
static inline uint64_t my_f32_to_ui64(float32_t a) { return llrintf(a.f); }


typedef union { uint64_t v; double f; } float64_t;
static inline float64_t float64(double f) { float64_t r = { .f = f }; return r; }

static inline float64_t f64_add(float64_t a, float64_t b) { return float64(a.f + b.f); }
static inline float64_t f64_sub(float64_t a, float64_t b) { return float64(a.f - b.f); }
static inline float64_t f64_mul(float64_t a, float64_t b) { return float64(a.f * b.f); }
static inline float64_t f64_div(float64_t a, float64_t b) { return float64(a.f / b.f); }
static inline float64_t f64_sqrt(float64_t a) { return float64(sqrt(a.f)); }
static inline float64_t f64_mulAdd(float64_t a, float64_t b,
    float64_t c) { return float64(fma(a.f, b.f, c.f)); }
static inline float64_t f64_min(float64_t a, float64_t b) { return float64(a.f < b.f ? a.f : b.f); }
static inline float64_t f64_max(float64_t a, float64_t b) { return float64(a.f > b.f ? a.f : b.f); }
static inline bool f64_le(float64_t a, float64_t b) { return a.f <= b.f; }
static inline bool f64_lt(float64_t a, float64_t b) { return a.f <  b.f; }
static inline bool f64_eq(float64_t a, float64_t b) { return a.f == b.f; }
static inline float64_t i32_to_f64 (rtlreg_t a) { return float64((int32_t)a); }
static inline float64_t ui32_to_f64(rtlreg_t a) { return float64((uint32_t)a); }
static inline float64_t i64_to_f64 (rtlreg_t a) { return float64((int64_t)a); }
static inline float64_t ui64_to_f64(rtlreg_t a) { return float64((uint64_t)a); }
static inline int32_t  my_f64_to_i32 (float64_t a) { return llrint(a.f); }
static inline uint32_t my_f64_to_ui32(float64_t a) { return llrint(a.f); }
static inline int64_t  my_f64_to_i64 (float64_t a) { return llrint(a.f); }
static inline uint64_t my_f64_to_ui64(float64_t a) { return llrint(a.f); }

static inline float64_t f32_to_f64(float32_t a) { return float64(a.f); }
static inline float32_t f64_to_f32(float64_t a) { return float32(a.f); }


static inline void fp_set_rm(int rm) {
  switch (rm) {
    case FPCALL_RM_RNE: rm = FE_TONEAREST; break;
    case FPCALL_RM_RTZ: rm = FE_TOWARDZERO; break;
    case FPCALL_RM_RDN: rm = FE_DOWNWARD; break;
    case FPCALL_RM_RUP: rm = FE_UPWARD; break;
    case FPCALL_RM_RMM: rm = FE_TONEAREST; break; // x86 does not support RMM
    default: assert(0);
  }
  fesetround(rm);
}

static inline uint32_t fp_get_exception() {
  uint32_t ex = 0;
#if 0
  uint32_t host_ex = fetestexcept(FE_ALL_EXCEPT);
  if (host_ex & FE_INEXACT  ) ex |= FPCALL_EX_NX;
  if (host_ex & FE_UNDERFLOW) ex |= FPCALL_EX_UF;
  if (host_ex & FE_OVERFLOW ) ex |= FPCALL_EX_OF;
  if (host_ex & FE_DIVBYZERO) ex |= FPCALL_EX_DZ;
  if (host_ex & FE_INVALID  ) ex |= FPCALL_EX_NV;
#endif
  return ex;
}

static inline void fp_clear_exception() {
  feclearexcept(FE_ALL_EXCEPT);
}

uint_fast16_t f32_classify( float32_t a )
{
  panic("host-fp does not support f32_classify");
}

uint_fast16_t f64_classify( float64_t a )
{
  panic("host-fp does not support f64_classify");
}

#endif
