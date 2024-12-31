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

#include <device/map.h>

#define NR_MAP 16

static IOMap maps[NR_MAP] = {};
static int nr_map = 0;

#ifdef CONFIG_ENABLE_CONFIG_MMIO_SPACE

static const uint64_t mmio_spec_bound[] = {__MMIO_SPECE_RANGE__};
#define MMIO_SPEC_NUM (sizeof(mmio_spec_bound) / sizeof(uint64_t))
#define MMIO_SPEC_PAIR_NUM MMIO_SPEC_NUM / 2
static_assert(MMIO_SPEC_NUM % 2 == 0, "The address space of mmio needs to be specified in pairs.");

#endif // CONFIG_ENABLE_CONFIG_MMIO_SPACE

static inline IOMap* fetch_mmio_map(paddr_t addr) {
  int mapid = find_mapid_by_addr(maps, nr_map, addr);
  return (mapid == -1 ? NULL : &maps[mapid]);
}

bool is_in_mmio(paddr_t addr) {
#ifdef CONFIG_ENABLE_CONFIG_MMIO_SPACE
  for (int i = 0; i < MMIO_SPEC_PAIR_NUM; ++i) {
    if (mmio_spec_bound[i] <= addr && addr <= mmio_spec_bound[i + 1]) {
      Logm("is in mmio: " FMT_PADDR, addr);
      return true;
    }
  }
  return false;
#else
  int mapid = find_mapid_by_addr(maps, nr_map, addr);
  return (mapid == -1 ? false : true);
#endif // CONFIG_ENABLE_CONFIG_MMIO_SPACE

}

/* device interface */
void add_mmio_map(const char *name, paddr_t addr, void *space, uint32_t len, io_callback_t callback) {
  add_mmio_map_with_diff(name, addr, space, len, MMIO_READ|MMIO_WRITE|MMIO_EXEC, callback);
}

/* device interface */
void add_mmio_map_with_diff(const char *name, paddr_t addr, void *space, uint32_t len, int mmio_diff_type, io_callback_t callback) {
  assert(nr_map < NR_MAP);
  maps[nr_map] = (IOMap){ .name = name, .low = addr, .high = addr + len - 1,
    .space = space, .mmio_diff_type = mmio_diff_type, .callback = callback };
  nr_map ++;
}

bool mmio_is_real_device(paddr_t addr) {
  IOMap *map = fetch_mmio_map(addr);
  return map != NULL && addr <= map->high && addr >= map->low;
}


/* bus interface */
__attribute__((noinline))
word_t mmio_read(paddr_t addr, int len) {
  return map_read(addr, len, fetch_mmio_map(addr));
}

__attribute__((noinline))
void mmio_write(paddr_t addr, int len, word_t data) {
  map_write(addr, len, data, fetch_mmio_map(addr));
}
