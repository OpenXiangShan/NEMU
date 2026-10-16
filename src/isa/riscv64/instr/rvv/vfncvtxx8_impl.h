#ifndef __RISCV64_VFNCVTXX8_IMPL_H__
#define __RISCV64_VFNCVTXX8_IMPL_H__

#include <stdbool.h>
#include <stdint.h>
#include <rtl/fp.h>

enum {
  VFNCVTXX8_RNE = 0,
  VFNCVTXX8_RTZ = 1,
  VFNCVTXX8_RDN = 2,
  VFNCVTXX8_RUP = 3,
  VFNCVTXX8_RMM = 4,
};

typedef struct {
  uint8_t value;
  uint32_t flags;
} vfncvtxx8_result_t;

static float vfncvtxx8_float_from_bits(uint32_t bits) {
  union {
    uint32_t bits;
    float value;
  } data = { .bits = bits };
  return data.value;
}

static uint32_t vfncvtxx8_fp8_to_fp32_bits(
    uint8_t value, int fraction_bits, int exponent_bias) {
  const uint32_t fraction_mask = (1u << fraction_bits) - 1;
  const uint32_t exponent = value >> fraction_bits;
  const uint32_t fraction = value & fraction_mask;

  if (exponent != 0) {
    return ((exponent - exponent_bias + 127) << 23)
         | (fraction << (23 - fraction_bits));
  }
  if (fraction == 0) {
    return 0;
  }

  int leading_bit = 0;
  for (uint32_t bits = fraction; bits > 1; bits >>= 1) {
    leading_bit++;
  }
  const int unbiased_exponent = 1 - exponent_bias - fraction_bits + leading_bit;
  return ((uint32_t)(unbiased_exponent + 127) << 23)
       | ((fraction - (1u << leading_bit)) << (23 - leading_bit));
}

static vfncvtxx8_result_t vfncvtxx8_convert_int8(uint32_t source, uint32_t rm) {
  const bool sign = source >> 31;
  const uint32_t magnitude = source & 0x7fffffff;
  const uint32_t exponent = magnitude >> 23;
  const uint32_t fraction = magnitude & 0x7fffff;
  vfncvtxx8_result_t result = { .value = 0, .flags = 0 };

  if (exponent == 0xff) {
    result.value = fraction == 0 && sign ? 0x80 : 0x7f;
    result.flags = FPCALL_EX_NV;
    return result;
  }
  if (magnitude >= 0x4f000000u) {
    result.value = sign ? 0x80 : 0x7f;
    result.flags = FPCALL_EX_NV;
    return result;
  }

  const float source_value = vfncvtxx8_float_from_bits(source);
  int32_t rounded = (int32_t)source_value;
  const float remainder = source_value - (float)rounded;
  if (remainder != 0.0f) {
    const float absolute_remainder = remainder < 0.0f ? -remainder : remainder;
    const int32_t direction = sign ? -1 : 1;
    if (rm == VFNCVTXX8_RDN && sign) {
      rounded--;
    } else if (rm == VFNCVTXX8_RUP && !sign) {
      rounded++;
    } else if (rm == VFNCVTXX8_RNE &&
               (absolute_remainder > 0.5f ||
                (absolute_remainder == 0.5f && (rounded & 1)))) {
      rounded += direction;
    } else if (rm == VFNCVTXX8_RMM && absolute_remainder >= 0.5f) {
      rounded += direction;
    }
  }

  if (rounded > 127 || rounded < -128) {
    result.value = sign ? 0x80 : 0x7f;
    result.flags = FPCALL_EX_NV;
  } else {
    result.value = (uint8_t)rounded;
    if (remainder != 0.0f) {
      result.flags = FPCALL_EX_NX;
    }
  }
  return result;
}

static uint8_t vfncvtxx8_overflow_e5m2(bool sign, uint32_t rm) {
  const uint8_t maximum = sign ? 0xfb : 0x7b;
  const uint8_t infinity = sign ? 0xfc : 0x7c;
  if (rm == VFNCVTXX8_RTZ) {
    return maximum;
  }
  if (rm == VFNCVTXX8_RUP) {
    return sign ? maximum : infinity;
  }
  if (rm == VFNCVTXX8_RDN) {
    return sign ? infinity : maximum;
  }
  return infinity;
}

static vfncvtxx8_result_t vfncvtxx8_convert_fp8(
    uint32_t source, uint32_t rm, int fraction_bits, int exponent_bias,
    uint8_t maximum_code, uint8_t canonical_nan,
    bool has_infinity) {
  const bool sign = source >> 31;
  const uint32_t magnitude = source & 0x7fffffff;
  const uint32_t exponent = magnitude >> 23;
  const uint32_t fraction = magnitude & 0x7fffff;
  vfncvtxx8_result_t result = { .value = 0, .flags = 0 };

  if (exponent == 0xff) {
    if (fraction != 0) {
      result.value = canonical_nan;
      if ((fraction & 0x400000) == 0) {
        result.flags = FPCALL_EX_NV;
      }
    } else if (has_infinity) {
      result.value = sign ? 0xfc : 0x7c;
    } else {
      result.value = maximum_code | (sign ? 0x80 : 0);
      result.flags = FPCALL_EX_OF | FPCALL_EX_NX;
    }
    return result;
  }

  const uint32_t maximum_bits = vfncvtxx8_fp8_to_fp32_bits(
      maximum_code, fraction_bits, exponent_bias);
  if (magnitude > maximum_bits) {
    result.value = has_infinity
        ? vfncvtxx8_overflow_e5m2(sign, rm)
        : maximum_code | (sign ? 0x80 : 0);
    result.flags = FPCALL_EX_OF | FPCALL_EX_NX;
    return result;
  }

  uint8_t lower = 0;
  uint32_t lower_bits = 0;
  for (uint8_t code = 1; code <= maximum_code; code++) {
    const uint32_t candidate_bits = vfncvtxx8_fp8_to_fp32_bits(
        code, fraction_bits, exponent_bias);
    if (candidate_bits > magnitude) {
      break;
    }
    lower = code;
    lower_bits = candidate_bits;
  }

  uint8_t rounded = lower;
  if (lower_bits != magnitude) {
    const uint8_t upper = lower + 1;
    if ((rm == VFNCVTXX8_RUP && !sign) || (rm == VFNCVTXX8_RDN && sign)) {
      rounded = upper;
    } else if (rm == VFNCVTXX8_RNE || rm == VFNCVTXX8_RMM) {
      const float lower_value = vfncvtxx8_float_from_bits(lower_bits);
      const float upper_value = vfncvtxx8_float_from_bits(
          vfncvtxx8_fp8_to_fp32_bits(
              upper, fraction_bits, exponent_bias));
      const float midpoint = (lower_value + upper_value) * 0.5f;
      const float source_value = vfncvtxx8_float_from_bits(magnitude);
      if (source_value > midpoint ||
          (source_value == midpoint &&
           (rm == VFNCVTXX8_RMM || (lower & 1)))) {
        rounded = upper;
      }
    }
    result.flags = FPCALL_EX_NX;
    if ((rounded >> fraction_bits) == 0) {
      result.flags |= FPCALL_EX_UF;
    }
  }

  result.value = rounded | (sign ? 0x80 : 0);
  return result;
}

static vfncvtxx8_result_t vfncvtxx8_convert_e4m3(uint32_t source, uint32_t rm) {
  return vfncvtxx8_convert_fp8(source, rm, 3, 7, 0x7e, 0x7f, false);
}

static vfncvtxx8_result_t vfncvtxx8_convert_e5m2(uint32_t source, uint32_t rm) {
  return vfncvtxx8_convert_fp8(source, rm, 2, 15, 0x7b, 0x7e, true);
}

#endif
