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
int8_t check_comb(mcfg_t s1cfg, mcfg_t s2cfg, mcfg_t dcfg);
bool is_signed_int_mtype(uint64_t type_code);

#ifdef CONFIG_AME_MMACC_VECTORIZE
bool try_auto_vectorized_int8_mmacc(
  int td, int ts1, int ts2,
  uint64_t tile_m, uint64_t tile_n, uint64_t tile_k,
  uint64_t d_type, uint64_t s1_type, uint64_t s2_type,
  bool saturation
);
#endif

#endif // __RISCV64_MCOMPUTE_IMPL_H__
#endif // CONFIG_RV_AME
