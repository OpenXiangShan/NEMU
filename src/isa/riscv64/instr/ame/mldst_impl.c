/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

#if defined(CONFIG_RV_AME)

#include <limits.h>
#include <stdint.h>

#include "cpu/cpu.h"
#include "cpu/exec.h"
#include "mldst_impl.h"
#include "mreg.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#include "ame/mstore_queue_wrapper.h"
#include "mcommon.h"
#ifdef CONFIG_RV_AME_FP4
#include "ame/raw-fp4.h"
#endif

#ifdef PRINT_AMUCTRLIO
#include <stdio.h>
#endif

uint8_t get_size(mcfg_t cfg) {
  uint8_t dsize = 0;
  switch (cfg.type_code) {
    case MTYPECODE_INT8:
    case MTYPECODE_UINT8:
    case MTYPECODE_FP8E5M2:
    case MTYPECODE_FP8E4M3:
    case MTYPECODE_INT4:
    case MTYPECODE_UINT4:
    case MTYPECODE_NVFP4:
    case MTYPECODE_MXFP4:
    case MTYPECODE_FP2PACK4:
    case MTYPECODE_FP2PACK5:
      dsize = 0;
      break;
    case MTYPECODE_FP16:
    case MTYPECODE_BF16:
      dsize = 1;
      break;
    case MTYPECODE_INT32:
    case MTYPECODE_TF32:
    case MTYPECODE_FP32:
      dsize = 2;
      break;
    default:
      longjmp_exception(EX_II);
      break;
  }
  return dsize;
}

void check_size(Decode *s, uint64_t rmax_mreg, uint64_t cmax_mreg,
                char m_name, uint8_t dsize, bool raw_fp4) {
  // Validate CSR values before narrowing them for the int-based memory API.
  bool valid = rmax_mreg <= ROWNUM && rmax_mreg <= INT_MAX &&
               cmax_mreg <= INT_MAX;
  switch (m_name) {
    case 'a': case 'b':
      if (cmax_mreg > (raw_fp4 ? TRENUM8 * 2 : TRENUM8 / (1 << dsize))) {
        valid = false;
      }
      break;
    case 'c':
      if (raw_fp4 || cmax_mreg > ARENUM8 / (1 << dsize)) {
        valid = false;
      }
      break;
    case 'm':
      break;
    default:
      valid = false;
      break;
  }
  if (!valid) {
    longjmp_exception(EX_II);
  }
}

static bool validate_raw_fp4_access(mcfg_t cfg, uint64_t mreg_id,
                                    char m_name) {
#ifdef CONFIG_RV_AME_FP4
  bool raw_fp4 = is_raw_fp4(cfg);
  if (is_raw_fp4_type_code(cfg) && !raw_fp4) {
    longjmp_exception(EX_II);
  }
  if (raw_fp4 && (mreg_id >= 4 || (m_name != 'a' && m_name != 'b'))) {
    longjmp_exception(EX_II);
  }
  return raw_fp4;
#else
  (void)cfg;
  (void)mreg_id;
  (void)m_name;
  return false;
#endif
}

void exec_mld(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t td, int row, int column, uint8_t dsize,
              bool is_trans, char m_name, bool raw_fp4) {

#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=0, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    td, is_trans, base_addr, row_byte_stride, row, column, dsize, m_name);
#endif

#ifdef CONFIG_RV_AME_FP4
  if (raw_fp4) {
    if (raw_fp4_matrix_access(s, base_addr, row_byte_stride, row, column,
                              is_trans, false, td)) {
      mp_set_dirty();
    }
    return;
  }
#else
  (void)raw_fp4;
#endif

  mp_set_dirty();

  rtl_lmm(s, &base_addr, &row_byte_stride,
    row, column, dsize, is_trans,
    MMU_TRANSLATE, m_name, td);
}

void exec_mst(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t ts3, int row, int column, uint8_t dsize,
              bool is_trans, char m_name, bool raw_fp4) {
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=1, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    ts3, is_trans, base_addr, row_byte_stride, row, column, dsize, m_name);
#endif

#ifdef CONFIG_RV_AME_FP4
  if (raw_fp4) {
    if (raw_fp4_matrix_access(s, base_addr, row_byte_stride, row, column,
                              is_trans, true, ts3)) {
      mstore_queue_emplace(base_addr, row_byte_stride, row, column,
                           MSEW_FP4, is_trans);
    }
    return;
  }
#else
  (void)raw_fp4;
#endif

  rtl_smm(s, &base_addr, &row_byte_stride,
    row, column, dsize, is_trans,
    MMU_TRANSLATE, m_name, ts3);

  mstore_queue_emplace(base_addr, row_byte_stride, row, column, dsize, is_trans);
}

void mld(Decode *s, bool is_trans, char m_name) {
  uint64_t td = s->dest.reg;
  uint8_t dsize = get_size(cpu.mcfg[td]);
  bool raw_fp4 = validate_raw_fp4_access(cpu.mcfg[td], td, m_name);
  uint64_t rmax_mreg = 0, cmax_mreg = 0;

  switch (m_name) {
    case 'a':
      rmax_mreg = mtilem->val;
      cmax_mreg = mtilek->val;
      break;
    case 'b':
      rmax_mreg = mtilen->val;
      cmax_mreg = mtilek->val;
      break;
    case 'c':
      rmax_mreg = mtilem->val;
      cmax_mreg = mtilen->val;
      break;
    default:
      Assert(false, "mld %c: invalid matrix selection!\n", m_name);
      break;
  }

  if (cpu.mcfg[td].type_code == MTYPECODE_FP2PACK4) {
    Assert(cpu.mcfg[td].table_set == 0, "fp2pack4 requires table 0");
    Assert(td < 4 && m_name == 'b' && !is_trans,
           "fp2pack4 supports only a normal B load into tr0-tr3");
    Assert(rmax_mreg == 128 && cmax_mreg == 64,
           "fp2pack4 requires B shape 128x64");
    Assert(reg_l(s->src2.reg) == 16,
           "fp2pack4 requires a 16-byte B row stride");
    Assert((reg_l(s->src1.reg) & 63) == 0,
           "fp2pack4 B base must be 64-byte aligned");
    Assert((reg_l(s->src1.reg) & 0xfff) <= 0x800,
           "fp2pack4 B panel must fit in one 4-KiB page");
  }
  check_size(s, rmax_mreg, cmax_mreg, m_name, dsize, raw_fp4);
  exec_mld(s, reg_l(s->src1.reg), reg_l(s->src2.reg), td,
    (int)rmax_mreg, (int)cmax_mreg, dsize, is_trans, m_name, raw_fp4);
}

void mst(Decode *s, bool is_trans, char m_name) {
  uint64_t ts3 = s->dest.reg;
  uint8_t dsize = get_size(cpu.mcfg[ts3]);
  bool raw_fp4 = validate_raw_fp4_access(cpu.mcfg[ts3], ts3, m_name);
  Assert(cpu.mcfg[ts3].type_code != MTYPECODE_FP2PACK4,
         "fp2pack4 store is undefined; use a packed B load only");
  uint64_t rmax_mreg = 0, cmax_mreg = 0;

  switch (m_name) {
    case 'a':
      rmax_mreg = mtilem->val;
      cmax_mreg = mtilek->val;
      break;
    case 'b':
      rmax_mreg = mtilen->val;
      cmax_mreg = mtilek->val;
      break;
    case 'c':
      rmax_mreg = mtilem->val;
      cmax_mreg = mtilen->val;
      break;
    default:
      Assert(false, "mst %c: invalid matrix selection!\n", m_name);
      break;
  }

  check_size(s, rmax_mreg, cmax_mreg, m_name, dsize, raw_fp4);
  exec_mst(s, reg_l(s->src1.reg), reg_l(s->src2.reg), ts3,
    (int)rmax_mreg, (int)cmax_mreg, dsize, is_trans, m_name, raw_fp4);
}

void mld_whole(Decode *s, char m_name) {
  uint64_t td = s->dest.reg;
  bool raw_fp4 = validate_raw_fp4_access(cpu.mcfg[td], td, m_name);
  Assert(cpu.mcfg[td].type_code != MTYPECODE_FP2PACK4,
         "fp2pack4 whole-register load is undefined");
  uint8_t dsize = m_name == 'c' ? 2 : 0;
  uint64_t row_byte_stride = m_name == 'c' ? ARENUM8 : TRENUM8;

  exec_mld(s, reg_l(s->src1.reg), row_byte_stride, td,
    ROWNUM, m_name == 'c' ? ARENUM32 : (raw_fp4 ? TRENUM8 * 2 : TRENUM8),
    dsize, false, m_name, raw_fp4);
}

void mst_whole(Decode *s, char m_name) {
  uint64_t ts3 = s->dest.reg;
  bool raw_fp4 = validate_raw_fp4_access(cpu.mcfg[ts3], ts3, m_name);
  Assert(cpu.mcfg[ts3].type_code != MTYPECODE_FP2PACK4,
         "fp2pack4 whole-register store is undefined");
  uint8_t dsize = m_name == 'c' ? 2 : 0;
  uint64_t row_byte_stride = m_name == 'c' ? ARENUM8 : TRENUM8;

  exec_mst(s, reg_l(s->src1.reg), row_byte_stride, ts3,
    ROWNUM, m_name == 'c' ? ARENUM32 : (raw_fp4 ? TRENUM8 * 2 : TRENUM8),
    dsize, false, m_name, raw_fp4);
}

#endif // CONFIG_RV_AME
