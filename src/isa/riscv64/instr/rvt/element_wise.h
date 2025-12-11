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

#ifndef TENSOR_ELEMENT_WISE_H
#define TENSOR_ELEMENT_WISE_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  DTYPE_U8,
  DTYPE_S8,
  DTYPE_U16,
  DTYPE_S16,
  DTYPE_U32,
  DTYPE_S32,
  DTYPE_U64,
  DTYPE_S64
} DataType;

typedef enum {
  DTYPE_FP16 = 2,
  DTYPE_BF16 = 3,
  DTYPE_FP32 = 4,
  DTYPE_TF32 = 5,
  DTYPE_FP64 = 6
} FP_DataType;

typedef enum {
  OP_TENSOR_ADD,
  OP_TENSOR_SUB,
  OP_TENSOR_MUL,
  OP_TENSOR_SRL,
  OP_TENSOR_SRA,
  OP_TENSOR_SLL,
  OP_TENSOR_LUTOP = 16,
  OP_TENSOR_CMP
} OpType;

enum {
  OP_FPADD,
  OP_FPSUB,
  OP_FPMUL,
};

typedef enum {
  REDUCE_MAX,
  REDUCE_MIN,
  REDUCE_SUM,
  REDUCE_MEAN,
} ReduceOp;

bool tensor_op(void *input0, void *input1, void *output, uint32_t dim0,
               uint32_t dim1, uint32_t dim2, uint32_t dim3, uint32_t stride0,
               uint32_t stride1, uint32_t stride2, uint32_t stride3,
               DataType dtype, OpType op);
bool tensor_op_broadcast(void *input0, void *input1, void *output,
                         int32_t dim1[4], int32_t dim2[4], int32_t stride1[4],
                         int32_t stride2[4], DataType dtype, OpType op);

bool tensor_op_fp(void *input0, void *input1, void *output, uint32_t dim0,
                  uint32_t dim1, uint32_t dim2, uint32_t dim3, uint32_t stride0,
                  uint32_t stride1, uint32_t stride2, uint32_t stride3,
                  DataType dtype, OpType op);
bool tensor_op_broadcast_fp(void *input0, void *input1, void *output,
                            int32_t dim1[4], int32_t dim2[4],
                            int32_t stride1[4], int32_t stride2[4],
                            FP_DataType dtype, OpType op);

bool tensor_reduce(void *input, void *output, int32_t in_dim[4],
                   int32_t in_stride[4], int reduce_axis[4], DataType dtype,
                   ReduceOp op);

bool tensor_reduce_fp(void *input, void *output, int32_t in_dim[4],
                      int32_t in_stride[4], int reduce_axis[4],
                      FP_DataType dtype, ReduceOp op);
#endif

#endif  /* CONFIG_CUSTOM_TENSOR */
