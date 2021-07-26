#ifndef __SOFTFLOAT_FP_H__
#define __SOFTFLOAT_FP_H__

#include <softfloat.h>
#include <specialize.h>
#include <internals.h>

#define F32_SIGN ((uint64_t)1ul << 31)
#define F64_SIGN ((uint64_t)1ul << 63)

static inline float32_t rtlToF32(rtlreg_t r);
static inline float64_t rtlToF64(rtlreg_t r);

static inline float32_t f32_min(float32_t a, float32_t b){
  bool less = f32_lt_quiet(a, b) || (f32_eq(a, b) && (a.v & F32_SIGN));
  if(isNaNF32UI(a.v) && isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return(less || isNaNF32UI(b.v) ? a : b);
}

static inline float32_t f32_max(float32_t a, float32_t b){
  bool greater = f32_lt_quiet(b, a) || (f32_eq(b, a) && (b.v & F32_SIGN));
  if(isNaNF32UI(a.v) && isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return(greater || isNaNF32UI(b.v) ? a : b);
}

static inline float64_t f64_min(float64_t a, float64_t b){
  bool less = f64_lt_quiet(a, b) || (f64_eq(a, b) && (a.v & F64_SIGN));
  if(isNaNF64UI(a.v) && isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return(less || isNaNF64UI(b.v) ? a : b);
}

static inline float64_t f64_max(float64_t a, float64_t b){
  bool greater = f64_lt_quiet(b, a) || (f64_eq(b, a) && (b.v & F64_SIGN));
  if(isNaNF64UI(a.v) && isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return(greater || isNaNF64UI(b.v) ? a : b);
}

static inline int32_t  my_f32_to_i32 (float32_t a) {
  return f32_to_i32 (a, softfloat_roundingMode, true);
}
static inline uint32_t my_f32_to_ui32(float32_t a) {
  return f32_to_ui32(a, softfloat_roundingMode, true);
}
static inline int64_t  my_f32_to_i64 (float32_t a) {
  return f32_to_i64 (a, softfloat_roundingMode, true);
}
static inline uint64_t my_f32_to_ui64(float32_t a) {
  return f32_to_ui64(a, softfloat_roundingMode, true);
}
static inline int32_t  my_f64_to_i32 (float64_t a) {
  return f64_to_i32 (a, softfloat_roundingMode, true);
}
static inline uint32_t my_f64_to_ui32(float64_t a) {
  return f64_to_ui32(a, softfloat_roundingMode, true);
}
static inline int64_t  my_f64_to_i64 (float64_t a) {
  return f64_to_i64 (a, softfloat_roundingMode, true);
}
static inline uint64_t my_f64_to_ui64(float64_t a) {
  return f64_to_ui64(a, softfloat_roundingMode, true);
}

uint_fast16_t f32_classify( float32_t a )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;

    uA.f = a;
    uiA = uA.ui;

    uint_fast16_t infOrNaN = expF32UI( uiA ) == 0xFF;
    uint_fast16_t subnormalOrZero = expF32UI( uiA ) == 0;
    bool sign = signF32UI( uiA );
    bool fracZero = fracF32UI( uiA ) == 0;
    bool isNaN = isNaNF32UI( uiA );
    bool isSNaN = softfloat_isSigNaNF32UI( uiA );

    return
        (  sign && infOrNaN && fracZero )          << 0 |
        (  sign && !infOrNaN && !subnormalOrZero ) << 1 |
        (  sign && subnormalOrZero && !fracZero )  << 2 |
        (  sign && subnormalOrZero && fracZero )   << 3 |
        ( !sign && infOrNaN && fracZero )          << 7 |
        ( !sign && !infOrNaN && !subnormalOrZero ) << 6 |
        ( !sign && subnormalOrZero && !fracZero )  << 5 |
        ( !sign && subnormalOrZero && fracZero )   << 4 |
        ( isNaN &&  isSNaN )                       << 8 |
        ( isNaN && !isSNaN )                       << 9;
}

static inline uint_fast16_t f64_classify( float64_t a ) {
  union ui64_f64 uA;
  uint_fast64_t uiA;

  uA.f = a;
  uiA = uA.ui;

  uint_fast16_t infOrNaN = expF64UI( uiA ) == 0x7FF;
  uint_fast16_t subnormalOrZero = expF64UI( uiA ) == 0;
  bool sign = signF64UI( uiA );
  bool fracZero = fracF64UI( uiA ) == 0;
  bool isNaN = isNaNF64UI( uiA );
  bool isSNaN = softfloat_isSigNaNF64UI( uiA );

  return
    (  sign && infOrNaN && fracZero )          << 0 |
    (  sign && !infOrNaN && !subnormalOrZero ) << 1 |
    (  sign && subnormalOrZero && !fracZero )  << 2 |
    (  sign && subnormalOrZero && fracZero )   << 3 |
    ( !sign && infOrNaN && fracZero )          << 7 |
    ( !sign && !infOrNaN && !subnormalOrZero ) << 6 |
    ( !sign && subnormalOrZero && !fracZero )  << 5 |
    ( !sign && subnormalOrZero && fracZero )   << 4 |
    ( isNaN &&  isSNaN )                       << 8 |
    ( isNaN && !isSNaN )                       << 9;
}

static inline void fp_set_rm(int rm) {
  switch (rm) {
    case FPCALL_RM_RNE: softfloat_roundingMode = softfloat_round_near_even; break;
    case FPCALL_RM_RTZ: softfloat_roundingMode = softfloat_round_minMag; break;
    case FPCALL_RM_RDN: softfloat_roundingMode = softfloat_round_min; break;
    case FPCALL_RM_RUP: softfloat_roundingMode = softfloat_round_max; break;
    case FPCALL_RM_RMM: softfloat_roundingMode = softfloat_round_near_maxMag; break;
    default: assert(0);
  }
}

static inline uint32_t fp_get_exception() {
  uint32_t ex = 0;
  uint32_t softfp_ex = softfloat_exceptionFlags;
  if (softfp_ex & softfloat_flag_inexact  ) ex |= FPCALL_EX_NX;
  if (softfp_ex & softfloat_flag_underflow) ex |= FPCALL_EX_UF;
  if (softfp_ex & softfloat_flag_overflow ) ex |= FPCALL_EX_OF;
  if (softfp_ex & softfloat_flag_infinite ) ex |= FPCALL_EX_DZ;
  if (softfp_ex & softfloat_flag_invalid  ) ex |= FPCALL_EX_NV;
  return ex;
}

static inline void fp_clear_exception() {
  softfloat_exceptionFlags = 0;
}
#endif
