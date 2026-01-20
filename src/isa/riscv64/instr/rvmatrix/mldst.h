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

#ifdef CONFIG_RVMATRIX

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

void check_size(Decode *s, int rmax_mreg, int cmax_mreg, char m_name) {
  bool valid = true;
  if (rmax_mreg > ROWNUM) {
    valid = false;
  }
  switch (m_name) {
    case 'a': case 'b':
      if (cmax_mreg > TRENUM8 / (1 << s->m_d_sz)) {
        valid = false;
      }
      break;
    case 'c':
      if (cmax_mreg > ARENUM8 / (1 << s->m_d_sz)) {
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

void mld(Decode *s, bool is_trans, char m_name) {
  mp_set_dirty();
  uint64_t base_addr = reg_l(s->src1.reg);
  uint64_t row_byte_stride = reg_l(s->src2.reg);
  uint64_t td = s->dest.reg;
  int rmax_mreg = 0, cmax_mreg = 0;
  switch (m_name) {
    case 'a':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilek->val;
      Assert(s->dest.reg < 4,
        "%u is not a valid tile matrix reg idx!\n", s->dest.reg);
      break;
    case 'b':
      rmax_mreg  = mtilen->val;
      cmax_mreg  = mtilek->val;
      Assert(s->dest.reg < 4,
        "%u is not a valid tile matrix reg idx!\n", s->dest.reg);
      break;
    case 'c':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilen->val;
      Assert(s->dest.reg >= 4,
        "%u is not a valid acc matrix reg idx!\n", s->dest.reg);
      break;
    case 'm':
      rmax_mreg  = ROWNUM;
      if (s->dest.reg < 4) { // tile register
        cmax_mreg  = TRENUM8 / (1 << s->m_d_sz);
      } else { // acc register
        cmax_mreg  = ARENUM8 / (1 << s->m_d_sz);
      }
      break;
    default:
      Assert(false, "mld %c: invalid reg selection!\n", m_name);
      break;
  }

  check_size(s, rmax_mreg, cmax_mreg, m_name);

#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=0, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    td, is_trans, base_addr, row_byte_stride, rmax_mreg, cmax_mreg, s->m_d_sz, m_name);
#endif

  rtl_lmm(s, &base_addr, &row_byte_stride,
    rmax_mreg, cmax_mreg, s->m_d_sz, is_trans,
    MMU_TRANSLATE, m_name, td);
}

void mst(Decode *s, bool is_trans, char m_name) {
  uint64_t base_addr = reg_l(s->src1.reg);
  uint64_t row_byte_stride = reg_l(s->src2.reg);
  uint64_t ts3 = s->dest.reg;
  int rmax_mreg = 0, cmax_mreg = 0;
  switch (m_name) {
    case 'a':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilek->val;
      Assert(s->dest.reg < 4,
        "%u is not a valid tile matrix reg idx!\n", s->dest.reg);
      break;
    case 'b':
      rmax_mreg  = mtilen->val;
      cmax_mreg  = mtilek->val;
      Assert(s->dest.reg < 4,
        "%u is not a valid tile matrix reg idx!\n", s->dest.reg);
      break;
    case 'c':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilen->val;
      Assert(s->dest.reg >= 4,
        "%u is not a valid acc matrix reg idx!\n", s->dest.reg);
      break;
    case 'm':
      rmax_mreg  = ROWNUM;
      if (s->dest.reg < 4) { // tile register
        cmax_mreg  = TRENUM8 / (1 << s->m_d_sz);
      } else { // acc register
        cmax_mreg  = ARENUM8 / (1 << s->m_d_sz);
      }
      break;
    default:
      Assert(false, "mst %c: invalid reg selection!\n", m_name);
      break;
  }
  
  check_size(s, rmax_mreg, cmax_mreg, m_name);

#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=1, transpose=%d, baseVAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, m_name=%c\n",
    ts3, is_trans, base_addr, row_byte_stride, rmax_mreg, cmax_mreg, s->m_d_sz, m_name);
#endif

  rtl_smm(s, &base_addr, &row_byte_stride,
    rmax_mreg, cmax_mreg, s->m_d_sz, is_trans,
    MMU_TRANSLATE, m_name, ts3);

  mstore_queue_emplace(base_addr, row_byte_stride, rmax_mreg, cmax_mreg, s->m_d_sz, is_trans);
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

def_EHelper(mlm) {
  mld(s, false, 'm');
}

def_EHelper(msm) {
  mst(s, false, 'm');
}

#endif // CONFIG_RVMATRIX