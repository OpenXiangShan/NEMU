#include <device/map.h>

#define I8253_PORT 0x40

static uint8_t *i8253_base = NULL;

static void i8253_io_handler(uint32_t offset, int len, bool is_write) {
}

void init_i8253() {
  i8253_base = (void *)new_space(4);
  add_pio_map("i8253-pit", I8253_PORT, i8253_base, 4, i8253_io_handler);
}
