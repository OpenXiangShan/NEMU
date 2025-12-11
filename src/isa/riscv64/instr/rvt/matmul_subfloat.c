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

#include "matmul_subfloat.h"
#include "type.h"

// ======================= TYPE DEFINITIONS =======================
typedef float (*convert_func_t)(void *data, int idx);
typedef float (*load_func_t)(void *data, int idx);
typedef void (*store_func_t)(void *data, int idx, float value);

// ======================= CORE KERNEL =======================
static inline void matmul_accum_kernel(void *input0, void *input1, void *output,
                                       const MatMulConfig *config,
                                       const MatMulStrideConfig *strides,
                                       convert_func_t convert0,
                                       convert_func_t convert1,
                                       load_func_t load_output,
                                       store_func_t store_output) {

  // Unpack configuration
  int input0_dim0 = config->input0_dim0;
  int input0_dim1 = config->input0_dim1;
  int input1_dim0 = config->input1_dim0;
  int input1_dim1 = config->input1_dim1;

  // Unpack strides
  int input0_stride0 = strides->input0_stride0;
  int input0_stride1 = strides->input0_stride1;
  int input1_stride0 = strides->input1_stride0;
  int input1_stride1 = strides->input1_stride1;
  int output_stride0 = strides->output_stride0;
  int output_stride1 = strides->output_stride1;

  if (input0_dim1 != input1_dim0) {
    return;
  }

  for (int i = 0; i < input0_dim0; i++) {
    for (int j = 0; j < input1_dim1; j++) {
      int out_idx = i * output_stride0 + j * output_stride1;
      float sum = load_output(output, out_idx);

      for (int k = 0; k < input0_dim1; k++) {
        int idx0 = i * input0_stride0 + k * input0_stride1;
        int idx1 = k * input1_stride0 + j * input1_stride1;

        float a = convert0(input0, idx0);
        float b = convert1(input1, idx1);
        sum += a * b;
      }
      store_output(output, out_idx, sum);
    }
  }
}

// ======================= DECOMPRESSION UTILITIES =======================
static inline float fp_uncompress(int exp, int frac, int is_unsigned,
                                  uint32_t raw) {
  assert(exp <= 8 && frac <= 23);

  int sign = !is_unsigned ? (raw >> (exp + frac)) & 0x1 : 0;
  int exponent = (raw >> frac) & ((1 << exp) - 1);
  int mantissa = raw & ((1 << frac) - 1);

  // Handle denormals and special values
  if (exponent == 0) {
    if (mantissa != 0) {
      // Denormal
      exponent = 1 - (1 << (exp - 1)) - 1;
    } else {
      // Zero
      return sign ? -0.0f : 0.0f;
    }
  } else {
    exponent += 127 - (1 << (exp - 1)) - 1;
  }

  int result = (sign << 31) | (exponent << 23) | (mantissa << (23 - frac));
  float r = 0;
  memcpy(&r, &result, sizeof(float));
  return r;
}

// ======================= CONVERSION FUNCTIONS =======================
/* ue1m7 (unsigned exp1 mant7) */
static float convert_ue1m7(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx] & 0x7F;
  return fp_uncompress(1, 7, 1, val);
}

/* ue1m6 (unsigned exp1 mant6) */
static float convert_ue1m6(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx] & 0x3F;
  return fp_uncompress(1, 6, 1, val);
}

/* ue1m5 (unsigned exp1 mant5) */
static float convert_ue1m5(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx] & 0x1F;
  return fp_uncompress(1, 5, 1, val);
}

/* ue1m4 (unsigned exp1 mant4) */
static float convert_ue1m4(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx] & 0x0F;
  return fp_uncompress(1, 4, 1, val);
}

/* ue1m3 (unsigned exp1 mant3) */
static float convert_ue1m3(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx] & 0x07;
  return fp_uncompress(1, 3, 1, val);
}

/* ue1m2 (unsigned exp1 mant2) */
static float convert_ue1m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx] & 0x03;
  return fp_uncompress(1, 2, 1, val);
}

/* ue1m1 (unsigned exp1 mant1) */
static float convert_ue1m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx] & 0x01;
  return fp_uncompress(1, 1, 1, val);
}

/* ue1m0 (unsigned exp1 mant0) */
static float convert_ue1m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx] & 0x01;
  return fp_uncompress(1, 0, 1, val);
}

/* ue2m6 (unsigned exp2 mant6) */
static float convert_ue2m6(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 6, 1, val);
}

/* ue2m5 (unsigned exp2 mant5) */
static float convert_ue2m5(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 5, 1, val);
}

/* ue2m4 (unsigned exp2 mant4) */
static float convert_ue2m4(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 4, 1, val);
}

/* ue2m3 (unsigned exp2 mant3) */
static float convert_ue2m3(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 3, 1, val);
}

/* ue2m2 (unsigned exp2 mant2) */
static float convert_ue2m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 2, 1, val);
}

/* ue2m1 (unsigned exp2 mant1) */
static float convert_ue2m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 1, 1, val);
}

/* ue2m0 (unsigned exp2 mant0) */
static float convert_ue2m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 0, 1, val);
}

/* ue3m5 (unsigned exp3 mant5) */
static float convert_ue3m5(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 5, 1, val);
}

/* ue3m4 (unsigned exp3 mant4) */
static float convert_ue3m4(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 4, 1, val);
}

/* ue3m3 (unsigned exp3 mant3) */
static float convert_ue3m3(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 3, 1, val);
}

/* ue3m2 (unsigned exp3 mant2) */
static float convert_ue3m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 2, 1, val);
}

/* ue3m1 (unsigned exp3 mant1) */
static float convert_ue3m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 1, 1, val);
}

/* ue3m0 (unsigned exp3 mant0) */
static float convert_ue3m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 0, 1, val);
}

/* ue4m4 (unsigned exp4 mant4) */
static float convert_ue4m4(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(4, 4, 1, val);
}

/* ue4m3 (unsigned exp4 mant3) */
static float convert_ue4m3(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(4, 3, 1, val);
}

/* ue4m2 (unsigned exp4 mant2) */
static float convert_ue4m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(4, 2, 1, val);
}

/* ue4m1 (unsigned exp4 mant1) */
static float convert_ue4m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(4, 1, 1, val);
}

/* ue4m0 (unsigned exp4 mant0) */
static float convert_ue4m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(4, 0, 1, val);
}

/* ue5m3 (unsigned exp5 mant3) */
static float convert_ue5m3(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(5, 3, 1, val);
}

/* ue5m2 (unsigned exp5 mant2) */
static float convert_ue5m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(5, 2, 1, val);
}

/* ue5m1 (unsigned exp5 mant1) */
static float convert_ue5m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(5, 1, 1, val);
}

/* ue5m0 (unsigned exp5 mant0) */
static float convert_ue5m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(5, 0, 1, val);
}

/* ue6m2 (unsigned exp6 mant2) */
static float convert_ue6m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(6, 2, 1, val);
}

/* ue6m1 (unsigned exp6 mant1) */
static float convert_ue6m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(6, 1, 1, val);
}

/* ue6m0 (unsigned exp6 mant0) */
static float convert_ue6m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(6, 0, 1, val);
}

/* ue7m1 (unsigned exp7 mant1) */
static float convert_ue7m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(7, 1, 1, val);
}

/* ue7m0 (unsigned exp7 mant0) */
static float convert_ue7m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(7, 0, 1, val);
}

/* ue8m0 (unsigned exp8 mant0) */
static float convert_ue8m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(8, 0, 1, val);
}

/* e1m6 (signed exp1 mant6) */
static float convert_e1m6(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(1, 6, 0, val);
}

/* e1m5 (signed exp1 mant5) */
static float convert_e1m5(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(1, 5, 0, val);
}

/* e1m4 (signed exp1 mant4) */
static float convert_e1m4(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(1, 4, 0, val);
}

/* e1m3 (signed exp1 mant3) */
static float convert_e1m3(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(1, 3, 0, val);
}

/* e1m2 (signed exp1 mant2) */
static float convert_e1m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(1, 2, 0, val);
}

/* e1m1 (signed exp1 mant1) */
static float convert_e1m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(1, 1, 0, val);
}

/* e1m0 (signed exp1 mant0) */
static float convert_e1m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(1, 0, 0, val);
}

/* e2m5 (signed exp2 mant5) */
static float convert_e2m5(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 5, 0, val);
}

/* e2m4 (signed exp2 mant4) */
static float convert_e2m4(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 4, 0, val);
}

/* e2m3 (signed exp2 mant3) */
static float convert_e2m3(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 3, 0, val);
}

/* e2m2 (signed exp2 mant2) */
static float convert_e2m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 2, 0, val);
}

/* e2m1 (signed exp2 mant1) */
static float convert_e2m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 1, 0, val);
}

/* e2m0 (signed exp2 mant0) */
static float convert_e2m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(2, 0, 0, val);
}

/* e3m4 (signed exp3 mant4) */
static float convert_e3m4(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 4, 0, val);
}

/* e3m3 (signed exp3 mant3) */
static float convert_e3m3(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 3, 0, val);
}

/* e3m2 (signed exp3 mant2) */
static float convert_e3m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 2, 0, val);
}

/* e3m1 (signed exp3 mant1) */
static float convert_e3m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 1, 0, val);
}

/* e3m0 (signed exp3 mant0) */
static float convert_e3m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(3, 0, 0, val);
}

/* e4m3 (signed exp4 mant3) */
static float convert_e4m3(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp8_e4m3_to_fp32(val);
}

/* e4m2 (signed exp4 mant2) */
static float convert_e4m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(4, 2, 0, val);
}

/* e4m1 (signed exp4 mant1) */
static float convert_e4m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(4, 1, 0, val);
}

/* e4m0 (signed exp4 mant0) */
static float convert_e4m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(4, 0, 0, val);
}

/* e5m2 (signed exp5 mant2) */
static float convert_e5m2(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(5, 2, 0, val);
}

/* e5m1 (signed exp5 mant1) */
static float convert_e5m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(5, 1, 0, val);
}

/* e5m0 (signed exp5 mant0) */
static float convert_e5m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(5, 0, 0, val);
}

/* e6m1 (signed exp6 mant1) */
static float convert_e6m1(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(6, 1, 0, val);
}

/* e6m0 (signed exp6 mant0) */
static float convert_e6m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(6, 0, 0, val);
}

/* e7m0 (signed exp7 mant0) */
static float convert_e7m0(void *data, int idx) {
  uint8_t val = ((uint8_t *)data)[idx];
  return fp_uncompress(7, 0, 0, val);
}

// ======================= STORAGE FUNCTIONS =======================

static float load_fp32(void *data, int idx) { return ((float *)data)[idx]; }

static void store_fp32(void *data, int idx, float value) {
  ((float *)data)[idx] = value;
}

static float load_fp16(void *data, int idx) {
  uint16_t val = ((uint16_t *)data)[idx];
  return fp_uncompress(5, 10, 0, val);
}

static void store_fp16(void *data, int idx, float value) {
  uint32_t f32 = 0;
  memcpy(&f32, &value, sizeof(float));
  uint16_t f16 = ((f32 >> 16) & 0x8000) |
                 (((f32 & 0x7F800000) - 0x38000000) >> 13) |
                 ((f32 >> 13) & 0x03FF);
  ((uint16_t *)data)[idx] = f16;
}

// ======================= KERNEL DEFINITIONS =======================
#define DEFINE_SUBFLOAT_KERNEL(in0_type, in1_type, out_type)                   \
  void matmul_##out_type##_##in0_type##_##in1_type(                            \
      void *i0, void *i1, void *o, const MatMulConfig *config,                 \
      const MatMulStrideConfig *strides) {                                     \
    matmul_accum_kernel(i0, i1, o, config, strides, convert_##in0_type,        \
                        convert_##in1_type, load_##out_type,                   \
                        store_##out_type);                                     \
  }

DEFINE_SUBFLOAT_KERNEL(ue1m7, ue1m7, fp32)
DEFINE_SUBFLOAT_KERNEL(ue1m7, ue1m7, fp16)
DEFINE_SUBFLOAT_KERNEL(e1m6, e1m6, fp32)
DEFINE_SUBFLOAT_KERNEL(e1m6, e1m6, fp16)
DEFINE_SUBFLOAT_KERNEL(e1m5, e1m5, fp32)
DEFINE_SUBFLOAT_KERNEL(e1m5, e1m5, fp16)
DEFINE_SUBFLOAT_KERNEL(e1m4, e1m4, fp32)
DEFINE_SUBFLOAT_KERNEL(e1m4, e1m4, fp16)
DEFINE_SUBFLOAT_KERNEL(e1m3, e1m3, fp32)
DEFINE_SUBFLOAT_KERNEL(e1m3, e1m3, fp16)
DEFINE_SUBFLOAT_KERNEL(e1m2, e1m2, fp32)
DEFINE_SUBFLOAT_KERNEL(e1m2, e1m2, fp16)
DEFINE_SUBFLOAT_KERNEL(e1m1, e1m1, fp32)
DEFINE_SUBFLOAT_KERNEL(e1m1, e1m1, fp16)
DEFINE_SUBFLOAT_KERNEL(e1m0, e1m0, fp32)
DEFINE_SUBFLOAT_KERNEL(e1m0, e1m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue1m6, ue1m6, fp32)
DEFINE_SUBFLOAT_KERNEL(ue1m6, ue1m6, fp16)
DEFINE_SUBFLOAT_KERNEL(ue2m6, ue2m6, fp32)
DEFINE_SUBFLOAT_KERNEL(ue2m6, ue2m6, fp16)
DEFINE_SUBFLOAT_KERNEL(e2m5, e2m5, fp32)
DEFINE_SUBFLOAT_KERNEL(e2m5, e2m5, fp16)
DEFINE_SUBFLOAT_KERNEL(e2m4, e2m4, fp32)
DEFINE_SUBFLOAT_KERNEL(e2m4, e2m4, fp16)
DEFINE_SUBFLOAT_KERNEL(e2m3, e2m3, fp32)
DEFINE_SUBFLOAT_KERNEL(e2m3, e2m3, fp16)
DEFINE_SUBFLOAT_KERNEL(e2m2, e2m2, fp32)
DEFINE_SUBFLOAT_KERNEL(e2m2, e2m2, fp16)
DEFINE_SUBFLOAT_KERNEL(e2m1, e2m1, fp32)
DEFINE_SUBFLOAT_KERNEL(e2m1, e2m1, fp16)
DEFINE_SUBFLOAT_KERNEL(e2m0, e2m0, fp32)
DEFINE_SUBFLOAT_KERNEL(e2m0, e2m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue1m5, ue1m5, fp32)
DEFINE_SUBFLOAT_KERNEL(ue1m5, ue1m5, fp16)
DEFINE_SUBFLOAT_KERNEL(ue2m5, ue2m5, fp32)
DEFINE_SUBFLOAT_KERNEL(ue2m5, ue2m5, fp16)
DEFINE_SUBFLOAT_KERNEL(ue3m5, ue3m5, fp32)
DEFINE_SUBFLOAT_KERNEL(ue3m5, ue3m5, fp16)
DEFINE_SUBFLOAT_KERNEL(e3m4, e3m4, fp32)
DEFINE_SUBFLOAT_KERNEL(e3m4, e3m4, fp16)
DEFINE_SUBFLOAT_KERNEL(e3m3, e3m3, fp32)
DEFINE_SUBFLOAT_KERNEL(e3m3, e3m3, fp16)
DEFINE_SUBFLOAT_KERNEL(e3m2, e3m2, fp32)
DEFINE_SUBFLOAT_KERNEL(e3m2, e3m2, fp16)
DEFINE_SUBFLOAT_KERNEL(e3m1, e3m1, fp32)
DEFINE_SUBFLOAT_KERNEL(e3m1, e3m1, fp16)
DEFINE_SUBFLOAT_KERNEL(e3m0, e3m0, fp32)
DEFINE_SUBFLOAT_KERNEL(e3m0, e3m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue1m4, ue1m4, fp32)
DEFINE_SUBFLOAT_KERNEL(ue1m4, ue1m4, fp16)
DEFINE_SUBFLOAT_KERNEL(ue2m4, ue2m4, fp32)
DEFINE_SUBFLOAT_KERNEL(ue2m4, ue2m4, fp16)
DEFINE_SUBFLOAT_KERNEL(ue3m4, ue3m4, fp32)
DEFINE_SUBFLOAT_KERNEL(ue3m4, ue3m4, fp16)
DEFINE_SUBFLOAT_KERNEL(ue4m4, ue4m4, fp32)
DEFINE_SUBFLOAT_KERNEL(ue4m4, ue4m4, fp16)
DEFINE_SUBFLOAT_KERNEL(e4m3, e4m3, fp32)
DEFINE_SUBFLOAT_KERNEL(e4m3, e4m3, fp16)
DEFINE_SUBFLOAT_KERNEL(e4m2, e4m2, fp32)
DEFINE_SUBFLOAT_KERNEL(e4m2, e4m2, fp16)
DEFINE_SUBFLOAT_KERNEL(e4m1, e4m1, fp32)
DEFINE_SUBFLOAT_KERNEL(e4m1, e4m1, fp16)
DEFINE_SUBFLOAT_KERNEL(e4m0, e4m0, fp32)
DEFINE_SUBFLOAT_KERNEL(e4m0, e4m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue1m3, ue1m3, fp32)
DEFINE_SUBFLOAT_KERNEL(ue1m3, ue1m3, fp16)
DEFINE_SUBFLOAT_KERNEL(ue2m3, ue2m3, fp32)
DEFINE_SUBFLOAT_KERNEL(ue2m3, ue2m3, fp16)
DEFINE_SUBFLOAT_KERNEL(ue3m3, ue3m3, fp32)
DEFINE_SUBFLOAT_KERNEL(ue3m3, ue3m3, fp16)
DEFINE_SUBFLOAT_KERNEL(ue4m3, ue4m3, fp32)
DEFINE_SUBFLOAT_KERNEL(ue4m3, ue4m3, fp16)
DEFINE_SUBFLOAT_KERNEL(ue5m3, ue5m3, fp32)
DEFINE_SUBFLOAT_KERNEL(ue5m3, ue5m3, fp16)
DEFINE_SUBFLOAT_KERNEL(e5m2, e5m2, fp32)
DEFINE_SUBFLOAT_KERNEL(e5m2, e5m2, fp16)
DEFINE_SUBFLOAT_KERNEL(e5m1, e5m1, fp32)
DEFINE_SUBFLOAT_KERNEL(e5m1, e5m1, fp16)
DEFINE_SUBFLOAT_KERNEL(e5m0, e5m0, fp32)
DEFINE_SUBFLOAT_KERNEL(e5m0, e5m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue1m2, ue1m2, fp32)
DEFINE_SUBFLOAT_KERNEL(ue1m2, ue1m2, fp16)
DEFINE_SUBFLOAT_KERNEL(ue2m2, ue2m2, fp32)
DEFINE_SUBFLOAT_KERNEL(ue2m2, ue2m2, fp16)
DEFINE_SUBFLOAT_KERNEL(ue3m2, ue3m2, fp32)
DEFINE_SUBFLOAT_KERNEL(ue3m2, ue3m2, fp16)
DEFINE_SUBFLOAT_KERNEL(ue4m2, ue4m2, fp32)
DEFINE_SUBFLOAT_KERNEL(ue4m2, ue4m2, fp16)
DEFINE_SUBFLOAT_KERNEL(ue5m2, ue5m2, fp32)
DEFINE_SUBFLOAT_KERNEL(ue5m2, ue5m2, fp16)
DEFINE_SUBFLOAT_KERNEL(ue6m2, ue6m2, fp32)
DEFINE_SUBFLOAT_KERNEL(ue6m2, ue6m2, fp16)
DEFINE_SUBFLOAT_KERNEL(e6m1, e6m1, fp32)
DEFINE_SUBFLOAT_KERNEL(e6m1, e6m1, fp16)
DEFINE_SUBFLOAT_KERNEL(e6m0, e6m0, fp32)
DEFINE_SUBFLOAT_KERNEL(e6m0, e6m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue1m1, ue1m1, fp32)
DEFINE_SUBFLOAT_KERNEL(ue1m1, ue1m1, fp16)
DEFINE_SUBFLOAT_KERNEL(ue2m1, ue2m1, fp32)
DEFINE_SUBFLOAT_KERNEL(ue2m1, ue2m1, fp16)
DEFINE_SUBFLOAT_KERNEL(ue3m1, ue3m1, fp32)
DEFINE_SUBFLOAT_KERNEL(ue3m1, ue3m1, fp16)
DEFINE_SUBFLOAT_KERNEL(ue4m1, ue4m1, fp32)
DEFINE_SUBFLOAT_KERNEL(ue4m1, ue4m1, fp16)
DEFINE_SUBFLOAT_KERNEL(ue5m1, ue5m1, fp32)
DEFINE_SUBFLOAT_KERNEL(ue5m1, ue5m1, fp16)
DEFINE_SUBFLOAT_KERNEL(ue6m1, ue6m1, fp32)
DEFINE_SUBFLOAT_KERNEL(ue6m1, ue6m1, fp16)
DEFINE_SUBFLOAT_KERNEL(ue7m1, ue7m1, fp32)
DEFINE_SUBFLOAT_KERNEL(ue7m1, ue7m1, fp16)
DEFINE_SUBFLOAT_KERNEL(e7m0, e7m0, fp32)
DEFINE_SUBFLOAT_KERNEL(e7m0, e7m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue1m0, ue1m0, fp32)
DEFINE_SUBFLOAT_KERNEL(ue1m0, ue1m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue2m0, ue2m0, fp32)
DEFINE_SUBFLOAT_KERNEL(ue2m0, ue2m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue3m0, ue3m0, fp32)
DEFINE_SUBFLOAT_KERNEL(ue3m0, ue3m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue4m0, ue4m0, fp32)
DEFINE_SUBFLOAT_KERNEL(ue4m0, ue4m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue5m0, ue5m0, fp32)
DEFINE_SUBFLOAT_KERNEL(ue5m0, ue5m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue6m0, ue6m0, fp32)
DEFINE_SUBFLOAT_KERNEL(ue6m0, ue6m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue7m0, ue7m0, fp32)
DEFINE_SUBFLOAT_KERNEL(ue7m0, ue7m0, fp16)
DEFINE_SUBFLOAT_KERNEL(ue8m0, ue8m0, fp32)
DEFINE_SUBFLOAT_KERNEL(ue8m0, ue8m0, fp16)
#endif
