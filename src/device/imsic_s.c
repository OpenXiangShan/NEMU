#include <utils.h>
#include <device/map.h>

uint8_t *imsic_s_base = NULL;
#define IMSIC_S_SIZE (0x80000)

static void imsic_s_io_handler(uint32_t offset, int len, bool is_write) {
  // Fake imsic_s handler, empty now
  return;
}

void init_imsic_s(const char *flash_img) {
  imsic_s_base = new_space(IMSIC_S_SIZE);
  add_mmio_map("imsic_s", CONFIG_IMSIC_S_ADDRESS, imsic_s_base, IMSIC_S_SIZE, imsic_s_io_handler);
}
