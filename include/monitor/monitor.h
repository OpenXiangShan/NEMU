#ifndef __MONITOR_H__
#define __MONITOR_H__

void init_monitor(int argc, char *argv[]);
long load_img(char* img_name, const char *which_img, uint64_t load_start, size_t img_size);

#endif
