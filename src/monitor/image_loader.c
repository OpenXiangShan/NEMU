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

#include <assert.h>
#include <fcntl.h>
#include <isa.h>
#include <macro.h>
#include <memory/paddr.h>
#include <memory/sparseram.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#ifdef CONFIG_MEM_COMPRESS
#include <unistd.h>
#include <zlib.h>
#include <zstd.h>
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
  Assert(gzclose(compressed_mem) == Z_OK, "Error closing '%s'\n", filename);
  return curr_size;
}

long load_zstd_img(const char *filename){
  assert(filename);

  int fd = -1;
  int file_size = 0;
  int compressed_file_buffer_size = 0;
  uint8_t *compress_file_buffer = NULL;

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    printf("Cannot open compressed file %s\n", filename);
    return -1;
  }

  file_size = lseek(fd, 0, SEEK_END);
  if (file_size == 0) {
    printf("File size is zero\n");
    return -1;
  }

  lseek(fd, 0, SEEK_SET);

  compress_file_buffer = malloc(file_size);
  if (!compress_file_buffer) {
    printf("Compress file buffer create failed\n");
    close(fd);
    return -1;
  }

  // read compressed file
  compressed_file_buffer_size = read(fd, compress_file_buffer, file_size);
  printf("read file size %d\n", compressed_file_buffer_size);
  if (compressed_file_buffer_size != file_size) {
    printf("Compress file read failed\n");
    free(compress_file_buffer);
    close(fd);
    return -1;
  }

  close(fd);

  // create decompress input buffer
  ZSTD_inBuffer input = {compress_file_buffer, compressed_file_buffer_size, 0};

  // alloc decompress buffer
  const uint64_t decompress_file_buffer_size = 16384;
  uint64_t *decompress_file_buffer = (uint64_t *)calloc(decompress_file_buffer_size, sizeof(uint64_t));
  if (!decompress_file_buffer) {
    printf("Decompress file read failed\n");
    free(compress_file_buffer);
    return -1;
  }

  // create and init decompress stream object
  ZSTD_DStream *dstream = ZSTD_createDStream();
  if (!dstream) {
    printf("Cannot create zstd dstream object\n");
    free(compress_file_buffer);
    free(decompress_file_buffer);
    return -1;
  }

  size_t init_result = ZSTD_initDStream(dstream);
  if (ZSTD_isError(init_result)) {
    printf("Cannot init dstream object: %s\n", ZSTD_getErrorName(init_result));
    ZSTD_freeDStream(dstream);
    free(compress_file_buffer);
    free(decompress_file_buffer);
    return -1;
  }

  // def phymem
  uint8_t *pmem_start = (uint8_t *)guest_to_host(RESET_VECTOR);
  uint64_t *pmem_current;

  // decompress and write in memory
  uint64_t total_write_size = 0;
  while (total_write_size < MEMORY_SIZE) {

    ZSTD_outBuffer output = {decompress_file_buffer, decompress_file_buffer_size * sizeof(uint64_t), 0};

    size_t result = ZSTD_decompressStream(dstream, &output, &input);

    if (ZSTD_isError(result)) {
      printf("Decompress failed: %s\n", ZSTD_getErrorName(result));
      ZSTD_freeDStream(dstream);
      free(compress_file_buffer);
      free(decompress_file_buffer);

      return -1;
    }

    if (output.pos == 0) {
      break;
    }

    assert(decompress_file_buffer_size * sizeof(uint64_t) == output.pos);

    for (uint64_t x = 0; x < decompress_file_buffer_size; x++) {
      pmem_current = (uint64_t *)(pmem_start + total_write_size) + x;
      uint64_t read_data = *(decompress_file_buffer + x);
      if (read_data != 0 || *pmem_current != 0) {
        *pmem_current = read_data;
      }
    }
    total_write_size += output.pos;
  }

  ZSTD_outBuffer output = {decompress_file_buffer, decompress_file_buffer_size * sizeof(uint64_t), 0};
  size_t result = ZSTD_decompressStream(dstream, &output, &input);
  if (ZSTD_isError(result) || output.pos != 0) {
    printf("Decompress failed: %s\n", ZSTD_getErrorName(result));
    printf("Binary size larger than memory\n");
    ZSTD_freeDStream(dstream);
    free(compress_file_buffer);
    free(decompress_file_buffer);
    return -1;
  }

  ZSTD_freeDStream(dstream);
  free(compress_file_buffer);
  free(decompress_file_buffer);

  return total_write_size;
}

#endif  //  CONFIG_MEM_COMPRESS

// Return whether a file is a gz file, determined by its name.
// If the filename ends with ".gz", we treat it as a gz file.


long load_img(char* img_name, char *which_img, uint64_t load_start, size_t img_size) {
  char *loading_img = img_name;
  Log("Loading %s: %s\n", which_img, img_name);
  if (img_name == NULL) {
    Log("No image is given. Use the default built-in image/restorer.");
    return 4096;  // built-in image size
  }

  if (is_gz_file(loading_img)) {
#ifdef CONFIG_MEM_COMPRESS
    Log("Loading GZ image %s", loading_img);
    return load_gz_img(loading_img);
#else
    panic("CONFIG_MEM_COMPRESS is disabled, turn it on in memuconfig!");
#endif
  }

  if (is_zstd_file(loading_img)) {
#ifdef CONFIG_MEM_COMPRESS
    Log("Loading Zstd image %s", loading_img);
    return load_zstd_img(loading_img);
#else
    panic("CONFIG_MEM_COMPRESS is disabled, turn it on in memuconfig!");
#endif
  }

  // RAW image

  FILE *fp = fopen(loading_img, "rb");
  Assert(fp, "Can not open '%s'", loading_img);

  size_t size;
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  if (img_size != 0 && (size > img_size)) {
    Log("Warning: size is larger than img_size(upper limit), please check if code is missing. size:%lx img_size:%lx",
        size, img_size);
    size = img_size;
  }

#ifdef CONFIG_USE_SPARSEMM
  if (file_is_elf(loading_img)) {
    sparse_mem_elf(get_sparsemm(), loading_img);
    Log("load elf %s to sparse mem complete", loading_img);
    sparse_mem_info(get_sparsemm());
  } else {
    int fd = open(loading_img, O_RDONLY);
    char *buf = (char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    sparse_mem_write(get_sparsemm(), load_start, size, buf);
    close(fd);
    munmap(buf, size);
  }
#else
  int ret = fread(guest_to_host(load_start), size, 1, fp);
  assert(ret == 1);
#endif
  Log("Read %lu bytes from file %s to 0x%lx", size, img_name, load_start);

  fclose(fp);
  return size;
}

#endif  // CONFIG_MODE_USER
