#include <device/map.h>
#include <device/alarm.h>
#include <monitor/monitor.h>
#include <sys/time.h>
#include <time.h>

#define RTC_PORT 0x48   // Note that this is not the standard
#define RTC_MMIO 0xa1000048

static uint32_t *rtc_port_base = NULL;

static void rtc_io_handler(uint32_t offset, int len, bool is_write) {
  assert(offset == 0 || offset == 4);
  if (!is_write && offset == 4) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
    uint64_t us = now.tv_sec * 1000000 + now.tv_nsec / 1000;
    rtc_port_base[0] = (uint32_t)us;
    rtc_port_base[1] = us >> 32;
  }
}

static void timer_intr() {
  if (nemu_state.state == NEMU_RUNNING) {
    extern void dev_raise_intr();
    dev_raise_intr();
  }
}

void init_timer() {
  rtc_port_base = (void*)new_space(8);
  add_pio_map("rtc", RTC_PORT, (void *)rtc_port_base, 8, rtc_io_handler);
  add_mmio_map("rtc", RTC_MMIO, (void *)rtc_port_base, 8, rtc_io_handler);
  add_alarm_handle(timer_intr);
  assert(CLOCKS_PER_SEC == 1000000);
  assert(sizeof(clock_t) == 8);
}
