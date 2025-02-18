#include <utils.h>
#include <device/map.h>

uint8_t *aplic_m_base = NULL;
#define APLIC_M_SIZE (0x8000)

static void aplic_m_io_handler(uint32_t offset, int len, bool is_write) {
  // Fake aplic_m handler, empty now
  return;
}

void init_aplic_m(const char *flash_img) {
  aplic_m_base = new_space(APLIC_M_SIZE);
  add_mmio_map("aplic_m", CONFIG_APLIC_M_ADDRESS, aplic_m_base, APLIC_M_SIZE, aplic_m_io_handler);
}
