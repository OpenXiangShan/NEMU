/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#include <common.h>
#include <stdint.h>

#ifdef CONFIG_RV_AME

#include "mcompute_impl.h"
#include <cpu/cpu.h>
#include <fenv.h>
#include <float.h>
#include <rtl/fp.h>
#include "mcommon.h"

void require_matrix() {
  // if (mstatus->ms == 0) {
  //   longjmp_exception(EX_II);
  // }
  // #ifdef CONFIG_RVH
  // if (cpu.v && vsstatus->ms == 0) {
  //   longjmp_exception(EX_II);
  // }
  // #endif
}

uint8_t get_pack(mcfg_t cfg) {
  switch (cfg.type_code) {
    case MTYPECODE_INT4:
    case MTYPECODE_UINT4:
    case MTYPECODE_NVFP4:
    case MTYPECODE_MXFP4:
      return 2;
    case MTYPECODE_FP2PACK4:
      return 4;
    case MTYPECODE_FP2PACK5:
      return 5;
    default:
      return 1;
  }
}

mmacc_type_t get_mmacc_type(mcfg_t s1cfg, mcfg_t s2cfg, mcfg_t dcfg) {
  uint32_t s1_mask = 1u << s1cfg.type_code;
  uint32_t s2_mask = 1u << s2cfg.type_code;
  uint32_t d_mask = 1u << dcfg.type_code;
  const uint32_t int_mask = (1u << MTYPECODE_INT4) | (1u << MTYPECODE_UINT4) |
                            (1u << MTYPECODE_INT8) | (1u << MTYPECODE_UINT8) |
                            (1u << MTYPECODE_INT32);

  if ((s1_mask & int_mask) && (s2_mask & int_mask) && (d_mask & int_mask)) {
    return MMACC_TYPE_INTEGER;
  } else if (get_float_mmacc_type(
        dcfg.type_code, s1cfg.type_code, s2cfg.type_code
      ) != FLOAT_MMACC_UNSUPPORTED) {
    return MMACC_TYPE_FLOAT;
  } else {
    return MMACC_TYPE_INVALID;
  }
}

bool is_signed_int_mtype(uint64_t type_code) {
  switch (type_code) {
    case MTYPECODE_UINT4:
    case MTYPECODE_UINT8:
      return false;
    case MTYPECODE_INT4:
    case MTYPECODE_INT8:
    case MTYPECODE_INT32:
    default:
      return true;
  }
}

float_mmacc_type_t get_float_mmacc_type(
  uint64_t d_type, uint64_t s1_type, uint64_t s2_type
) {
  if (d_type == MTYPECODE_FP16 &&
      s1_type == MTYPECODE_FP16 && s2_type == MTYPECODE_FP16) {
    return FLOAT_MMACC_FP16_FP16_FP16;
  } else if (d_type == MTYPECODE_FP32 &&
      s1_type == MTYPECODE_FP16 && s2_type == MTYPECODE_FP16) {
    return FLOAT_MMACC_FP16_FP16_FP32;
  } else if (d_type == MTYPECODE_FP32 &&
      s1_type == MTYPECODE_BF16 && s2_type == MTYPECODE_BF16) {
    return FLOAT_MMACC_BF16_BF16_FP32;
  } else if (d_type == MTYPECODE_FP32 &&
      s1_type == MTYPECODE_FP32 && s2_type == MTYPECODE_FP32) {
    return FLOAT_MMACC_FP32_FP32_FP32;
  } else {
    return FLOAT_MMACC_UNSUPPORTED;
  }
}

#ifdef CONFIG_AME_MMACC_VECTORIZE

#define MACC_REG_BASE 4

// IEEE-754 field widths and positions, from least significant bit upward.
#define FP_SIGN_BITS 1

#define FP16_FRACTION_BITS 10
#define FP16_EXPONENT_BITS 5

#define BF16_FRACTION_BITS 7
#define BF16_EXPONENT_BITS 8

#define FP32_FRACTION_BITS 23
#define FP32_EXPONENT_BITS 8

#define EXPONENT_SHIFT(format) format##_FRACTION_BITS
#define SIGN_SHIFT(format) \
  (EXPONENT_SHIFT(format) + format##_EXPONENT_BITS)

#define FP_LOW_BITS_MASK(bits) \
  ((UINT32_C(1) << (bits)) - UINT32_C(1))
#define FP_FIELD_MASK(bits, shift) \
  (FP_LOW_BITS_MASK(bits) << (shift))

#define FRACTION_MASK(format) FP_LOW_BITS_MASK(format##_FRACTION_BITS)
#define EXPONENT_MASK(format) \
  FP_FIELD_MASK(format##_EXPONENT_BITS, EXPONENT_SHIFT(format))
#define SIGN_MASK(format) FP_FIELD_MASK(FP_SIGN_BITS, SIGN_SHIFT(format))
#define ABS_MASK(format) FP_LOW_BITS_MASK(SIGN_SHIFT(format))
#define FP_ABS_BITS(format, bits) ((bits) & ABS_MASK(format))
#define FP_IS_FINITE(format, bits) \
  ((((bits) >> EXPONENT_SHIFT(format)) & \
    FP_LOW_BITS_MASK(format##_EXPONENT_BITS)) != \
   FP_LOW_BITS_MASK(format##_EXPONENT_BITS))

static inline int macc_reg_index(int reg) {
  return reg - MACC_REG_BASE;
}

#define def_auto_vectorized_int8_mmacc(name, lhs_type, rhs_type) \
static void name( \
  int acc_idx, int ts1, int ts2, \
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k \
) { \
  for (uint64_t i = 0; i < tile_m; i++) { \
    const lhs_type *restrict lhs_row = \
      (const lhs_type *)cpu.mtr[ts1][i]._8; \
    uint32_t *restrict dest_row = cpu.macc[acc_idx][i]._32; \
    for (uint64_t j = 0; j < tile_n; j++) { \
      const rhs_type *restrict rhs_row = \
        (const rhs_type *)cpu.mtr[ts2][j]._8; \
      int32_t product_sum = 0; \
      for (uint64_t k = 0; k < tile_k; k++) { \
        product_sum += (int32_t)lhs_row[k] * (int32_t)rhs_row[k]; \
      } \
      dest_row[j] += (uint32_t)product_sum; \
    } \
  } \
}

def_auto_vectorized_int8_mmacc(mmacc_i8_i8, int8_t, int8_t)
def_auto_vectorized_int8_mmacc(mmacc_i8_u8, int8_t, uint8_t)
def_auto_vectorized_int8_mmacc(mmacc_u8_i8, uint8_t, int8_t)
def_auto_vectorized_int8_mmacc(mmacc_u8_u8, uint8_t, uint8_t)

#ifndef CONFIG_FPU_NONE

static bool map_fma_rounding_mode_to_host(
  uint64_t rounding_mode, int *host_rounding_mode
) {
  switch (rounding_mode) {
    case FPCALL_RM_RNE:
      *host_rounding_mode = FE_TONEAREST;
      return true;
    case FPCALL_RM_RTZ:
      *host_rounding_mode = FE_TOWARDZERO;
      return true;
    case FPCALL_RM_RDN:
      *host_rounding_mode = FE_DOWNWARD;
      return true;
    case FPCALL_RM_RUP:
      *host_rounding_mode = FE_UPWARD;
      return true;
    default:
      return false;
  }
}

static bool host_fma_env_begin(
  fenv_t *saved_env, int host_rounding_mode
) {
  if (feholdexcept(saved_env) != 0) {
    return false;
  }
  if (fesetround(host_rounding_mode) != 0) {
    fesetenv(saved_env);
    return false;
  }
  return true;
}

static void host_fma_env_end(const fenv_t *saved_env) {
  fesetenv(saved_env);
}

static inline float fp32_from_bits(uint32_t bits) {
  float value;
  __builtin_memcpy(&value, &bits, sizeof(value));
  return value;
}

static inline uint32_t fp32_to_bits(float value) {
  uint32_t bits;
  __builtin_memcpy(&bits, &value, sizeof(bits));
  return bits;
}

static inline float bf16_to_fp32(uint16_t value) {
  return fp32_from_bits((uint32_t)value <<
                        (SIGN_SHIFT(FP32) - SIGN_SHIFT(BF16)));
}

static inline float fp16_to_fp32(uint16_t value) {
  uint32_t sign = ((uint32_t)value & SIGN_MASK(FP16)) <<
                  (SIGN_SHIFT(FP32) - SIGN_SHIFT(FP16));
  uint32_t exponent = ((uint32_t)value & EXPONENT_MASK(FP16)) >>
                      EXPONENT_SHIFT(FP16);
  uint32_t fraction = (uint32_t)value & FRACTION_MASK(FP16);
  uint32_t bits;

  if (exponent == 0) {
    if (fraction == 0) {
      bits = sign;
    } else {
      uint32_t shift = (uint32_t)__builtin_clz(fraction) - 21;
      bits = sign | ((UINT32_C(113) - shift) << EXPONENT_SHIFT(FP32)) |
             (((fraction << shift) & FRACTION_MASK(FP16)) <<
              (FP32_FRACTION_BITS - FP16_FRACTION_BITS));
    }
  } else if (exponent == (EXPONENT_MASK(FP16) >> EXPONENT_SHIFT(FP16))) {
    bits = sign | EXPONENT_MASK(FP32) |
           (fraction << (FP32_FRACTION_BITS - FP16_FRACTION_BITS));
  } else {
    bits = sign | ((exponent + UINT32_C(112)) << EXPONENT_SHIFT(FP32)) |
           (fraction << (FP32_FRACTION_BITS - FP16_FRACTION_BITS));
  }
  return fp32_from_bits(bits);
}

static inline uint16_t fp16_from_fp32(float value) {
  _Float16 result = (_Float16)value;
  uint16_t bits;
  __builtin_memcpy(&bits, &result, sizeof(bits));
  return bits;
}

static bool mmacc_fp16_fp16_fp16(
  int acc_idx, int ts1, int ts2,
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k,
  int host_rounding_mode
) {
  float rhs_by_k[TRENUM16][ROWNUM];

  for (uint64_t i = 0; i < tile_m; i++) {
    const uint16_t *lhs_row = cpu.mtr[ts1][i]._16;
    for (uint64_t k = 0; k < tile_k; k++) {
      if (unlikely(!FP_IS_FINITE(FP16, lhs_row[k]))) {
        return false;
      }
    }
  }
  for (uint64_t k = 0; k < tile_k; k++) {
    for (uint64_t j = 0; j < tile_n; j++) {
      uint16_t bits = cpu.mtr[ts2][j]._16[k];
      if (unlikely(!FP_IS_FINITE(FP16, bits))) {
        return false;
      }
      rhs_by_k[k][j] = fp16_to_fp32(bits);
    }
  }
  for (uint64_t i = 0; i < tile_m; i++) {
    const uint16_t *dest_row = cpu.macc[acc_idx][i]._16;
    for (uint64_t j = 0; j < tile_n; j++) {
      if (unlikely(!FP_IS_FINITE(FP16, dest_row[j]))) {
        return false;
      }
    }
  }

  fenv_t saved_env;
  if (!host_fma_env_begin(&saved_env, host_rounding_mode)) {
    return false;
  }
  for (uint64_t i = 0; i < tile_m; i++) {
    const uint16_t *restrict lhs_row = cpu.mtr[ts1][i]._16;
    uint16_t *restrict dest_row = cpu.macc[acc_idx][i]._16;
    float acc_values[ROWNUM];
    for (uint64_t j = 0; j < tile_n; j++) {
      acc_values[j] = fp16_to_fp32(dest_row[j]);
    }
    for (uint64_t k = 0; k < tile_k; k++) {
      const float lhs = fp16_to_fp32(lhs_row[k]);
      const float *restrict rhs_row = rhs_by_k[k];
#pragma GCC ivdep
      for (uint64_t j = 0; j < tile_n; j++) {
        acc_values[j] = __builtin_fmaf(lhs, rhs_row[j], acc_values[j]);
      }
    }
    // AME leaves intermediate accumulation precision open. Keep the dot
    // product in FP32 and round once when storing the FP16 accumulator.
    for (uint64_t j = 0; j < tile_n; j++) {
      dest_row[j] = fp16_from_fp32(acc_values[j]);
    }
  }
  host_fma_env_end(&saved_env);
  return true;
}

#define def_auto_vectorized_float16_mmacc( \
  name, to_fp32, format \
) \
static bool name( \
  int acc_idx, int ts1, int ts2, \
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k, \
  int host_rounding_mode \
) { \
  /* Transpose B so the vectorized j loop preserves k accumulation order. */ \
  float rhs_by_k[TRENUM16][ROWNUM]; \
  long double lhs_abs_max = 0.0; \
  long double rhs_abs_max = 0.0; \
  long double dest_abs_max = 0.0; \
  for (uint64_t i = 0; i < tile_m; i++) { \
    const uint16_t *lhs_row = cpu.mtr[ts1][i]._16; \
    for (uint64_t k = 0; k < tile_k; k++) { \
      uint16_t lhs_bits = lhs_row[k]; \
      if (unlikely(!FP_IS_FINITE(format, lhs_bits))) { \
        return false; \
      } \
      float abs_value = to_fp32(FP_ABS_BITS(format, lhs_bits)); \
      if (abs_value > lhs_abs_max) { \
        lhs_abs_max = abs_value; \
      } \
    } \
  } \
  for (uint64_t k = 0; k < tile_k; k++) { \
    for (uint64_t j = 0; j < tile_n; j++) { \
      uint16_t value = cpu.mtr[ts2][j]._16[k]; \
      if (unlikely(!FP_IS_FINITE(format, value))) { \
        return false; \
      } \
      rhs_by_k[k][j] = to_fp32(value); \
      float abs_value = to_fp32(FP_ABS_BITS(format, value)); \
      if (abs_value > rhs_abs_max) { \
        rhs_abs_max = abs_value; \
      } \
    } \
  } \
  for (uint64_t i = 0; i < tile_m; i++) { \
    const uint32_t *dest_row = cpu.macc[acc_idx][i]._32; \
    for (uint64_t j = 0; j < tile_n; j++) { \
      uint32_t dest_bits = dest_row[j]; \
      if (unlikely(!FP_IS_FINITE(FP32, dest_bits))) { \
        return false; \
      } \
      float abs_value = fp32_from_bits(FP_ABS_BITS(FP32, dest_bits)); \
      if (abs_value > dest_abs_max) { \
        dest_abs_max = abs_value; \
      } \
    } \
  } \
  /* Keep exceptional operands and possible overflow on the SoftFloat path. */ \
  long double result_abs_bound = dest_abs_max + \
    (long double)tile_k * lhs_abs_max * rhs_abs_max; \
  if (unlikely(result_abs_bound > FLT_MAX)) { \
    return false; \
  } \
  fenv_t saved_env; \
  if (!host_fma_env_begin(&saved_env, host_rounding_mode)) { \
    return false; \
  } \
  for (uint64_t i = 0; i < tile_m; i++) { \
    const uint16_t *restrict lhs_row = cpu.mtr[ts1][i]._16; \
    uint32_t *restrict dest_row = cpu.macc[acc_idx][i]._32; \
    for (uint64_t k = 0; k < tile_k; k++) { \
      const float lhs = to_fp32(lhs_row[k]); \
      const float *restrict rhs_row = rhs_by_k[k]; \
      _Pragma("GCC ivdep") \
      for (uint64_t j = 0; j < tile_n; j++) { \
        float dest = fp32_from_bits(dest_row[j]); \
        dest_row[j] = fp32_to_bits(__builtin_fmaf(lhs, rhs_row[j], dest)); \
      } \
    } \
  } \
  host_fma_env_end(&saved_env); \
  return true; \
}

def_auto_vectorized_float16_mmacc(
  mmacc_fp16_fp16_fp32, fp16_to_fp32, FP16)
def_auto_vectorized_float16_mmacc(
  mmacc_bf16_bf16_fp32, bf16_to_fp32, BF16)

#undef def_auto_vectorized_float16_mmacc

static bool mmacc_fp32_fp32_fp32(
  int acc_idx, int ts1, int ts2,
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k,
  int host_rounding_mode
) {
  float rhs_by_k[TRENUM32][ROWNUM];

  for (uint64_t i = 0; i < tile_m; i++) {
    const uint32_t *lhs_row = cpu.mtr[ts1][i]._32;
    for (uint64_t k = 0; k < tile_k; k++) {
      if (unlikely(!FP_IS_FINITE(FP32, lhs_row[k]))) {
        return false;
      }
    }
  }
  for (uint64_t k = 0; k < tile_k; k++) {
    for (uint64_t j = 0; j < tile_n; j++) {
      uint32_t bits = cpu.mtr[ts2][j]._32[k];
      if (unlikely(!FP_IS_FINITE(FP32, bits))) {
        return false;
      }
      rhs_by_k[k][j] = fp32_from_bits(bits);
    }
  }
  for (uint64_t i = 0; i < tile_m; i++) {
    const uint32_t *dest_row = cpu.macc[acc_idx][i]._32;
    for (uint64_t j = 0; j < tile_n; j++) {
      if (unlikely(!FP_IS_FINITE(FP32, dest_row[j]))) {
        return false;
      }
    }
  }

  fenv_t saved_env;
  if (!host_fma_env_begin(&saved_env, host_rounding_mode)) {
    return false;
  }
  for (uint64_t i = 0; i < tile_m; i++) {
    const uint32_t *restrict lhs_row = cpu.mtr[ts1][i]._32;
    uint32_t *restrict dest_row = cpu.macc[acc_idx][i]._32;
    float dest_values[ROWNUM];
    for (uint64_t j = 0; j < tile_n; j++) {
      dest_values[j] = fp32_from_bits(dest_row[j]);
    }
    for (uint64_t k = 0; k < tile_k; k++) {
      const float lhs = fp32_from_bits(lhs_row[k]);
      const float *restrict rhs_row = rhs_by_k[k];
#pragma GCC ivdep
      for (uint64_t j = 0; j < tile_n; j++) {
        dest_values[j] = __builtin_fmaf(lhs, rhs_row[j], dest_values[j]);
      }
    }
    for (uint64_t j = 0; j < tile_n; j++) {
      dest_row[j] = fp32_to_bits(dest_values[j]);
    }
  }
  host_fma_env_end(&saved_env);
  return true;
}

#endif // CONFIG_FPU_NONE

bool try_auto_vectorized_int8_mmacc(
  int td, int ts1, int ts2,
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k,
  uint64_t d_type, uint64_t s1_type, uint64_t s2_type,
  bool saturation
) {
  bool valid_registers = td >= 4 && td < 8 &&
                         ts1 >= 0 && ts1 < 4 &&
                         ts2 >= 0 && ts2 < 4;
  bool valid_types = d_type == MTYPECODE_INT32 &&
                     (s1_type == MTYPECODE_INT8 || s1_type == MTYPECODE_UINT8) &&
                     (s2_type == MTYPECODE_INT8 || s2_type == MTYPECODE_UINT8);
  const uint64_t max_safe_tile_k = INT32_MAX / (UINT8_MAX * UINT8_MAX);
  if (!valid_registers || !valid_types || saturation ||
      tile_k > max_safe_tile_k) {
    return false;
  }

  int acc_idx = macc_reg_index(td);
  if (s1_type == MTYPECODE_INT8) {
    if (s2_type == MTYPECODE_INT8) {
      mmacc_i8_i8(acc_idx, ts1, ts2, tile_m, tile_n, tile_k);
    } else {
      mmacc_i8_u8(acc_idx, ts1, ts2, tile_m, tile_n, tile_k);
    }
  } else if (s2_type == MTYPECODE_INT8) {
    mmacc_u8_i8(acc_idx, ts1, ts2, tile_m, tile_n, tile_k);
  } else {
    mmacc_u8_u8(acc_idx, ts1, ts2, tile_m, tile_n, tile_k);
  }
  return true;
}

bool try_auto_vectorized_float_mmacc(
  int td, int ts1, int ts2,
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k,
  float_mmacc_type_t float_mmacc_type,
  uint64_t rounding_mode
) {
#ifndef CONFIG_FPU_NONE
  int host_rounding_mode;
  if (!map_fma_rounding_mode_to_host(rounding_mode, &host_rounding_mode)) {
    return false;
  }
  int acc_idx = macc_reg_index(td);
  switch (float_mmacc_type) {
    case FLOAT_MMACC_FP16_FP16_FP16:
      return mmacc_fp16_fp16_fp16(
        acc_idx, ts1, ts2, tile_m, tile_n, tile_k, host_rounding_mode);
    case FLOAT_MMACC_FP16_FP16_FP32:
      return mmacc_fp16_fp16_fp32(
        acc_idx, ts1, ts2, tile_m, tile_n, tile_k, host_rounding_mode);
    case FLOAT_MMACC_BF16_BF16_FP32:
      return mmacc_bf16_bf16_fp32(
        acc_idx, ts1, ts2, tile_m, tile_n, tile_k, host_rounding_mode);
    case FLOAT_MMACC_FP32_FP32_FP32:
      return mmacc_fp32_fp32_fp32(
        acc_idx, ts1, ts2, tile_m, tile_n, tile_k, host_rounding_mode);
    case FLOAT_MMACC_UNSUPPORTED:
    default:
      return false;
  }
#endif
  return false;
}

#endif // CONFIG_AME_MMACC_VECTORIZE

#endif // CONFIG_RV_AME
