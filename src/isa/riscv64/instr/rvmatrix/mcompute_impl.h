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

#include <common.h>
#ifdef CONFIG_RVMATRIX

#ifndef __RISCV64_MCOMPUTE_IMPL_H__
#define __RISCV64_MCOMPUTE_IMPL_H__

#include "mreg.h"
#include "../local-include/intr.h"
#include "mcommon.h"

void require_matrix();

#endif // __RISCV64_MCOMPUTE_IMPL_H__
#endif // CONFIG_RVMATRIX