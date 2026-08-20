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

#ifndef __RISCV64_MLDST_IMPL_H__
#define __RISCV64_MLDST_IMPL_H__

#include <common.h>
#include <cpu/decode.h>
#include "mreg.h"

#if defined(CONFIG_RV_AME)

uint8_t get_size(mcfg_t cfg);
void check_size(Decode *s, uint64_t rmax_mreg, uint64_t cmax_mreg,
                char m_name, uint8_t dsize, bool raw_fp4);
void exec_mld(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t td, int row, int column, uint8_t dsize,
              bool is_trans, char m_name, bool raw_fp4);
void exec_mst(Decode *s, uint64_t base_addr, uint64_t row_byte_stride,
              uint64_t ts3, int row, int column, uint8_t dsize,
              bool is_trans, char m_name, bool raw_fp4);
void mld(Decode *s, bool is_trans, char m_name);
void mst(Decode *s, bool is_trans, char m_name);
void mld_whole(Decode *s, char m_name);
void mst_whole(Decode *s, char m_name);

#endif // CONFIG_RV_AME

#endif // __RISCV64_MLDST_IMPL_H__
