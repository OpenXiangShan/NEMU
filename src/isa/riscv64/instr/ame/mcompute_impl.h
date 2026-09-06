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
#ifdef CONFIG_RV_AME

#ifndef __RISCV64_MCOMPUTE_IMPL_H__
#define __RISCV64_MCOMPUTE_IMPL_H__

#include "mreg.h"
#include "mldst_impl.h"
#include "../local-include/intr.h"
#include "mcommon.h"

void require_matrix();
uint8_t get_pack(mcfg_t cfg);
bool is_signed_int_mtype(uint64_t type_code);

typedef enum {
  MMACC_TYPE_INVALID,
  MMACC_TYPE_INTEGER,
  MMACC_TYPE_FLOAT,
} mmacc_type_t;

typedef enum {
  FLOAT_MMACC_UNSUPPORTED,
  FLOAT_MMACC_FP16_FP16_FP16,
  FLOAT_MMACC_FP16_FP16_FP32,
  FLOAT_MMACC_BF16_BF16_FP32,
  FLOAT_MMACC_FP32_FP32_FP32,
} float_mmacc_type_t;

mmacc_type_t get_mmacc_type(mcfg_t s1cfg, mcfg_t s2cfg, mcfg_t dcfg);

float_mmacc_type_t get_float_mmacc_type(
  uint64_t d_type, uint64_t s1_type, uint64_t s2_type
);

#ifdef CONFIG_AME_MMACC_VECTORIZE
bool try_auto_vectorized_int8_mmacc(
  int td, int ts1, int ts2,
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k,
  uint64_t d_type, uint64_t s1_type, uint64_t s2_type,
  bool saturation
);

bool try_auto_vectorized_float_mmacc(
  int td, int ts1, int ts2,
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k,
  float_mmacc_type_t float_mmacc_type,
  uint64_t rounding_mode
);
#endif

#endif // __RISCV64_MCOMPUTE_IMPL_H__
#endif // CONFIG_RV_AME
