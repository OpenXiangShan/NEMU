#include <common.h>

bool is_gz_file(const char *filename) {
  if (filename == NULL || strlen(filename) < 3) {
    return false;
  }
  return !strcmp(filename + (strlen(filename) - 3), ".gz");
}