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

#include <stdio.h>
#include "element_wise.h"

typedef uint16_t fp16_t;
typedef uint16_t bf16_t;
bool tensor_op(void *input0, void *input1, void *output, uint32_t dim0,
               uint32_t dim1, uint32_t dim2, uint32_t dim3, uint32_t stride0,
               uint32_t stride1, uint32_t stride2, uint32_t stride3,
               DataType dtype, OpType op) {
  if (!input0 || !input1 || !output || stride0 == 0 || stride1 == 0 ||
      stride2 == 0 || stride3 == 0) {
    return false;
  }

  switch (dtype) {
#define PROCESS_CASE(TYPE, OP_EXPR)                                            \
  for (uint32_t i = 0; i < dim3; ++i) {                                        \
    for (uint32_t j = 0; j < dim2; ++j) {                                      \
      for (uint32_t k = 0; k < dim1; ++k) {                                    \
        for (uint32_t l = 0; l < dim0; ++l) {                                  \
          uint32_t offset =                                                    \
              i * stride3 + j * stride2 + k * stride1 + l * stride0;           \
          ((TYPE *)output)[offset] = (TYPE)(OP_EXPR);                          \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  break

  case DTYPE_U8:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_CASE(uint8_t,
                   ((uint8_t *)input0)[offset] + ((uint8_t *)input1)[offset]);
    case OP_TENSOR_SUB:
      PROCESS_CASE(uint8_t,
                   ((uint8_t *)input0)[offset] - ((uint8_t *)input1)[offset]);
    case OP_TENSOR_MUL:
      PROCESS_CASE(uint8_t,
                   ((uint8_t *)input0)[offset] * ((uint8_t *)input1)[offset]);
    case OP_TENSOR_SRL:
      PROCESS_CASE(uint8_t,
                   ((uint8_t *)input0)[offset] >> ((uint8_t *)input1)[offset]);
    case OP_TENSOR_SLL:
      PROCESS_CASE(uint8_t,
                   ((uint8_t *)input0)[offset] << ((uint8_t *)input1)[offset]);
    case OP_TENSOR_CMP:
      PROCESS_CASE(
          uint8_t,
          ((uint8_t *)input0)[offset] > ((uint8_t *)input1)[offset] ? 1 : 0);
    default:
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_CASE(uint8_t,
                     ((~((uint8_t *)input0)[offset] & ~((uint8_t *)input1)[offset]) * (lutop & 0x01)) |
                     ((~((uint8_t *)input0)[offset] &  ((uint8_t *)input1)[offset]) * ((lutop >> 1) & 0x01)) |
                     (( ((uint8_t *)input0)[offset] & ~((uint8_t *)input1)[offset]) * ((lutop >> 2) & 0x01)) |
                     (( ((uint8_t *)input0)[offset] &  ((uint8_t *)input1)[offset]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_S8:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_CASE(int8_t,
                   ((int8_t *)input0)[offset] + ((int8_t *)input1)[offset]);
    case OP_TENSOR_SUB:
      PROCESS_CASE(int8_t,
                   ((int8_t *)input0)[offset] - ((int8_t *)input1)[offset]);
    case OP_TENSOR_MUL:
      PROCESS_CASE(int8_t,
                   ((int8_t *)input0)[offset] * ((int8_t *)input1)[offset]);
    case OP_TENSOR_SRL:
      PROCESS_CASE(int8_t, ((uint8_t)((int8_t *)input0)[offset] >>
                            ((uint8_t)((int8_t *)input1)[offset])));
    case OP_TENSOR_SRA:
      PROCESS_CASE(int8_t,
                   ((int8_t *)input0)[offset] >> ((int8_t *)input1)[offset]);
    case OP_TENSOR_SLL:
      PROCESS_CASE(int8_t,
                   ((int8_t *)input0)[offset] << ((int8_t *)input1)[offset]);
    case OP_TENSOR_CMP:
      PROCESS_CASE(int8_t,
                   ((int8_t *)input0)[offset] > ((int8_t *)input1)[offset] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_CASE(int8_t,
                     ((~((int8_t *)input0)[offset] & ~((int8_t *)input1)[offset]) * (lutop & 0x01)) |
                     ((~((int8_t *)input0)[offset] &  ((int8_t *)input1)[offset]) * ((lutop >> 1) & 0x01)) |
                     (( ((int8_t *)input0)[offset] & ~((int8_t *)input1)[offset]) * ((lutop >> 2) & 0x01)) |
                     (( ((int8_t *)input0)[offset] &  ((int8_t *)input1)[offset]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_U16:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_CASE(uint16_t,
                   ((uint16_t *)input0)[offset] + ((uint16_t *)input1)[offset]);
    case OP_TENSOR_SUB:
      PROCESS_CASE(uint16_t,
                   ((uint16_t *)input0)[offset] - ((uint16_t *)input1)[offset]);
    case OP_TENSOR_MUL:
      PROCESS_CASE(uint16_t,
                   ((uint16_t *)input0)[offset] * ((uint16_t *)input1)[offset]);
    case OP_TENSOR_SRL:
      PROCESS_CASE(uint16_t, ((uint16_t *)input0)[offset] >>
                                 ((uint16_t *)input1)[offset]);
    case OP_TENSOR_SLL:
      PROCESS_CASE(uint16_t, ((uint16_t *)input0)[offset]
                                 << ((uint16_t *)input1)[offset]);
    case OP_TENSOR_CMP:
      PROCESS_CASE(
          uint16_t,
          ((uint16_t *)input0)[offset] > ((uint16_t *)input1)[offset] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_CASE(uint16_t,
                     ((~((uint16_t *)input0)[offset] & ~((uint16_t *)input1)[offset]) * (lutop & 0x01)) |
                     ((~((uint16_t *)input0)[offset] &  ((uint16_t *)input1)[offset]) * ((lutop >> 1) & 0x01)) |
                     (( ((uint16_t *)input0)[offset] & ~((uint16_t *)input1)[offset]) * ((lutop >> 2) & 0x01)) |
                     (( ((uint16_t *)input0)[offset] &  ((uint16_t *)input1)[offset]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_S16:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_CASE(int16_t,
                   ((int16_t *)input0)[offset] + ((int16_t *)input1)[offset]);
    case OP_TENSOR_SUB:
      PROCESS_CASE(int16_t,
                   ((int16_t *)input0)[offset] - ((int16_t *)input1)[offset]);
    case OP_TENSOR_MUL:
      PROCESS_CASE(int16_t,
                   ((int16_t *)input0)[offset] * ((int16_t *)input1)[offset]);
    case OP_TENSOR_SRL:
      PROCESS_CASE(int16_t, ((uint16_t)((int16_t *)input0)[offset] >>
                             ((uint16_t)((int16_t *)input1)[offset])));
    case OP_TENSOR_SRA:
      PROCESS_CASE(int16_t,
                   ((int16_t *)input0)[offset] >> ((int16_t *)input1)[offset]);
    case OP_TENSOR_SLL:
      PROCESS_CASE(int16_t,
                   ((int16_t *)input0)[offset] << ((int16_t *)input1)[offset]);
    case OP_TENSOR_CMP:
      PROCESS_CASE(
          int16_t,
          ((int16_t *)input0)[offset] > ((int16_t *)input1)[offset] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_CASE(int16_t,
                     ((~((int16_t *)input0)[offset] & ~((int16_t *)input1)[offset]) * (lutop & 0x01)) |
                     ((~((int16_t *)input0)[offset] &  ((int16_t *)input1)[offset]) * ((lutop >> 1) & 0x01)) |
                     (( ((int16_t *)input0)[offset] & ~((int16_t *)input1)[offset]) * ((lutop >> 2) & 0x01)) |
                     (( ((int16_t *)input0)[offset] &  ((int16_t *)input1)[offset]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_U32:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_CASE(uint32_t,
                   ((uint32_t *)input0)[offset] + ((uint32_t *)input1)[offset]);
    case OP_TENSOR_SUB:
      PROCESS_CASE(uint32_t,
                   ((uint32_t *)input0)[offset] - ((uint32_t *)input1)[offset]);
    case OP_TENSOR_MUL:
      PROCESS_CASE(uint32_t,
                   ((uint32_t *)input0)[offset] * ((uint32_t *)input1)[offset]);
    case OP_TENSOR_SRL:
      PROCESS_CASE(uint32_t, ((uint32_t *)input0)[offset] >>
                                 ((uint32_t *)input1)[offset]);
    case OP_TENSOR_SLL:
      PROCESS_CASE(uint32_t, ((uint32_t *)input0)[offset]
                                 << ((uint32_t *)input1)[offset]);
    case OP_TENSOR_CMP:
      PROCESS_CASE(
          uint32_t,
          ((uint32_t *)input0)[offset] > ((uint32_t *)input1)[offset] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_CASE(uint32_t,
                     ((~((uint32_t *)input0)[offset] & ~((uint32_t *)input1)[offset]) * (lutop & 0x01)) |
                     ((~((uint32_t *)input0)[offset] &  ((uint32_t *)input1)[offset]) * ((lutop >> 1) & 0x01)) |
                     (( ((uint32_t *)input0)[offset] & ~((uint32_t *)input1)[offset]) * ((lutop >> 2) & 0x01)) |
                     (( ((uint32_t *)input0)[offset] &  ((uint32_t *)input1)[offset]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_S32:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_CASE(int32_t,
                   ((int32_t *)input0)[offset] + ((int32_t *)input1)[offset]);
    case OP_TENSOR_SUB:
      PROCESS_CASE(int32_t,
                   ((int32_t *)input0)[offset] - ((int32_t *)input1)[offset]);
    case OP_TENSOR_MUL:
      PROCESS_CASE(int32_t,
                   ((int32_t *)input0)[offset] * ((int32_t *)input1)[offset]);
    case OP_TENSOR_SRL:
      PROCESS_CASE(int32_t, ((uint32_t)((int32_t *)input0)[offset] >>
                             ((uint32_t)((int32_t *)input1)[offset])));
    case OP_TENSOR_SRA:
      PROCESS_CASE(int32_t,
                   ((int32_t *)input0)[offset] >> ((int32_t *)input1)[offset]);
    case OP_TENSOR_SLL:
      PROCESS_CASE(int32_t,
                   ((int32_t *)input0)[offset] << ((int32_t *)input1)[offset]);
    case OP_TENSOR_CMP:
      PROCESS_CASE(
          int32_t,
          ((int32_t *)input0)[offset] > ((int32_t *)input1)[offset] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_CASE(int32_t,
                     ((~((int32_t *)input0)[offset] & ~((int32_t *)input1)[offset]) * (lutop & 0x01)) |
                     ((~((int32_t *)input0)[offset] &  ((int32_t *)input1)[offset]) * ((lutop >> 1) & 0x01)) |
                     (( ((int32_t *)input0)[offset] & ~((int32_t *)input1)[offset]) * ((lutop >> 2) & 0x01)) |
                     (( ((int32_t *)input0)[offset] &  ((int32_t *)input1)[offset]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_U64:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_CASE(uint64_t,
                   ((uint64_t *)input0)[offset] + ((uint64_t *)input1)[offset]);
    case OP_TENSOR_SUB:
      PROCESS_CASE(uint64_t,
                   ((uint64_t *)input0)[offset] - ((uint64_t *)input1)[offset]);
    case OP_TENSOR_MUL:
      PROCESS_CASE(uint64_t,
                   ((uint64_t *)input0)[offset] * ((uint64_t *)input1)[offset]);
    case OP_TENSOR_SRL:
      PROCESS_CASE(uint64_t, ((uint64_t *)input0)[offset] >>
                                 ((uint64_t *)input1)[offset]);
    case OP_TENSOR_SLL:
      PROCESS_CASE(uint64_t, ((uint64_t *)input0)[offset]
                                 << ((uint64_t *)input1)[offset]);
    case OP_TENSOR_CMP:
      PROCESS_CASE(
          uint64_t,
          ((uint64_t *)input0)[offset] > ((uint64_t *)input1)[offset] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_CASE(uint64_t,
                     ((~((uint64_t *)input0)[offset] & ~((uint64_t *)input1)[offset]) * (lutop & 0x01)) |
                     ((~((uint64_t *)input0)[offset] &  ((uint64_t *)input1)[offset]) * ((lutop >> 1) & 0x01)) |
                     (( ((uint64_t *)input0)[offset] & ~((uint64_t *)input1)[offset]) * ((lutop >> 2) & 0x01)) |
                     (( ((uint64_t *)input0)[offset] &  ((uint64_t *)input1)[offset]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_S64:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_CASE(int64_t,
                   ((int64_t *)input0)[offset] + ((int64_t *)input1)[offset]);
    case OP_TENSOR_SUB:
      PROCESS_CASE(int64_t,
                   ((int64_t *)input0)[offset] - ((int64_t *)input1)[offset]);
    case OP_TENSOR_MUL:
      PROCESS_CASE(int64_t,
                   ((int64_t *)input0)[offset] * ((int64_t *)input1)[offset]);
    case OP_TENSOR_SRL:
      PROCESS_CASE(int64_t, ((uint64_t)((int64_t *)input0)[offset] >> ((uint64_t)((int64_t *)input1)[offset])));
    case OP_TENSOR_SRA:
      PROCESS_CASE(int64_t, ((int64_t *)input0)[offset] >> ((int64_t *)input1)[offset]);
    case OP_TENSOR_SLL:
      PROCESS_CASE(int64_t, ((int64_t *)input0)[offset] << ((int64_t *)input1)[offset]);
    case OP_TENSOR_CMP:
      PROCESS_CASE(
          int64_t,
          ((int64_t *)input0)[offset] > ((int64_t *)input1)[offset] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_CASE(int64_t,
                     ((~((int64_t *)input0)[offset] & ~((int64_t *)input1)[offset]) * (lutop & 0x01)) |
                     ((~((int64_t *)input0)[offset] &  ((int64_t *)input1)[offset]) * ((lutop >> 1) & 0x01)) |
                     (( ((int64_t *)input0)[offset] & ~((int64_t *)input1)[offset]) * ((lutop >> 2) & 0x01)) |
                     (( ((int64_t *)input0)[offset] &  ((int64_t *)input1)[offset]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  default:
    return false;
  }
  return true;
}

bool tensor_op_fp(void *input0, void *input1, void *output, uint32_t dim0,
                  uint32_t dim1, uint32_t dim2, uint32_t dim3, uint32_t stride0,
                  uint32_t stride1, uint32_t stride2, uint32_t stride3,
                  DataType dtype, OpType op) {
  if (!input0 || !input1 || !output || stride0 == 0 || stride1 == 0 ||
      stride2 == 0 || stride3 == 0) {
    return false;
  }

  switch (dtype) {
#define PROCESS_CASE(TYPE, OP_EXPR)                                            \
  for (uint32_t i = 0; i < dim3; ++i) {                                        \
    for (uint32_t j = 0; j < dim2; ++j) {                                      \
      for (uint32_t k = 0; k < dim1; ++k) {                                    \
        for (uint32_t l = 0; l < dim0; ++l) {                                  \
          uint32_t offset =                                                    \
              i * stride3 + j * stride2 + k * stride1 + l * stride0;           \
          ((TYPE *)output)[offset] = (TYPE)(OP_EXPR);                          \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  break

  case DTYPE_FP16:
    switch (op) {
    case OP_FPADD:
      PROCESS_CASE(fp16_t,
                   ((fp16_t *)input0)[offset] + ((fp16_t *)input1)[offset]);
    case OP_FPSUB:
      PROCESS_CASE(fp16_t,
                   ((fp16_t *)input0)[offset] - ((fp16_t *)input1)[offset]);
    case OP_FPMUL:
      PROCESS_CASE(fp16_t,
                   ((fp16_t *)input0)[offset] * ((fp16_t *)input1)[offset]);
    default:
      return false;
    }
    break;
  case DTYPE_BF16:
    switch (op) {
    case OP_FPADD:
      PROCESS_CASE(bf16_t,
                   ((bf16_t *)input0)[offset] + ((bf16_t *)input1)[offset]);
    case OP_FPSUB:
      PROCESS_CASE(bf16_t,
                   ((bf16_t *)input0)[offset] - ((bf16_t *)input1)[offset]);
    case OP_FPMUL:
      PROCESS_CASE(bf16_t,
                   ((bf16_t *)input0)[offset] * ((bf16_t *)input1)[offset]);
    default:
      return false;
    }
    break;
  case DTYPE_FP32:
    switch (op) {
    case OP_FPADD:
      PROCESS_CASE(float,
                   ((float *)input0)[offset] + ((float *)input1)[offset]);
    case OP_FPSUB:
      PROCESS_CASE(float,
                   ((float *)input0)[offset] - ((float *)input1)[offset]);
    case OP_FPMUL:
      PROCESS_CASE(float,
                   ((float *)input0)[offset] * ((float *)input1)[offset]);
    default:
      return false;
    }
    break;
  // TODO
  case DTYPE_TF32:
    switch (op) {
    case OP_FPADD:
      PROCESS_CASE(float,
                   ((float *)input0)[offset] + ((float *)input1)[offset]);
    case OP_FPSUB:
      PROCESS_CASE(float,
                   ((float *)input0)[offset] - ((float *)input1)[offset]);
    case OP_FPMUL:
      PROCESS_CASE(float,
                   ((float *)input0)[offset] * ((float *)input1)[offset]);
    default:
      return false;
    }
    break;
  case DTYPE_FP64:
    switch (op) {
    case OP_FPADD:
      PROCESS_CASE(double,
                   ((double *)input0)[offset] + ((double *)input1)[offset]);
    case OP_FPSUB:
      PROCESS_CASE(double,
                   ((double *)input0)[offset] - ((double *)input1)[offset]);
    case OP_FPMUL:
      PROCESS_CASE(double,
                   ((double *)input0)[offset] * ((double *)input1)[offset]);
    default:
      return false;
    }
  default:
    return false;
  }
  return true;
}

bool tensor_op_broadcast(void *input0, void *input1, void *output,
                         int32_t dim1[4], int32_t dim2[4], int32_t stride1[4],
                         int32_t stride2[4], DataType dtype, OpType op) {
  if (!input0 || !input1 || !output) {
    printf("TX_INTB: 1\n");
    return false;
  }

  for (int i = 0; i < 4; i++) {
    if (dim1[i] != dim2[i] && dim1[i] != 1 && dim2[i] != 1) {
      printf("TX_INTB: 2\n");
      return false;
    }
  }

  int32_t out_dim[4];
  for (int i = 0; i < 4; i++) {
    out_dim[i] = dim1[i] > dim2[i] ? dim1[i] : dim2[i];
  }

  switch (dtype) {
#define PROCESS_BROADCAST_CASE(TYPE, OP_EXPR)                                  \
 for (int32_t l = 0; l < out_dim[3]; ++l) {                                   \
    int32_t idx1_3 = dim1[3] == 1 ? 0 : l;                                     \
    int32_t idx2_3 = dim2[3] == 1 ? 0 : l;                                     \
    for (int32_t k = 0; k < out_dim[2]; ++k) {                                 \
      int32_t idx1_2 = dim1[2] == 1 ? 0 : k;                                   \
      int32_t idx2_2 = dim2[2] == 1 ? 0 : k;                                   \
      for (int32_t j = 0; j < out_dim[1]; ++j) {                               \
        int32_t idx1_1 = dim1[1] == 1 ? 0 : j;                                 \
        int32_t idx2_1 = dim2[1] == 1 ? 0 : j;                                 \
        for (int32_t i = 0; i < out_dim[0]; ++i) {                             \
          int32_t idx1_0 = dim1[0] == 1 ? 0 : i;                               \
          int32_t idx2_0 = dim2[0] == 1 ? 0 : i;                               \
          int32_t offset0 = idx1_0 * stride1[0] + idx1_1 * stride1[1] +        \
                            idx1_2 * stride1[2] + idx1_3 * stride1[3];         \
          int32_t offset1 = idx2_0 * stride2[0] + idx2_1 * stride2[1] +       \
                            idx2_2 * stride2[2] + idx2_3 * stride2[3];         \
          int32_t out_offset = i * stride1[0] + j * stride1[1] +               \
                               k * stride1[2] + l * stride1[3];                \
          ((TYPE *)output)[out_offset] = (TYPE)(OP_EXPR);                      \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  return true;

  case DTYPE_U8:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(uint8_t,
                   ((uint8_t *)input0)[offset0] + ((uint8_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(uint8_t,
                   ((uint8_t *)input0)[offset0] - ((uint8_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(uint8_t,
                   ((uint8_t *)input0)[offset0] * ((uint8_t *)input1)[offset1]);
    case OP_TENSOR_SRL:
      PROCESS_BROADCAST_CASE(uint8_t,
                   ((uint8_t *)input0)[offset0] >> ((uint8_t *)input1)[offset1]);
    case OP_TENSOR_SLL:
      PROCESS_BROADCAST_CASE(uint8_t,
                   ((uint8_t *)input0)[offset0] << ((uint8_t *)input1)[offset1]);
    case OP_TENSOR_CMP:
      PROCESS_BROADCAST_CASE(
          uint8_t,
          ((uint8_t *)input0)[offset0] > ((uint8_t *)input1)[offset1] ? 1 : 0);
    default:
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_BROADCAST_CASE(uint8_t,
                     ((~((uint8_t *)input0)[offset0] & ~((uint8_t *)input1)[offset1]) * (lutop & 0x01)) |
                     ((~((uint8_t *)input0)[offset0] &  ((uint8_t *)input1)[offset1]) * ((lutop >> 1) & 0x01)) |
                     (( ((uint8_t *)input0)[offset0] & ~((uint8_t *)input1)[offset1]) * ((lutop >> 2) & 0x01)) |
                     (( ((uint8_t *)input0)[offset0] &  ((uint8_t *)input1)[offset1]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_S8:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(int8_t,
                   ((int8_t *)input0)[offset0] + ((int8_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(int8_t,
                   ((int8_t *)input0)[offset0] - ((int8_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(int8_t,
                   ((int8_t *)input0)[offset0] * ((int8_t *)input1)[offset1]);
    case OP_TENSOR_SRL:
      PROCESS_BROADCAST_CASE(int8_t, ((uint8_t)((int8_t *)input0)[offset0] >>
                            ((uint8_t)((int8_t *)input1)[offset1])));
    case OP_TENSOR_SRA:
      PROCESS_BROADCAST_CASE(int8_t,
                   ((int8_t *)input0)[offset0] >> ((int8_t *)input1)[offset1]);
    case OP_TENSOR_SLL:
      PROCESS_BROADCAST_CASE(int8_t,
                   ((int8_t *)input0)[offset0] << ((int8_t *)input1)[offset1]);
    case OP_TENSOR_CMP:
      PROCESS_BROADCAST_CASE(int8_t,
                   ((int8_t *)input0)[offset0] > ((int8_t *)input1)[offset1] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_BROADCAST_CASE(int8_t,
                     ((~((int8_t *)input0)[offset0] & ~((int8_t *)input1)[offset1]) * (lutop & 0x01)) |
                     ((~((int8_t *)input0)[offset0] &  ((int8_t *)input1)[offset1]) * ((lutop >> 1) & 0x01)) |
                     (( ((int8_t *)input0)[offset0] & ~((int8_t *)input1)[offset1]) * ((lutop >> 2) & 0x01)) |
                     (( ((int8_t *)input0)[offset0] &  ((int8_t *)input1)[offset1]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_U16:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(uint16_t,
                   ((uint16_t *)input0)[offset0] + ((uint16_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(uint16_t,
                   ((uint16_t *)input0)[offset0] - ((uint16_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(uint16_t,
                   ((uint16_t *)input0)[offset0] * ((uint16_t *)input1)[offset1]);
    case OP_TENSOR_SRL:
      PROCESS_BROADCAST_CASE(uint16_t, ((uint16_t *)input0)[offset0] >>
                                 ((uint16_t *)input1)[offset1]);
    case OP_TENSOR_SLL:
      PROCESS_BROADCAST_CASE(uint16_t, ((uint16_t *)input0)[offset0]
                                 << ((uint16_t *)input1)[offset1]);
    case OP_TENSOR_CMP:
      PROCESS_BROADCAST_CASE(
          uint16_t,
          ((uint16_t *)input0)[offset0] > ((uint16_t *)input1)[offset1] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_BROADCAST_CASE(uint16_t,
                     ((~((uint16_t *)input0)[offset0] & ~((uint16_t *)input1)[offset1]) * (lutop & 0x01)) |
                     ((~((uint16_t *)input0)[offset0] &  ((uint16_t *)input1)[offset1]) * ((lutop >> 1) & 0x01)) |
                     (( ((uint16_t *)input0)[offset0] & ~((uint16_t *)input1)[offset1]) * ((lutop >> 2) & 0x01)) |
                     (( ((uint16_t *)input0)[offset0] &  ((uint16_t *)input1)[offset1]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_S16:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(int16_t,
                   ((int16_t *)input0)[offset0] + ((int16_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(int16_t,
                   ((int16_t *)input0)[offset0] - ((int16_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(int16_t,
                   ((int16_t *)input0)[offset0] * ((int16_t *)input1)[offset1]);
    case OP_TENSOR_SRL:
      PROCESS_BROADCAST_CASE(int16_t, ((uint16_t)((int16_t *)input0)[offset0] >>
                             ((uint16_t)((int16_t *)input1)[offset1])));
    case OP_TENSOR_SRA:
      PROCESS_BROADCAST_CASE(int16_t,
                   ((int16_t *)input0)[offset0] >> ((int16_t *)input1)[offset1]);
    case OP_TENSOR_SLL:
      PROCESS_BROADCAST_CASE(int16_t,
                   ((int16_t *)input0)[offset0] << ((int16_t *)input1)[offset1]);
    case OP_TENSOR_CMP:
      PROCESS_BROADCAST_CASE(
          int16_t,
          ((int16_t *)input0)[offset0] > ((int16_t *)input1)[offset1] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_BROADCAST_CASE(int16_t,
                     ((~((int16_t *)input0)[offset0] & ~((int16_t *)input1)[offset1]) * (lutop & 0x01)) |
                     ((~((int16_t *)input0)[offset0] &  ((int16_t *)input1)[offset1]) * ((lutop >> 1) & 0x01)) |
                     (( ((int16_t *)input0)[offset0] & ~((int16_t *)input1)[offset1]) * ((lutop >> 2) & 0x01)) |
                     (( ((int16_t *)input0)[offset0] &  ((int16_t *)input1)[offset1]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_U32:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(uint32_t,
                   ((uint32_t *)input0)[offset0] + ((uint32_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(uint32_t,
                   ((uint32_t *)input0)[offset0] - ((uint32_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(uint32_t,
                   ((uint32_t *)input0)[offset0] * ((uint32_t *)input1)[offset1]);
    case OP_TENSOR_SRL:
      PROCESS_BROADCAST_CASE(uint32_t, ((uint32_t *)input0)[offset0] >>
                                 ((uint32_t *)input1)[offset1]);
    case OP_TENSOR_SLL:
      PROCESS_BROADCAST_CASE(uint32_t, ((uint32_t *)input0)[offset0]
                                 << ((uint32_t *)input1)[offset1]);
    case OP_TENSOR_CMP:
      PROCESS_BROADCAST_CASE(
          uint32_t,
          ((uint32_t *)input0)[offset0] > ((uint32_t *)input1)[offset1] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_BROADCAST_CASE(uint32_t,
                     ((~((uint32_t *)input0)[offset0] & ~((uint32_t *)input1)[offset1]) * (lutop & 0x01)) |
                     ((~((uint32_t *)input0)[offset0] &  ((uint32_t *)input1)[offset1]) * ((lutop >> 1) & 0x01)) |
                     (( ((uint32_t *)input0)[offset0] & ~((uint32_t *)input1)[offset1]) * ((lutop >> 2) & 0x01)) |
                     (( ((uint32_t *)input0)[offset0] &  ((uint32_t *)input1)[offset1]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_S32:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(int32_t,
                   ((int32_t *)input0)[offset0] + ((int32_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(int32_t,
                   ((int32_t *)input0)[offset0] - ((int32_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(int32_t,
                   ((int32_t *)input0)[offset0] * ((int32_t *)input1)[offset1]);
    case OP_TENSOR_SRL:
      PROCESS_BROADCAST_CASE(int32_t, ((uint32_t)((int32_t *)input0)[offset0] >>
                             ((uint32_t)((int32_t *)input1)[offset1])));
    case OP_TENSOR_SRA:
      PROCESS_BROADCAST_CASE(int32_t,
                   ((int32_t *)input0)[offset0] >> ((int32_t *)input1)[offset1]);
    case OP_TENSOR_SLL:
      PROCESS_BROADCAST_CASE(int32_t,
                   ((int32_t *)input0)[offset0] << ((int32_t *)input1)[offset1]);
    case OP_TENSOR_CMP:
      PROCESS_BROADCAST_CASE(
          int32_t,
          ((int32_t *)input0)[offset0] > ((int32_t *)input1)[offset1] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_BROADCAST_CASE(int32_t,
                     ((~((int32_t *)input0)[offset0] & ~((int32_t *)input1)[offset1]) * (lutop & 0x01)) |
                     ((~((int32_t *)input0)[offset0] &  ((int32_t *)input1)[offset1]) * ((lutop >> 1) & 0x01)) |
                     (( ((int32_t *)input0)[offset0] & ~((int32_t *)input1)[offset1]) * ((lutop >> 2) & 0x01)) |
                     (( ((int32_t *)input0)[offset0] &  ((int32_t *)input1)[offset1]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_U64:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(uint64_t,
                   ((uint64_t *)input0)[offset0] + ((uint64_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(uint64_t,
                   ((uint64_t *)input0)[offset0] - ((uint64_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(uint64_t,
                   ((uint64_t *)input0)[offset0] * ((uint64_t *)input1)[offset1]);
    case OP_TENSOR_SRL:
      PROCESS_BROADCAST_CASE(uint64_t, ((uint64_t *)input0)[offset0] >>
                                 ((uint64_t *)input1)[offset1]);
    case OP_TENSOR_SLL:
      PROCESS_BROADCAST_CASE(uint64_t, ((uint64_t *)input0)[offset0]
                                 << ((uint64_t *)input1)[offset1]);
    case OP_TENSOR_CMP:
      PROCESS_BROADCAST_CASE(
          uint64_t,
          ((uint64_t *)input0)[offset0] > ((uint64_t *)input1)[offset1] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_BROADCAST_CASE(uint64_t,
                     ((~((uint64_t *)input0)[offset0] & ~((uint64_t *)input1)[offset1]) * (lutop & 0x01)) |
                     ((~((uint64_t *)input0)[offset0] &  ((uint64_t *)input1)[offset1]) * ((lutop >> 1) & 0x01)) |
                     (( ((uint64_t *)input0)[offset0] & ~((uint64_t *)input1)[offset1]) * ((lutop >> 2) & 0x01)) |
                     (( ((uint64_t *)input0)[offset0] &  ((uint64_t *)input1)[offset1]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  case DTYPE_S64:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(int64_t,
                   ((int64_t *)input0)[offset0] + ((int64_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(int64_t,
                   ((int64_t *)input0)[offset0] - ((int64_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(int64_t,
                   ((int64_t *)input0)[offset0] * ((int64_t *)input1)[offset1]);
    case OP_TENSOR_SRL:
      PROCESS_BROADCAST_CASE(int64_t, ((uint64_t)((int64_t *)input0)[offset0] >> ((uint64_t)((int64_t *)input1)[offset1])));
    case OP_TENSOR_SRA:
      PROCESS_BROADCAST_CASE(int64_t, ((int64_t *)input0)[offset0] >> ((int64_t *)input1)[offset1]);
    case OP_TENSOR_SLL:
      PROCESS_BROADCAST_CASE(int64_t, ((int64_t *)input0)[offset0] << ((int64_t *)input1)[offset1]);
    case OP_TENSOR_CMP:
      PROCESS_BROADCAST_CASE(
          int64_t,
          ((int64_t *)input0)[offset0] > ((int64_t *)input1)[offset1] ? 1 : 0);
    default: // LUTOP
      {
        uint8_t lutop = op & 0x0F;
        PROCESS_BROADCAST_CASE(int64_t,
                     ((~((int64_t *)input0)[offset0] & ~((int64_t *)input1)[offset1]) * (lutop & 0x01)) |
                     ((~((int64_t *)input0)[offset0] &  ((int64_t *)input1)[offset1]) * ((lutop >> 1) & 0x01)) |
                     (( ((int64_t *)input0)[offset0] & ~((int64_t *)input1)[offset1]) * ((lutop >> 2) & 0x01)) |
                     (( ((int64_t *)input0)[offset0] &  ((int64_t *)input1)[offset1]) * ((lutop >> 3) & 0x01)));
      }
    }
    break;

  default:
    return false;
  }
  return true;
#undef PROCESS_BROADCAST_CASE
}

bool tensor_op_broadcast_fp(void *input0, void *input1, void *output,
                            int32_t dim1[4], int32_t dim2[4],
                            int32_t stride1[4], int32_t stride2[4],
                            FP_DataType dtype, OpType op) {
  if (!input0 || !input1 || !output) {
    return false;
  }

  for (int i = 0; i < 4; i++) {
    if (dim1[i] != dim2[i] && dim1[i] != 1 && dim2[i] != 1) {
      return false;
    }
  }

  int32_t out_dim[4];
  for (int i = 0; i < 4; i++) {
    out_dim[i] = dim1[i] > dim2[i] ? dim1[i] : dim2[i];
  }

  switch (dtype) {
#define PROCESS_BROADCAST_CASE(TYPE, OP_EXPR)    \
 for (int32_t l = 0; l < out_dim[3]; ++l) {                                  \
    int32_t idx1_3 = dim1[3] == 1 ? 0 : l;                                    \
    int32_t idx2_3 = dim2[3] == 1 ? 0 : l;                                    \
    for (int32_t k = 0; k < out_dim[2]; ++k) {                                \
      int32_t idx1_2 = dim1[2] == 1 ? 0 : k;                                  \
      int32_t idx2_2 = dim2[2] == 1 ? 0 : k;                                  \
      for (int32_t j = 0; j < out_dim[1]; ++j) {                              \
        int32_t idx1_1 = dim1[1] == 1 ? 0 : j;                                \
        int32_t idx2_1 = dim2[1] == 1 ? 0 : j;                                \
        for (int32_t i = 0; i < out_dim[0]; ++i) {                            \
          int32_t idx1_0 = dim1[0] == 1 ? 0 : i;                              \
          int32_t idx2_0 = dim2[0] == 1 ? 0 : i;                              \
          int32_t offset0 = idx1_0 * stride1[0] + idx1_1 * stride1[1] +        \
                           idx1_2 * stride1[2] + idx1_3 * stride1[3];         \
          int32_t offset1 = idx2_0 * stride2[0] + idx2_1 * stride2[1] +        \
                           idx2_2 * stride2[2] + idx2_3 * stride2[3];          \
          int32_t out_offset = i * stride1[0] + j * stride1[1] +               \
                              k * stride1[2] + l * stride1[3];                 \
          ((TYPE *)output)[out_offset] = (TYPE)(OP_EXPR);                      \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }                                                                            \
  return true;

  case DTYPE_FP16:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(fp16_t, ((fp16_t *)input0)[offset0] +
                                         ((fp16_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(fp16_t, ((fp16_t *)input0)[offset0] -
                                         ((fp16_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(fp16_t, ((fp16_t *)input0)[offset0] *
                                         ((fp16_t *)input1)[offset1]);
    default:
      return false;
    }
  case DTYPE_BF16:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(bf16_t, ((bf16_t *)input0)[offset0] +
                                         ((bf16_t *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(bf16_t, ((bf16_t *)input0)[offset0] -
                                         ((bf16_t *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(bf16_t, ((bf16_t *)input0)[offset0] *
                                         ((bf16_t *)input1)[offset1]);
    default:
      return false;
    }
  case DTYPE_FP32:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(float, ((float *)input0)[offset0] +
                                        ((float *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(float, ((float *)input0)[offset0] -
                                        ((float *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(float, ((float *)input0)[offset0] *
                                        ((float *)input1)[offset1]);
    default:
      return false;
    }

  // TODO
  case DTYPE_TF32:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(float, ((float *)input0)[offset0] +
                                        ((float *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(float, ((float *)input0)[offset0] -
                                        ((float *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(float, ((float *)input0)[offset0] *
                                        ((float *)input1)[offset1]);

    default:
      return false;
    }
  case DTYPE_FP64:
    switch (op) {
    case OP_TENSOR_ADD:
      PROCESS_BROADCAST_CASE(double, ((double *)input0)[offset0] +
                                         ((double *)input1)[offset1]);
    case OP_TENSOR_SUB:
      PROCESS_BROADCAST_CASE(double, ((double *)input0)[offset0] -
                                         ((double *)input1)[offset1]);
    case OP_TENSOR_MUL:
      PROCESS_BROADCAST_CASE(double, ((double *)input0)[offset0] *
                                         ((double *)input1)[offset1]);
    default:
      return false;
    }
  default:
    return false;
  }
  return true;
  #undef PROCESS_BROADCAST_CASE
}

bool tensor_reduce(void *input, void *output, int32_t in_dim[4],
                   int32_t in_stride[4], int reduce_axis[4], DataType dtype,
                   ReduceOp op) {

  if (!input || !output)
    return false;

  int reduce_count = 0;
  int reduce_dim = -1;
  for (int i = 0; i < 4; i++) {
    if (reduce_axis[i]) {
      reduce_count++;
      reduce_dim = i;
    }
  }
  if (reduce_count != 1)
    return false;

  int32_t out_dim[4];
  for (int i = 0; i < 4; i++) {
    out_dim[i] = reduce_axis[i] ? 1 : in_dim[i];
  }

  int32_t out_stride[4] = {0};
  out_stride[3] = 1;
  for (int i = 2; i >= 0; i--) {
    out_stride[i] = out_stride[i + 1] * out_dim[i + 1];
  }

  switch (dtype) {
#define PROCESS_REDUCE_CASE(TYPE, INIT_VAL, REDUCE_EXPR)                       \
  do {                                                                         \
    TYPE *in = (TYPE *)input;                                                  \
    TYPE *out = (TYPE *)output;                                                \
    for (int32_t l = 0; l < out_dim[3]; l++) {                                 \
      for (int32_t k = 0; k < out_dim[2]; k++) {                               \
        for (int32_t j = 0; j < out_dim[1]; j++) {                             \
          for (int32_t i = 0; i < out_dim[0]; i++) {                           \
            TYPE acc = INIT_VAL;                                               \
            for (int32_t m = 0; m < in_dim[reduce_dim]; m++) {                 \
              int32_t ll = (reduce_dim == 3) ? m : l;                          \
              int32_t kk = (reduce_dim == 2) ? m : k;                          \
              int32_t jj = (reduce_dim == 1) ? m : j;                          \
              int32_t ii = (reduce_dim == 0) ? m : i;                          \
              int32_t in_idx = ll * in_stride[3] + kk * in_stride[2] +         \
                               jj * in_stride[1] + ii * in_stride[0];          \
              TYPE val = in[in_idx];                                           \
              acc = REDUCE_EXPR;                                               \
            }                                                                  \
            int32_t out_idx = l * out_stride[3] + k * out_stride[2] +          \
                              j * out_stride[1] + i * out_stride[0];           \
            out[out_idx] = acc;                                                \
          }                                                                    \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    break;                                                                     \
  } while (0)

  case DTYPE_U8:
    switch (op) {
    case REDUCE_MAX:
      PROCESS_REDUCE_CASE(uint8_t, 0, val > acc ? val : acc);
      break;
    case REDUCE_MIN:
      PROCESS_REDUCE_CASE(uint8_t, UINT8_MAX, val < acc ? val : acc);
      break;
    case REDUCE_SUM:
      PROCESS_REDUCE_CASE(uint8_t, 0, acc + val);
      break;
    default:
      return false;
    }
    break;

  case DTYPE_S8:
    switch (op) {
    case REDUCE_MAX:
      PROCESS_REDUCE_CASE(int8_t, INT8_MIN, val > acc ? val : acc);
      break;
    case REDUCE_MIN:
      PROCESS_REDUCE_CASE(int8_t, INT8_MAX, val < acc ? val : acc);
      break;
    case REDUCE_SUM:
      PROCESS_REDUCE_CASE(int8_t, 0, acc + val);
      break;
    default:
      return false;
    }
    break;

  case DTYPE_U16:
    switch (op) {
    case REDUCE_MAX:
      PROCESS_REDUCE_CASE(uint16_t, 0, val > acc ? val : acc);
      break;
    case REDUCE_MIN:
      PROCESS_REDUCE_CASE(uint16_t, UINT16_MAX, val < acc ? val : acc);
      break;
    case REDUCE_SUM:
      PROCESS_REDUCE_CASE(uint16_t, 0, acc + val);
      break;
    default:
      return false;
    }
    break;

  case DTYPE_S16:
    switch (op) {
    case REDUCE_MAX:
      PROCESS_REDUCE_CASE(int16_t, INT16_MIN, val > acc ? val : acc);
      break;
    case REDUCE_MIN:
      PROCESS_REDUCE_CASE(int16_t, INT16_MAX, val < acc ? val : acc);
      break;
    case REDUCE_SUM:
      PROCESS_REDUCE_CASE(int16_t, 0, acc + val);
      break;
    default:
      return false;
    }
    break;

  case DTYPE_U32:
    switch (op) {
    case REDUCE_MAX:
      PROCESS_REDUCE_CASE(uint32_t, 0, val > acc ? val : acc);
      break;
    case REDUCE_MIN:
      PROCESS_REDUCE_CASE(uint32_t, UINT32_MAX, val < acc ? val : acc);
      break;
    case REDUCE_SUM:
      PROCESS_REDUCE_CASE(uint32_t, 0, acc + val);
      break;
    default:
      return false;
    }
    break;

  case DTYPE_S32:
    switch (op) {
    case REDUCE_MAX:
      PROCESS_REDUCE_CASE(int32_t, INT32_MIN, val > acc ? val : acc);
      break;
    case REDUCE_MIN:
      PROCESS_REDUCE_CASE(int32_t, INT32_MAX, val < acc ? val : acc);
      break;
    case REDUCE_SUM:
      PROCESS_REDUCE_CASE(int32_t, 0, acc + val);
      break;
    default:
      return false;
    }
    break;
  case DTYPE_U64:
    switch (op) {
    case REDUCE_MAX:
      PROCESS_REDUCE_CASE(uint64_t, 0, val > acc ? val : acc);
      break;
    case REDUCE_MIN:
      PROCESS_REDUCE_CASE(uint64_t, UINT64_MAX, val < acc ? val : acc);
      break;
    case REDUCE_SUM:
      PROCESS_REDUCE_CASE(uint64_t, 0, acc + val);
      break;
    default:
      return false;
    }
    break;

  case DTYPE_S64:
    switch (op) {
    case REDUCE_MAX:
      PROCESS_REDUCE_CASE(int64_t, INT64_MIN, val > acc ? val : acc);
      break;
    case REDUCE_MIN:
      PROCESS_REDUCE_CASE(int64_t, INT64_MAX, val < acc ? val : acc);
      break;
    case REDUCE_SUM:
      PROCESS_REDUCE_CASE(int64_t, 0, acc + val);
      break;
    default:
      return false;
    }
    break;
  default:
    return false;
  }
  return true;
}

#define DEFINE_REDUCE_OP(TYPE, INIT_MAX, INIT_MIN, INIT_SUM, OP_EXPR)          \
  do {                                                                         \
    TYPE *in = (TYPE *)input;                                                  \
    TYPE *out = (TYPE *)output;                                                \
    int reduce_dim = -1;                                                       \
    for (int d = 0; d < 4; d++) {                                              \
      if (reduce_axis[d]) {                                                    \
        reduce_dim = d;                                                        \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    for (int32_t l = 0; l < out_dim[3]; l++) {                                 \
      for (int32_t k = 0; k < out_dim[2]; k++) {                               \
        for (int32_t j = 0; j < out_dim[1]; j++) {                             \
          for (int32_t i = 0; i < out_dim[0]; i++) {                           \
            TYPE acc;                                                          \
            int32_t count = 0;                                                 \
            switch (op) {                                                      \
            case REDUCE_MAX:                                                   \
              acc = INIT_MAX;                                                  \
              break;                                                           \
            case REDUCE_MIN:                                                   \
              acc = INIT_MIN;                                                  \
              break;                                                           \
            case REDUCE_SUM:                                                   \
            case REDUCE_MEAN:                                                  \
              acc = INIT_SUM;                                                  \
              break;                                                           \
            default:                                                           \
              return false;                                                    \
            }                                                                  \
            for (int32_t m = 0; m < in_dim[reduce_dim]; m++) {                 \
              int32_t ll = (reduce_dim == 3) ? m : l;                          \
              int32_t kk = (reduce_dim == 2) ? m : k;                          \
              int32_t jj = (reduce_dim == 1) ? m : j;                          \
              int32_t ii = (reduce_dim == 0) ? m : i;                          \
              int32_t in_idx = ll * in_stride[3] + kk * in_stride[2] +         \
                               jj * in_stride[1] + ii * in_stride[0];          \
              TYPE val = in[in_idx];                                           \
              acc = OP_EXPR;                                                   \
              count++;                                                         \
            }                                                                  \
            if (op == REDUCE_MEAN)                                             \
              acc /= count;                                                    \
            int32_t out_idx = l * out_stride[3] + k * out_stride[2] +          \
                              j * out_stride[1] + i * out_stride[0];           \
            out[out_idx] = acc;                                                \
          }                                                                    \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  } while (0)

bool tensor_reduce_fp(void *input, void *output, int32_t in_dim[4],
                      int32_t in_stride[4], int reduce_axis[4],
                      FP_DataType dtype, ReduceOp op) {
  if (!input || !output)
    return false;

  int reduce_count = 0;
  //int reduce_dim = -1;
  for (int i = 0; i < 4; i++) {
    if (reduce_axis[i]) {
      reduce_count++;
      //re_dim = i;
    }
  }
  if (reduce_count != 1)
    return false;

  int32_t out_dim[4];
  for (int i = 0; i < 4; i++) {
    out_dim[i] = reduce_axis[i] ? 1 : in_dim[i];
  }

  int32_t out_stride[4] = {0};
  out_stride[3] = 1;
  for (int i = 2; i >= 0; i--) {
    out_stride[i] = out_stride[i + 1] * out_dim[i + 1];
  }

  switch (dtype) {
  case DTYPE_FP16: {
#define FP16_MAX (fp16_t) INFINITY
#define FP16_MIN (fp16_t) - INFINITY
    DEFINE_REDUCE_OP(fp16_t, FP16_MIN, FP16_MAX, (fp16_t)0.0,
                     (op == REDUCE_MAX)
                         ? (val > acc ? val : acc)
                         : (op == REDUCE_MIN) ? (val < acc ? val : acc)
                                              : (acc + val));
    break;
  }

  case DTYPE_BF16: {
#define BF16_MAX (bf16_t) INFINITY
#define BF16_MIN (bf16_t) - INFINITY
    DEFINE_REDUCE_OP(bf16_t, BF16_MIN, BF16_MAX, (bf16_t)0.0,
                     (op == REDUCE_MAX)
                         ? (val > acc ? val : acc)
                         : (op == REDUCE_MIN) ? (val < acc ? val : acc)
                                              : (acc + val));
    break;
  }

  case DTYPE_FP32: {
    DEFINE_REDUCE_OP(float, -INFINITY, INFINITY, 0.0f,
                     (op == REDUCE_MAX)
                         ? fmaxf(acc, val)
                         : (op == REDUCE_MIN) ? fminf(acc, val) : (acc + val));
    break;
  }

  case DTYPE_FP64: {
    DEFINE_REDUCE_OP(double, -INFINITY, INFINITY, 0.0,
                     (op == REDUCE_MAX)
                         ? fmax(acc, val)
                         : (op == REDUCE_MIN) ? fmin(acc, val) : (acc + val));
    break;
  }

  default:
    return false;
  }
  return true;
}
#endif
