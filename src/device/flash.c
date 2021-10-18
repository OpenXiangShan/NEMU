#include <utils.h>
#include <device/map.h>

static uint8_t *flash_base = NULL;
static FILE *fp = NULL;
static uint32_t preset_flash[] = {
  0x00000413,
  0x0000a117,
  0xffc10113,
  0x00002537,
  0x30052073,
  0x5006b
};

static void flash_io_handler(uint32_t offset, int len, bool is_write) {
  // if(!is_write){
  //   printf("Flash read offset %x len %x\n", offset, len);
  //   printf("data %lx\n", *(uint64_t*)&flash_base[offset]);
  // }
  return;
}

void init_flash() {
  flash_base = new_space(CONFIG_FLASH_SIZE);
  const char *img = CONFIG_FLASH_IMG_PATH;
  fp = fopen(img, "r");
  if (fp == NULL) {
    Log("Can not find flash image: %s", img);
    Log("Use built-in image instead");
    flash_base = (uint8_t*) preset_flash;
  } else {
    __attribute__((unused)) int ret;
    ret = fread(flash_base, 1, CONFIG_FLASH_SIZE, fp);
  }
  add_mmio_map("flash", CONFIG_FLASH_START_ADDR, flash_base, CONFIG_FLASH_SIZE, flash_io_handler);
}
