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

#ifdef CONFIG_CUSTOM_TENSOR

#ifndef RISCV_CUSTOM_TENSOR_CONVERT_H
#define RISCV_CUSTOM_TENSOR_CONVERT_H
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum {
  CONVERT_U8 = 0b1000000,
  CONVERT_S8 = 0b1000001,
  CONVERT_U16 = 0b1000010,
  CONVERT_S16 = 0b1000011,
  CONVERT_U32 = 0b1000100,
  CONVERT_S32 = 0b1000101,
  CONVERT_U64 = 0b1000110,
  CONVERT_S64 = 0b1000111,
  CONVERT_FP16 = 0b1001010,
  CONVERT_BF16 = 0b1001011,
  CONVERT_FP32 = 0b1001100,
  CONVERT_TF32 = 0b1001101,
  CONVERT_FP64 = 0b1001110,
  CONVERT_E4M3 = 0b0011100
} ConvertType;

bool is_integer_type(ConvertType type);
size_t get_type_size(ConvertType type);

bool tensor_convert(void *input, void *output, int32_t input_dims[4],
                    int32_t input_strides[4], int32_t output_strides[4],
                    ConvertType in_dtype, ConvertType out_dtype, uint8_t mode);
#endif

#endif  /* CONFIG_CUSTOM_TENSOR */
