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
#include <stdint.h>

#ifdef CONFIG_RVMATRIX

#include "mcompute_impl.h"
#include <cpu/cpu.h>
#include "mcommon.h"

void require_matrix() {
  // if (mstatus->ms == 0) {
  //   longjmp_exception(EX_II);
  // }
  // #ifdef CONFIG_RVH
  // if (cpu.v && vsstatus->ms == 0) {
  //   longjmp_exception(EX_II);
  // }
  // #endif
}

#endif // CONFIG_RVMATRIX