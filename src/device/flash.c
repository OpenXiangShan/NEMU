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
#include <sys/mman.h>

static uint8_t *flash_base = NULL;
static FILE *fp = NULL;

static void flash_io_handler(uint32_t offset, int len, bool is_write) {
  Assert(!is_write, "write to flash is illegal");
  return;
}

void load_flash_contents(const char *flash_img) {
  // create mmap with zero contents
  flash_base = mmap(NULL, CONFIG_FLASH_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (flash_base == MAP_FAILED) {
    Log("mmap for flash failed");
    assert(0);
  }

  if (!flash_img || !(fp = fopen(flash_img, "r"))) {
    Log("Can not find flash image: %s", flash_img);
    Log("Use built-in image instead");
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
    flash_base = new_space(CONFIG_FLASH_SIZE);
    ret = fread(flash_base, 1, size, fp);
    fclose(fp);
  }
}

void init_flash(const char *flash_img) {
  add_mmio_map("flash", CONFIG_FLASH_START_ADDR, flash_base, CONFIG_FLASH_SIZE, flash_io_handler);
}
