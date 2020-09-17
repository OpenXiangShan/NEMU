#include <device/map.h>

#define I8259A_MASTER_PORT 0x20
#define I8259A_SLAVE_PORT  0xa0

static uint8_t *i8259a_base = NULL;

static void i8259a_io_handler(uint32_t offset, int len, bool is_write) {
}

void init_i8259a() {
  i8259a_base = (void *)new_space(2);
  add_pio_map("i8259a-master", I8259A_MASTER_PORT, i8259a_base, 2, i8259a_io_handler);
  add_pio_map("i8259a-slave",  I8259A_SLAVE_PORT,  i8259a_base, 2, i8259a_io_handler);
}
