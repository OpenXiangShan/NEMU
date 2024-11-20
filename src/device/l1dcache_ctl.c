/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#include <utils.h>
#include <device/map.h>

static const uint64_t l1dcache_ctl_space_bound[] = {__L1DCACHE_CTL_SPACE_RANGE__};
#define L1DCACHE_CTL_SPEC_NUM      (sizeof(l1dcache_ctl_space_bound) / sizeof(uint64_t))
#define L1DCACHE_CTL_SPEC_PAIR_NUM (L1DCACHE_CTL_SPEC_NUM / 2)
static_assert(L1DCACHE_CTL_SPEC_NUM % 2 == 0, "The address space of l1dcachectl needs to be spaceified in pairs.");

static void l1dcache_ctl_io_handler(uint32_t offset, int len, bool is_write) {
  // Fake l1dcachectl handler, empty now
  return;
}

void init_l1dcache_ctl() {
  uint32_t upper_bound = l1dcache_ctl_space_bound[1];
  uint32_t lower_bound = l1dcache_ctl_space_bound[0];
  uint32_t space_size = upper_bound - lower_bound + 1;
  uint32_t *base_addr = (uint32_t *)new_space(space_size);
  add_mmio_map("L1CacheCtl", lower_bound, base_addr, space_size, l1dcache_ctl_io_handler);
}
