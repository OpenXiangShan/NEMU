/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static const size_t bootloader_rng_seed_dtb_offset = 0x180000;
static const size_t bootloader_rng_seed_dtb_capacity = 0x80000;

enum {
  RNG_SEED_SIZE = 32,
};

#define FDT_MAGIC       0xd00dfeed
#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP         0x4
#define FDT_END         0x9

#define FDT_HDR_MAGIC          0
#define FDT_HDR_TOTALSIZE      4
#define FDT_HDR_OFF_STRUCT     8
#define FDT_HDR_OFF_STRINGS    12
#define FDT_HDR_SIZE_STRINGS   32
#define FDT_HDR_SIZE_STRUCT    36

typedef struct {
  bool found_chosen;
  bool has_rng_seed;
  size_t chosen_end;
  size_t bootargs_prop;
  size_t bootargs_data;
  uint32_t bootargs_len;
} fdt_scan_t;

static uint32_t fdt_get_u32(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void fdt_put_u32(uint8_t *p, uint32_t v) {
  p[0] = v >> 24;
  p[1] = v >> 16;
  p[2] = v >> 8;
  p[3] = v;
}

static size_t fdt_align4(size_t v) {
  return (v + 3) & ~(size_t)3;
}

static uint32_t fdt_hdr_get(uint8_t *fdt, size_t off) {
  return fdt_get_u32(fdt + off);
}

static void fdt_hdr_set(uint8_t *fdt, size_t off, uint32_t value) {
  fdt_put_u32(fdt + off, value);
}

static const char *fdt_string(uint8_t *fdt, uint32_t nameoff) {
  uint32_t off_strings = fdt_hdr_get(fdt, FDT_HDR_OFF_STRINGS);
  uint32_t size_strings = fdt_hdr_get(fdt, FDT_HDR_SIZE_STRINGS);
  if (nameoff >= size_strings) {
    return NULL;
  }
  return (const char *)fdt + off_strings + nameoff;
}

static bool fdt_insert(uint8_t *fdt, size_t capacity, size_t pos, size_t len,
                       bool in_struct) {
  uint32_t totalsize = fdt_hdr_get(fdt, FDT_HDR_TOTALSIZE);
  if (pos > totalsize || len > capacity - totalsize) {
    return false;
  }

  memmove(fdt + pos + len, fdt + pos, totalsize - pos);
  memset(fdt + pos, 0, len);
  fdt_hdr_set(fdt, FDT_HDR_TOTALSIZE, totalsize + len);

  if (in_struct) {
    fdt_hdr_set(fdt, FDT_HDR_OFF_STRINGS,
        fdt_hdr_get(fdt, FDT_HDR_OFF_STRINGS) + len);
    fdt_hdr_set(fdt, FDT_HDR_SIZE_STRUCT,
        fdt_hdr_get(fdt, FDT_HDR_SIZE_STRUCT) + len);
  } else {
    fdt_hdr_set(fdt, FDT_HDR_SIZE_STRINGS,
        fdt_hdr_get(fdt, FDT_HDR_SIZE_STRINGS) + len);
  }

  return true;
}

static bool fdt_append_string(uint8_t *fdt, size_t capacity, const char *name,
                              uint32_t *nameoff) {
  uint32_t off_strings = fdt_hdr_get(fdt, FDT_HDR_OFF_STRINGS);
  uint32_t size_strings = fdt_hdr_get(fdt, FDT_HDR_SIZE_STRINGS);
  size_t name_len = strlen(name) + 1;

  *nameoff = size_strings;
  if (!fdt_insert(fdt, capacity, off_strings + size_strings, name_len, false)) {
    return false;
  }

  memcpy(fdt + off_strings + size_strings, name, name_len);
  return true;
}

static bool fdt_scan_chosen(uint8_t *fdt, fdt_scan_t *scan) {
  memset(scan, 0, sizeof(*scan));
  scan->chosen_end = (size_t)-1;
  scan->bootargs_prop = (size_t)-1;

  uint32_t off_struct = fdt_hdr_get(fdt, FDT_HDR_OFF_STRUCT);
  uint32_t off_strings = fdt_hdr_get(fdt, FDT_HDR_OFF_STRINGS);
  uint32_t size_struct = fdt_hdr_get(fdt, FDT_HDR_SIZE_STRUCT);
  size_t pos = off_struct;
  size_t end = off_struct + size_struct;
  int depth = 0;
  int chosen_depth = -1;

  while (pos + 4 <= end) {
    size_t token_pos = pos;
    uint32_t token = fdt_get_u32(fdt + pos);
    pos += 4;

    switch (token) {
    case FDT_BEGIN_NODE: {
      const char *name = (const char *)fdt + pos;
      size_t max_name = end - pos;
      size_t name_len = strnlen(name, max_name);
      if (name_len == max_name) {
        return false;
      }

      depth++;
      if (depth == 2 && strcmp(name, "chosen") == 0) {
        scan->found_chosen = true;
        chosen_depth = depth;
      }

      pos = fdt_align4(pos + name_len + 1);
      break;
    }
    case FDT_END_NODE:
      if (chosen_depth == depth) {
        scan->chosen_end = token_pos;
        chosen_depth = -1;
      }
      depth--;
      break;
    case FDT_PROP: {
      if (pos + 8 > end) {
        return false;
      }
      uint32_t len = fdt_get_u32(fdt + pos);
      uint32_t nameoff = fdt_get_u32(fdt + pos + 4);
      const char *prop_name = fdt_string(fdt, nameoff);
      size_t data_pos = pos + 8;
      if (data_pos + len > end) {
        return false;
      }

      if (chosen_depth == depth && prop_name != NULL) {
        if (strcmp(prop_name, "rng-seed") == 0) {
          scan->has_rng_seed = true;
        } else if (strcmp(prop_name, "bootargs") == 0) {
          scan->bootargs_prop = token_pos;
          scan->bootargs_data = data_pos;
          scan->bootargs_len = len;
        }
      }

      pos = fdt_align4(data_pos + len);
      break;
    }
    case FDT_NOP:
      break;
    case FDT_END:
      return true;
    default:
      return false;
    }

    if (pos > off_strings) {
      return false;
    }
  }

  return false;
}

static bool fdt_patch_bootargs(uint8_t *fdt, size_t capacity) {
  const char trust_arg[] = "random.trust_bootloader=on";
  fdt_scan_t scan;
  if (!fdt_scan_chosen(fdt, &scan) || scan.bootargs_prop == (size_t)-1) {
    return false;
  }

  const char *old_args = (const char *)fdt + scan.bootargs_data;
  size_t old_len = strnlen(old_args, scan.bootargs_len);
  if (old_len == scan.bootargs_len) {
    return false;
  }
  if (strstr(old_args, trust_arg) != NULL) {
    return true;
  }

  size_t trust_len = strlen(trust_arg);
  size_t new_len = old_len + 1 + trust_len + 1;
  size_t old_aligned = fdt_align4(scan.bootargs_len);
  size_t new_aligned = fdt_align4(new_len);
  if (new_aligned > old_aligned &&
      !fdt_insert(fdt, capacity, scan.bootargs_data + old_aligned,
                  new_aligned - old_aligned, true)) {
    return false;
  }

  uint8_t *data = fdt + scan.bootargs_data;
  data[old_len] = ' ';
  memcpy(data + old_len + 1, trust_arg, trust_len + 1);
  memset(data + new_len, 0, new_aligned - new_len);
  fdt_put_u32(fdt + scan.bootargs_prop + 4, new_len);
  return true;
}

static bool read_host_random(uint8_t *buf, size_t len) {
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0) {
    return false;
  }

  size_t pos = 0;
  while (pos < len) {
    ssize_t ret = read(fd, buf + pos, len - pos);
    if (ret > 0) {
      pos += ret;
      continue;
    }
    if (ret < 0 && errno == EINTR) {
      continue;
    }
    break;
  }

  close(fd);
  return pos == len;
}

static bool fdt_add_rng_seed(uint8_t *fdt, size_t capacity) {
  fdt_scan_t scan;
  uint32_t nameoff = 0;
  uint8_t prop[12 + RNG_SEED_SIZE];
  uint8_t seed[RNG_SEED_SIZE];

  if (!fdt_scan_chosen(fdt, &scan) || !scan.found_chosen) {
    return false;
  }
  if (scan.has_rng_seed) {
    return true;
  }
  if (!fdt_append_string(fdt, capacity, "rng-seed", &nameoff)) {
    return false;
  }
  if (!fdt_scan_chosen(fdt, &scan) || scan.chosen_end == (size_t)-1) {
    return false;
  }

  if (!read_host_random(seed, sizeof(seed))) {
    Log("Failed to read host random for bootloader rng-seed");
    return false;
  }

  fdt_put_u32(prop, FDT_PROP);
  fdt_put_u32(prop + 4, sizeof(seed));
  fdt_put_u32(prop + 8, nameoff);
  memcpy(prop + 12, seed, sizeof(seed));

  if (!fdt_insert(fdt, capacity, scan.chosen_end, sizeof(prop), true)) {
    return false;
  }
  memcpy(fdt + scan.chosen_end, prop, sizeof(prop));
  return true;
}

void patch_bootloader_rng_seed(uint8_t *pmem_start) {
  uint8_t *fdt = pmem_start + bootloader_rng_seed_dtb_offset;
  size_t capacity = bootloader_rng_seed_dtb_capacity;

  if (capacity < 64 || fdt_hdr_get(fdt, FDT_HDR_MAGIC) != FDT_MAGIC) {
    return;
  }
  if (fdt_hdr_get(fdt, FDT_HDR_TOTALSIZE) > capacity) {
    Log("Skip bootloader rng-seed: FDT totalsize exceeds capacity");
    return;
  }

  bool ok = fdt_patch_bootargs(fdt, capacity) && fdt_add_rng_seed(fdt, capacity);
  if (ok) {
    Log("Patched FDT /chosen with rng-seed");
  } else {
    Log("Failed to patch FDT /chosen with rng-seed");
  }
}
