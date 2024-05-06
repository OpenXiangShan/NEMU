#include "load.h"
#include <stdlib.h>
#include <time.h>

static uint8_t *pmem = NULL;
unsigned long MEMORY_SIZE = 0x200000000;

void init_mem()
{
	void *ret = mmap((void *)PMEMBASE, MEMORY_SIZE, PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  	if (ret != (void *)PMEMBASE) {
    	perror("mmap");
    	assert(0);
  	}
  	pmem = ret;
}

uint8_t* guest_to_host(paddr_t paddr) 
{ 
	return paddr + HOST_PMEM_OFFSET; 
}

long load_gz_img(const char *filename) 
{
  gzFile compressed_mem = gzopen(filename, "rb");
  Assert(compressed_mem, "Can not open '%s'", filename);

  const uint32_t chunk_size = 16384;
  uint8_t *temp_page = (uint8_t *)calloc(chunk_size, sizeof(long));
  uint8_t *pmem_start = (uint8_t *)guest_to_host(RESET_VECTOR);
  uint8_t *pmem_current;


  clock_t read_start, read_end;
  double read_time;
  // load file byte by byte to pmem
  read_start = clock();
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
  read_end = clock();
  read_time = (double)(read_end - read_start) / CLOCKS_PER_SEC;
  printf("read_time ==> %.6fs\n", read_time);

  // check again to ensure the bin has been fully loaded
  uint32_t left_bytes = gzread(compressed_mem, temp_page, chunk_size);
  Assert(left_bytes == 0, "File size is larger than buf_size!\n");

  free(temp_page);
  Assert(gzclose(compressed_mem) == Z_OK, "Error closing '%s'\n", filename);
  return curr_size;
}

long load_raw_img(const char * filename)
{
  FILE *fp = fopen(filename, "rb");
  Assert(fp, "Can not open '%s'", filename);

  size_t size;
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  printf("Read %lu bytes from file %s to 0x%lx\n", size, filename, (uint64_t)RESET_VECTOR);

  fclose(fp);
  return size;

}