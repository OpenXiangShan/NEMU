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

#include <isa.h>
#include <stdlib.h>
#include <macro.h>
#include <memory/paddr.h>
#ifdef CONFIG_MEM_COMPRESS
#include <zlib.h>
#endif

#ifndef CONFIG_MODE_USER

#ifdef CONFIG_MEM_COMPRESS
long load_gz_img(const char *filename) {
  gzFile compressed_mem = gzopen(filename, "rb");
  Assert(compressed_mem, "Can not open '%s'", filename);

  const uint32_t chunk_size = 16384;
  uint8_t *temp_page = (uint8_t *)calloc(chunk_size, sizeof(long));
  uint8_t *pmem_start = (uint8_t *)guest_to_host(RESET_VECTOR);
  uint8_t *pmem_current;

  // load file byte by byte to pmem
  uint64_t curr_size = 0;
  while (curr_size < MEMORY_SIZE) {
    uint32_t bytes_read = gzread(compressed_mem, temp_page, chunk_size);
    if (bytes_read == 0) {
      break;
    }
    for (uint32_t x = 0; x < bytes_read; x++) {
      pmem_current = pmem_start + curr_size + x;
      uint8_t read_data = *(temp_page + x);
      if (read_data != 0 || *pmem_current != 0) {
        *pmem_current = read_data;
      }
    }
    curr_size += bytes_read;
  }

  // check again to ensure the bin has been fully loaded
  uint32_t left_bytes = gzread(compressed_mem, temp_page, chunk_size);
  Assert(left_bytes == 0, "File size is larger than buf_size!\n");

  free(temp_page);
  Assert(!gzclose(compressed_mem), "Error closing '%s'\n", filename);
  return curr_size;
}
#endif //  CONFIG_MEM_COMPRESS

// Return whether a file is a gz file, determined by its name.
// If the filename ends with ".gz", we treat it as a gz file.


long load_img(char* img_name, char *which_img, uint64_t load_start, size_t img_size) {
  char *loading_img = img_name;
  Log("Loading %s: %s\n", which_img, img_name);
  if (img_name == NULL) {
    Log("No image is given. Use the default build-in image/restorer.");
    return 4096; // built-in image size
  }

  if (is_gz_file(loading_img)) {
#ifdef CONFIG_MEM_COMPRESS
      Log("Loading GZ image %s", loading_img);
      return load_gz_img(loading_img);
#else
      panic("CONFIG_MEM_COMPRESS is disabled, turn it on in memuconfig!");
#endif
  }

  FILE *fp = fopen(loading_img, "rb");
  Assert(fp, "Can not open '%s'", loading_img);

  size_t size;
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (img_size != 0 && (size > img_size)) {
    Log("Warning: size is larger than img_size(upper limit), please check if code is missing. size:%lx img_size:%lx", size, img_size);
    size = img_size;
  }

  int ret = fread(guest_to_host(load_start), size, 1, fp);
  assert(ret == 1);
  Log("Read %lu bytes from file %s to 0x%lx", size, img_name, load_start);

  fclose(fp);
  return size;
}

#endif // CONFIG_MODE_USER
