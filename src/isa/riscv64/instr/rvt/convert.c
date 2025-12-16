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
#include "convert.h"
#include "type.h"

bool is_integer_type(ConvertType type) { return type <= CONVERT_S64; }

static double apply_rounding_mode(double value, uint8_t mode) {
  switch (mode) {
  case TX_CONVERT_FLOOR:
    return floor(value);
  case TX_CONVERT_CEIL:
    return ceil(value);
  case TX_CONVERT_ROUND:
  default:
    return round(value);
  }
}

size_t get_type_size(ConvertType type) {
  switch (type) {
  case CONVERT_U8:
    return sizeof(uint8_t);
  case CONVERT_S8:
    return sizeof(int8_t);
  case CONVERT_U16:
    return sizeof(uint16_t);
  case CONVERT_S16:
    return sizeof(int16_t);
  case CONVERT_U32:
    return sizeof(uint32_t);
  case CONVERT_S32:
    return sizeof(int32_t);
  case CONVERT_U64:
    return sizeof(uint64_t);
  case CONVERT_S64:
    return sizeof(int64_t);
  case CONVERT_FP16:
    return sizeof(uint16_t);
  case CONVERT_BF16:
    return sizeof(uint16_t);
  case CONVERT_FP32:
    return sizeof(float);
  case CONVERT_TF32:
    return sizeof(float);
  case CONVERT_FP64:
    return sizeof(double);
  case CONVERT_E4M3:
    return sizeof(uint8_t);
  default:
    return 0;
  }
}

static void convert_value(const void *src, ConvertType src_type, void *dst,
                          ConvertType dst_type, uint8_t mode) {
  double temp = 0.0;

  // Source conversion to double
  switch (src_type) {
  case CONVERT_U8:
    temp = *(const uint8_t *)src;
    break;
  case CONVERT_S8:
    temp = *(const int8_t *)src;
    break;
  case CONVERT_U16:
    temp = *(const uint16_t *)src;
    break;
  case CONVERT_S16:
    temp = *(const int16_t *)src;
    break;
  case CONVERT_U32:
    temp = *(const uint32_t *)src;
    break;
  case CONVERT_S32:
    temp = *(const int32_t *)src;
    break;
  case CONVERT_U64:
    temp = *(const uint64_t *)src;
    break;
  case CONVERT_S64:
    temp = *(const int64_t *)src;
    break;
  case CONVERT_FP16:
    temp = fp16_to_fp32(*(const uint16_t *)src);
    break;
  case CONVERT_BF16:
    temp = bf16_to_fp32(*(const uint16_t *)src);
    break;
  case CONVERT_FP32:
    temp = *(const float *)src;
    break;
  case CONVERT_TF32:
    temp = *(const float *)src;
    break;
  case CONVERT_FP64:
    temp = *(const double *)src;
    break;
  default:
    // TODO. It should throw an exception.
    return;
  }

  // Destination conversion with rounding mode
  switch (dst_type) {
  case CONVERT_U8:
    *(uint8_t *)dst =
        (uint8_t)apply_rounding_mode(fmax(0, fmin(temp, UINT8_MAX)), mode);
    break;
  case CONVERT_S8:
    *(int8_t *)dst =
        (int8_t)apply_rounding_mode(fmax(INT8_MIN, fmin(temp, INT8_MAX)), mode);
    break;
  case CONVERT_U16:
    *(uint16_t *)dst =
        (uint16_t)apply_rounding_mode(fmax(0, fmin(temp, UINT16_MAX)), mode);
    break;
  case CONVERT_S16:
    *(int16_t *)dst = (int16_t)apply_rounding_mode(
        fmax(INT16_MIN, fmin(temp, INT16_MAX)), mode);
    break;
  case CONVERT_U32:
    *(uint32_t *)dst =
        (uint32_t)apply_rounding_mode(fmax(0, fmin(temp, UINT32_MAX)), mode);
    break;
  case CONVERT_S32:
    *(int32_t *)dst = (int32_t)apply_rounding_mode(
        fmax(INT32_MIN, fmin(temp, INT32_MAX)), mode);
    break;
  case CONVERT_U64:
    *(uint64_t *)dst =
        (uint64_t)apply_rounding_mode(fmax(0, fmin(temp, (double)UINT64_MAX)), mode);
    break;
  case CONVERT_S64:
    *(int64_t *)dst = (int64_t)apply_rounding_mode(
        fmax(INT64_MIN, fmin(temp, (double)INT64_MAX)), mode);
    break;
  case CONVERT_FP16:
    *(uint16_t *)dst = fp32_to_fp16_with_mode((float)temp, mode);
    break;
  case CONVERT_BF16:
    *(uint16_t *)dst = fp32_to_bf16_with_mode((float)temp, mode);
    break;
  case CONVERT_FP32:
    *(float *)dst = (float)temp;
    break;
  case CONVERT_TF32:
    *(float *)dst = (float)temp; // TF32 is treated same as FP32 here
    break;
  case CONVERT_FP64:
    *(double *)dst = temp;
    break;
  case CONVERT_E4M3:
    *(uint8_t *)dst = fp32_to_fp8_e4m3_with_mode((float)temp, mode);
    break;
  default:
    // TODO. It should throw an exception.
    break;
  }
}

bool tensor_convert(void *input, void *output, int32_t input_dims[4],
                    int32_t input_strides[4], int32_t output_strides[4],
                    ConvertType in_dtype, ConvertType out_dtype, uint8_t mode) {
  if (!input || !output) {
    return false;
  }

  // Skip if both types are integer (float-to-float is allowed)
  if (is_integer_type(in_dtype) && is_integer_type(out_dtype)) {
    return false;
  }

  size_t in_type_size = get_type_size(in_dtype);
  size_t out_type_size = get_type_size(out_dtype);
  if (in_type_size == 0 || out_type_size == 0) {
    return false;
  }
  for (int l = 0; l < input_dims[3]; l++) {
    for (int k = 0; k < input_dims[2]; k++) {
      for (int j = 0; j < input_dims[1]; j++) {
        for (int i = 0; i < input_dims[0]; i++) {
          const char *in_ptr = (const char *)input +
                               l * input_strides[3] * in_type_size +
                               k * input_strides[2] * in_type_size +
                               j * input_strides[1] * in_type_size +
                               i * input_strides[0] * in_type_size;

          char *out_ptr = (char *)output +
                          l * output_strides[3] * out_type_size +
                          k * output_strides[2] * out_type_size +
                          j * output_strides[1] * out_type_size +
                          i * output_strides[0] * out_type_size;

          convert_value(in_ptr, in_dtype, out_ptr, out_dtype, mode);
        }
      }
    }
  }
  return true;
}
#endif
