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

// #define PRINT_AMUCTRLIO

// TODO: not consider mstart now
void mld(Decode *s, bool is_trans, char m_name) {
  uint64_t base_addr = reg_l(s->src1.reg);
  uint64_t row_byte_stride = reg_l(s->src2.reg);
  uint64_t td = s->dest.reg;
  int rmax_mreg = 0, cmax_mreg = 0;
  switch (m_name) {
    case 'a':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilek->val;
      break;
    case 'b':
      rmax_mreg  = mtilen->val;
      cmax_mreg  = mtilek->val;
      break;
    case 'c':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilen->val;
      break;
    default:
      Assert(false, "mld %c: invalid reg selection!\n", m_name);
      break;
  }

  Assert((rmax_mreg <= MRNUM),
         "mld %c: mtile config should not larger than tile_reg size!\n", m_name);
  if (m_name == 'c') {
    Assert((cmax_mreg <= MRENUM8 * MAMUL / (s->m_width)),
           "mld %c: mtile config should not larger than tile_reg size!\n", m_name);
  } else {
    Assert((cmax_mreg <= MRENUM8 / (s->m_width)),
           "mld %c: mtile config should not larger than tile_reg size!\n", m_name);
  }

#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=0, transpose=%d, baseAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, isacc=%d\n",
    td, is_trans, base_addr, row_byte_stride, rmax_mreg, cmax_mreg, s->m_eew, m_name == 'c');
#endif

  rtl_lmm(s, &base_addr, &row_byte_stride,
    rmax_mreg, cmax_mreg, s->m_eew, is_trans ^ (m_name == 'b'),
    MMU_TRANSLATE, m_name == 'c', td);
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
      break;
    case 'b':
      rmax_mreg  = mtilek->val;
      cmax_mreg  = mtilen->val;
      break;
    case 'c':
      rmax_mreg  = mtilem->val;
      cmax_mreg  = mtilen->val;
      break;
    default:
      Assert(false, "mst %c: invalid reg selection!\n", m_name);
      break;
  }

  Assert((rmax_mreg <= MRNUM),
         "mst %c: mtile config should not larger than tile_reg size!\n", m_name);
  if (m_name == 'c') {
    Assert((cmax_mreg <= MRENUM8 * MAMUL / (s->m_width)),
         "mst %c: mtile config should not larger than tile_reg size!\n", m_name);
  } else {
    Assert((cmax_mreg <= MRENUM8 / (s->m_width)),
         "mst %c: mtile config should not larger than tile_reg size!\n", m_name);
  }

#ifdef PRINT_AMUCTRLIO
  fprintf(stderr,
    "[AmuCtrlIO] op=1 \n"
    "            ms=%ld, ls=1, transpose=%d, baseAddr=%#lx, stride=%#lx\n"
    "            row=%d, col=%d, width=%#x, isacc=%d\n",
    ts3, is_trans, base_addr, row_byte_stride, rmax_mreg, cmax_mreg, s->m_eew, m_name == 'c');
#endif

  rtl_smm(s, &base_addr, &row_byte_stride,
    rmax_mreg, cmax_mreg, s->m_eew, is_trans ^ (m_name == 'b'),
    MMU_TRANSLATE, m_name == 'c', ts3);
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

def_EHelper(mltr) {
}

def_EHelper(mstr) {
}

def_EHelper(mlacc) {
}

def_EHelper(msacc) {
}

#endif // CONFIG_RVMATRIX