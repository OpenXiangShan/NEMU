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

#ifdef CONFIG_RV_AME

#include "cpu/exec.h"
#include "mreg.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/difftest/ame/amu_ctrl_queue_wrapper.h>
#include <ctrl/ame/cutest.h>
#include "mcommon.h"
#include "mcompute_impl.h"
#include "rtl/fp.h"

// #define PRINT_AMUCTRLIO

#ifdef PRINT_AMUCTRLIO
#include <stdio.h>
#endif // PRINT_AMUCTRLIO

uint8_t get_pack(mcfg_t cfg) {
  uint8_t pack = 0;
  switch (cfg.type_code) {
    case MTYPECODE_INT4:
    case MTYPECODE_UINT4:
    case MTYPECODE_NVFP4:
    case MTYPECODE_MXFP4:
      pack = 2;
      break;
    case MTYPECODE_FP2PACK4:
      pack = 4;
      break;
    case MTYPECODE_FP2PACK5:
      pack = 5;
      break;
    default:
      pack = 1;
      break;
  }
  return pack;
}

// Return 0 for int mma, 1 for float mma, -1 for invalid combination
int8_t check_comb(mcfg_t s1cfg, mcfg_t s2cfg, mcfg_t dcfg) {
  uint32_t s1_mask = 1u << s1cfg.type_code;
  uint32_t s2_mask = 1u << s2cfg.type_code;
  uint32_t d_mask = 1u << dcfg.type_code;
  const uint32_t int_mask = (1u << MTYPECODE_INT4) | (1u << MTYPECODE_UINT4) |
                            (1u << MTYPECODE_INT8) | (1u << MTYPECODE_UINT8) |
                            (1u << MTYPECODE_INT32);
  const uint32_t fp_mask = (1u << MTYPECODE_NVFP4) | (1u << MTYPECODE_MXFP4) |
                           (1u << MTYPECODE_FP8E5M2) | (1u << MTYPECODE_FP8E4M3) |
                           (1u << MTYPECODE_FP16) | (1u << MTYPECODE_BF16) |
                           (1u << MTYPECODE_TF32) | (1u << MTYPECODE_FP32) |
                           (1u << MTYPECODE_FP2PACK4) | (1u << MTYPECODE_FP2PACK5);

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

def_EHelper(mmacc) {
  // Check MS
  require_matrix();
  mp_set_dirty();
  int tile_m = mtilem->val;
  int tile_k = mtilek->val;
  int tile_n = mtilen->val;
  uint64_t ts1 = s->src1.reg;
  uint64_t ts2 = s->src2.reg;
  uint64_t td = s->dest.reg;
  mcfg_t dmcfg = cpu.mcfg[td];
  mcfg_t s1mcfg = cpu.mcfg[ts1];
  mcfg_t s2mcfg = cpu.mcfg[ts2];
  uint8_t dsize = get_size(dmcfg);
  uint8_t s1size = get_size(s1mcfg);
  uint8_t s2size = get_size(s2mcfg);
  // Check validity of tile size
  bool tilem_valid = tile_m <= ROWNUM;
  bool tilen_valid = tile_n <= ROWNUM && tile_n <= (ARLEN >> dsize);
  bool tilek_valid = (tile_k + get_pack(s1mcfg) - 1) / get_pack(s1mcfg) <= (TRLEN >> s1size) &&
                     (tile_k + get_pack(s2mcfg) - 1) / get_pack(s2mcfg) <= (TRLEN >> s2size);
  if (!tilem_valid || !tilen_valid || !tilek_valid) {
    longjmp_exception(EX_II);
  }
  // Check combination of ms1/ms2 and md type
  int8_t comb = check_comb(s1mcfg, s2mcfg, dmcfg);
  if (comb == -1) {
    // Invalid type combination
    longjmp_exception(EX_II);
  }
#if defined(CONFIG_DIFFTEST_AMU_CTRL) || defined(CONFIG_SHARE_CTRL) || defined(PRINT_AMUCTRLIO)
  uint8_t m_s_sz = s1size;
  uint8_t m_d_sz = dsize;
#endif
  bool s1_signed = is_signed_int_mtype(s1mcfg.type_code);
  bool s2_signed = is_signed_int_mtype(s2mcfg.type_code);
#ifndef CONFIG_SHARE_REF
  // When NEMU is not used as a reference model, execute MMA here directly.
  if (comb == 0) /* int mma */ {
    int64_t int_max = INT64_MAX >> (64 - 8 * (1 << dsize));
    int64_t int_min = INT64_MIN >> (64 - 8 * (1 << dsize));
    for (int i = 0; i < tile_m; i++) {
      for (int j = 0; j < tile_n; j++) {
        for (int k = 0; k < tile_k; k++) {
          get_mreg(ts1, i, k, &tmp_reg[1], s1size, s1_signed);
          get_mreg(ts2, j, k, &tmp_reg[2], s2size, s2_signed);
          get_mreg(td, i, j, &tmp_reg[0], dsize, true);
          if (msaten->val) {
            int64_t result = (int64_t)tmp_reg[1] * (int64_t)tmp_reg[2] + (int64_t)tmp_reg[0];
            if (result > int_max) {
              result = int_max;
            } else if (result < int_min) {
              result = int_min;
            }
            set_mreg(td, i, j, result, dsize);
          } else {
            tmp_reg[0] = tmp_reg[1] * tmp_reg[2] + tmp_reg[0];
            set_mreg(td, i, j, tmp_reg[0], dsize);
          }
        }
      }
    }
  } else /* comb == 1, float mma */ {
    word_t FPCALL_TYPE = FPCALL_W64;
    switch (s1size) {
      case 0:
        Loge("fp8 and lower precision's mma not supported"); longjmp_exception(EX_II);
        break;
      case 1:
        switch (dsize) {
          case 1:
            FPCALL_TYPE = FPCALL_W16;
            break;
          case 2:
            FPCALL_TYPE = FPCALL_W16_to_32;
            break;
          default:
            Loge("type not supported"); longjmp_exception(EX_II);
            break;
        }
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
    for (int i = 0; i < tile_m; i++) {
      for (int j = 0; j < tile_n; j++) {
        for (int k = 0; k < tile_k; k++) {
          get_mreg(ts1, i, k, &tmp_reg[1], s1size, false);
          get_mreg(ts2, j, k, &tmp_reg[2], s2size, false);

          get_mreg(td, i, j, &tmp_reg[0], dsize, false);
          rtl_hostcall(s, HOSTCALL_MFP, &tmp_reg[0], &tmp_reg[1], &tmp_reg[2], FPCALL_CMD(FPCALL_MADD, FPCALL_TYPE));
          set_mreg(td, i, j, tmp_reg[0], dsize);
        }
      }
    }
  }
#endif // CONFIG_SHARE_REF
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mma_emplace(td, mxrm->val, msaten->val, comb, ts1, ts2,
                      mtilem->val, mtilen->val, mtilek->val,
                      (s1_signed << 2) | m_s_sz, (s2_signed << 2) | m_s_sz, m_d_sz);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mma_emplace(td, msaten->val, comb, ts1, ts2,
              mtilem->val, mtilen->val, mtilek->val,
              4 | m_s_sz, 4 | m_s_sz, m_d_sz);
#endif // CONFIG_SHARE_CTRL
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=0 \n"
    "            md=%ld, sat=%ld, isfp=%d, ms1=%ld, ms2=%ld\n"
    "            mtilem=%ld, mtilen=%ld, mtilek=%ld, types1=%#x, types2=%#x, typed=%#x\n",
    td, msaten->val, comb, ts1, ts2,
    mtilem->val, mtilen->val, mtilek->val, 4 | m_s_sz, 4 | m_s_sz, m_d_sz);
#endif // PRINT_AMUCTRLIO
}

def_EHelper(mzero) {
  mp_set_dirty();
#ifndef CONFIG_SHARE_REF
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
#endif // CONFIG_SHARE_REF
#ifdef CONFIG_DIFFTEST_AMU_CTRL
  amu_ctrl_queue_mzero_emplace(true, s->dest.reg);
#endif // CONFIG_DIFFTEST_AMU_CTRL
#ifdef CONFIG_SHARE_CTRL
  cutest_mzero_emplace(true, s->dest.reg);
#endif // CONFIG_SHARE_CTRL
}

#endif // CONFIG_RV_AME
