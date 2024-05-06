#ifndef __TEST_LOAD_H__
#define __TEST_LOAD_H__

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <zlib.h>

#define Assert(cond, ...) \
  do { \
    if (!(cond)) { \
      fflush(stdout); \
      fprintf(stderr, "\33[1;31m"); \
      fprintf(stderr, __VA_ARGS__); \
      fprintf(stderr, "\33[0m\n"); \
      assert(cond); \
    } \
  } while (0)

#define RESET_VECTOR 0x80000000
#define CONFIG_MBASE 0x80000000
#define CONFIG_MSIZE 0x200000000
#define PMEMBASE 0x100000000ul
#define HOST_PMEM_OFFSET (uint8_t *)(pmem - CONFIG_MBASE)

typedef uint64_t paddr_t;

void init_mem();
uint8_t* guest_to_host(paddr_t); 
long load_gz_img(const char *);
long load_raw_img(const char *);

#endif