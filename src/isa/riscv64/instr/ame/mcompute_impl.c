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

// Return 0 for int mma, 1 for float mma, -1 for invalid combination.
int8_t check_comb(mcfg_t s1cfg, mcfg_t s2cfg, mcfg_t dcfg) {
  uint32_t s1_mask = 1u << s1cfg.type_code;
  uint32_t s2_mask = 1u << s2cfg.type_code;
  uint32_t d_mask = 1u << dcfg.type_code;
  const uint32_t int_mask = (1u << MTYPECODE_INT4) | (1u << MTYPECODE_UINT4) |
                            (1u << MTYPECODE_INT8) | (1u << MTYPECODE_UINT8) |
                            (1u << MTYPECODE_INT32) | (1u << MTYPECODE_FP2PACK4) |
                            (1u << MTYPECODE_FP2PACK5);
  const uint32_t fp_mask = (1u << MTYPECODE_NVFP4) | (1u << MTYPECODE_MXFP4) |
                           (1u << MTYPECODE_FP8E5M2) | (1u << MTYPECODE_FP8E4M3) |
                           (1u << MTYPECODE_FP16) | (1u << MTYPECODE_BF16) |
                           (1u << MTYPECODE_TF32) | (1u << MTYPECODE_FP32);

  if ((s1_mask & int_mask) && (s2_mask & int_mask) && (d_mask & int_mask)) {
    return 0;
  }
  if ((s1_mask & fp_mask) && (s2_mask & fp_mask) && (d_mask & fp_mask)) {
    return 1;
  }
  return -1;
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

#ifdef CONFIG_AME_MMACC_VECTORIZE

#define def_auto_vectorized_int8_mmacc(name, lhs_type, rhs_type) \
static void name( \
  int td, int ts1, int ts2, \
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k \
) { \
  for (uint64_t i = 0; i < tile_m; i++) { \
    const lhs_type *restrict lhs_row = \
      (const lhs_type *)cpu.mtr[ts1][i]._8; \
    uint32_t *restrict dest_row = cpu.macc[td - 4][i]._32; \
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

  if (s1_type == MTYPECODE_INT8) {
    if (s2_type == MTYPECODE_INT8) {
      mmacc_i8_i8(td, ts1, ts2, tile_m, tile_n, tile_k);
    } else {
      mmacc_i8_u8(td, ts1, ts2, tile_m, tile_n, tile_k);
    }
  } else if (s2_type == MTYPECODE_INT8) {
    mmacc_u8_i8(td, ts1, ts2, tile_m, tile_n, tile_k);
  } else {
    mmacc_u8_u8(td, ts1, ts2, tile_m, tile_n, tile_k);
  }
  return true;
}

#endif // CONFIG_AME_MMACC_VECTORIZE

#endif // CONFIG_RV_AME
