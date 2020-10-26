#include <device/map.h>

#define RTC_PORT 0x70

static uint8_t *rtc_base = NULL;

static void rtc_io_handler(uint32_t offset, int len, bool is_write) {
}

void init_mc146818rtc() {
  rtc_base = (void *)new_space(2);
  add_pio_map("mc146818rtc", RTC_PORT, rtc_base, 2, rtc_io_handler);
}
