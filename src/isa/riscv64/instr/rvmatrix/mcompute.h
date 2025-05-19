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
#include "rtl/fp.h"

typedef __uint128_t uint128_t;
typedef __int128_t int128_t;

// #define PRINT_AMUCTRLIO

#define MMA_LOOP_BEGIN \
  int tile_m = mtilem->val; \
  int tile_k = mtilek->val; \
  int tile_n = mtilen->val; \
  uint64_t ts1 = s->src1.reg; \
  uint64_t ts2 = s->src2.reg; \
  uint64_t td = s->dest.reg; \
  uint64_t msew = mtype->msew; \
  \
  for (int i = 0; i < tile_m; i++) { \
    for (int j = 0; j < tile_n; j++) { \
      for (int k = 0; k < tile_k; k++) { \

#define MMA_LOOP_END \
      } \
    } \
  } \

def_EHelper(mmau) {
  MMA_LOOP_BEGIN
          get_mreg(false, ts1, i, k, &tmp_reg[1], msew, false);
          get_mreg(false, ts2, k, j, &tmp_reg[2], msew, false);

          get_mreg(true, td, i, j, &tmp_reg[0], msew, false);
          rtl_mulu_lo(s, &tmp_reg[1], &tmp_reg[1], &tmp_reg[2]);   //(Decode *s, rtlreg_t *dest, rtlreg_t *src1, rtlreg_t *src2)
          rtl_add(s, &tmp_reg[0], &tmp_reg[0], &tmp_reg[1]);  // td = td + ts1 * ts2
          set_mreg(true, td, i, j, tmp_reg[0], msew);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, false, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew);
#endif
}

def_EHelper(mwmau) {
  Assert(mtype->msew <= 2, "e64 not support double widen compute!\n");

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, false);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, false);

        // TODO: temp use 2 output treg for all double widen mma
        int widen_idx = j / (tile_n/2);
        int j_offset = j - widen_idx*(tile_n/2);
        get_mreg(true, td + widen_idx, i, j_offset, &tmp_reg[0], msew + 1, false);
        rtl_mulu_lo(s, &tmp_reg[1], &tmp_reg[1], &tmp_reg[2]);
        rtl_add(s, &tmp_reg[0], &tmp_reg[0], &tmp_reg[1]);
        set_mreg(true, td + widen_idx, i, j_offset, tmp_reg[0], msew + 1);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, false, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew + 1);
#endif
}

def_EHelper(mqmau) {
  Assert(mtype->msew <= 1, "e32/e64 not support quadruple widen compute!\n");

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, false);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, false);

        // TODO: temp use 4 output treg for all quadruple widen mma
        int widen_idx = j / (tile_n/4);
        int j_offset = j - widen_idx*(tile_n/4);
        get_mreg(true, td + widen_idx, i, j_offset, &tmp_reg[0], msew + 2, false);
        rtl_mulu_lo(s, &tmp_reg[1], &tmp_reg[1], &tmp_reg[2]);
        rtl_add(s, &tmp_reg[0], &tmp_reg[0], &tmp_reg[1]);
        set_mreg(true, td + widen_idx, i, j_offset, tmp_reg[0], msew + 2);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, false, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew + 2);
#endif
}

def_EHelper(msmau) {
  uint64_t uint_max = ((uint64_t) UINT64_MAX) >> (64 - 8 * s->m_width);

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, false);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, false);

        get_mreg(true, td, i, j, &tmp_reg[0], msew, false);
        uint128_t result = (uint128_t)tmp_reg[1] * (uint128_t)tmp_reg[2] + (uint128_t)tmp_reg[0];
        bool overflow = false;  
        if (result > uint_max) overflow = true;
        if (overflow) {
          result = uint_max;
        }
        set_mreg(true, td, i, j, result, msew);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, true, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew);
#endif
}

def_EHelper(mswmau) {
  uint64_t uint_max = ((uint64_t) UINT64_MAX) >> (64 - 2 * 8 * s->m_width);
  Assert(mtype->msew <= 2, "e64 not support double widen compute!\n");

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, false);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, false);

        // TODO: temp use 2 output treg for all double widen mma
        int widen_idx = j / (tile_n/2);
        int j_offset = j - widen_idx*(tile_n/2);
        get_mreg(true, td + widen_idx, i, j_offset, &tmp_reg[0], msew + 1, false);
        uint128_t result = (uint128_t)tmp_reg[1] * (uint128_t)tmp_reg[2] + (uint128_t)tmp_reg[0];
        bool overflow = false;  
        if (result > uint_max) overflow = true;
        if (overflow) {
          result = uint_max;
        }
        set_mreg(true, td + widen_idx, i, j_offset, result, msew + 1);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, true, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew + 1);
#endif
}

def_EHelper(msqmau) {
  uint64_t uint_max = ((uint64_t) UINT64_MAX) >> (64 - 4 * 8 * s->m_width);
  Assert(mtype->msew <= 1, "e32/e64 not support quadruple widen compute!\n");

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, false);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, false);

        // TODO: temp use 4 output treg for all quadruple widen mma
        int widen_idx = j / (tile_n/4);
        int j_offset = j - widen_idx*(tile_n/4);
        get_mreg(true, td + widen_idx, i, j_offset, &tmp_reg[0], msew + 2, false);
        uint128_t result = (uint128_t)tmp_reg[1] * (uint128_t)tmp_reg[2] + (uint128_t)tmp_reg[0];
        bool overflow = false;  
        if (result > uint_max) overflow = true;
        if (overflow) {
          result = uint_max;
        }
        set_mreg(true, td + widen_idx, i, j_offset, result, msew + 2);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, true, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew + 2);
#endif
}

def_EHelper(mma) {
  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, true);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, true);

        get_mreg(true, td, i, j, &tmp_reg[0], msew, true);
        tmp_reg[0] = tmp_reg[1] * tmp_reg[2] + tmp_reg[0];
        set_mreg(true, td, i, j, tmp_reg[0], msew);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, false, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew);
#endif
}

def_EHelper(mwma) {
  Assert(mtype->msew <= 2, "e64 not support double widen compute!\n");

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, true);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, true);

        // TODO: temp use 2 output treg for all double widen mma
        int widen_idx = j / (tile_n/2);
        int j_offset = j - widen_idx*(tile_n/2);
        get_mreg(true, td + widen_idx, i, j_offset, &tmp_reg[0], msew + 1, true);
        tmp_reg[0] = tmp_reg[1] * tmp_reg[2] + tmp_reg[0];
        set_mreg(true, td + widen_idx, i, j_offset, tmp_reg[0], msew + 1);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, false, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew + 1);
#endif
}

def_EHelper(mqma) {
  Assert(mtype->msew <= 1, "e32/e64 not support quadruple widen compute!\n");

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, true);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, true);

        // TODO: temp use 4 output treg for all quadruple widen mma
        int widen_idx = j / (tile_n/4);
        int j_offset = j - widen_idx*(tile_n/4);
        get_mreg(true, td + widen_idx, i, j_offset, &tmp_reg[0], msew + 2, true);
        tmp_reg[0] = tmp_reg[1] * tmp_reg[2] + tmp_reg[0];
        set_mreg(true, td + widen_idx, i, j_offset, tmp_reg[0], msew + 2);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, false, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew + 2);
#endif
}

def_EHelper(msma) {
  int64_t int_max = INT64_MAX >> (64 - 8 * s->m_width);
  int64_t int_min = INT64_MIN >> (64 - 8 * s->m_width);

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, true);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, true);

        get_mreg(true, td, i, j, &tmp_reg[0], msew, true);
        int128_t result = (int128_t)(int64_t)tmp_reg[1] * (int128_t)(int64_t)tmp_reg[2] + (int128_t)(int64_t)tmp_reg[0];
        if (result > int_max){
          result = int_max;
        } else if (result < int_min){
          result = int_min;
        }
        set_mreg(true, td, i, j, result, msew);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, true, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew);
#endif
}

def_EHelper(mswma) {
  int64_t int_max = INT64_MAX >> (64 - 2 * 8 * s->m_width);
  int64_t int_min = INT64_MIN >> (64 - 2 * 8 * s->m_width);
  Assert(mtype->msew <= 2, "e64 not support double widen compute!\n");

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, true);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, true);

        // TODO: temp use 2 output treg for all double widen mma
        int widen_idx = j / (tile_n/2);
        int j_offset = j - widen_idx*(tile_n/2);
        get_mreg(true, td + widen_idx, i, j_offset, &tmp_reg[0], msew + 1, true);
        int128_t result = (int128_t)(int64_t)tmp_reg[1] * (int128_t)(int64_t)tmp_reg[2] + (int128_t)(int64_t)tmp_reg[0];
        if (result > int_max){
          result = int_max;
        } else if (result < int_min){
          result = int_min;
        }
        set_mreg(true, td + widen_idx, i, j_offset, result, msew + 1);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, true, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew + 1);
#endif
}

def_EHelper(msqma) {
  int64_t int_max = INT64_MAX >> (64 - 4 * 8 * s->m_width);
  int64_t int_min = INT64_MIN >> (64 - 4 * 8 * s->m_width);
  Assert(mtype->msew <= 1, "e32/e64 not support double widen compute!\n");

  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, true);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, true);

        // TODO: temp use 4 output treg for all quadruple widen mma
        int widen_idx = j / (tile_n/4);
        int j_offset = j - widen_idx*(tile_n/4);
        get_mreg(true, td + widen_idx, i, j_offset, &tmp_reg[0], msew + 2, true);
        int128_t result = (int128_t)(int64_t)tmp_reg[1] * (int128_t)(int64_t)tmp_reg[2] + (int128_t)(int64_t)tmp_reg[0];
        if (result > int_max){
          result = int_max;
        } else if (result < int_min){
          result = int_min;
        }
        set_mreg(true, td + widen_idx, i, j_offset, result, msew + 2);
  MMA_LOOP_END
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, true, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew + 2);
#endif
}

def_EHelper(mfma) {
  word_t FPCALL_TYPE = FPCALL_W64;
  switch (mtype->msew) {
    case 0:
      Loge("fp8 mma not supported"); longjmp_exception(EX_II);
      break;
    case 1:
      FPCALL_TYPE = FPCALL_W16;
      break;
    case 2:
      FPCALL_TYPE = FPCALL_W32;
      break;
    case 3:
      FPCALL_TYPE = FPCALL_W64;
      break;
    default:
      Loge("other fp type not supported"); longjmp_exception(EX_II);
      break;
  }
  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, false);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, false);

        get_mreg(true, td, i, j, &tmp_reg[0], msew, false);
        rtl_hostcall(s, HOSTCALL_MFP, &tmp_reg[0], &tmp_reg[1], &tmp_reg[2], FPCALL_CMD(FPCALL_MADD, FPCALL_TYPE));
        set_mreg(true, td, i, j, tmp_reg[0], msew);
  MMA_LOOP_END
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mma_emplace(td, false, ts1, ts2,
                            mtilem->val, mtilen->val, mtilek->val,
                            s->m_eew, s->m_eew);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, false, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew);
#endif
}

def_EHelper(mfwma) {
  word_t FPCALL_TYPE = FPCALL_W64;
  switch (mtype->msew) {
    case 0:
      Loge("fp8 mma not supported"); longjmp_exception(EX_II);
      break;
    case 1:
      FPCALL_TYPE = FPCALL_W16_to_32;
      break;
    case 2:
      FPCALL_TYPE = FPCALL_W32_to_64;
      break;
    default:
      Loge("other fp type not supported"); longjmp_exception(EX_II);
      break;
  }
  MMA_LOOP_BEGIN
        get_mreg(false, ts1, i, k, &tmp_reg[1], msew, false);
        get_mreg(false, ts2, k, j, &tmp_reg[2], msew, false);
        
        // TODO: temp use 2 output treg for all double widen mma
        int widen_idx = j / (tile_n/2);
        int j_offset = j - widen_idx*(tile_n/2);
        get_mreg(true, td + widen_idx, i, j_offset, &tmp_reg[0], msew + 1, false);
        rtl_hostcall(s, HOSTCALL_MFP, &tmp_reg[0], &tmp_reg[1], &tmp_reg[2], FPCALL_CMD(FPCALL_MADD, FPCALL_TYPE));
        set_mreg(true, td + widen_idx, i, j_offset, tmp_reg[0], msew + 1);
  MMA_LOOP_END
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mma_emplace(td, false, ts1, ts2,
                            mtilem->val, mtilen->val, mtilek->val,
                            s->m_eew, s->m_eew + 1);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
          "[AmuCtrlIO] op=0 \n"
          "            md=%ld, sat=%d, ms1=%ld, ms2=%ld\n"
          "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types=%#x, typed=%#x\n",
          td, false, ts1, ts2,
          mtilem->val, mtilen->val, mtilek->val, s->m_eew, s->m_eew + 1);
#endif
}

#endif // CONFIG_RVMATRIX