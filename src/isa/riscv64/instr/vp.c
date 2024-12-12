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
#ifdef CONFIG_RVV
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include <cpu/cpu.h>
#include <cpu/difftest.h>

bool vp_enable() {
#ifdef CONFIG_MODE_USER
  return true;
#else // !CONFIG_MODE_USER
  if (mstatus->vs == 0) {
    return false;
  }
#ifdef CONFIG_RVH
  if (cpu.v && vsstatus->vs == 0) {
    return false;
  }
#endif // CONFIG_RVH
  return true;
#endif // CONFIG_MODE_USER
}

void vp_set_dirty() {
  mstatus->vs = EXT_CONTEXT_DIRTY;
#ifdef CONFIG_RVH
  if (cpu.v == 1) {
    vsstatus->vs = EXT_CONTEXT_DIRTY;
  }
#endif //CONFIG_RVH
  IFDEF(CONFIG_DIFFTEST_DIRTY_FS_VS, ref_difftest_dirty_fsvs(SSTATUS_VS));
}
#endif // CONFIG_RVV