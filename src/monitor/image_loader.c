#include <isa.h>
#include <stdlib.h>
#include <macro.h>
#include <memory/paddr.h>
#include <zlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


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
  while (curr_size < CONFIG_MSIZE) {
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
    extern bool map_image_as_output_cpt;
    assert (!map_image_as_output_cpt && "Cannot map gz as output cpt");
#ifdef CONFIG_MEM_COMPRESS
    Log("Loading GZ image %s", loading_img);
    return load_gz_img(loading_img);
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
    Log("Warning: size is larger than img_size(upper limit), please check if code is missing. size:%lx img_size:%lx", size, img_size);
    size = img_size;
  }

  if (size < 512UL*1024UL*1024UL) {
    Log("Fread from file because less than 512MB\n");
    int ret = fread(guest_to_host(load_start), size, 1, fp);
    assert(ret == 1);
    fclose(fp);

  } else {
    fclose(fp); // we will mmap directly later
    assert(!ISDEF(CONFIG_SHARE));
    assert(size % 8 == 0);  // assuming 64 bit aligned

    Log("Mmap and read from file because larger than 512MB\n");
    extern uint8_t *get_pmem();
    uint64_t *current = (uint64_t *) get_pmem();
    int fd = open(img_name, O_RDONLY, (mode_t)0600);
    if (!fd) {
      panic("Failed to read file %s", img_name);
    }
    uint64_t *img_start = (uint64_t*) mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    uint64_t *img_end = img_start + (size / sizeof(size_t));
    uint64_t *img_p = img_start;
    while(img_p < img_end) { 
      if (*img_p != 0) {  // Keep COW
        *current = *img_p;
      }
      current++;
      img_p++;
    }
    // clean up mapped and opened file
    munmap(img_start, size);
    close(fd);
  }

  Log("Read %lu bytes from file %s to 0x%lx", size, img_name, load_start);

  return size;
}

#endif // CONFIG_MODE_USER
