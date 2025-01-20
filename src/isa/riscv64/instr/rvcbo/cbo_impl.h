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

#include "cpu/decode.h"

void cbo_zero(Decode *s);
void cbo_inval(Decode *s);
void cbo_flush(Decode *s);
void cbo_clean(Decode *s);

void cbo_zero_mmu(Decode *s);
void cbo_inval_mmu(Decode *s);
void cbo_flush_mmu(Decode *s);
void cbo_clean_mmu(Decode *s);