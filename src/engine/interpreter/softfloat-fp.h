#ifndef __SOFTFLOAT_FP_H__
#define __SOFTFLOAT_FP_H__

#include <softfloat.h>
#include <specialize.h>
#include <internals.h>

#define F16_SIGN ((uint64_t)1ul << 15)
#define F32_SIGN ((uint64_t)1ul << 31)
#define F64_SIGN ((uint64_t)1ul << 63)

#define defaultNaNF16UI 0x7E00
#define defaultNaNF32UI 0x7FC00000
#define defaultNaNF64UI UINT64_C( 0x7FF8000000000000 )

#define ui8_fromPosOverflow  0xFF
#define ui8_fromNegOverflow  0
#define ui8_fromNaN          0xFF
#define i8_fromPosOverflow   0x7F
#define i8_fromNegOverflow   (-0x7F - 1)
#define i8_fromNaN           0x7F

#define ui16_fromPosOverflow 0xFFFF
#define ui16_fromNegOverflow 0
#define ui16_fromNaN         0xFFFF
#define i16_fromPosOverflow  0x7FFF
#define i16_fromNegOverflow  (-0x7FFF - 1)
#define i16_fromNaN          0x7FFF

#define fsgnj16(a, b, n, x) \
  (uint16_t) ((a.v & ~F16_SIGN) | ((((x) ? a.v : (n) ? F16_SIGN : 0) ^ b.v) & F16_SIGN))
#define fsgnj32(a, b, n, x) \
  (uint32_t) ((a.v & ~F32_SIGN) | ((((x) ? a.v : (n) ? F32_SIGN : 0) ^ b.v) & F32_SIGN))
#define fsgnj64(a, b, n, x) \
  (uint64_t) ((a.v & ~F64_SIGN) | ((((x) ? a.v : (n) ? F64_SIGN : 0) ^ b.v) & F64_SIGN))

static inline float16_t rtlToF16(rtlreg_t r);
static inline float32_t rtlToF32(rtlreg_t r);
static inline float32_t rtlToVF32(rtlreg_t r);
static inline float64_t rtlToF64(rtlreg_t r);

static inline float16_t f16_neg(float16_t a) {
  return (float16_t){.v = (uint16_t)(a.v ^ F16_SIGN)};
}

static inline float32_t f32_neg(float32_t a) {
  return (float32_t){.v = (uint32_t)(a.v ^ F32_SIGN)};
}

static inline float64_t f64_neg(float64_t a) {
  return (float64_t){.v = a.v ^ F64_SIGN};
}

static inline float16_t f16_neg_zero() {
  return (float16_t){.v = F16_SIGN};
}

static inline float32_t f32_neg_zero() {
  return (float32_t){.v = F32_SIGN};
}

static inline float64_t f64_neg_zero() {
  return (float64_t){.v = F64_SIGN};
}

static inline float16_t f16_min(float16_t a, float16_t b){
  bool less = f16_lt_quiet(a, b) || (f16_eq(a, b) && (a.v & F16_SIGN));
  if(isNaNF16UI(a.v) && isNaNF16UI(b.v)) return rtlToF16(defaultNaNF16UI);
  else return(less || isNaNF16UI(b.v) ? a : b);
}

static inline float16_t f16_max(float16_t a, float16_t b){
  bool greater = f16_lt_quiet(b, a) || (f16_eq(b, a) && (b.v & F16_SIGN));
  if(isNaNF16UI(a.v) && isNaNF16UI(b.v)) return rtlToF16(defaultNaNF16UI);
  else return(greater || isNaNF16UI(b.v) ? a : b);
}

static inline float32_t f32_min(float32_t a, float32_t b){
  bool less = f32_lt_quiet(a, b) || (f32_eq(a, b) && (a.v & F32_SIGN));
  if(isNaNF32UI(a.v) && isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return(less || isNaNF32UI(b.v) ? a : b);
}

static inline float16_t f16_minm(float16_t a, float16_t b) {
  bool less = f16_lt_quiet(a, b) || (f16_eq(a, b) && (a.v & F16_SIGN));
  if (isNaNF16UI(a.v) || isNaNF16UI(b.v)) return rtlToF16(defaultNaNF16UI);
  else return (less ? a : b);
}

static inline float32_t f32_minm(float32_t a, float32_t b) {
  bool less = f32_lt_quiet(a, b) || (f32_eq(a, b) && (a.v & F32_SIGN));
  if (isNaNF32UI(a.v) || isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return (less ? a : b);
}

static inline float32_t f32_max(float32_t a, float32_t b){
  bool greater = f32_lt_quiet(b, a) || (f32_eq(b, a) && (b.v & F32_SIGN));
  if(isNaNF32UI(a.v) && isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return(greater || isNaNF32UI(b.v) ? a : b);
}

static inline float16_t f16_maxm(float16_t a, float16_t b) {
  bool greater = f16_lt_quiet(b, a) || (f16_eq(b, a) && (b.v & F16_SIGN));
  if (isNaNF16UI(a.v) || isNaNF16UI(b.v)) return rtlToF16(defaultNaNF16UI);
  else return (greater ? a : b);
}

static inline float32_t f32_maxm(float32_t a, float32_t b) {
  bool greater = f32_lt_quiet(b, a) || (f32_eq(b, a) && (b.v & F32_SIGN));
  if (isNaNF32UI(a.v) || isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return (greater ? a : b);
}

static inline float64_t f64_min(float64_t a, float64_t b){
  bool less = f64_lt_quiet(a, b) || (f64_eq(a, b) && (a.v & F64_SIGN));
  if(isNaNF64UI(a.v) && isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return(less || isNaNF64UI(b.v) ? a : b);
}

static inline float64_t f64_minm(float64_t a, float64_t b) {
  bool less = f64_lt_quiet(a, b) || (f64_eq(a, b) && (a.v & F64_SIGN));
  if (isNaNF64UI(a.v) || isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return (less ? a : b);
}

static inline float64_t f64_max(float64_t a, float64_t b){
  bool greater = f64_lt_quiet(b, a) || (f64_eq(b, a) && (b.v & F64_SIGN));
  if(isNaNF64UI(a.v) && isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return(greater || isNaNF64UI(b.v) ? a : b);
}

static inline float64_t f64_maxm(float64_t a, float64_t b) {
  bool greater = f64_lt_quiet(b, a) || (f64_eq(b, a) && (b.v & F64_SIGN));
  if (isNaNF64UI(a.v) || isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return (greater ? a : b);
}

static inline float16_t f16_fli(uint16_t a) {
  const uint64_t bits[32] = {
    [0b00000] = 0xFFFFFFFFFFFFBC00, /* -1.0 */
    [0b00001] = 0xFFFFFFFFFFFF0400, /* minimum positive normal */
    [0b00010] = 0xFFFFFFFFFFFF0100, /* 1.0 * 2^-16 */
    [0b00011] = 0xFFFFFFFFFFFF0200, /* 1.0 * 2^-15 */
    [0b00100] = 0xFFFFFFFFFFFF1C00, /* 1.0 * 2^-8 */
    [0b00101] = 0xFFFFFFFFFFFF2000, /* 1.0 * 2^-7 */
    [0b00110] = 0xFFFFFFFFFFFF2C00, /* 1.0 * 2^-4 */
    [0b00111] = 0xFFFFFFFFFFFF3000, /* 1.0 * 2^-3 */
    [0b01000] = 0xFFFFFFFFFFFF3400, /* 0.25 */
    [0b01001] = 0xFFFFFFFFFFFF3500, /* 0.3125 */
    [0b01010] = 0xFFFFFFFFFFFF3600, /* 0.375 */
    [0b01011] = 0xFFFFFFFFFFFF3700, /* 0.4375 */
    [0b01100] = 0xFFFFFFFFFFFF3800, /* 0.5 */
    [0b01101] = 0xFFFFFFFFFFFF3900, /* 0.625 */
    [0b01110] = 0xFFFFFFFFFFFF3A00, /* 0.75 */
    [0b01111] = 0xFFFFFFFFFFFF3B00, /* 0.875 */
    [0b10000] = 0xFFFFFFFFFFFF3C00, /* 1.0 */
    [0b10001] = 0xFFFFFFFFFFFF3D00, /* 1.25 */
    [0b10010] = 0xFFFFFFFFFFFF3E00, /* 1.5 */
    [0b10011] = 0xFFFFFFFFFFFF3F00, /* 1.75 */
    [0b10100] = 0xFFFFFFFFFFFF4000, /* 2.0 */
    [0b10101] = 0xFFFFFFFFFFFF4100, /* 2.5 */
    [0b10110] = 0xFFFFFFFFFFFF4200, /* 3 */
    [0b10111] = 0xFFFFFFFFFFFF4400, /* 4 */
    [0b11000] = 0xFFFFFFFFFFFF4800, /* 8 */
    [0b11001] = 0xFFFFFFFFFFFF4C00, /* 16 */
    [0b11010] = 0xFFFFFFFFFFFF5800, /* 2^7 */
    [0b11011] = 0xFFFFFFFFFFFF5C00, /* 2^8 */
    [0b11100] = 0xFFFFFFFFFFFF7800, /* 2^15 */
    [0b11101] = 0xFFFFFFFFFFFF7C00, /* +inf (2^16 is not expressible) */
    [0b11110] = 0xFFFFFFFFFFFF7C00, /* +inf */
    [0b11111] = 0xFFFFFFFFFFFF7E00
  };
  return rtlToF16(bits[a]);
}

static inline float32_t f32_fli(uint32_t a) {
  const uint64_t bits[32] = {
    [0b00000] = 0xFFFFFFFFbf800000,  /* -1.0 */
    [0b00001] = 0xFFFFFFFF00800000,  /* minimum positive normal */
    [0b00010] = 0xFFFFFFFF37800000,  /* 1.0 * 2^-16 */
    [0b00011] = 0xFFFFFFFF38000000,  /* 1.0 * 2^-15 */
    [0b00100] = 0xFFFFFFFF3b800000,  /* 1.0 * 2^-8  */
    [0b00101] = 0xFFFFFFFF3c000000,  /* 1.0 * 2^-7  */
    [0b00110] = 0xFFFFFFFF3d800000,  /* 1.0 * 2^-4  */
    [0b00111] = 0xFFFFFFFF3e000000,  /* 1.0 * 2^-3  */
    [0b01000] = 0xFFFFFFFF3e800000,  /* 0.25 */
    [0b01001] = 0xFFFFFFFF3ea00000,  /* 0.3125 */
    [0b01010] = 0xFFFFFFFF3ec00000,  /* 0.375 */
    [0b01011] = 0xFFFFFFFF3ee00000,  /* 0.4375 */
    [0b01100] = 0xFFFFFFFF3f000000,  /* 0.5 */
    [0b01101] = 0xFFFFFFFF3f200000,  /* 0.625 */
    [0b01110] = 0xFFFFFFFF3f400000,  /* 0.75 */
    [0b01111] = 0xFFFFFFFF3f600000,  /* 0.875 */
    [0b10000] = 0xFFFFFFFF3f800000,  /* 1.0 */
    [0b10001] = 0xFFFFFFFF3fa00000,  /* 1.25 */
    [0b10010] = 0xFFFFFFFF3fc00000,  /* 1.5 */
    [0b10011] = 0xFFFFFFFF3fe00000,  /* 1.75 */
    [0b10100] = 0xFFFFFFFF40000000,  /* 2.0 */
    [0b10101] = 0xFFFFFFFF40200000,  /* 2.5 */
    [0b10110] = 0xFFFFFFFF40400000,  /* 3 */
    [0b10111] = 0xFFFFFFFF40800000,  /* 4 */
    [0b11000] = 0xFFFFFFFF41000000,  /* 8 */
    [0b11001] = 0xFFFFFFFF41800000,  /* 16 */
    [0b11010] = 0xFFFFFFFF43000000,  /* 2^7 */
    [0b11011] = 0xFFFFFFFF43800000,  /* 2^8 */
    [0b11100] = 0xFFFFFFFF47000000,  /* 2^15 */
    [0b11101] = 0xFFFFFFFF47800000,  /* 2^16 */
    [0b11110] = 0xFFFFFFFF7f800000,  /* +inf */
    [0b11111] = 0xFFFFFFFF7fc00000
  };

  return rtlToF32(bits[a]);
}

static inline float64_t f64_fli(uint64_t a) {
  const uint64_t bits[32] = {
    [0b00000] = 0xbff0000000000000ull,  /* -1.0 */
    [0b00001] = 0x0010000000000000ull,  /* minimum positive normal */
    [0b00010] = 0x3ef0000000000000ull,  /* 1.0 * 2^-16 */
    [0b00011] = 0x3f00000000000000ull,  /* 1.0 * 2^-15 */
    [0b00100] = 0x3f70000000000000ull,  /* 1.0 * 2^-8  */
    [0b00101] = 0x3f80000000000000ull,  /* 1.0 * 2^-7  */
    [0b00110] = 0x3fb0000000000000ull,  /* 1.0 * 2^-4  */
    [0b00111] = 0x3fc0000000000000ull,  /* 1.0 * 2^-3  */
    [0b01000] = 0x3fd0000000000000ull,  /* 0.25 */
    [0b01001] = 0x3fd4000000000000ull,  /* 0.3125 */
    [0b01010] = 0x3fd8000000000000ull,  /* 0.375 */
    [0b01011] = 0x3fdc000000000000ull,  /* 0.4375 */
    [0b01100] = 0x3fe0000000000000ull,  /* 0.5 */
    [0b01101] = 0x3fe4000000000000ull,  /* 0.625 */
    [0b01110] = 0x3fe8000000000000ull,  /* 0.75 */
    [0b01111] = 0x3fec000000000000ull,  /* 0.875 */
    [0b10000] = 0x3ff0000000000000ull,  /* 1.0 */
    [0b10001] = 0x3ff4000000000000ull,  /* 1.25 */
    [0b10010] = 0x3ff8000000000000ull,  /* 1.5 */
    [0b10011] = 0x3ffc000000000000ull,  /* 1.75 */
    [0b10100] = 0x4000000000000000ull,  /* 2.0 */
    [0b10101] = 0x4004000000000000ull,  /* 2.5 */
    [0b10110] = 0x4008000000000000ull,  /* 3 */
    [0b10111] = 0x4010000000000000ull,  /* 4 */
    [0b11000] = 0x4020000000000000ull,  /* 8 */
    [0b11001] = 0x4030000000000000ull,  /* 16 */
    [0b11010] = 0x4060000000000000ull,  /* 2^7 */
    [0b11011] = 0x4070000000000000ull,  /* 2^8 */
    [0b11100] = 0x40e0000000000000ull,  /* 2^15 */
    [0b11101] = 0x40f0000000000000ull,  /* 2^16 */
    [0b11110] = 0x7ff0000000000000ull,  /* +inf */
    [0b11111] = defaultNaNF64UI
  };
  return rtlToF64(bits[a]);
}

static inline int64_t f64_fcvtmod(float64_t a) {
  union ui64_f64 uA;
  uint_fast64_t uiA;
  uA.f = a;
  uiA = uA.ui;

  uint32_t sign = signF64UI(uiA);
  uint32_t exp  = expF64UI(uiA);
  uint64_t frac = fracF64UI(uiA);

  bool inexact = false;
  bool invalid = false;

  if (exp == 0) {
    inexact = (frac != 0);
    frac = 0;
  } else if (exp == 0x7ff) {
    /* inf of NaN */
    invalid = true;
    frac = 0;
  } else {
    int true_exp = exp - 1023;
    int shift = true_exp - 52;

    /* Restore inplicit bit */
    frac |= 1ull << 52;

    /* Shift the fraction into place. */
    if (shift >= 64) {
      /* The fraction is shifted out entirelu. */
      frac = 0;
    } else if ((shift >= 0) && (shift < 64)) {
      /* The number is so large we must shift the fraction left. */
      frac <<= shift;
    } else if ((shift > -64) && (shift < 0)) {
      /* Normal case -- shift right and notice if bits shift out. */
      inexact = (frac << (64 + shift)) != 0;
      frac >>= -shift;
    } else {
      /* The fraction is shifted out entirely. */
      frac = 0;
      inexact = true;
    }

    /* Handle overflows */
    if  (true_exp > 31 || frac > (sign ? 0x80000000ull : 0x7fffffff)) {
      /* Overflow, for which this operation raises invalid. */
      invalid = true;
      inexact = false; /* invalid takes precedence */
    }

    /* Honor the sign. */
    if (sign) {
      frac = -frac;
    }
  }
  
  if (inexact) {
    softfloat_exceptionFlags |= softfloat_flag_inexact;
  }
  
  if (invalid) {
    softfloat_exceptionFlags |= softfloat_flag_invalid;
  }

  return SEXT(frac, 32);
}

static inline float16_t  my_f32_to_f16 (float32_t a) {
  return f32_to_f16(a);
}
static inline float32_t  my_f16_to_f32 (float16_t a) {
  return f16_to_f32(a);
}
static inline float64_t  my_f16_to_f64 (float16_t a) {
  return f16_to_f64(a);
}
static inline float16_t  my_f64_to_f16 (float64_t a) {
  return f64_to_f16(a);
}

static inline float16_t  my_i32_to_f16 (int32_t a) {
  return i32_to_f16(a);
}
static inline float16_t  my_ui32_to_f16 (uint32_t a) {
  return ui32_to_f16(a);
}
static inline float16_t  my_i64_to_f16 (int64_t a) {
  return i64_to_f16(a);
}
static inline float16_t  my_ui64_to_f16 (uint64_t a) {
  return ui64_to_f16(a);
}

static inline int32_t  my_f16_to_i32 (float16_t a) {
  return f16_to_i32 (a, softfloat_roundingMode, true);
}
static inline uint32_t my_f16_to_ui32(float16_t a) {
  return f16_to_ui32(a, softfloat_roundingMode, true);
}
static inline int64_t  my_f16_to_i64 (float16_t a) {
  return f16_to_i64 (a, softfloat_roundingMode, true);
}
static inline uint64_t my_f16_to_ui64(float16_t a) {
  return f16_to_ui64(a, softfloat_roundingMode, true);
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

uint_fast16_t f16_classify( float16_t a )
{
    union ui16_f16 uA;
    uint_fast16_t uiA;

    uA.f = a;
    uiA = uA.ui;

    uint_fast16_t infOrNaN = expF16UI( uiA ) == 0x1F;
    uint_fast16_t subnormalOrZero = expF16UI( uiA ) == 0;
    bool sign = signF16UI( uiA );
    bool fracZero = fracF16UI( uiA ) == 0;
    bool isNaN = isNaNF16UI( uiA );
    bool isSNaN = softfloat_isSigNaNF16UI( uiA );

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

static inline uint64_t extract64(uint64_t val, int pos, int len)
{
  assert(pos >= 0 && len > 0 && len <= 64 - pos);
  return (val >> pos) & (~UINT64_C(0) >> (64 - len));
}

static inline uint64_t make_mask64(int pos, int len)
{
    assert(pos >= 0 && len > 0 && pos < 64 && len <= 64);
    return (UINT64_MAX >> (64 - len)) << pos;
}

//user needs to truncate output to required length
static inline uint64_t rsqrte7(uint64_t val, int e, int s, bool sub) {
  uint64_t exp = extract64(val, s, e);
  uint64_t sig = extract64(val, 0, s);
  uint64_t sign = extract64(val, s + e, 1);
  const int p = 7;

  static const uint8_t table[] = {
      52, 51, 50, 48, 47, 46, 44, 43,
      42, 41, 40, 39, 38, 36, 35, 34,
      33, 32, 31, 30, 30, 29, 28, 27,
      26, 25, 24, 23, 23, 22, 21, 20,
      19, 19, 18, 17, 16, 16, 15, 14,
      14, 13, 12, 12, 11, 10, 10, 9,
      9, 8, 7, 7, 6, 6, 5, 4,
      4, 3, 3, 2, 2, 1, 1, 0,
      127, 125, 123, 121, 119, 118, 116, 114,
      113, 111, 109, 108, 106, 105, 103, 102,
      100, 99, 97, 96, 95, 93, 92, 91,
      90, 88, 87, 86, 85, 84, 83, 82,
      80, 79, 78, 77, 76, 75, 74, 73,
      72, 71, 70, 70, 69, 68, 67, 66,
      65, 64, 63, 63, 62, 61, 60, 59,
      59, 58, 57, 56, 56, 55, 54, 53};

  if (sub) {
      while (extract64(sig, s - 1, 1) == 0)
          exp--, sig <<= 1;

      sig = (sig << 1) & make_mask64(0 ,s);
  }

  int idx = ((exp & 1) << (p-1)) | (sig >> (s-p+1));
  uint64_t out_sig = (uint64_t)(table[idx]) << (s-p);
  uint64_t out_exp = (3 * make_mask64(0, e - 1) + ~exp) / 2;

  return (sign << (s+e)) | (out_exp << s) | out_sig;
}

static inline float16_t f16_rsqrte7(float16_t in)
{
    union ui16_f16 uA;

    uA.f = in;
    unsigned int ret = f16_classify(in);
    bool sub = false;
    switch(ret) {
    case 0x001: // -inf
    case 0x002: // -normal
    case 0x004: // -subnormal
    case 0x100: // sNaN
        softfloat_exceptionFlags |= softfloat_flag_invalid;
    case 0x200: //qNaN
        uA.ui = defaultNaNF16UI;
        break;
    case 0x008: // -0
        uA.ui = 0xfc00;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x010: // +0
        uA.ui = 0x7c00;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x080: //+inf
        uA.ui = 0x0;
        break;
    case 0x020: //+ sub
        sub = true;
    default: // +num
        uA.ui = rsqrte7(uA.ui, 5, 10, sub);
        break;
    }

    return uA.f;
}

static inline float32_t f32_rsqrte7(float32_t in)
{
    union ui32_f32 uA;

    uA.f = in;
    unsigned int ret = f32_classify(in);
    bool sub = false;
    switch(ret) {
    case 0x001: // -inf
    case 0x002: // -normal
    case 0x004: // -subnormal
    case 0x100: // sNaN
        softfloat_exceptionFlags |= softfloat_flag_invalid;
    case 0x200: //qNaN
        uA.ui = defaultNaNF32UI;
        break;
    case 0x008: // -0
        uA.ui = 0xff800000;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x010: // +0
        uA.ui = 0x7f800000;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x080: //+inf
        uA.ui = 0x0;
        break;
    case 0x020: //+ sub
        sub = true;
    default: // +num
        uA.ui = rsqrte7(uA.ui, 8, 23, sub);
        break;
    }

    return uA.f;
}

static inline float64_t f64_rsqrte7(float64_t in)
{
    union ui64_f64 uA;

    uA.f = in;
    unsigned int ret = f64_classify(in);
    bool sub = false;
    switch(ret) {
    case 0x001: // -inf
    case 0x002: // -normal
    case 0x004: // -subnormal
    case 0x100: // sNaN
        softfloat_exceptionFlags |= softfloat_flag_invalid;
    case 0x200: //qNaN
        uA.ui = defaultNaNF64UI;
        break;
    case 0x008: // -0
        uA.ui = 0xfff0000000000000ul;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x010: // +0
        uA.ui = 0x7ff0000000000000ul;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x080: //+inf
        uA.ui = 0x0;
        break;
    case 0x020: //+ sub
        sub = true;
    default: // +num
        uA.ui = rsqrte7(uA.ui, 11, 52, sub);
        break;
    }

    return uA.f;
}

//user needs to truncate output to required length
static inline uint64_t recip7(uint64_t val, int e, int s, int rm, bool sub,
                              bool *round_abnormal)
{
    uint64_t exp = extract64(val, s, e);
    uint64_t sig = extract64(val, 0, s);
    uint64_t sign = extract64(val, s + e, 1);
    const int p = 7;

    static const uint8_t table[] = {
        127, 125, 123, 121, 119, 117, 116, 114,
        112, 110, 109, 107, 105, 104, 102, 100,
        99, 97, 96, 94, 93, 91, 90, 88,
        87, 85, 84, 83, 81, 80, 79, 77,
        76, 75, 74, 72, 71, 70, 69, 68,
        66, 65, 64, 63, 62, 61, 60, 59,
        58, 57, 56, 55, 54, 53, 52, 51,
        50, 49, 48, 47, 46, 45, 44, 43,
        42, 41, 40, 40, 39, 38, 37, 36,
        35, 35, 34, 33, 32, 31, 31, 30,
        29, 28, 28, 27, 26, 25, 25, 24,
        23, 23, 22, 21, 21, 20, 19, 19,
        18, 17, 17, 16, 15, 15, 14, 14,
        13, 12, 12, 11, 11, 10, 9, 9,
        8, 8, 7, 7, 6, 5, 5, 4,
        4, 3, 3, 2, 2, 1, 1, 0};

    if (sub) {
        while (extract64(sig, s - 1, 1) == 0)
            exp--, sig <<= 1;

        sig = (sig << 1) & make_mask64(0 ,s);

        if (exp != 0 && exp != UINT64_MAX) {
            *round_abnormal = true;
            if (rm == 1 ||
                (rm == 2 && !sign) ||
                (rm == 3 && sign))
                return ((sign << (s+e)) | make_mask64(s, e)) - 1;
            else
                return (sign << (s+e)) | make_mask64(s, e);
        }
    }

    int idx = sig >> (s-p);
    uint64_t out_sig = (uint64_t)(table[idx]) << (s-p);
    uint64_t out_exp = 2 * make_mask64(0, e - 1) + ~exp;
    if (out_exp == 0 || out_exp == UINT64_MAX) {
        out_sig = (out_sig >> 1) | make_mask64(s - 1, 1);
        if (out_exp == UINT64_MAX) {
            out_sig >>= 1;
            out_exp = 0;
        }
    }

    return (sign << (s+e)) | (out_exp << s) | out_sig;
}

static inline float16_t f16_recip7(float16_t in)
{
    union ui16_f16 uA;

    uA.f = in;
    unsigned int ret = f16_classify(in);
    bool sub = false;
    bool round_abnormal = false;
    switch(ret) {
    case 0x001: // -inf
        uA.ui = 0x8000;
        break;
    case 0x080: //+inf
        uA.ui = 0x0;
        break;
    case 0x008: // -0
        uA.ui = 0xfc00;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x010: // +0
        uA.ui = 0x7c00;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x100: // sNaN
        softfloat_exceptionFlags |= softfloat_flag_invalid;
    case 0x200: //qNaN
        uA.ui = defaultNaNF16UI;
        break;
    case 0x004: // -subnormal
    case 0x020: //+ sub
        sub = true;
    default: // +- normal
        uA.ui = recip7(uA.ui, 5, 10,
                       softfloat_roundingMode, sub, &round_abnormal);
        if (round_abnormal)
            softfloat_exceptionFlags |= softfloat_flag_inexact |
                                        softfloat_flag_overflow;
        break;
    }

    return uA.f;
}

static inline float32_t f32_recip7(float32_t in)
{
    union ui32_f32 uA;

    uA.f = in;
    unsigned int ret = f32_classify(in);
    bool sub = false;
    bool round_abnormal = false;
    switch(ret) {
    case 0x001: // -inf
        uA.ui = 0x80000000;
        break;
    case 0x080: //+inf
        uA.ui = 0x0;
        break;
    case 0x008: // -0
        uA.ui = 0xff800000;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x010: // +0
        uA.ui = 0x7f800000;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x100: // sNaN
        softfloat_exceptionFlags |= softfloat_flag_invalid;
    case 0x200: //qNaN
        uA.ui = defaultNaNF32UI;
        break;
    case 0x004: // -subnormal
    case 0x020: //+ sub
        sub = true;
    default: // +- normal
        uA.ui = recip7(uA.ui, 8, 23,
                       softfloat_roundingMode, sub, &round_abnormal);
        if (round_abnormal)
          softfloat_exceptionFlags |= softfloat_flag_inexact |
                                      softfloat_flag_overflow;
        break;
    }

    return uA.f;
}

static inline float64_t f64_recip7(float64_t in)
{
    union ui64_f64 uA;

    uA.f = in;
    unsigned int ret = f64_classify(in);
    bool sub = false;
    bool round_abnormal = false;
    switch(ret) {
    case 0x001: // -inf
        uA.ui = 0x8000000000000000;
        break;
    case 0x080: //+inf
        uA.ui = 0x0;
        break;
    case 0x008: // -0
        uA.ui = 0xfff0000000000000;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x010: // +0
        uA.ui = 0x7ff0000000000000;
        softfloat_exceptionFlags |= softfloat_flag_infinite;
        break;
    case 0x100: // sNaN
        softfloat_exceptionFlags |= softfloat_flag_invalid;
    case 0x200: //qNaN
        uA.ui = defaultNaNF64UI;
        break;
    case 0x004: // -subnormal
    case 0x020: //+ sub
        sub = true;
    default: // +- normal
        uA.ui = recip7(uA.ui, 11, 52,
                       softfloat_roundingMode, sub, &round_abnormal);
        if (round_abnormal)
            softfloat_exceptionFlags |= softfloat_flag_inexact |
                                        softfloat_flag_overflow;
        break;
    }

    return uA.f;
}

static inline void fp_set_rm(int rm) {
  switch (rm) {
    case FPCALL_RM_RNE: softfloat_roundingMode = softfloat_round_near_even; break;
    case FPCALL_RM_RTZ: softfloat_roundingMode = softfloat_round_minMag; break;
    case FPCALL_RM_RDN: softfloat_roundingMode = softfloat_round_min; break;
    case FPCALL_RM_RUP: softfloat_roundingMode = softfloat_round_max; break;
    case FPCALL_RM_RMM: softfloat_roundingMode = softfloat_round_near_maxMag; break;
    default: softfloat_roundingMode = rm;
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

uint_fast16_t f16_to_ui16( float16_t a, uint_fast8_t roundingMode, bool exact )
{
    uint_fast8_t old_flags = softfloat_exceptionFlags;

    uint_fast32_t sig32 = f16_to_ui32(a, roundingMode, exact);

    if (sig32 > UINT16_MAX) {
        softfloat_exceptionFlags = old_flags | softfloat_flag_invalid;
        return ui16_fromPosOverflow;
    } else {
        return sig32;
    }
}

static inline int_fast16_t f16_to_i16( float16_t a, uint_fast8_t roundingMode, bool exact )
{
    uint_fast8_t old_flags = softfloat_exceptionFlags;

    int_fast32_t sig32 = f16_to_i32(a, roundingMode, exact);

    if (sig32 > INT16_MAX) {
        softfloat_exceptionFlags = old_flags | softfloat_flag_invalid;
        return i16_fromPosOverflow;
    } else if (sig32 < INT16_MIN) {
        softfloat_exceptionFlags = old_flags | softfloat_flag_invalid;
        return i16_fromNegOverflow;
    } else {
        return sig32;
    }
}

static inline uint_fast8_t f16_to_ui8( float16_t a, uint_fast8_t roundingMode, bool exact )
{
    uint_fast8_t old_flags = softfloat_exceptionFlags;

    uint_fast32_t sig32 = f16_to_ui32(a, roundingMode, exact);

    if (sig32 > UINT8_MAX) {
        softfloat_exceptionFlags = old_flags | softfloat_flag_invalid;
        return ui8_fromPosOverflow;
    } else {
        return sig32;
    }
}

static inline int_fast8_t f16_to_i8( float16_t a, uint_fast8_t roundingMode, bool exact )
{
    uint_fast8_t old_flags = softfloat_exceptionFlags;

    int_fast32_t sig32 = f16_to_i32(a, roundingMode, exact);

    if (sig32 > INT8_MAX) {
        softfloat_exceptionFlags = old_flags | softfloat_flag_invalid;
        return i8_fromPosOverflow;
    } else if (sig32 < INT8_MIN) {
        softfloat_exceptionFlags = old_flags | softfloat_flag_invalid;
        return i8_fromNegOverflow;
    } else {
        return sig32;
    }
}

static inline uint_fast16_t f32_to_ui16( float32_t a, uint_fast8_t roundingMode, bool exact )
{
    uint_fast8_t old_flags = softfloat_exceptionFlags;

    uint_fast32_t sig32 = f32_to_ui32(a, roundingMode, exact);

    if (sig32 > UINT16_MAX) {
        softfloat_exceptionFlags = old_flags | softfloat_flag_invalid;
        return ui16_fromPosOverflow;
    } else {
        return sig32;
    }
}

static inline int_fast16_t f32_to_i16( float32_t a, uint_fast8_t roundingMode, bool exact )
{
    uint_fast8_t old_flags = softfloat_exceptionFlags;

    int_fast32_t sig32 = f32_to_i32(a, roundingMode, exact);

    if (sig32 > INT16_MAX) {
        softfloat_exceptionFlags = old_flags | softfloat_flag_invalid;
        return i16_fromPosOverflow;
    } else if (sig32 < INT16_MIN) {
        softfloat_exceptionFlags = old_flags | softfloat_flag_invalid;
        return i16_fromNegOverflow;
    } else {
        return sig32;
    }
}

static inline void fp_clear_exception() {
  softfloat_exceptionFlags = 0;
}
#endif
