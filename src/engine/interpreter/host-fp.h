/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __HOSTFP_H__
#define __HOSTFP_H__

// Best-effort host FPU backend (the CONFIG_FPU_HOST alternative to
// softfloat-fp.h). f32/f64 arithmetic runs on the native float/double types;
// f16/bf16 are emulated by promoting to float, so the backend builds on hosts
// whose compiler lacks native _Float16/__bf16 (e.g. GCC < 12/13). The
// integer/table bit algorithms (fli, recip7/rsqrte7, classify, min/max,
// fcvtmod, sign-inject) are ported from softfloat. Results are imprecise: host
// NaN payloads differ from the RISC-V reference and fflags are not tracked.
// Intended for performance work (e.g. checkpoint generation), not as a bit-exact
// difftest reference. The default backend remains FPU_SOFT.

#include <rtl/fp.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <fenv.h>

// ---------------- types ----------------
typedef union { uint16_t v; } float16_t;   // emulated via float (see conversions)
typedef union { uint16_t v; } bfloat16_t;  // emulated via float (see conversions)
typedef union { uint32_t v; float    f; } float32_t;
typedef union { uint64_t v; double   f; } float64_t;

#define F16_SIGN ((uint64_t)1ul << 15)
#define F32_SIGN ((uint64_t)1ul << 31)
#define F64_SIGN ((uint64_t)1ul << 63)

#define defaultNaNF16UI  0x7E00
#define defaultNaNBF16UI 0x7FC0
#define defaultNaNF32UI  0x7FC00000
#define defaultNaNF64UI  UINT64_C(0x7FF8000000000000)

// ---------------- softfloat-compatible bit accessors ----------------
#define signF16UI(a)  (((a) >> 15) & 1)
#define expF16UI(a)   (((a) >> 10) & 0x1F)
#define fracF16UI(a)  ((a) & 0x3FF)
#define isNaNF16UI(a) (expF16UI(a) == 0x1F && fracF16UI(a) != 0)
#define softfloat_isSigNaNF16UI(a) (expF16UI(a) == 0x1F && !((a) & 0x200) && fracF16UI(a) != 0)

#define signF32UI(a)  (((a) >> 31) & 1)
#define expF32UI(a)   (((a) >> 23) & 0xFF)
#define fracF32UI(a)  ((a) & 0x7FFFFF)
#define isNaNF32UI(a) (expF32UI(a) == 0xFF && fracF32UI(a) != 0)
#define softfloat_isSigNaNF32UI(a) (expF32UI(a) == 0xFF && !((a) & 0x400000) && fracF32UI(a) != 0)

#define signF64UI(a)  (((a) >> 63) & 1)
#define expF64UI(a)   (((a) >> 52) & 0x7FF)
#define fracF64UI(a)  ((a) & UINT64_C(0xFFFFFFFFFFFFF))
#define isNaNF64UI(a) (expF64UI(a) == 0x7FF && fracF64UI(a) != 0)
#define softfloat_isSigNaNF64UI(a) (expF64UI(a) == 0x7FF && !((a) & UINT64_C(0x8000000000000)) && fracF64UI(a) != 0)

#define fsgnj16(a, b, n, x) \
  (uint16_t)((a.v & ~F16_SIGN) | ((((x) ? a.v : (n) ? F16_SIGN : 0) ^ b.v) & F16_SIGN))
#define fsgnj32(a, b, n, x) \
  (uint32_t)((a.v & ~F32_SIGN) | ((((x) ? a.v : (n) ? F32_SIGN : 0) ^ b.v) & F32_SIGN))
#define fsgnj64(a, b, n, x) \
  (uint64_t)((a.v & ~F64_SIGN) | ((((x) ? a.v : (n) ? F64_SIGN : 0) ^ b.v) & F64_SIGN))

// ---------------- rounding mode ----------------
// Values match the FPCALL_RM_* encoding (RNE=0 is the host_round_dbl default and
// needs no name); only the modes referenced by name are listed.
enum {
  softfloat_round_minMag      = 1,
  softfloat_round_min         = 2,
  softfloat_round_max         = 3,
  softfloat_round_near_maxMag = 4,
  softfloat_round_odd         = 6,
};

// RISC-V rounding mode (an FPCALL_RM_* value): fp.c writes it before
// rounding-sensitive ops; fp_set_rm mirrors it into the host fenv and the ported
// bit ops (recip7) read it. Static, as this header is included only by fp.c.
static uint_fast8_t softfloat_roundingMode = 0;

// Mirror the RISC-V rounding mode into the host fenv (cached: the guest changes
// it rarely and nothing else in the interpreter touches host fenv).
static inline void fp_set_rm(int rm) {
  static int last_rm = -1;
  if (rm == last_rm) return;
  last_rm = rm;
  switch (rm) {
    case FPCALL_RM_RNE: fesetround(FE_TONEAREST);  break;
    case FPCALL_RM_RTZ: fesetround(FE_TOWARDZERO); break;
    case FPCALL_RM_RDN: fesetround(FE_DOWNWARD);   break;
    case FPCALL_RM_RUP: fesetround(FE_UPWARD);     break;
    case FPCALL_RM_RMM: fesetround(FE_TONEAREST);  break; // x86 has no round-nearest-max-magnitude
    default: break;
  }
}

// fflags are not tracked (exact flags need softfloat). fp.c calls these on every
// FP op, so keep them trivial; the zero result keeps the exception path cold.
static inline uint32_t fp_get_exception(void) { return 0; }
static inline void fp_clear_exception(void) { }

// ---------------- binary16 / bfloat16 software conversions ----------------
// The host toolchain may lack native _Float16/__bf16 (e.g. GCC < 12/13), so f16
// and bf16 are emulated by promoting to float. These bit-level converts round to
// nearest-even; NaNs are canonicalized (this backend is not bit-exact).
static inline uint32_t h2f_bits(uint16_t h) {
  uint32_t sign = (uint32_t)(h & 0x8000) << 16;
  uint32_t exp  = (h >> 10) & 0x1F;
  uint32_t mant = h & 0x3FF;
  if (exp == 0) {
    if (mant == 0) return sign;                 // +/-0
    exp = 127 - 15 + 1;                          // subnormal: normalize
    while ((mant & 0x400) == 0) { mant <<= 1; exp--; }
    mant &= 0x3FF;
    return sign | (exp << 23) | (mant << 13);
  }
  if (exp == 0x1F) return sign | 0x7F800000 | (mant << 13); // inf / NaN
  return sign | ((exp - 15 + 127) << 23) | (mant << 13);    // normal
}
static inline uint16_t f2h_bits(uint32_t x) {
  uint32_t sign = (x >> 16) & 0x8000;
  uint32_t xexp = (x >> 23) & 0xFF;
  uint32_t mant = x & 0x7FFFFF;
  if (xexp == 0xFF) return (uint16_t)(sign | (mant ? 0x7E00 : 0x7C00)); // NaN / inf
  int32_t exp = (int32_t)xexp - 127 + 15;
  if (exp >= 0x1F) return (uint16_t)(sign | 0x7C00);                    // overflow -> inf
  if (exp <= 0) {                                                       // subnormal / zero
    if (exp < -10) return (uint16_t)sign;
    mant |= 0x800000;                                                   // restore implicit 1
    int shift = 14 - exp;                                               // 14..24
    uint32_t half = mant >> shift, rem = mant & ((1u << shift) - 1), mid = 1u << (shift - 1);
    if (rem > mid || (rem == mid && (half & 1))) half++;                // round to nearest-even
    return (uint16_t)(sign | half);
  }
  uint16_t h = (uint16_t)((exp << 10) | (mant >> 13));
  uint32_t rem = mant & 0x1FFF;
  if (rem > 0x1000 || (rem == 0x1000 && (h & 1))) h++;                  // round to nearest-even (may carry)
  return (uint16_t)(sign | h);
}
static inline float     f16_to_host(float16_t a) { float32_t t = {.v = h2f_bits(a.v)}; return t.f; }
static inline float16_t host_to_f16(float x)      { float32_t t = {.f = x}; return (float16_t){.v = f2h_bits(t.v)}; }
static inline double    f16_to_dbl(float16_t a)   { return (double)f16_to_host(a); }
static inline double    f32_to_dbl(float32_t a)   { return (double)a.f; }
static inline double    f64_to_dbl(float64_t a)   { return a.f; }

// bf16 is the high 16 bits of a binary32; conversions are exact / round-to-even.
static inline uint32_t bf16_to_f32_bits(uint16_t b) { return (uint32_t)b << 16; }
static inline uint16_t f32_to_bf16_bits(uint32_t x) {
  if (((x >> 23) & 0xFF) == 0xFF && (x & 0x7FFFFF)) return (uint16_t)((x >> 16) | 0x40); // quiet NaN
  return (uint16_t)((x + 0x7FFF + ((x >> 16) & 1)) >> 16);
}

// ---------------- native arithmetic / compares ----------------
#define DEF_ARITH(PFX, FT, T, SQRT, FMA)                                             \
  static inline FT PFX##_add(FT a, FT b){ return (FT){.f = (T)(a.f + b.f)}; }         \
  static inline FT PFX##_sub(FT a, FT b){ return (FT){.f = (T)(a.f - b.f)}; }         \
  static inline FT PFX##_mul(FT a, FT b){ return (FT){.f = (T)(a.f * b.f)}; }         \
  static inline FT PFX##_div(FT a, FT b){ return (FT){.f = (T)(a.f / b.f)}; }         \
  static inline FT PFX##_sqrt(FT a){ return (FT){.f = (T)SQRT(a.f)}; }                \
  static inline FT PFX##_mulAdd(FT a, FT b, FT c){ return (FT){.f = (T)FMA(a.f, b.f, c.f)}; } \
  static inline bool PFX##_le(FT a, FT b){ return a.f <= b.f; }                       \
  static inline bool PFX##_lt(FT a, FT b){ return a.f <  b.f; }                       \
  static inline bool PFX##_eq(FT a, FT b){ return a.f == b.f; }                       \
  static inline bool PFX##_le_quiet(FT a, FT b){ return a.f <= b.f; }                 \
  static inline bool PFX##_lt_quiet(FT a, FT b){ return a.f <  b.f; }
DEF_ARITH(f32, float32_t, float,    sqrtf, fmaf)
DEF_ARITH(f64, float64_t, double,   sqrt,  fma)
#undef DEF_ARITH

// f16 arithmetic is evaluated in float and rounded back to binary16.
static inline float16_t f16_add(float16_t a, float16_t b){ return host_to_f16(f16_to_host(a) + f16_to_host(b)); }
static inline float16_t f16_sub(float16_t a, float16_t b){ return host_to_f16(f16_to_host(a) - f16_to_host(b)); }
static inline float16_t f16_mul(float16_t a, float16_t b){ return host_to_f16(f16_to_host(a) * f16_to_host(b)); }
static inline float16_t f16_div(float16_t a, float16_t b){ return host_to_f16(f16_to_host(a) / f16_to_host(b)); }
static inline float16_t f16_sqrt(float16_t a){ return host_to_f16(sqrtf(f16_to_host(a))); }
static inline float16_t f16_mulAdd(float16_t a, float16_t b, float16_t c){ return host_to_f16(fmaf(f16_to_host(a), f16_to_host(b), f16_to_host(c))); }
static inline bool f16_le(float16_t a, float16_t b){ return f16_to_host(a) <= f16_to_host(b); }
static inline bool f16_lt(float16_t a, float16_t b){ return f16_to_host(a) <  f16_to_host(b); }
static inline bool f16_eq(float16_t a, float16_t b){ return f16_to_host(a) == f16_to_host(b); }
static inline bool f16_le_quiet(float16_t a, float16_t b){ return f16_to_host(a) <= f16_to_host(b); }
static inline bool f16_lt_quiet(float16_t a, float16_t b){ return f16_to_host(a) <  f16_to_host(b); }

static inline float16_t f16_neg(float16_t a){ return (float16_t){.v = (uint16_t)(a.v ^ F16_SIGN)}; }
static inline float32_t f32_neg(float32_t a){ return (float32_t){.v = (uint32_t)(a.v ^ F32_SIGN)}; }
static inline float64_t f64_neg(float64_t a){ return (float64_t){.v = a.v ^ F64_SIGN}; }
static inline float16_t f16_neg_zero(void){ return (float16_t){.v = F16_SIGN}; }
static inline float32_t f32_neg_zero(void){ return (float32_t){.v = F32_SIGN}; }
static inline float64_t f64_neg_zero(void){ return (float64_t){.v = F64_SIGN}; }

// ---------------- min / max (RISC-V semantics, ported) ----------------
#define DEF_MINMAX(PFX, FT, UT, SIGN, DNAN)                                              \
  static inline FT PFX##_min(FT a, FT b){                                                \
    bool less = PFX##_lt_quiet(a,b) || (PFX##_eq(a,b) && (a.v & SIGN));                  \
    if (isNaN##UT(a.v) && isNaN##UT(b.v)) return (FT){.v = DNAN};                         \
    return (less || isNaN##UT(b.v)) ? a : b; }                                            \
  static inline FT PFX##_max(FT a, FT b){                                                \
    bool greater = PFX##_lt_quiet(b,a) || (PFX##_eq(b,a) && (b.v & SIGN));               \
    if (isNaN##UT(a.v) && isNaN##UT(b.v)) return (FT){.v = DNAN};                         \
    return (greater || isNaN##UT(b.v)) ? a : b; }                                         \
  static inline FT PFX##_minm(FT a, FT b){                                               \
    bool less = PFX##_lt_quiet(a,b) || (PFX##_eq(a,b) && (a.v & SIGN));                  \
    if (isNaN##UT(a.v) || isNaN##UT(b.v)) return (FT){.v = DNAN};                         \
    return less ? a : b; }                                                                \
  static inline FT PFX##_maxm(FT a, FT b){                                               \
    bool greater = PFX##_lt_quiet(b,a) || (PFX##_eq(b,a) && (b.v & SIGN));               \
    if (isNaN##UT(a.v) || isNaN##UT(b.v)) return (FT){.v = DNAN};                         \
    return greater ? a : b; }
DEF_MINMAX(f16, float16_t, F16UI, F16_SIGN, defaultNaNF16UI)
DEF_MINMAX(f32, float32_t, F32UI, F32_SIGN, defaultNaNF32UI)
DEF_MINMAX(f64, float64_t, F64UI, F64_SIGN, defaultNaNF64UI)
#undef DEF_MINMAX

// ---------------- classify (ported) ----------------
#define DEF_CLASSIFY(PFX, FT, UT, EMAX)                                                   \
  static inline uint_fast16_t PFX##_classify(FT a){                                       \
    typeof(a.v) u = a.v;                                                                  \
    bool infOrNaN = exp##UT(u) == EMAX, subZero = exp##UT(u) == 0;                        \
    bool sign = sign##UT(u), fracZero = frac##UT(u) == 0;                                 \
    bool isNaN = isNaN##UT(u), isSNaN = softfloat_isSigNaN##UT(u);                        \
    return (uint_fast16_t)(                                                               \
      (( sign && infOrNaN && fracZero)          << 0) | (( sign && !infOrNaN && !subZero) << 1) | \
      (( sign && subZero && !fracZero)          << 2) | (( sign && subZero && fracZero)   << 3) | \
      ((!sign && infOrNaN && fracZero)          << 7) | ((!sign && !infOrNaN && !subZero) << 6) | \
      ((!sign && subZero && !fracZero)          << 5) | ((!sign && subZero && fracZero)   << 4) | \
      ((isNaN && isSNaN) << 8) | ((isNaN && !isSNaN) << 9)); }
DEF_CLASSIFY(f16, float16_t, F16UI, 0x1F)
DEF_CLASSIFY(f32, float32_t, F32UI, 0xFF)
DEF_CLASSIFY(f64, float64_t, F64UI, 0x7FF)
#undef DEF_CLASSIFY

// ---------------- round-to-integer in FP format ----------------
static inline double host_round_dbl(double x, uint_fast8_t rm){
  switch (rm) {
    case softfloat_round_minMag:      return trunc(x);
    case softfloat_round_min:         return floor(x);
    case softfloat_round_max:         return ceil(x);
    case softfloat_round_near_maxMag: return round(x);
    default: { double fl = floor(x), d = x - fl; // round-nearest-even
               // a tie (d == 0.5) only occurs when |x| < 2^52, so fl fits in int64
               return (d < 0.5) ? fl : (d > 0.5) ? fl + 1 : (((int64_t)fl & 1) == 0 ? fl : fl + 1); }
  }
}
#define DEF_ROUNDTOINT(PFX, FT, T)                                                \
  static inline FT PFX##_roundToInt(FT a, uint_fast8_t rm, bool exact){           \
    (void)exact;                                                                  \
    return (FT){.f = (T)host_round_dbl((double)a.f, rm)}; }
DEF_ROUNDTOINT(f32, float32_t, float)
DEF_ROUNDTOINT(f64, float64_t, double)
#undef DEF_ROUNDTOINT
static inline float16_t f16_roundToInt(float16_t a, uint_fast8_t rm, bool exact){
  (void)exact; return host_to_f16((float)host_round_dbl(f16_to_dbl(a), rm)); }

// ---------------- unsupported exotic ops (Zfa fli/fcvtmod, vector recip7/rsqrte7) ----------------
// The shared fp.c dispatcher names these, so they need definitions, but the host
// FPU backend does not implement them -- they are Zfa / vector-approximation
// instructions the scalar rv64gc target never executes. Hitting one panics.
#define HOST_FP_TODO(op) do { panic(op " is not supported by the host FPU backend"); } while (0)
static inline float16_t f16_fli(uint16_t a){ (void)a; HOST_FP_TODO("fli.h");  return (float16_t){0}; }
static inline float32_t f32_fli(uint32_t a){ (void)a; HOST_FP_TODO("fli.s");  return (float32_t){0}; }
static inline float64_t f64_fli(uint64_t a){ (void)a; HOST_FP_TODO("fli.d");  return (float64_t){0}; }
static inline int64_t   f64_fcvtmod(float64_t a){ (void)a; HOST_FP_TODO("fcvtmod.w.d"); return 0; }
static inline float16_t f16_rsqrte7(float16_t a){ (void)a; HOST_FP_TODO("vfrsqrt7.v (f16)"); return (float16_t){0}; }
static inline float32_t f32_rsqrte7(float32_t a){ (void)a; HOST_FP_TODO("vfrsqrt7.v (f32)"); return (float32_t){0}; }
static inline float64_t f64_rsqrte7(float64_t a){ (void)a; HOST_FP_TODO("vfrsqrt7.v (f64)"); return (float64_t){0}; }
static inline float16_t f16_recip7(float16_t a){ (void)a; HOST_FP_TODO("vfrec7.v (f16)"); return (float16_t){0}; }
static inline float32_t f32_recip7(float32_t a){ (void)a; HOST_FP_TODO("vfrec7.v (f32)"); return (float32_t){0}; }
static inline float64_t f64_recip7(float64_t a){ (void)a; HOST_FP_TODO("vfrec7.v (f64)"); return (float64_t){0}; }
#undef HOST_FP_TODO

// ---------------- format conversions ----------------
static inline float32_t f16_to_f32(float16_t a){ return (float32_t){.v = h2f_bits(a.v)}; }
static inline float64_t f16_to_f64(float16_t a){ return (float64_t){.f = f16_to_dbl(a)}; }
static inline float16_t f32_to_f16(float32_t a){ return (float16_t){.v = f2h_bits(a.v)}; }
static inline float64_t f32_to_f64(float32_t a){ return (float64_t){.f = (double)a.f}; }
static inline float16_t f64_to_f16(float64_t a){ float32_t t = {.f = (float)a.f}; return (float16_t){.v = f2h_bits(t.v)}; }
static inline float32_t f64_to_f32(float64_t a){ return (float32_t){.f = (float)a.f}; }
static inline float32_t bf16_to_f32(bfloat16_t a){ return (float32_t){.v = bf16_to_f32_bits(a.v)}; }
static inline bfloat16_t f32_to_bf16(float32_t a){ return (bfloat16_t){.v = f32_to_bf16_bits(a.v)}; }

// int -> fp (host conversions honor the current fenv rounding mode)
static inline float16_t i32_to_f16 (int32_t a){ return host_to_f16((float)a); }
static inline float16_t ui32_to_f16(uint32_t a){ return host_to_f16((float)a); }
static inline float16_t i64_to_f16 (int64_t a){ return host_to_f16((float)a); }
static inline float16_t ui64_to_f16(uint64_t a){ return host_to_f16((float)a); }
static inline float32_t i32_to_f32 (int32_t a){ return (float32_t){.f = (float)a}; }
static inline float32_t ui32_to_f32(uint32_t a){ return (float32_t){.f = (float)a}; }
static inline float32_t i64_to_f32 (int64_t a){ return (float32_t){.f = (float)a}; }
static inline float32_t ui64_to_f32(uint64_t a){ return (float32_t){.f = (float)a}; }
static inline float64_t i32_to_f64 (int32_t a){ return (float64_t){.f = (double)a}; }
static inline float64_t ui32_to_f64(uint32_t a){ return (float64_t){.f = (double)a}; }
static inline float64_t i64_to_f64 (int64_t a){ return (float64_t){.f = (double)a}; }
static inline float64_t ui64_to_f64(uint64_t a){ return (float64_t){.f = (double)a}; }

// fp -> int with RISC-V rounding-mode and saturation semantics.
static inline int64_t host_f_to_i(double x, uint_fast8_t rm, bool is_signed, int width){
  // Saturation bounds. sat_min for width 64 uses the bit pattern directly to
  // avoid negating INT64_MIN (signed-overflow UB); narrower widths negate safely.
  int64_t sat_max = is_signed ? (int64_t)((1ull << (width - 1)) - 1)
                              : (int64_t)(width == 64 ? UINT64_MAX : ((1ull << width) - 1));
  int64_t sat_min = is_signed ? (width == 64 ? (int64_t)(1ull << 63) : -((int64_t)1 << (width - 1))) : 0;
  if (isnan(x)) return sat_max;
  double r = host_round_dbl(x, rm);
  double hi = is_signed ? ldexp(1.0, width - 1) : ldexp(1.0, width); // exclusive upper bound
  double lo = is_signed ? -ldexp(1.0, width - 1) : 0.0;
  if (r >= hi) return sat_max;
  if (r < lo)  return sat_min;
  return is_signed ? (int64_t)r : (int64_t)(uint64_t)r;
}
#define DEF_FTOI(PFX, FT)                                                                                 \
  static inline uint_fast8_t  PFX##_to_ui8 (FT a, uint_fast8_t rm, bool e){ (void)e; return (uint8_t) host_f_to_i(PFX##_to_dbl(a), rm, false, 8);  } \
  static inline int_fast8_t   PFX##_to_i8  (FT a, uint_fast8_t rm, bool e){ (void)e; return (int8_t)  host_f_to_i(PFX##_to_dbl(a), rm, true,  8);  } \
  static inline uint_fast16_t PFX##_to_ui16(FT a, uint_fast8_t rm, bool e){ (void)e; return (uint16_t)host_f_to_i(PFX##_to_dbl(a), rm, false, 16); } \
  static inline int_fast16_t  PFX##_to_i16 (FT a, uint_fast8_t rm, bool e){ (void)e; return (int16_t) host_f_to_i(PFX##_to_dbl(a), rm, true,  16); } \
  static inline uint_fast32_t PFX##_to_ui32(FT a, uint_fast8_t rm, bool e){ (void)e; return (uint32_t)host_f_to_i(PFX##_to_dbl(a), rm, false, 32); } \
  static inline int_fast32_t  PFX##_to_i32 (FT a, uint_fast8_t rm, bool e){ (void)e; return (int32_t) host_f_to_i(PFX##_to_dbl(a), rm, true,  32); } \
  static inline uint_fast64_t PFX##_to_ui64(FT a, uint_fast8_t rm, bool e){ (void)e; return (uint64_t)host_f_to_i(PFX##_to_dbl(a), rm, false, 64); } \
  static inline int_fast64_t  PFX##_to_i64 (FT a, uint_fast8_t rm, bool e){ (void)e; return (int64_t) host_f_to_i(PFX##_to_dbl(a), rm, true,  64); }
DEF_FTOI(f16, float16_t)
DEF_FTOI(f32, float32_t)
DEF_FTOI(f64, float64_t)
#undef DEF_FTOI

// ---------------- NaN-boxing (my_*) wrappers used by the scalar dispatch ----------------
static inline float16_t my_f32_to_f16 (float32_t a){ return f32_to_f16(a); }
static inline bfloat16_t my_f32_to_bf16(float32_t a){ return f32_to_bf16(a); }
static inline float32_t  my_f16_to_f32 (float16_t a){ return f16_to_f32(a); }
static inline float32_t  my_bf16_to_f32(bfloat16_t a){ return bf16_to_f32(a); }
static inline float64_t  my_f16_to_f64 (float16_t a){ return f16_to_f64(a); }
static inline float16_t  my_f64_to_f16 (float64_t a){ return f64_to_f16(a); }
static inline float16_t  my_i32_to_f16 (int32_t a){ return i32_to_f16(a); }
static inline float16_t  my_ui32_to_f16(uint32_t a){ return ui32_to_f16(a); }
static inline float16_t  my_i64_to_f16 (int64_t a){ return i64_to_f16(a); }
static inline float16_t  my_ui64_to_f16(uint64_t a){ return ui64_to_f16(a); }
#define DEF_MY_FTOI(PFX, FT)                                                                        \
  static inline int32_t  my_##PFX##_to_i32 (FT a){ return PFX##_to_i32 (a, softfloat_roundingMode, true); } \
  static inline uint32_t my_##PFX##_to_ui32(FT a){ return PFX##_to_ui32(a, softfloat_roundingMode, true); } \
  static inline int64_t  my_##PFX##_to_i64 (FT a){ return PFX##_to_i64 (a, softfloat_roundingMode, true); } \
  static inline uint64_t my_##PFX##_to_ui64(FT a){ return PFX##_to_ui64(a, softfloat_roundingMode, true); }
DEF_MY_FTOI(f16, float16_t)
DEF_MY_FTOI(f32, float32_t)
DEF_MY_FTOI(f64, float64_t)
#undef DEF_MY_FTOI

#endif // __HOSTFP_H__
