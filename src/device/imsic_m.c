#include <utils.h>
#include <device/map.h>

uint8_t *imsic_m_base = NULL;
#define IMSIC_M_SIZE (0x10000)

static void imsic_m_io_handler(uint32_t offset, int len, bool is_write) {
  // Fake imsic_m handler, empty now
  return;
}

void init_imsic_m(const char *flash_img) {
  imsic_m_base = new_space(IMSIC_M_SIZE);
  add_mmio_map("imsic_m", CONFIG_IMSIC_M_ADDRESS, imsic_m_base, IMSIC_M_SIZE, imsic_m_io_handler);
}
