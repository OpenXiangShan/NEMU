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
#include <stdlib.h>

// put flash below the physical memory and allow a max size of 256MB.
// See the notes at paddr.c:79 for why we fix the address here.
uint8_t *flash_base  = (uint8_t *)0xf0000000ul;

uint64_t get_flash_size(){
  return CONFIG_FLASH_SIZE;
}

static void flash_io_handler(uint32_t offset, int len, bool is_write) {
  Assert(!is_write, "write to flash is illegal");
  return;
}

void convert_to_absolute_path(const char *input_path, char *output_path) {
#define PATH_MAX 4096
  if (input_path == NULL || output_path == NULL) {
      fprintf(stderr, "Invalid input or output path\n");
      return;
  }

  char expanded_path[PATH_MAX] = {0};

  // Check for $(ENV) pattern
  if (input_path[0] == '$' && input_path[1] == '(') {
      const char *end = strchr(input_path, ')');
      if (end) {
          char env_var[256] = {0};
          size_t var_length = end - input_path - 2;

          // Extract environment variable name
          strncpy(env_var, input_path + 2, var_length);
          env_var[var_length] = '\0';

          // Get environment variable value
          const char *env_value = getenv(env_var);
          if (env_value) {
              snprintf(expanded_path, sizeof(expanded_path), "%s%s", env_value, end + 1);
          } else {
              fprintf(stderr, "Environment variable %s not found\n", env_var);
              return;
          }
      } else {
          fprintf(stderr, "Invalid input format\n");
          return;
      }
  } else {
      // If no $(ENV), copy the input path
      strncpy(expanded_path, input_path, sizeof(expanded_path) - 1);
  }

  // Convert to absolute path
  if (realpath(expanded_path, output_path) == NULL) {
      perror("realpath");
  }
#undef PATH_MAX
}


void load_flash_contents(const char *flash_img) {
  // create mmap with zero contents
  assert(CONFIG_FLASH_SIZE <= 0x10000000UL);
  Log("Flash load img from %s\n", flash_img);
  FILE *fp = NULL;
  void *ret = mmap((void *)flash_base, CONFIG_FLASH_SIZE, PROT_READ | PROT_WRITE,
    MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (ret != flash_base) {
    perror("mmap");
    assert(0);
  }

  char real_flash_img[4096];

  if(flash_img) {
    convert_to_absolute_path(flash_img, real_flash_img);
  }

  if (!flash_img || !(fp = fopen(real_flash_img, "r"))) {
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
    ret = fread(flash_base, 1, size, fp);
    fclose(fp);
  }
}

void init_flash() {
#ifndef CONFIG_SHARE
  load_flash_contents(CONFIG_FLASH_IMG_PATH);
#endif
  add_mmio_map("flash", CONFIG_FLASH_START_ADDR, flash_base, CONFIG_FLASH_SIZE, SKIP_FREE, flash_io_handler);
}
