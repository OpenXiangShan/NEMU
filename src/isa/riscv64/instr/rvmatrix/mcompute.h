/***************************************************************************************
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifdef CONFIG_RVMATRIX

#include "cpu/exec.h"
#include "mreg.h"
#include "../local-include/csr.h"
#include <stdio.h>
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#include <cpu/cpu.h>
#include <ext/amu_ctrl_queue_wrapper.h>
#include <ext/cutest.h>
#include "mcommon.h"
#include "mcompute_impl.h"
#include "rtl/fp.h"

typedef __uint128_t uint128_t;
typedef __int128_t int128_t;

// #define PRINT_AMUCTRLIO

#define MMA_LOOP_BEGIN \
  require_matrix(); \
  mp_set_dirty(); \
  int tile_m = mtilem->val; \
  int tile_k = mtilek->val; \
  int tile_n = mtilen->val; \
  uint64_t ts1 = s->src1.reg; \
  uint64_t ts2 = s->src2.reg; \
  uint64_t td = s->dest.reg; \
  uint8_t m_d_sz = s->m_d_sz; \
  uint8_t m_s_sz = s->m_s_sz; \
  bool tilem_valid = tile_m <= ROWNUM; \
  bool tilen_valid = tile_n <= ROWNUM && tile_n <= ARLEN / m_d_sz; \
  bool tilek_valid; \
  if (m_s_sz == 0 && (s->m_sz_sup & (1 << 2))) { \
    tilek_valid = tile_k <= (TRLEN << 1); \
  } else { \
    tilek_valid = tile_k <= (TRLEN >> m_s_sz); \
  } \
  if (!tilem_valid || !tilen_valid || !tilek_valid) { \
    longjmp_exception(EX_II); \
  } \
  \
  for (int i = 0; i < tile_m; i++) { \
    for (int j = 0; j < tile_n; j++) { \
      for (int k = 0; k < tile_k; k++) { \

#define MMA_LOOP_END \
      } \
    } \
  } \

def_EHelper(mmacc) {
  int64_t int_max = INT64_MAX >> (64 - 8 * s->m_d_sz);
  int64_t int_min = INT64_MIN >> (64 - 8 * s->m_d_sz);
  MMA_LOOP_BEGIN
    get_mreg(ts1, i, k, &tmp_reg[1], m_s_sz, false);
    get_mreg(ts2, j, k, &tmp_reg[2], m_s_sz, false);

    get_mreg(td, i, j, &tmp_reg[0], m_d_sz, false);
    if (xmsaten->val) {
      int128_t result = (int128_t)(int64_t)tmp_reg[1] * (int128_t)(int64_t)tmp_reg[2] + (int128_t)(int64_t)tmp_reg[0];
      if (result > int_max){
        result = int_max;
      } else if (result < int_min){
        result = int_min;
      }
      set_mreg(td, i, j, result, m_d_sz);
    } else {
      tmp_reg[0] = tmp_reg[1] * tmp_reg[2] + tmp_reg[0];
      set_mreg(td, i, j, tmp_reg[0], m_d_sz);
    }
  MMA_LOOP_END
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mma_emplace(td, xmxrm->val, xmsaten->val, false, ts1, ts2,
                      mtilem->val, mtilen->val, mtilek->val,
                      4 | m_s_sz, 4 | m_s_sz, m_d_sz);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mma_emplace(td, xmsaten->val, false, ts1, ts2,
              mtilem->val, mtilen->val, mtilek->val,
              4 | m_s_sz, 4 | m_s_sz, m_d_sz);
#endif // CONFIG_SHARE_CTRL
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=0 \n"
    "            md=%ld, sat=%ld, isfp=%d, ms1=%ld, ms2=%ld\n"
    "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types1=%#x, types2=%#x, typed=%#x\n",
    td, xmsaten->val, false, ts1, ts2,
    mtilem->val, mtilen->val, mtilek->val, 4 | m_s_sz, 4 | m_s_sz, m_d_sz);
#endif // PRINT_AMUCTRLIO
}

def_EHelper(mmaccu) {
  uint64_t uint_max = ((uint64_t) UINT64_MAX) >> (64 - 8 * s->m_d_sz);
  MMA_LOOP_BEGIN
    get_mreg(ts1, i, k, &tmp_reg[1], m_s_sz, false);
    get_mreg(ts2, j, k, &tmp_reg[2], m_s_sz, false);

    get_mreg(td, i, j, &tmp_reg[0], m_d_sz, false);
    if (xmsaten->val) {
      uint128_t result = (uint128_t)tmp_reg[1] * (uint128_t)tmp_reg[2] + (uint128_t)tmp_reg[0];
      if (result > uint_max) {
        result = uint_max;
      }
      set_mreg(td, i, j, result, m_d_sz);
    } else {
      rtl_mulu_lo(s, &tmp_reg[1], &tmp_reg[1], &tmp_reg[2]);
      rtl_add(s, &tmp_reg[0], &tmp_reg[0], &tmp_reg[1]);
      set_mreg(td, i, j, tmp_reg[0], m_d_sz);
    }
  MMA_LOOP_END
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mma_emplace(td, xmxrm->val, xmsaten->val, false, ts1, ts2,
                      mtilem->val, mtilen->val, mtilek->val,
                      m_s_sz, m_s_sz, m_d_sz);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mma_emplace(td, xmsaten->val, false, ts1, ts2,
              mtilem->val, mtilen->val, mtilek->val,
              m_s_sz, m_s_sz, m_d_sz);
#endif // CONFIG_SHARE_CTRL
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=0 \n"
    "            md=%ld, sat=%ld, isfp=%d, ms1=%ld, ms2=%ld\n"
    "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types1=%#x, types2=%#x, typed=%#x\n",
    td, xmsaten->val, false, ts1, ts2,
    mtilem->val, mtilen->val, mtilek->val, m_s_sz, m_s_sz, m_d_sz);
#endif // PRINT_AMUCTRLIO
}

def_EHelper(mmaccus) {
  MMA_LOOP_BEGIN
    get_mreg(ts1, i, k, &tmp_reg[1], m_s_sz, false);
    get_mreg(ts2, j, k, &tmp_reg[2], m_s_sz, false);

    get_mreg(td, i, j, &tmp_reg[0], m_d_sz, false);
    // TODO: Implement me!
    set_mreg(td, i, j, tmp_reg[0], m_d_sz);
  MMA_LOOP_END
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mma_emplace(td, xmxrm->val, xmsaten->val, false, ts1, ts2,
                      mtilem->val, mtilen->val, mtilek->val,
                      m_s_sz, 4 | m_s_sz, m_d_sz);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mma_emplace(td, xmsaten->val, false, ts1, ts2,
              mtilem->val, mtilen->val, mtilek->val,
              m_s_sz, 4 | m_s_sz, m_d_sz);
#endif // CONFIG_SHARE_CTRL
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=0 \n"
    "            md=%ld, sat=%ld, isfp=%d, ms1=%ld, ms2=%ld\n"
    "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types1=%#x, types2=%#x, typed=%#x\n",
    td, xmsaten->val, false, ts1, ts2,
    mtilem->val, mtilen->val, mtilek->val, m_s_sz, 4 | m_s_sz, m_d_sz);
#endif // PRINT_AMUCTRLIO
}

def_EHelper(mmaccsu) {
  MMA_LOOP_BEGIN
    get_mreg(ts1, i, k, &tmp_reg[1], m_s_sz, false);
    get_mreg(ts2, j, k, &tmp_reg[2], m_s_sz, false);

    get_mreg(td, i, j, &tmp_reg[0], m_d_sz, false);
    // TODO: Implement me!
    set_mreg(td, i, j, tmp_reg[0], m_d_sz);
  MMA_LOOP_END
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mma_emplace(td, xmxrm->val, xmsaten->val, false, ts1, ts2,
                      mtilem->val, mtilen->val, mtilek->val,
                      4 | m_s_sz, m_s_sz, m_d_sz);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mma_emplace(td, xmsaten->val, false, ts1, ts2,
              mtilem->val, mtilen->val, mtilek->val,
              4 | m_s_sz, m_s_sz, m_d_sz);
#endif // CONFIG_SHARE_CTRL
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=0 \n"
    "            md=%ld, sat=%ld, isfp=%d, ms1=%ld, ms2=%ld\n"
    "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types1=%#x, types2=%#x, typed=%#x\n",
    td, xmsaten->val, false, ts1, ts2,
    mtilem->val, mtilen->val, mtilek->val, 4 | m_s_sz, m_s_sz, m_d_sz);
#endif // PRINT_AMUCTRLIO
}

#define MFMA_LOOP_BEGIN \
  require_matrix(); \
  mp_set_dirty(); \
  int tile_m = mtilem->val; \
  int tile_k = mtilek->val; \
  int tile_n = mtilen->val; \
  uint64_t ts1 = s->src1.reg; \
  uint64_t ts2 = s->src2.reg; \
  uint64_t td = s->dest.reg; \
  uint8_t m_d_sz = s->m_d_sz; \
  uint8_t m_s_sz = s->m_s_sz; \
  bool tilem_valid = tile_m <= ROWNUM; \
  bool tilen_valid = tile_n <= ROWNUM && tile_n <= ARLEN / m_d_sz; \
  bool tilek_valid = tile_k <= TRLEN / m_s_sz; \
  if (!tilem_valid || !tilen_valid || !tilek_valid) { \
    longjmp_exception(EX_II); \
  } \
  \
  word_t FPCALL_TYPE = FPCALL_W64; \
  switch (m_s_sz) { \
    case 0: \
      Loge("fp8 mma not supported"); longjmp_exception(EX_II); \
      break; \
    case 1: \
      FPCALL_TYPE = FPCALL_W16; \
      break; \
    case 2: \
      FPCALL_TYPE = FPCALL_W32; \
      break; \
    case 3: \
      FPCALL_TYPE = FPCALL_W64; \
      break; \
    default: \
      Loge("other fp type not supported"); longjmp_exception(EX_II); \
      break; \
  } \
  for (int i = 0; i < tile_m; i++) { \
    for (int j = 0; j < tile_n; j++) { \
      for (int k = 0; k < tile_k; k++) { \

#define MFMA_LOOP_END \
      } \
    } \
  } \

def_EHelper(mfmacc) {
  MFMA_LOOP_BEGIN
    get_mreg(ts1, i, k, &tmp_reg[1], m_s_sz, false);
    get_mreg(ts2, j, k, &tmp_reg[2], m_s_sz, false);

    get_mreg(td, i, j, &tmp_reg[0], m_d_sz, false);
    rtl_hostcall(s, HOSTCALL_MFP, &tmp_reg[0], &tmp_reg[1], &tmp_reg[2], FPCALL_CMD(FPCALL_MADD, FPCALL_TYPE));
    set_mreg(td, i, j, tmp_reg[0], m_d_sz);
  MFMA_LOOP_END
  if (m_s_sz == 1 && s->m_sz_sup & (1 << 2)) {
    m_s_sz |= 4;
  } else if (m_s_sz == 0 && s->m_sz_sup & 1) {
    m_s_sz |= 4;
  }
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mma_emplace(td, xmfrm->val, false, false, ts1, ts2,
                      mtilem->val, mtilen->val, mtilek->val,
                      m_s_sz, m_s_sz, m_d_sz);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mma_emplace(td, false, false, ts1, ts2,
              mtilem->val, mtilen->val, mtilek->val,
              m_s_sz, m_s_sz, m_d_sz);
#endif // CONFIG_SHARE_CTRL
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=0 \n"
    "            md=%ld, sat=%d, isfp=%d, issigned=%d, ms1=%ld, ms2=%ld\n"
    "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
    td, false, false, false, ts1, ts2,
    mtilem->val, mtilen->val, mtilek->val, m_s_sz, m_d_sz);
#endif // PRINT_AMUCTRLIO
}

def_EHelper(mzero) {
  mp_set_dirty();
  if (s->dest.reg >= 4) {
    for (int i = 0; i < ROWNUM; i++) {
      for (int j = 0; j < ARENUM64; j++) {
        set_mreg(s->dest.reg, i, j, 0, 3);
      }
    }
  } else {
    for (int i = 0; i < ROWNUM; i++) {
      for (int j = 0; j < TRENUM64; j++) {
        set_mreg(s->dest.reg, i, j, 0, 3);
      }
    }
  }
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mzero_emplace(true, s->dest.reg);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mzero_emplace(true, s->dest.reg);
#endif // CONFIG_SHARE_CTRL
}

#endif // CONFIG_RVMATRIX