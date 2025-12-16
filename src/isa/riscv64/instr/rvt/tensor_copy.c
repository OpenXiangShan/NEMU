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

#include "tensor_copy.h"
#include <assert.h>
#include <string.h>

/**
 * @brief Copy tensor data with configurable strides and dimensions
 *
 * Implementation details:
 * - Iterates through all elements in source tensor
 * - Calculates offsets using stride information
 * - Copies elements with specified size (1 << log2_length bytes)
 * - Supports different memory layouts between source and destination
 */
void tensor_copy(void *src, void *dst, const TensorCopyConfig *config) {
  uint64_t src_idx0 = 0, src_idx1 = 0, src_idx2 = 0, src_idx3 = 0;
  uint64_t dst_idx0 = 0, dst_idx1 = 0, dst_idx2 = 0, dst_idx3 = 0;

  // Calculate element size from log2_length
  size_t element_size = 1 << config->log2_length;

  // Use unsigned integers for loop counters
  uint32_t src_dim0 = config->src_dims.dim0;
  uint32_t src_dim1 = config->src_dims.dim1;
  uint32_t src_dim2 = config->src_dims.dim2;
  uint32_t src_dim3 = config->src_dims.dim3;

  uint32_t dst_dim0 = config->dst_dims.dim0;
  uint32_t dst_dim1 = config->dst_dims.dim1;
  uint32_t dst_dim2 = config->dst_dims.dim2;
  uint32_t dst_dim3 = config->dst_dims.dim3;

  // Validate total number of elements
  uint64_t src_total = (uint64_t)src_dim0 * src_dim1 * src_dim2 * src_dim3;
  uint64_t dst_total = (uint64_t)dst_dim0 * dst_dim1 * dst_dim2 * dst_dim3;
  assert(src_total == dst_total &&
         "Source and destination must have same total elements");

  while (1) {
    // Calculate offsets using strides
    uint64_t src_offset = src_idx0 * config->src_strides.stride0 +
                          src_idx1 * config->src_strides.stride1 +
                          src_idx2 * config->src_strides.stride2 +
                          src_idx3 * config->src_strides.stride3;

    uint64_t dst_offset = dst_idx0 * config->dst_strides.stride0 +
                          dst_idx1 * config->dst_strides.stride1 +
                          dst_idx2 * config->dst_strides.stride2 +
                          dst_idx3 * config->dst_strides.stride3;

    // Perform the copy
    memcpy((uint8_t *)dst + dst_offset * element_size,
           (uint8_t *)src + src_offset * element_size, element_size);

    // Increment source indices (row-major order)
    src_idx0 += 1;
    if (src_idx0 >= src_dim0) {
      src_idx0 = 0;
      src_idx1 += 1;
    }
    if (src_idx1 >= src_dim1) {
      src_idx1 = 0;
      src_idx2 += 1;
    }
    if (src_idx2 >= src_dim2) {
      src_idx2 = 0;
      src_idx3 += 1;
    }
    if (src_idx3 >= src_dim3) {
      // All source elements processed
      break;
    }

    // Increment destination indices (row-major order)
    dst_idx0 += 1;
    if (dst_idx0 >= dst_dim0) {
      dst_idx0 = 0;
      dst_idx1 += 1;
    }
    if (dst_idx1 >= dst_dim1) {
      dst_idx1 = 0;
      dst_idx2 += 1;
    }
    if (dst_idx2 >= dst_dim2) {
      dst_idx2 = 0;
      dst_idx3 += 1;
    }
  }
}

/**
 * @brief Create tensor dimension configuration
 */
TensorDims create_tensor_dims(uint32_t dim0, uint32_t dim1, uint32_t dim2,
                              uint32_t dim3) {
  TensorDims dims = {dim0, dim1, dim2, dim3};
  return dims;
}

/**
 * @brief Create tensor stride configuration
 */
TensorStrides create_tensor_strides(uint32_t stride0, uint32_t stride1,
                                    uint32_t stride2, uint32_t stride3) {
  TensorStrides strides = {stride0, stride1, stride2, stride3};
  return strides;
}

/**
 * @brief Create complete tensor copy configuration
 */
TensorCopyConfig create_tensor_copy_config(const TensorDims *src_dims,
                                           const TensorStrides *src_strides,
                                           const TensorDims *dst_dims,
                                           const TensorStrides *dst_strides,
                                           uint32_t log2_length) {
  TensorCopyConfig config = {.src_dims = *src_dims,
                             .src_strides = *src_strides,
                             .dst_dims = *dst_dims,
                             .dst_strides = *dst_strides,
                             .log2_length = log2_length};
  return config;
}

#endif /* CONFIG_CUSTOM_TENSOR */
