/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
*
* XiangShan is licensed under Mulan PSL v2.
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

#include <monitor/compress.h>

double calcTime(struct timeval s, struct timeval e) {
  double sec, usec;
  sec = e.tv_sec - s.tv_sec;
  usec = e.tv_usec - s.tv_usec;
  return 1000*sec + usec/1000.0;
}

// Return whether the file is a gz file
int isGzFile(const char *filename) {
  assert(filename != NULL && strlen(filename) >= 4);
  return !strcmp(filename + (strlen(filename) - 3), ".gz");
}

long readFromGz(void* ptr, const char *file_name, long buf_size, uint8_t load_type) {
  assert(buf_size > 0);
  gzFile compressed_mem = gzopen(file_name, "rb");

  if (compressed_mem == NULL) {
    printf("Can't open compressed binary file '%s'", file_name);
    return -1;
  }

  uint64_t curr_size = 0;
  const uint32_t chunk_size = 16384;

  // Only load from RAM need check
  if (load_type == LOAD_RAM && (buf_size % chunk_size) != 0) {
    printf("buf_size must be divisible by chunk_size\n");
    assert(0);
  }
  
  long temp_page[chunk_size];
  long *pmem_current = (long*)ptr;

  while (curr_size < buf_size) {
    uint32_t bytes_read = gzread(compressed_mem, temp_page, chunk_size);
    if (bytes_read == 0) {
      break;
    }
    assert(load_type != LOAD_RAM || bytes_read % sizeof(long) == 0);
    for (uint32_t x = 0; x < bytes_read / sizeof(long) + 1; x++) {
      if (*(temp_page + x) != 0) {
        pmem_current = (long*)((uint8_t*)ptr + curr_size + x * sizeof(long));
        *pmem_current = *(temp_page + x);
      }
    }
    curr_size += bytes_read;
  }

  if(gzread(compressed_mem, temp_page, chunk_size) > 0) {
    printf("File size is larger than buf_size!\n");
    assert(0);
  }
  // printf("Read %lu bytes from gz stream in total\n", curr_size);

  if(gzclose(compressed_mem)) {
    printf("Error closing '%s'\n", file_name);
    return -1;
  }
  return curr_size;
}

void nonzero_large_memcpy(const void* __restrict dest, const void* __restrict src, size_t n) {
  uint64_t *_dest = (uint64_t *)dest;
  uint64_t *_src  = (uint64_t *)src;
  while (n >= sizeof(uint64_t)) {
    if (*_src != 0) {
      *_dest = *_src;
    }
    _dest++;
    _src++;
    n -= sizeof(uint64_t);
  }
  if (n > 0) {
    uint8_t *dest8 = (uint8_t *)_dest;
    uint8_t *src8  = (uint8_t *)_src;
    while (n > 0) {
      *dest8 = *src8;
      dest8++;
      src8++;
      n--;
    }
  }
}