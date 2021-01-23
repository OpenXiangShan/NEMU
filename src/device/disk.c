#include <common.h>
#include <device/map.h>
#include <memory/paddr.h>
#include <isa.h>

#define DISK_PORT 0x300 // Note that this is not the standard
#define DISK_MMIO 0xa1000300

enum {
  SIZE,
  CMD,
  BUF,
  START,
  COUNT,
  NR_REG
};

static uint32_t *disk_base = NULL;
static FILE *fp = NULL;

static void disk_io_handler(uint32_t offset, int len, bool is_write) {
#ifndef __ICS_EXPORT
  if (offset == CMD * sizeof(uint32_t) && len == 4 && is_write) {
    assert(disk_base[CMD] == 0);
    fseek(fp, disk_base[START] * 512, SEEK_SET);
    int ret = fread(guest_to_host(disk_base[BUF] - PMEM_BASE), disk_base[COUNT] * 512l, 1, fp);
    assert(ret == 1);
  }
#endif
}

void init_disk() {
  uint32_t space_size = sizeof(uint32_t) * NR_REG;
  disk_base = (void *)new_space(space_size);
  add_pio_map ("disk", DISK_PORT, (void *)disk_base, space_size, disk_io_handler);
  add_mmio_map("disk", DISK_MMIO, (void *)disk_base, space_size, disk_io_handler);

  const char *path ="/home/yzh/projectn/nanos-lite/build/ramdisk.img";
  fp = fopen(path, "r");
  if (fp) {
    fseek(fp, 0, SEEK_END);
    disk_base[SIZE] = ftell(fp) / 512 + 1;
    fseek(fp, 0, SEEK_SET);
  } else {
    Log("Can not open %s. Disable disk...", path);
    disk_base[SIZE] = 0;
  }
}
