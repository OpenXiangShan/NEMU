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

#include "cpu/cpu.h"
#include <common.h>

#ifdef CONFIG_RV_AME

#include "cpu/exec.h"
#include "mreg.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"
#include "../local-include/reg.h"
#include "ext/mstore_queue_wrapper.h"
#include "mcommon.h"

// #define PRINT_AMUCTRLIO

#ifdef PRINT_AMUCTRLIO
#include <stdio.h>
#endif // PRINT_AMUCTRLIO

uint8_t get_size(mcfg_t cfg) {
  uint8_t dsize = 0;
  switch (cfg.type_code) {
    // set m_d_sz
    // 0 for 8bit, 1 for 16bit, 2 for 32bit, 3 for 4bit
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

void check_size(Decode *s, int rmax_mreg, int cmax_mreg, char m_name, uint8_t dsize) {
  bool valid = true;
  if (rmax_mreg > ROWNUM) {
    valid = false;
  }
  switch (m_name) {
    case 'a': case 'b':
      if (cmax_mreg > TRENUM8 / (1 << dsize)) {
        valid = false;
      }
      break;
    case 'c':
      if (cmax_mreg > ARENUM8 / (1 << dsize)) {
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

void exec_mld(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t td, int row, int column, uint8_t dsize,
              bool is_trans, char m_name) {
  mp_set_dirty();

#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=0, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    td, is_trans, base_addr, row_byte_stride, row, column, dsize, m_name);
#endif

  rtl_lmm(s, &base_addr, &row_byte_stride,
    row, column, dsize, is_trans,
    MMU_TRANSLATE, m_name, td);
}

void exec_mst(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t ts3, int row, int column, uint8_t dsize,
              bool is_trans, char m_name) {
#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=1, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    ts3, is_trans, base_addr, row_byte_stride, row, column, dsize, m_name);
#endif

  rtl_smm(s, &base_addr, &row_byte_stride,
    row, column, dsize, is_trans,
    MMU_TRANSLATE, m_name, ts3);

  mstore_queue_emplace(base_addr, row_byte_stride, row, column, dsize, is_trans);
}

// m_name: 'a'/'b'/'c'
void mld(Decode *s, bool is_trans, char m_name) {
  uint64_t td = s->dest.reg;
  uint8_t dsize = get_size(cpu.mcfg[td]);
  int rmax_mreg = 0, cmax_mreg = 0;

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

  check_size(s, rmax_mreg, cmax_mreg, m_name, dsize);
  exec_mld(s, reg_l(s->src1.reg), reg_l(s->src2.reg), td,
    rmax_mreg, cmax_mreg, dsize, is_trans, m_name);
}

void mst(Decode *s, bool is_trans, char m_name) {
  uint64_t ts3 = s->dest.reg;
  uint8_t dsize = get_size(cpu.mcfg[ts3]);
  int rmax_mreg = 0, cmax_mreg = 0;

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

  check_size(s, rmax_mreg, cmax_mreg, m_name, dsize);
  exec_mst(s, reg_l(s->src1.reg), reg_l(s->src2.reg), ts3,
    rmax_mreg, cmax_mreg, dsize, is_trans, m_name);
}

void mld_whole(Decode *s, char m_name) {
  uint64_t td = s->dest.reg;
  uint8_t dsize = m_name == 'c' ? 2 : 0;
  uint64_t row_byte_stride = m_name == 'c' ? ARENUM8 : TRENUM8;

  exec_mld(s, reg_l(s->src1.reg), row_byte_stride, td,
    ROWNUM, m_name == 'c' ? ARENUM32 : TRENUM8, dsize, false, m_name);
}

void mst_whole(Decode *s, char m_name) {
  uint64_t ts3 = s->dest.reg;
  uint8_t dsize = m_name == 'c' ? 2 : 0;
  uint64_t row_byte_stride = m_name == 'c' ? ARENUM8 : TRENUM8;

  exec_mst(s, reg_l(s->src1.reg), row_byte_stride, ts3,
    ROWNUM, m_name == 'c' ? ARENUM32 : TRENUM8, dsize, false, m_name);
}


def_EHelper(mla) {
  mld(s, false, 'a');
}

def_EHelper(mlb) {
  mld(s, false, 'b');
}

def_EHelper(mlc) {
  mld(s, false, 'c');
}

def_EHelper(mlat) {
  mld(s, true, 'a');
}

def_EHelper(mlbt) {
  mld(s, true, 'b');
}

def_EHelper(mlct) {
  mld(s, true, 'c');
}

def_EHelper(msa) {
  mst(s, false, 'a');
}

def_EHelper(msb) {
  mst(s, false, 'b');
}

def_EHelper(msc) {
  mst(s, false, 'c');
}

def_EHelper(msat) {
  mst(s, true, 'a');
}

def_EHelper(msbt) {
  mst(s, true, 'b');
}

def_EHelper(msct) {
  mst(s, true, 'c');
}

def_EHelper(mlawhole) {
  mld_whole(s, 'a');
}

def_EHelper(mlbwhole) {
  mld_whole(s, 'b');
}

def_EHelper(mlcwhole) {
  mld_whole(s, 'c');
}

def_EHelper(msawhole) {
  mst_whole(s, 'a');
}

def_EHelper(msbwhole) {
  mst_whole(s, 'b');
}

def_EHelper(mscwhole) {
  mst_whole(s, 'c');
}

#endif // CONFIG_RV_AME
