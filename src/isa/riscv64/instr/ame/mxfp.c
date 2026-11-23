#include <common.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mxfp.h"

#ifdef CONFIG_FPU_SOFT
#include <softfloat.h>
#endif

static float mxfp_as_float(uint32_t bits) {
  float value;
  memcpy(&value, &bits, sizeof(value));
  return value;
}

// Every unscaled finite MX element is exactly representable as binary32.
// Decode through integer fields so host rounding/FTZ never enters the golden.
static uint32_t mxfp_decode_bits(uint8_t bits, int is_fp4, int is_e5m2) {
  const unsigned mant_bits = is_fp4 ? 1 : (is_e5m2 ? 2 : 3);
  const unsigned exp_mask = is_fp4 ? 3 : (is_e5m2 ? 31 : 15);
  const unsigned exp = (bits >> mant_bits) & exp_mask;
  unsigned sig = bits & ((1u << mant_bits) - 1);
  const uint32_t sign = (uint32_t)(bits >> (is_fp4 ? 3 : 7) & 1) << 31;
  const int bias = is_fp4 ? 1 : (is_e5m2 ? 15 : 7);
  if (!is_fp4 && exp == exp_mask) {
    // E4M3 has finite 0x78..0x7e and a quiet NaN at 0x7f.
    if (!is_e5m2 && sig == 7) return sign | 0x7fc00000u;
    // E5M2 retains the IEEE quiet/signaling NaN bit for exception handling.
    if (is_e5m2) return sign | 0x7f800000u | (sig << 21);
  }
  int power = (exp ? (int)exp : 1) - bias - (int)mant_bits;
  if (exp) sig |= 1u << mant_bits;
  if (!sig) return sign;
  while (sig < 0x800000u) {
    sig <<= 1;
    power--;
  }
  return sign | (uint32_t)(power + 23 + 127) << 23 | (sig & 0x7fffffu);
}

float mxfp_e8m0_to_float(uint8_t scale) {
  return mxfp_as_float(scale == 255 ? 0x7fc00000u :
    (scale ? (uint32_t)scale << 23 : 0x00400000u));
}

float mxfp_decode_e2m1(uint8_t bits) {
  return mxfp_as_float(mxfp_decode_bits(bits, 1, 0));
}

float mxfp_decode_e4m3(uint8_t bits) {
  return mxfp_as_float(mxfp_decode_bits(bits, 0, 0));
}

float mxfp_decode_e5m2(uint8_t bits) {
  return mxfp_as_float(mxfp_decode_bits(bits, 0, 1));
}

float mxfp_decode_element(uint8_t bits, uint8_t scale, int is_fp4, int is_e5m2) {
  const float value = is_fp4 ? mxfp_decode_e2m1(bits) :
    (is_e5m2 ? mxfp_decode_e5m2(bits) : mxfp_decode_e4m3(bits));
  return value * mxfp_e8m0_to_float(scale);
}

bool mxfp_encode_supplied_block(const uint32_t values[32], uint8_t e,
    int is_fp4, int is_e5m2, uint8_t *payload) {
#ifdef CONFIG_FPU_SOFT
  assert(values && payload);
  const unsigned bytes = is_fp4 ? 16 : 32;
  if (e > (is_fp4 ? 251 : 254)) {
    memset(payload, 0, bytes);
    return false;
  }
  // Classify raw bits before widening, so even an sNaN cannot affect the
  // caller's FP state. All rejects clear the complete corresponding payload.
  for (unsigned k = 0; k < 32; ++k) {
    if ((values[k] & 0x7f800000u) == 0x7f800000u) {
      memset(payload, 0, bytes);
      return false;
    }
  }
  const uint_fast8_t saved_rm = softfloat_roundingMode;
  const uint_fast8_t saved_flags = softfloat_exceptionFlags;
  const uint_fast8_t saved_tininess = softfloat_detectTininess;
  softfloat_roundingMode = softfloat_round_near_even;
  softfloat_exceptionFlags = 0;
  softfloat_detectTininess = softfloat_tininess_afterRounding;

  // Reuse the canonical (undoubled) E2M1/FP8 decoder with full E8M0 scale.
  // E2M1*2^(e-127) equals the CPU's doubled codes*2^(e-128).
  float128_t scale, half, previous, envelope, midpoints[126];
  const int power = (int)e - 127;
  f64_to_f128M((float64_t){.v = (uint64_t)(1023 + power) << 52}, &scale);
  f64_to_f128M((float64_t){.v = UINT64_C(0x3fe0000000000000)}, &half);
  f32_to_f128M((float32_t){.v = 0}, &previous);
  const unsigned last = is_fp4 ? 7 : (is_e5m2 ? 0x7b : 0x7e);
  for (unsigned code = 1; code <= last; ++code) {
    float128_t decoded, level, sum;
    f32_to_f128M((float32_t){.v = mxfp_decode_bits(code, is_fp4, is_e5m2)}, &decoded);
    f128M_mul(&decoded, &scale, &level);
    f128M_add(&previous, &level, &sum);
    f128M_mul(&sum, &half, &midpoints[code - 1]);
    previous = level;
  }
  // Adjacent levels and midpoints are exact short dyadics in binary128,
  // including e0 F32/FP8 subnormals. Never scale/round a lane through FP32.
  // FP4's envelope is 16*half-scale, not its largest code (12*half-scale).
  envelope = previous;
  if (is_fp4) {
    float128_t eight;
    f32_to_f128M((float32_t){.v = 0x41000000u}, &eight);
    f128M_mul(&eight, &scale, &envelope);
  }
  bool accepted = true;
  uint8_t encoded[32] = {0};
  for (unsigned k = 0; k < 32; ++k) {
    float128_t magnitude;
    f32_to_f128M((float32_t){.v = values[k] & 0x7fffffffu}, &magnitude);
    if (f128M_lt(&envelope, &magnitude)) {
      accepted = false;
      break;
    }
    unsigned lo = 0, hi = last;
    while (lo < hi) {
      const unsigned mid = (lo + hi) / 2;
      const bool upper = f128M_lt(&midpoints[mid], &magnitude) ||
        (!is_fp4 && (mid & 1) && f128M_eq(&midpoints[mid], &magnitude));
      if (upper) lo = mid + 1;
      else hi = mid;
    }
    const unsigned sign = values[k] >> 31;
    if (is_fp4) {
      const uint8_t code = lo ? lo | (sign << 3) : 0;
      encoded[k / 2] |= code << (4 * (k & 1));
    } else {
      encoded[k] = lo | (sign << 7); // Preserve -0, including rounded -tiny.
    }
  }
  softfloat_roundingMode = saved_rm;
  softfloat_exceptionFlags = saved_flags;
  softfloat_detectTininess = saved_tininess;
  if (accepted) memcpy(payload, encoded, bytes);
  else memset(payload, 0, bytes);
  return accepted;
#else
  panic("exact supplied-scale MX encoding requires CONFIG_FPU_SOFT");
  abort();
#endif
}

bool mxfp_dynamic_scale_e(const uint32_t values[32], uint8_t *e) {
  assert(values && e);
  uint32_t maximum = 0;
  bool special = false;
  for (unsigned lane = 0; lane < 32; ++lane) {
    const uint32_t magnitude = values[lane] & 0x7fffffffu;
    special |= (magnitude & 0x7f800000u) == 0x7f800000u;
    if (maximum < magnitude) maximum = magnitude;
  }
  if (special) {
    *e = 0;
    return false;
  }
  if (maximum == 0) {
    *e = 0;
    return true;
  }
  const unsigned exponent = (maximum >> 23) & 0xff;
  const unsigned fraction = maximum & 0x7fffff;
  if (exponent < 2 || exponent > 253 ||
      (fraction != 0 && (fraction < 0x400 || fraction > 0x7ffc00))) {
    *e = 0;
    return false;
  }
  *e = (uint8_t)(exponent - 2);
  return true;
}

uint32_t mxfp_encode_f32_lane(uint32_t value, uint32_t e, bool *invalid) {
  *invalid = true;
  if (e > 251) return 0;
  // Reuse the exact supplied-scale block encoder; padding lanes are exact
  // zeros and never contribute exceptions. Its SoftFloat state is preserved.
  const uint32_t values[32] = {value};
  uint8_t payload[16];
  *invalid = !mxfp_encode_supplied_block(values, (uint8_t)e, 1, 0, payload);
  return *invalid ? 0 : payload[0] & 15;
}

uint32_t mxfp_accumulate_block(uint32_t c, const uint8_t *a, const uint8_t *b,
    unsigned lanes, uint8_t scale_a, uint8_t scale_b, int is_fp4, int is_e5m2,
    uint8_t *flags) {
#ifdef CONFIG_FPU_SOFT
  assert(lanes > 0 && lanes <= 32);
  const uint_fast8_t saved_rm = softfloat_roundingMode;
  const uint_fast8_t saved_flags = softfloat_exceptionFlags;
  const uint_fast8_t saved_tininess = softfloat_detectTininess;
  softfloat_roundingMode = softfloat_round_near_even;
  softfloat_exceptionFlags = 0;
  softfloat_detectTininess = softfloat_tininess_afterRounding;

  float128_t dot;
  if (scale_a == 255 || scale_b == 255) {
    // An E8M0 NaN scale disables the whole dot: element encodings must not
    // be evaluated, even for sNaN or Inf*0. Still add the qNaN contribution
    // to C below, so an external signaling C raises NV normally.
    f32_to_f128M((float32_t){.v = 0x7fc00000u}, &dot);
  } else {
    // E5M2 is the widest case: products lie on a 2^-32 lattice, each
    // magnitude is <2^32, and <=32 terms need at most 69 significant bits.
    // Binary128's 113 bits therefore retain every finite product and every
    // partial sum exactly. The common scale changes only the exponent, whose
    // full MX range also fits binary128. No per-lane FP32 rounding occurs.
    for (unsigned k = 0; k < lanes; k++) {
      float128_t av, bv, product;
      f32_to_f128M((float32_t){.v = mxfp_decode_bits(a[k], is_fp4, is_e5m2)}, &av);
      f32_to_f128M((float32_t){.v = mxfp_decode_bits(b[k], is_fp4, is_e5m2)}, &bv);
      f128M_mul(&av, &bv, &product);
      // No synthetic +0: an all-negative-zero block must remain -0.
      if (k == 0) dot = product;
      else f128M_add(&dot, &product, &dot);
    }
    float128_t scale;
    const int exponent = (int)scale_a + (int)scale_b - 254;
    f64_to_f128M((float64_t){.v = (uint64_t)(1023 + exponent) << 52}, &scale);
    f128M_mul(&dot, &scale, &dot);
  }

  float128_t acc, sum;
  f32_to_f128M((float32_t){.v = c}, &acc);
  // C can be arbitrarily far below an exact block midpoint. Round-to-odd
  // preserves that sticky information; intermediate RNE would double-round.
  softfloat_roundingMode = softfloat_round_odd;
  f128M_add(&acc, &dot, &sum);
  softfloat_roundingMode = softfloat_round_near_even;
  const uint32_t result = f128M_to_f32(&sum).v;
  _Static_assert(softfloat_flag_inexact == 1 && softfloat_flag_underflow == 2 &&
    softfloat_flag_overflow == 4 && softfloat_flag_infinite == 8 &&
    softfloat_flag_invalid == 16, "matrix IEEE flag layout changed");
  *flags |= softfloat_exceptionFlags;
  softfloat_roundingMode = saved_rm;
  softfloat_exceptionFlags = saved_flags;
  softfloat_detectTininess = saved_tininess;
  return result;
#else
  panic("exact MX block accumulation requires CONFIG_FPU_SOFT");
  abort();
#endif
}
