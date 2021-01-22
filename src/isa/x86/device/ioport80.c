#include <device/map.h>

// this is an empty device in QEMU
#define IOPORT80_PORT 0x80

static uint8_t *ioport80_base = NULL;

static void ioport80_io_handler(uint32_t offset, int len, bool is_write) {
}

void init_ioport80() {
  ioport80_base = (void *)new_space(1);
  add_pio_map("ioport80", IOPORT80_PORT, ioport80_base, 1, ioport80_io_handler);
}
