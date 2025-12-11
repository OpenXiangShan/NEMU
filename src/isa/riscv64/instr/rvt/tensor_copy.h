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

#ifndef TENSOR_COPY_H
#define TENSOR_COPY_H

#include <stddef.h>
#include <stdint.h>

// ====================================================
// Tensor Copy Configuration Structures
// ====================================================

/**
 * @brief Tensor dimension configuration structure
 * Represents a 4-dimensional tensor shape.
 */
typedef struct {
  uint32_t dim0; /* Dimension 0  */
  uint32_t dim1; /* Dimension 1 */
  uint32_t dim2; /* Dimension 2 */
  uint32_t dim3; /* Dimension 3  */
} TensorDims;

/**
 * @brief Tensor stride configuration structure
 * Represents strides for a 4-dimensional tensor.
 */
typedef struct {
  uint32_t stride0; /* Stride for dimension 0 */
  uint32_t stride1; /* Stride for dimension 1 */
  uint32_t stride2; /* Stride for dimension 2 */
  uint32_t stride3; /* Stride for dimension 3 */
} TensorStrides;

/**
 * @brief Tensor copy configuration structure
 * Contains complete configuration for tensor copy operation.
 */
typedef struct {
  TensorDims src_dims;       /* Source tensor dimensions */
  TensorStrides src_strides; /* Source tensor strides */
  TensorDims dst_dims;       /* Destination tensor dimensions */
  TensorStrides dst_strides; /* Destination tensor strides */
  uint32_t log2_length;      /* Log2 of element size */
} TensorCopyConfig;

// ====================================================
// Function Declarations
// ====================================================

/**
 * @brief Copy tensor data with configurable strides and dimensions
 *
 * This function performs a generalized tensor copy operation that handles
 * different memory layouts between source and destination tensors.
 *
 * @param src Pointer to source tensor data
 * @param dst Pointer to destination tensor data
 * @param config Tensor copy configuration
 */
void tensor_copy(void *src, void *dst, const TensorCopyConfig *config);

/**
 * @brief Create tensor dimension configuration
 *
 * @param dim0 Dimension 0 value
 * @param dim1 Dimension 1 value
 * @param dim2 Dimension 2 value
 * @param dim3 Dimension 3 value
 * @return TensorDims structure
 */
TensorDims create_tensor_dims(uint32_t dim0, uint32_t dim1, uint32_t dim2,
                              uint32_t dim3);

/**
 * @brief Create tensor stride configuration
 *
 * @param stride0 Stride for dimension 0
 * @param stride1 Stride for dimension 1
 * @param stride2 Stride for dimension 2
 * @param stride3 Stride for dimension 3
 * @return TensorStrides structure
 */
TensorStrides create_tensor_strides(uint32_t stride0, uint32_t stride1,
                                    uint32_t stride2, uint32_t stride3);

/**
 * @brief Create complete tensor copy configuration
 *
 * @param src_dims Source tensor dimensions
 * @param src_strides Source tensor strides
 * @param dst_dims Destination tensor dimensions
 * @param dst_strides Destination tensor strides
 * @param log2_length Log2 of element size
 * @return TensorCopyConfig structure
 */
TensorCopyConfig create_tensor_copy_config(const TensorDims *src_dims,
                                           const TensorStrides *src_strides,
                                           const TensorDims *dst_dims,
                                           const TensorStrides *dst_strides,
                                           uint32_t log2_length);

#endif /* TENSOR_COPY_H */

#endif /* CONFIG_CUSTOM_TENSOR */
