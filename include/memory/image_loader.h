#ifndef __IMAGE_LOADER_H__
#define __IMAGE_LOADER_H__

#include<stddef.h>


long load_gz_img(const char *filename);

long load_img(char* img_name, char *which_img, unsigned load_start, size_t img_size);

#endif //  __IMAGE_LOADER_H__