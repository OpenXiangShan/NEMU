#include <utils.h>
#include <device/map.h>

uint8_t *aplic_s_base = NULL;
#define APLIC_S_SIZE (0x8000)

static void aplic_s_io_handler(uint32_t offset, int len, bool is_write) {
  // Fake aplic_s handler, empty now
  return;
}

void init_aplic_s(const char *flash_img) {
  aplic_s_base = new_space(APLIC_S_SIZE);
  add_mmio_map("aplic_s", CONFIG_APLIC_S_ADDRESS, aplic_s_base, APLIC_S_SIZE, aplic_s_io_handler);
}
