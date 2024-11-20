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
#include <common.h>
#include <device/map.h>

#ifdef CONFIG_ENABLE_CONFIG_MMIO_SPACE

// L1 DCache Ctl
static const uint64_t l1dcachectl_space_bound[] = {__L1DCACHECTL_SPACE_RANGE__};
#define L1DCACHECTL_SPEC_NUM      (sizeof(l1dcachectl_space_bound) / sizeof(uint64_t))
#define L1DCACHECTL_SPEC_PAIR_NUM (L1DCACHECTL_SPEC_NUM / 2)
static_assert(L1DCACHECTL_SPEC_NUM % 2 == 0, "The address space of l1dcachectl needs to be spaceified in pairs.");

static void l1dcachectl_io_handler(uint32_t offset, int len, bool is_write) {
  // nothing to return here
  return ;
}

void init_l1dcachectl() {
  uint32_t space_size = l1dcachectl_space_bound[1] - l1dcachectl_space_bound[0] + 1;
  uint32_t *base_addr = (uint32_t *)new_space(space_size);

  add_mmreg_map(
    "L1DCacheCtl",
    l1dcachectl_space_bound[0],
    base_addr,
    space_size,
    l1dcachectl_io_handler
  );
}

// L3 Cache Ctl
static const uint64_t l3cachectl_space_bound[] = {__L3CACHECTL_SPACE_RANGE__};
#define L3CACHECTL_SPEC_NUM       (sizeof(l3cachectl_space_bound) / sizeof(uint64_t))
#define L3CACHECTL_SPEC_PAIR_NUM  (L3CACHECTL_SPEC_NUM / 2)
static_assert(L3CACHECTL_SPEC_NUM % 2 == 0, "The address space of l3cachectl needs to be spaceified in pairs.");

static void l3cachectl_io_handler(uint32_t offset, int len, bool is_write) {
  // nothing to return here
  return ;
}

void init_l3cachectl() {
  uint32_t space_size = l3cachectl_space_bound[1] - l3cachectl_space_bound[0] + 1;
  uint32_t *base_addr = (uint32_t *)new_space(space_size);

  add_mmreg_map(
    "L3CacheCtl",
    l3cachectl_space_bound[0],
    base_addr,
    space_size,
    l3cachectl_io_handler
  );
}

#endif // CONFIG_ENABLE_CONFIG_MMIO_SPACE