/***************************************************************************************
 * Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of
 *Sciences
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <common.h>
#ifdef CONFIG_CUSTOM_TENSOR

#include "type.h"
#include <stdint.h>
#include <string.h>

float fp8_e4m3_to_fp32(fp8 x) {
  int sign = (x & 0x80) ? -1 : 1;
  int exponent = (x >> 3) & 0x0F;
  float mantissa = (x & 0x07) / 8.0f;
  if (exponent == 0) {
    if (mantissa == 0.0f) {
      return 0.0f * sign;
    }
    return sign * mantissa * 0.015625f;
  }

  mantissa += 1.0f;
  float exp_value = 1.0f;
  int exp = exponent - 7;

  if (exp > 0) {
    for (int i = 0; i < exp; i++) {
      exp_value *= 2.0f;
    }
  } else {
    for (int i = 0; i < -exp; i++) {
      exp_value /= 2.0f;
    }
  }

  return sign * mantissa * exp_value;
}

fp8 fp32_to_fp8_e4m3(float f) {
  if (f == 0.0f)
    return 0;

  uint32_t bits;
  memcpy(&bits, &f, sizeof(float));
  int sign = (bits >> 31) & 0x1;
  int exponent = ((bits >> 23) & 0xFF) - 127;
  uint32_t mantissa = bits & 0x007FFFFF;

  if (exponent > 7) {
    return sign ? 0x80 : 0x7F;
  }
  if (exponent < -6) {
    return sign ? 0x80 : 0x00;
  }

  fp8 result = 0;
  result |= (sign << 7);

  if (exponent <= -6) {

    float val = f < 0 ? -f : f;
    int m = (int)(val * 512.0f);
    if (m > 7)
      m = 7;
    result |= m;
    return result;
  }

  int fp8_exponent = exponent + 7;
  result |= (fp8_exponent << 3);
  result |= ((mantissa >> 20) & 0x07);
  return result;
}

float fp16_to_fp32(fp16 h) {
  uint32_t x = ((h & 0x8000) << 16) | (((h & 0x7c00) + 0x1c000) << 13) |
               ((h & 0x03ff) << 13);
  if ((h & 0x7c00) == 0) {
    if ((h & 0x03ff) != 0) {
      uint32_t sign = (h & 0x8000) << 16;
      uint32_t mantissa = (h & 0x03ff);
      int32_t exponent = -14;

      while ((mantissa & 0x0400) == 0) {
        mantissa <<= 1;
        exponent--;
      }

      mantissa &= 0x03ff;
      x = sign | ((exponent + 127) << 23) | (mantissa << 13);
    }
  }
  float result;
  memcpy(&result, &x, sizeof(x));
  return result;
}

bf16 fp32_to_bf16_with_mode(float f, ConvertMode mode) {
  uint32_t x = 0;
  memcpy(&x, &f, sizeof(float));
  uint32_t sign = x & 0x80000000;
  int32_t exponent = (x >> 23) & 0xFF;
  // uint32_t mantissa = x & 0x007FFFFF;

  if (exponent == 0xFF) {
    return (x >> 16) | 0x007F;
  }

  if (exponent == 0) {
    return (x >> 16) & 0x8000;
  }

  uint32_t truncated = x >> 16;
  uint32_t round_bit = (x >> 15) & 1;
  uint32_t sticky_bits = x & 0x7FFF;

  switch (mode) {
  case TX_CONVERT_ROUND:
    if (round_bit && (sticky_bits || (truncated & 1))) {
      truncated += 1;
    }
    break;

  case TX_CONVERT_CEIL:
    if (!sign && (round_bit || sticky_bits)) {
      truncated += 1;
    }
    break;

  case TX_CONVERT_FLOOR:
    if (sign && (round_bit || sticky_bits)) {
      truncated += 1;
    }
    break;
  }

  if ((truncated & 0x7F80) == 0x7F80) {
    truncated = (truncated & 0x8000) | 0x7F80;
  }

  return truncated;
}

fp16 fp32_to_fp16(float f) {
  uint32_t x = 0;
  memcpy(&x, &f, sizeof(float));
  fp16 h = ((x >> 16) & 0x8000) |
           ((((x & 0x7f800000) - 0x38000000) >> 13) & 0x7c00) |
           ((x >> 13) & 0x03ff);

  if ((x & 0x7f800000) == 0x7f800000) {
    h = (h & 0x8000) | 0x7c00 | ((x >> 13) & 0x03ff);
  } else if ((x & 0x7f800000) > 0x477fe000) {
    h = (h & 0x8000) | 0x7c00;
  } else if ((x & 0x7f800000) < 0x38000000) {
    uint32_t sign = x & 0x80000000;
    uint32_t mantissa = (x & 0x007fffff) | 0x00800000;
    int32_t exponent = ((x >> 23) & 0xff) - 127 - 14;

    if (exponent < -24) {
      h = (uint16_t)(sign >> 16);
    } else {
      mantissa >>= (-exponent);
      h = (uint16_t)((sign >> 16) | (mantissa >> 13));
    }
  }
  return h;
}

fp16 fp32_to_fp16_with_mode(float f, ConvertMode mode) {
  uint32_t x = 0;
  memcpy(&x, &f, sizeof(float));
  uint32_t sign = x & 0x80000000;
  int32_t exponent = ((x >> 23) & 0xFF) - 127;
  uint32_t mantissa = x & 0x007FFFFF;

  if ((x & 0x7F800000) == 0x7F800000) {
    return (sign >> 16) | 0x7C00 | ((mantissa >> 13) & 0x03FF);
  }

  if (exponent > 15) {
    return (sign >> 16) | 0x7C00;
  }

  if (exponent < -14) {
    int shift = -14 - exponent;
    if (shift > 24) {
      return (sign >> 16);
    }

    mantissa |= 0x00800000;
    mantissa >>= shift;

    uint32_t round_bit = 1 << (shift - 1);
    uint32_t sticky_bit = (mantissa & ((1 << (shift - 1)) - 1)) ? 1 : 0;

    switch (mode) {
    case TX_CONVERT_ROUND:
      mantissa += (round_bit & (mantissa | sticky_bit));
      break;
    case TX_CONVERT_CEIL:
      if (sign) {
      } else {
        mantissa += (round_bit | sticky_bit);
      }
      break;
    case TX_CONVERT_FLOOR:
      if (sign) {
        mantissa += (round_bit | sticky_bit);
      }
      break;
    }

    return (sign >> 16) | (mantissa >> 13);
  }

  uint32_t fp16_mantissa = mantissa >> 13;
  uint32_t round_bit = (mantissa >> 12) & 1;
  uint32_t sticky_bit = (mantissa & 0xFFF) ? 1 : 0;

  switch (mode) {
  case TX_CONVERT_ROUND:
    fp16_mantissa += (round_bit & (fp16_mantissa | sticky_bit));
    break;
  case TX_CONVERT_CEIL:
    if (sign) {
    } else {
      fp16_mantissa += (round_bit | sticky_bit);
    }
    break;
  case TX_CONVERT_FLOOR:
    if (sign) {
      fp16_mantissa += (round_bit | sticky_bit);
    }
    break;
  }

  if (fp16_mantissa > 0x03FF) {
    exponent++;
    fp16_mantissa = 0;
  }

  return (sign >> 16) | (((exponent + 15) << 10) & 0x7C00) |
         (fp16_mantissa & 0x03FF);
}

float bf16_to_fp32(bf16 h) {
  uint32_t f = h << 16;
  float r;
  memcpy(&r, &f, sizeof(uint32_t));
  return r;
}

uint8_t fp32_to_fp8_e4m3_with_mode(float f, ConvertMode mode) {
  uint8_t out;
  uint32_t in;
  memcpy(&in, &f, sizeof(float));
  int32_t exponent = ((in >> 23) & 0xFF) - 127;
  uint32_t mantissa = in & 0x007FFFFF;

  out = (in >> 24) & 0x80;

  in &= 0x7fffffff;

  if ((exponent > 8) || ((exponent == 8) && (mantissa >= 0x600000))) {
      out |= 0x7E;
  } else if (exponent < -7) { // => 0.
      // out |= 0x1;
  } else {
      in = (in & 0x807fffff) | (((exponent + 7) & 0xff) << 23);
      uint32_t eps = (0x3fffff>>3) + ((in >> (23-3)) & 0x1);
      in += eps;
      out |= (in >> (23-3)) & 0x7F ;
  }
  return out;
}

bf16 fp32_to_bf16(float f) {
  uint32_t x;
  memcpy(&x, &f, sizeof(float));
  return (x >> 16) & 0xFFFF;
}

float tf32_to_fp32(uint32_t t) {
  //TODO
  float r = 0;
  memcpy(&r, &t, sizeof(uint32_t));
  return r;
}

uint32_t fp32_to_tf32(float f) { 
    //TODO
    uint32_t r = 0;
    memcpy(&r, &f, sizeof(f));
    return r & 0xFFFFE000; 
}

#endif /* CONFIG_CUSTOM_TENSOR */
