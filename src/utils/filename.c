/***************************************************************************************
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "debug.h"
#include <common.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

bool is_gz_file(const char *filename)
{
  int fd = -1;

  fd = open(filename, O_RDONLY);
  assert(fd);

  uint8_t buf[2];

  size_t sz = read(fd, buf, 2);
  if (sz != 2) {
    close(fd);
    xpanic("Couldn't read magic bytes from object file %s", filename);
  }

  close(fd);

  const uint8_t gz_magic[2] = {0x1f, 0x8B};
  return memcmp(buf, gz_magic, 2) == 0;
}

bool is_zstd_file(const char *filename){
  int fd = -1;

  fd = open(filename, O_RDONLY);
  assert(fd);

  uint8_t buf[4];

  size_t sz = read(fd, buf, 4);
  if (sz != 4) {
    close(fd);
    xpanic("Couldn't read magic bytes from object file %s", filename);
  }

  close(fd);

  const uint8_t frame_magic[4] = {0x28, 0xB5, 0x2F, 0xFD};
  if (memcmp(buf, frame_magic, sizeof(buf)) == 0) {
    return true;
  }

  // A stream can start with any of the 16 skippable-frame magic values.
  const uint8_t skippable_magic[4] = {0x50, 0x2A, 0x4D, 0x18};
  const uint8_t skippable_magic_mask = 0xF0;
  buf[0] &= skippable_magic_mask;
  return memcmp(buf, skippable_magic, sizeof(buf)) == 0;
}
