#include <utils.h>
#include <device/map.h>

uint8_t *plic_base = NULL;
#define PLIC_SIZE (0x4000000)

static void plic_io_handler(uint32_t offset, int len, bool is_write) {
  // Fake plic handler, empty now
  return;
}

void init_plic(const char *flash_img) {
  printf("init_plic\n");
  plic_base = new_space(PLIC_SIZE); // TOO MUCH
  add_mmio_map("plic", CONFIG_PLIC_ADDRESS, plic_base, PLIC_SIZE, plic_io_handler);
}
