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

#define NR_MMIO_MAP   16
#define NR_MMREG_MAP  32

static IOMap mmio_maps[NR_MMIO_MAP] = {};
static int nr_mmio_map = 0;

static IOMap mmreg_maps[NR_MMREG_MAP] = {};
static int nr_mmreg_map = 0;

#ifdef CONFIG_ENABLE_CONFIG_MMIO_SPACE

static const uint64_t mmio_spec_bound[] = {__MMIO_SPECE_RANGE__};
#define MMIO_SPEC_NUM (sizeof(mmio_spec_bound) / sizeof(uint64_t))
#define MMIO_SPEC_PAIR_NUM MMIO_SPEC_NUM / 2
static_assert(MMIO_SPEC_NUM % 2 == 0, "The address space of mmio needs to be specified in pairs.");

#endif // CONFIG_ENABLE_CONFIG_MMIO_SPACE

static inline IOMap* fetch_mm_map(IOMap* maps, int nr_map, paddr_t addr) {
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
  int mapid = find_mapid_by_addr(mmio_maps, nr_mmio_map, addr);
  return (mapid == -1 ? false : true);
#endif // CONFIG_ENABLE_CONFIG_MMIO_SPACE

}

/* device interface */
void add_mmio_map(const char *name, paddr_t addr, void *space, uint32_t len, io_callback_t callback) {
  assert(nr_mmio_map < NR_MMIO_MAP);
  mmio_maps[nr_mmio_map] = (IOMap){ .name = name, .low = addr, .high = addr + len - 1,
    .space = space, .callback = callback };
  // Log("Add mmio map '%s' at [" FMT_PADDR ", " FMT_PADDR "]",
  //     maps[nr_map].name, maps[nr_map].low, maps[nr_map].high);
  // fflush(stdout);

  nr_mmio_map ++;
}

void add_mmreg_map(const char *name, paddr_t addr, void *space, uint32_t len, io_callback_t callback) {
  assert(nr_mmreg_map < NR_MMREG_MAP);
  printf("add memory map reg: 0x%lx\n", addr);
  mmreg_maps[nr_mmreg_map] = (IOMap){ .name = name, .low = addr, .high = addr + len - 1,
    .space = space, .callback = callback };
  nr_mmreg_map ++;
}

bool mmio_is_real_device(paddr_t addr) {
  IOMap *map = fetch_mm_map(mmio_maps, nr_mmio_map, addr);
  return map != NULL && addr <= map->high && addr >= map->low;
}

bool mmio_is_real_mmreg(paddr_t addr) {
  IOMap *map = fetch_mm_map(mmreg_maps, nr_mmreg_map, addr);
  return map != NULL && addr <= map->high && addr >= map->low;
}


/* bus interface */
__attribute__((noinline))
word_t mmio_read(paddr_t addr, int len) {
  IOMap* maps = mmio_is_real_device(addr) ? mmio_maps : mmreg_maps;
  int nr_maps = mmio_is_real_device(addr) ? nr_mmio_map : nr_mmreg_map;
  return map_read(addr, len, fetch_mm_map(maps, nr_maps, addr));
}

__attribute__((noinline))
void mmio_write(paddr_t addr, int len, word_t data) {
  IOMap* maps = mmio_is_real_device(addr) ? mmio_maps : mmreg_maps;
  int nr_maps = mmio_is_real_device(addr) ? nr_mmio_map : nr_mmreg_map;
  map_write(addr, len, data, fetch_mm_map(maps, nr_maps, addr));
}