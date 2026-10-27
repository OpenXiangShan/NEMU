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
#ifdef CONFIG_FPU_SOFT
#include <softfloat.h>
#endif // CONFIG_FPU_SOFT
#include "mcommon.h"

#ifndef CONFIG_FPU_NONE
void isa_fp_rm_check(uint32_t rm);

void ame_fp_matrix_begin(uint32_t rm) {
  isa_fp_rm_check(rm);
#ifdef CONFIG_FPU_HOST
  static const int host_rounding_modes[] = {
    FE_TONEAREST, FE_TOWARDZERO, FE_DOWNWARD, FE_UPWARD, FE_TONEAREST,
  };
  fesetround(host_rounding_modes[rm]);
  feclearexcept(FE_ALL_EXCEPT);
#else
  static const uint_fast8_t softfloat_rounding_modes[] = {
    softfloat_round_near_even,
    softfloat_round_minMag,
    softfloat_round_min,
    softfloat_round_max,
    softfloat_round_near_maxMag,
  };
  softfloat_roundingMode = softfloat_rounding_modes[rm];
  softfloat_exceptionFlags = 0;
#endif // CONFIG_FPU_HOST
}

uint32_t ame_fp_matrix_end(void) {
  uint32_t ex = 0;
#ifdef CONFIG_FPU_HOST
  // Host-FPU scalar operations intentionally do not update the architectural
  // fflags CSR. Matrix operations have their own sticky mfflags, so collect
  // and clear host exceptions only within this matrix-instruction boundary.
  int host_ex = fetestexcept(FE_ALL_EXCEPT);
  if (host_ex & FE_INEXACT) {
    ex |= FPCALL_EX_NX;
  }
  if (host_ex & FE_UNDERFLOW) {
    ex |= FPCALL_EX_UF;
  }
  if (host_ex & FE_OVERFLOW) {
    ex |= FPCALL_EX_OF;
  }
  if (host_ex & FE_INVALID) {
    ex |= FPCALL_EX_NV;
  }
  feclearexcept(FE_ALL_EXCEPT);
#else
  if (softfloat_exceptionFlags & softfloat_flag_inexact) {
    ex |= FPCALL_EX_NX;
  }
  if (softfloat_exceptionFlags & softfloat_flag_underflow) {
    ex |= FPCALL_EX_UF;
  }
  if (softfloat_exceptionFlags & softfloat_flag_overflow) {
    ex |= FPCALL_EX_OF;
  }
  if (softfloat_exceptionFlags & softfloat_flag_invalid) {
    ex |= FPCALL_EX_NV;
  }
  softfloat_exceptionFlags = 0;
#endif // CONFIG_FPU_HOST
  return ex;
}
#endif // CONFIG_FPU_NONE

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

int_mmacc_type_t get_int_mmacc_type(
  uint64_t d_type, uint64_t s1_type, uint64_t s2_type
) {
  if (d_type != MTYPECODE_INT32) {
    return INT_MMACC_UNSUPPORTED;
  }
  bool s1_int4 = s1_type == MTYPECODE_INT4 || s1_type == MTYPECODE_UINT4;
  bool s2_int4 = s2_type == MTYPECODE_INT4 || s2_type == MTYPECODE_UINT4;
  if (s1_int4 && s2_int4) {
    return INT_MMACC_INT4_INT4_INT32;
  }
  bool s1_int8 = s1_type == MTYPECODE_INT8 || s1_type == MTYPECODE_UINT8;
  bool s2_int8 = s2_type == MTYPECODE_INT8 || s2_type == MTYPECODE_UINT8;
  if (s1_int8 && s2_int8) {
    return INT_MMACC_INT8_INT8_INT32;
  }
  return INT_MMACC_UNSUPPORTED;
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
  } else if (d_type == MTYPECODE_FP32 &&
      s1_type == MTYPECODE_FP8E4M3 && s2_type == MTYPECODE_FP8E4M3) {
    return FLOAT_MMACC_FP8E4M3_FP8E4M3_FP32;
  } else if (d_type == MTYPECODE_FP32 &&
      s1_type == MTYPECODE_FP8E5M2 && s2_type == MTYPECODE_FP8E5M2) {
    return FLOAT_MMACC_FP8E5M2_FP8E5M2_FP32;
  } else {
    return FLOAT_MMACC_UNSUPPORTED;
  }
}

mmacc_type_t get_mmacc_type(mcfg_t s1cfg, mcfg_t s2cfg, mcfg_t dcfg) {
  if (s1cfg.table_set != 0 || s2cfg.table_set != 0 || dcfg.table_set != 0) {
    return MMACC_TYPE_INVALID;
  }
  if (get_int_mmacc_type(
        dcfg.type_code, s1cfg.type_code, s2cfg.type_code
      ) != INT_MMACC_UNSUPPORTED) {
    return MMACC_TYPE_INTEGER;
  }

  if (get_float_mmacc_type(
        dcfg.type_code, s1cfg.type_code, s2cfg.type_code
      ) != FLOAT_MMACC_UNSUPPORTED) {
    return MMACC_TYPE_FLOAT;
  }
  return MMACC_TYPE_INVALID;
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

// Encode an MMA operand's mcfg type_code for AMU control types1/types2/typed.
// Bits[1:0] encode element width (8/16/32/4).
// Bit 2 selects signed 4/8-bit integers or the alternate floating-point format.
// This is distinct from the byte-storage size returned by get_size().
uint8_t get_mma_op_amuctrl(uint64_t type_code) {
  switch (type_code) {
    case MTYPECODE_UINT8:
    case MTYPECODE_FP8E5M2:
      return 0;
    case MTYPECODE_FP16:
      return 1;
    case MTYPECODE_INT32:
    case MTYPECODE_FP32:
      return 2;
    case MTYPECODE_UINT4:
    case MTYPECODE_NVFP4:
      return 3;
    case MTYPECODE_INT8:
    case MTYPECODE_FP8E4M3:
      return 4;
    case MTYPECODE_BF16:
      return 5;
    case MTYPECODE_TF32:
      return 6;
    case MTYPECODE_INT4:
    case MTYPECODE_MXFP4:
      return 7;
    default:
      longjmp_exception(EX_II);
  }
}

#ifdef CONFIG_AME_MMACC_VECTORIZE

#define MACC_REG_BASE 4

// IEEE-754 field widths and positions, from least significant bit upward.
#define FP_SIGN_BITS 1

#define FP8E4M3_FRACTION_BITS 3
#define FP8E4M3_EXPONENT_BITS 4

#define FP8E5M2_FRACTION_BITS 2
#define FP8E5M2_EXPONENT_BITS 5

#define FP16_FRACTION_BITS 10
#define FP16_EXPONENT_BITS 5

#define BF16_FRACTION_BITS 7
#define BF16_EXPONENT_BITS 8

#define FP32_FRACTION_BITS 23
#define FP32_EXPONENT_BITS 8

#if defined(__FLT16_MANT_DIG__) && defined(__FLT16_MAX_EXP__) && \
    __FLT16_MANT_DIG__ == 11 && __FLT16_MAX_EXP__ == 16
#define HOST_HAS_IEEE_FLOAT16 1
#else
#define HOST_HAS_IEEE_FLOAT16 0
#endif

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
  int exceptions = fetestexcept(FE_ALL_EXCEPT);
  fesetenv(saved_env);
#ifdef CONFIG_FPU_SOFT
  // Retain fast-path exceptions for ame_fp_matrix_end(), just like SoftFloat.
  if (exceptions & FE_INEXACT) softfloat_exceptionFlags |= softfloat_flag_inexact;
  if (exceptions & FE_UNDERFLOW) softfloat_exceptionFlags |= softfloat_flag_underflow;
  if (exceptions & FE_OVERFLOW) softfloat_exceptionFlags |= softfloat_flag_overflow;
  if (exceptions & FE_INVALID) softfloat_exceptionFlags |= softfloat_flag_invalid;
#else
  feraiseexcept(exceptions);
#endif
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

static inline float fp8e4m3_to_fp32(uint8_t value) {
  uint32_t sign = (uint32_t)(value & 0x80) << 24;
  unsigned exponent = (value >> 3) & 0x0f;
  unsigned fraction = value & 0x07;
  if ((value & 0x7f) == 0x7f) {
    return fp32_from_bits(sign | UINT32_C(0x7fc00000));
  }
  if (exponent == 0) {
    return fp32_from_bits(sign | fp32_to_bits((float)fraction * 0x1p-9f));
  }
  return fp16_to_fp32(((uint16_t)(value & 0x80) << 8) |
                      ((exponent + 8) << 10) | (fraction << 7));
}

static inline float fp8e5m2_to_fp32(uint8_t value) {
  // E5M2 has the FP16 exponent/bias and its two highest fraction bits.
  return fp16_to_fp32((uint16_t)value << 8);
}

#if HOST_HAS_IEEE_FLOAT16

static inline uint16_t fp16_from_fp32(float value) {
  _Float16 result = (_Float16)value;
  uint16_t bits;
  __builtin_memcpy(&bits, &result, sizeof(bits));
  return bits;
}

#endif // HOST_HAS_IEEE_FLOAT16

// Storage, conversion and finite-value checks for the shared FMA kernel.
#define FP8E4M3_SEW 8
#define FP8E5M2_SEW 8
#define FP16_SEW 16
#define BF16_SEW 16
#define FP32_SEW 32

#define FP8E4M3_TO_FP32 fp8e4m3_to_fp32
#define FP8E5M2_TO_FP32 fp8e5m2_to_fp32
#define FP16_TO_FP32 fp16_to_fp32
#define BF16_TO_FP32 bf16_to_fp32
#define FP32_TO_FP32 fp32_from_bits
#define FP16_FROM_FP32 fp16_from_fp32
#define FP32_FROM_FP32 fp32_to_bits

#define FP8E4M3_IS_FINITE(bits) (FP_ABS_BITS(FP8E4M3, bits) != 0x7f)
#define FP8E5M2_IS_FINITE(bits) FP_IS_FINITE(FP8E5M2, bits)
#define FP16_IS_FINITE(bits) FP_IS_FINITE(FP16, bits)
#define BF16_IS_FINITE(bits) FP_IS_FINITE(BF16, bits)
#define FP32_IS_FINITE(bits) FP_IS_FINITE(FP32, bits)

#define def_auto_vectorized_float_mmacc( \
  name, src_format, dest_format \
) \
static bool name( \
  int acc_idx, int ts1, int ts2, \
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k, \
  int host_rounding_mode \
) { \
  typedef concat3(uint, src_format##_SEW, _t) src_type; \
  typedef concat3(uint, dest_format##_SEW, _t) dest_type; \
  /* Transpose B so the vectorized j loop preserves k accumulation order. */ \
  float rhs_by_k[concat(TRENUM, src_format##_SEW)][ROWNUM]; \
  long double lhs_abs_max = 0.0; \
  long double rhs_abs_max = 0.0; \
  long double dest_abs_max = 0.0; \
  for (uint64_t i = 0; i < tile_m; i++) { \
    const src_type *lhs_row = cpu.mtr[ts1][i].concat(_, src_format##_SEW); \
    for (uint64_t k = 0; k < tile_k; k++) { \
      uint32_t lhs_bits = lhs_row[k]; \
      if (unlikely(!src_format##_IS_FINITE(lhs_bits))) { \
        return false; \
      } \
      float abs_value = src_format##_TO_FP32(FP_ABS_BITS(src_format, lhs_bits)); \
      if (abs_value > lhs_abs_max) { \
        lhs_abs_max = abs_value; \
      } \
    } \
  } \
  for (uint64_t k = 0; k < tile_k; k++) { \
    for (uint64_t j = 0; j < tile_n; j++) { \
      uint32_t rhs_bits = cpu.mtr[ts2][j].concat(_, src_format##_SEW)[k]; \
      if (unlikely(!src_format##_IS_FINITE(rhs_bits))) { \
        return false; \
      } \
      rhs_by_k[k][j] = src_format##_TO_FP32(rhs_bits); \
      float abs_value = src_format##_TO_FP32(FP_ABS_BITS(src_format, rhs_bits)); \
      if (abs_value > rhs_abs_max) { \
        rhs_abs_max = abs_value; \
      } \
    } \
  } \
  for (uint64_t i = 0; i < tile_m; i++) { \
    const dest_type *dest_row = cpu.macc[acc_idx][i].concat(_, dest_format##_SEW); \
    for (uint64_t j = 0; j < tile_n; j++) { \
      uint32_t dest_bits = dest_row[j]; \
      if (unlikely(!dest_format##_IS_FINITE(dest_bits))) { \
        return false; \
      } \
      float abs_value = dest_format##_TO_FP32(FP_ABS_BITS(dest_format, dest_bits)); \
      if (abs_value > dest_abs_max) { \
        dest_abs_max = abs_value; \
      } \
    } \
  } \
  /* Retain the existing overflow gate for widened FP32 accumulation. */ \
  if (dest_format##_SEW == 32 && src_format##_SEW < 32) { \
    long double result_abs_bound = dest_abs_max + \
      (long double)tile_k * lhs_abs_max * rhs_abs_max; \
    if (unlikely(result_abs_bound > FLT_MAX)) { \
      return false; \
    } \
  } \
  fenv_t saved_env; \
  if (!host_fma_env_begin(&saved_env, host_rounding_mode)) { \
    return false; \
  } \
  for (uint64_t i = 0; i < tile_m; i++) { \
    const src_type *restrict lhs_row = cpu.mtr[ts1][i].concat(_, src_format##_SEW); \
    dest_type *restrict dest_row = cpu.macc[acc_idx][i].concat(_, dest_format##_SEW); \
    float acc_values[ROWNUM]; \
    for (uint64_t j = 0; j < tile_n; j++) { \
      acc_values[j] = dest_format##_TO_FP32(dest_row[j]); \
    } \
    for (uint64_t k = 0; k < tile_k; k++) { \
      const float lhs = src_format##_TO_FP32(lhs_row[k]); \
      const float *restrict rhs_row = rhs_by_k[k]; \
      _Pragma("GCC ivdep") \
      for (uint64_t j = 0; j < tile_n; j++) { \
        acc_values[j] = __builtin_fmaf(lhs, rhs_row[j], acc_values[j]); \
      } \
    } \
    /* FP16 destinations retain FP32 intermediates and round once on store. */ \
    for (uint64_t j = 0; j < tile_n; j++) { \
      dest_row[j] = dest_format##_FROM_FP32(acc_values[j]); \
    } \
  } \
  host_fma_env_end(&saved_env); \
  return true; \
}

#if HOST_HAS_IEEE_FLOAT16
def_auto_vectorized_float_mmacc(mmacc_fp16_fp16_fp16, FP16, FP16)
#endif

def_auto_vectorized_float_mmacc(mmacc_fp8e4m3_fp8e4m3_fp32, FP8E4M3, FP32)
def_auto_vectorized_float_mmacc(mmacc_fp8e5m2_fp8e5m2_fp32, FP8E5M2, FP32)
def_auto_vectorized_float_mmacc(mmacc_fp16_fp16_fp32, FP16, FP32)
def_auto_vectorized_float_mmacc(mmacc_bf16_bf16_fp32, BF16, FP32)
def_auto_vectorized_float_mmacc(mmacc_fp32_fp32_fp32, FP32, FP32)
#undef def_auto_vectorized_float_mmacc

#endif // CONFIG_FPU_NONE

bool try_auto_vectorized_int8_mmacc(
  int td, int ts1, int ts2,
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k,
  uint64_t d_type, uint64_t s1_type, uint64_t s2_type,
  bool saturation
) {
  bool valid_types = d_type == MTYPECODE_INT32 &&
                     (s1_type == MTYPECODE_INT8 || s1_type == MTYPECODE_UINT8) &&
                     (s2_type == MTYPECODE_INT8 || s2_type == MTYPECODE_UINT8);
  const uint64_t max_safe_tile_k = INT32_MAX / (UINT8_MAX * UINT8_MAX);
  if (!valid_types || saturation || tile_k > max_safe_tile_k) {
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
#if HOST_HAS_IEEE_FLOAT16
      return mmacc_fp16_fp16_fp16(
        acc_idx, ts1, ts2, tile_m, tile_n, tile_k, host_rounding_mode);
#else
      return false;
#endif
    case FLOAT_MMACC_FP16_FP16_FP32:
      return mmacc_fp16_fp16_fp32(
        acc_idx, ts1, ts2, tile_m, tile_n, tile_k, host_rounding_mode);
    case FLOAT_MMACC_BF16_BF16_FP32:
      return mmacc_bf16_bf16_fp32(
        acc_idx, ts1, ts2, tile_m, tile_n, tile_k, host_rounding_mode);
    case FLOAT_MMACC_FP32_FP32_FP32:
      return mmacc_fp32_fp32_fp32(
        acc_idx, ts1, ts2, tile_m, tile_n, tile_k, host_rounding_mode);
    case FLOAT_MMACC_FP8E4M3_FP8E4M3_FP32:
      return mmacc_fp8e4m3_fp8e4m3_fp32(
        acc_idx, ts1, ts2, tile_m, tile_n, tile_k, host_rounding_mode);
    case FLOAT_MMACC_FP8E5M2_FP8E5M2_FP32:
      return mmacc_fp8e5m2_fp8e5m2_fp32(
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
