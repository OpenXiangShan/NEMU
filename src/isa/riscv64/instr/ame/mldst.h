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
#include "mcommon.h"
#include "mldst_impl.h"

// #define PRINT_AMUCTRLIO

#ifdef PRINT_AMUCTRLIO
#include <stdio.h>
#endif // PRINT_AMUCTRLIO

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
