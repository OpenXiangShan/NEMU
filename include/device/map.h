#ifndef __DEVICE_MAP_H__
#define __DEVICE_MAP_H__

#include <monitor/difftest.h>

typedef void(*io_callback_t)(uint32_t, int, bool);

#ifdef XIANGSHAN
  
  // SD
  #define SD_MMIO 0x40002000
  // CLINT
  #define CLINT_MMIO 0x38000000
  // Serial
  #define SERIAL_PORT 0x3F8
  #define SERIAL_MMIO 0x40600000
  // VGA
  #define VMEM 0x50000000
  #define SCREEN_PORT 0x100 // Note that this is not the standard
  #define SCREEN_MMIO 0x40001000
  #define SYNC_PORT 0x104 // Note that this is not the standard
  #define SYNC_MMIO 0x40001004

  //--- The following devices are unused in XiangShan ---
  // Timer
  #define RTC_PORT 0x48   // Note that this is not the standard
  #define RTC_MMIO 0xa1000048
  // Keyboard
  #define I8042_DATA_PORT 0x60
  #define I8042_DATA_MMIO 0xa1000060
  // Audio
  #define AUDIO_PORT 0x200 // Note that this is not the standard
  #define AUDIO_MMIO 0xa1000200
  #define STREAM_BUF 0xa0800000

#else

  // SD
  #define SD_MMIO 0xa3000000
  // Timer
  #define RTC_PORT 0x48   // Note that this is not the standard
  #define RTC_MMIO 0xa1000048
  // Serial
  #define SERIAL_PORT 0x3F8
  #define SERIAL_MMIO 0xa10003F8
  // Keyboard
  #define I8042_DATA_PORT 0x60
  #define I8042_DATA_MMIO 0xa1000060
  // VGA
  #define VMEM 0xa0000000
  #define SCREEN_PORT 0x100 // Note that this is not the standard
  #define SCREEN_MMIO 0xa1000100
  #define SYNC_PORT 0x104 // Note that this is not the standard
  #define SYNC_MMIO 0xa1000104
  // Audio
  #define AUDIO_PORT 0x200 // Note that this is not the standard
  #define AUDIO_MMIO 0xa1000200
  #define STREAM_BUF 0xa0800000
  // CLINT
  #define CLINT_MMIO 0xa2000000

#endif

uint8_t* new_space(int size);

typedef struct {
  char *name;
  // we treat ioaddr_t as paddr_t here
  paddr_t low;
  paddr_t high;
  uint8_t *space;
  io_callback_t callback;
} IOMap;

static inline bool map_inside(IOMap *map, paddr_t addr) {
  return (addr >= map->low && addr <= map->high);
}

static inline int find_mapid_by_addr(IOMap *maps, int size, paddr_t addr) {
  int i;
  for (i = 0; i < size; i ++) {
    if (map_inside(maps + i, addr)) {
      difftest_skip_ref();
      return i;
    }
  }
  return -1;
}

void add_pio_map(char *name, ioaddr_t addr, uint8_t *space, int len, io_callback_t callback);
void add_mmio_map(char *name, paddr_t addr, uint8_t* space, int len, io_callback_t callback);

word_t map_read(paddr_t addr, int len, IOMap *map);
void map_write(paddr_t addr, word_t data, int len, IOMap *map);

#endif
