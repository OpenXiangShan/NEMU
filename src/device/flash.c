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

#include <utils.h>
#include <device/map.h>
#include <device/flash.h>
#include <sys/mman.h>

// put flash below the physical memory and allow a max size of 256MB.
// See the notes at paddr.c:79 for why we fix the address here.
static uint8_t *flash_base  = (uint8_t *)0xf0000000ul;
static FILE *fp = NULL;

static void flash_io_handler(uint32_t offset, int len, bool is_write) {
  Assert(!is_write, "write to flash is illegal");
  return;
}

void load_flash_contents(const char *flash_img) {
  // create mmap with zero contents
  assert(CONFIG_FLASH_SIZE <= 0x20000000UL);
  void *ret = mmap((void *)flash_base, CONFIG_FLASH_SIZE, PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (ret != flash_base) {
    perror("mmap");
    assert(0);
  }

  if (!flash_img || !(fp = fopen(flash_img, "r"))) {
    // Log("Can not find flash image: %s", flash_img);
    // Log("Use built-in image instead");
    uint32_t *p = (uint32_t *)flash_base;
    sscanf(CONFIG_FLASH_PRESET_CONTENT, "%x,%x,%x", p, p + 1, p + 2);
  } else {
    __attribute__((unused)) int ret;
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    Assert(
      size <= CONFIG_FLASH_SIZE,
      "img size %d is larger than flash size %d",
      size, CONFIG_FLASH_SIZE
    );
    ret = fread(flash_base, 1, size, fp);
    fclose(fp);
  }
}

uint8_t* get_flash_base() {
  return flash_base;
}

uint64_t get_flash_size() {
  return CONFIG_FLASH_SIZE;
}

void init_flash() {
#ifndef CONFIG_SHARE
  IFDEF(CONFIG_HAS_FLASH, load_flash_contents(__FLASH_IMG_PATH__));
#endif // CONFIG_SHARE
  add_mmio_map_with_diff("flash", CONFIG_FLASH_START_ADDR, flash_base, CONFIG_FLASH_SIZE, SKIP_FREE, flash_io_handler);
}
