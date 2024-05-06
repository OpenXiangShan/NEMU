#include "load.h"
#include <time.h>

clock_t load_start, load_end;
double load_time;

int main(int argc, char** argv)
{
	assert(argc == 2);

	init_mem();

	printf("==========  load %-*s  ==========\n", 32, argv[1]);
	load_start = clock();
	load_gz_img(argv[1]);
	load_end = clock();
	load_time = (double)(load_end - load_start) / CLOCKS_PER_SEC;
	printf("load time ==> %.6fs\n", load_time);
}