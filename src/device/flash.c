#include <utils.h>
#include <device/map.h>

uint8_t *flash_base = NULL;
static FILE *fp = NULL;
// static const char *flash_img = CONFIG_FLASH_IMG_PATH;
static uint32_t preset_flash[] = {
  0x0010029b,
  0x01f29293,
  0x00028067
};

static void flash_io_handler(uint32_t offset, int len, bool is_write) {
  // if(!is_write){
  //   printf("Flash read offset %x len %x\n", offset, len);
  //   printf("data %lx\n", *(uint64_t*)&flash_base[offset]);
  // }
  Assert(!is_write, "write to flash is illegal");
  return;
}

void init_flash(const char *flash_img) {
  printf("[NEMU] Initializing NEMU flash device.\n");
#if CONFIG_HAS_FLASH == 1
  fp = fopen(flash_img, "r");
  if (fp == NULL) {
    // Log("Can not find flash image: %s", img);
    // Log("Use built-in image instead");
    printf("[NEMU] Can not find flash image: %s\n", flash_img);
    printf("[NEMU] Use built-in image instead\n");
    add_mmio_map("flash", CONFIG_FLASH_START_ADDR, (uint8_t *)preset_flash, CONFIG_FLASH_SIZE, flash_io_handler);
    return;
  } else {
    __attribute__((unused)) int ret;
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    Assert(
      size <= CONFIG_FLASH_SIZE,
      "img size %d is larget than flash size %d",
      size, CONFIG_FLASH_SIZE
    );
    flash_base = new_space(CONFIG_FLASH_SIZE);
    ret = fread(flash_base, 1, size, fp);
    fclose(fp);
  }
#else
  flash_base = (uint8_t*) preset_flash;
#endif  
  add_mmio_map("flash", CONFIG_FLASH_START_ADDR, flash_base, CONFIG_FLASH_SIZE, flash_io_handler);
}
