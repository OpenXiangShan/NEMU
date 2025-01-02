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
#include <device/flash.h>
#include <sys/mman.h>
#ifdef CONFIG_MEM_COMPRESS
#include <unistd.h>
#include <zlib.h>
#include <zstd.h>
#endif

#ifndef CONFIG_MODE_USER

#ifdef CONFIG_MEM_COMPRESS
long load_gz_img(const char *filename, uint8_t* load_start, size_t img_size) {
  gzFile compressed_mem = gzopen(filename, "rb");
  Assert(compressed_mem, "Can not open '%s'", filename);

  const uint32_t chunk_size = 16384;
  uint8_t *temp_page = (uint8_t *)calloc(chunk_size, sizeof(long));
  uint8_t *pmem_start = (uint8_t *)load_start;
  uint8_t *pmem_current;

  // load file byte by byte to pmem
  size_t load_size = img_size == 0 ? MEMORY_SIZE : img_size;
  uint64_t curr_size = 0;
  while (curr_size < load_size) {
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

long load_zstd_img(const char *filename, uint8_t* load_start, size_t img_size){
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
  uint8_t *pmem_start = (uint8_t *)(load_start);
  uint64_t *pmem_current;

  // decompress and write in memory
  uint64_t total_write_size = 0;
  size_t load_size = img_size == 0 ? MEMORY_SIZE : img_size;
  while (total_write_size < load_size) {

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

// Will check the magic number in file, if not gz or zstd archive,
// will read in as a raw image
long load_img(const char* img_name, const char *which_img, uint8_t* load_start, size_t img_size) {
  const char *loading_img = img_name;
  Log("Loading %s: %s\n", which_img, img_name);
  if (img_name == NULL) {
    Log("No image is given. Use the default built-in image/restorer.");
    return 4096;  // built-in image size
  }

  if (is_gz_file(loading_img)) {
#ifdef CONFIG_MEM_COMPRESS
    Log("Loading GZ image %s", loading_img);
    return load_gz_img(loading_img, load_start, img_size);
#else
    panic("CONFIG_MEM_COMPRESS is disabled, turn it on in memuconfig!");
#endif
  }

  if (is_zstd_file(loading_img)) {
#ifdef CONFIG_MEM_COMPRESS
    Log("Loading Zstd image %s", loading_img);
    return load_zstd_img(loading_img, load_start, img_size);
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
    paddr_t load_start_paddr = host_to_guest(load_start);
    sparse_mem_write(get_sparsemm(), load_start_paddr, size, buf);
    close(fd);
    munmap(buf, size);
  }
#else
  int ret = fread((uint8_t*)(load_start), size, 1, fp);
  assert(ret == 1);
#endif
  Log("Read %lu bytes from file %s to 0x%p", size, img_name, load_start);

  fclose(fp);
  return size;
}


// Notes:
//
// |                      | enable flash device | disable flash device |
// |  cpt_image not null  |    override flash   |   override memory    |
// | flash_image not null |   write into flash  |  undefined behavior  |
//
// img_file used to fill memory, flash_image used to fill flash, cpt_image used to override memory or flash
//
void fill_memory(const char* img_file, const char* flash_image, const char* cpt_image, int64_t* img_size, int64_t* flash_size) {
  assert(img_file);
  uint8_t* bbl_start = (uint8_t*)get_pmem();
  *img_size = load_img(img_file, "image (checkpoint/bare metal app/bbl) form cmdline", bbl_start, 0);

#ifdef CONFIG_HAS_FLASH
  uint8_t* flash_start = get_flash_base();
  if(flash_image) {
    *flash_size = load_img(flash_image, "flash image from cmdline", flash_start, get_flash_size());
  }
#else
  if(flash_image) {
    Log("The flash image %s will not load into flash", flash_image);
  }
#endif

  if (cpt_image) {
    FILE *restore_fp = fopen(cpt_image, "rb");
    Assert(restore_fp, "Can not open '%s'", cpt_image);

    int restore_size = 0;
    int restore_jmp_inst = 0;

    int ret = fread(&restore_jmp_inst, sizeof(int), 1, restore_fp);
    assert(ret == 1);
    assert(restore_jmp_inst != 0);

    ret = fread(&restore_size, sizeof(int), 1, restore_fp);
    assert(ret == 1);
    assert(restore_size != 0);

    fclose(restore_fp);

#ifdef CONFIG_HAS_FLASH
    load_img(cpt_image, "Gcpt restorer form cmdline", flash_start, restore_size);
#else
    load_img(cpt_image, "Gcpt restorer form cmdline", bbl_start, restore_size);
#endif
  }
}

#endif  // CONFIG_MODE_USER
