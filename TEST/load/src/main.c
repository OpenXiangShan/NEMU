#include "load.h"
#include <string.h>
#include <time.h>

#define MAX_PATH_LENGTH 256

static const char* path = "/home/xiongtianyu/xs/NEMU/TEST/load/checkpoint";
static const char* filenames[] = {
		"games-30m",
		"perlbench-100m",
		"milc-600m"
};
static char filepath[MAX_PATH_LENGTH];

static double all_load_time[3][5];
static double avg_load_time[3];

void join_paths(const char* path1, const char* path2, char* result) {
    strcpy(result, path1);
    strcat(result, "/");
    strcat(result, path2);
}

void calc_avg()
{
	for(int i = 0; i < 3; i++) {
		double sum = 0;
		for(int j = 0; j < 5; j++) {
			sum += all_load_time[i][j];
		}
		avg_load_time[i] = sum / 5;
	}
}

int main()
{
	init_mem();

	clock_t load_start, load_end;
	double load_time;
	for(int i = 0; i < 5; i++) {
		for(int j = 0; j < 3; j++) {
			load_start = clock();
			printf("==========  loop %d: load %-*s  ==========\n", i + 1, 20, filenames[j]);
			memset(filepath, 0, sizeof filepath);
			join_paths(path, filenames[j], filepath);
			load_raw_img(filepath);
			load_end = clock();
			load_time = (double)(load_end - load_start) / CLOCKS_PER_SEC;
			printf("load_time ==> %.6fs\n", load_time);
			all_load_time[j][i] = load_time;
		}
	}

	calc_avg();

	for(int i = 0; i < 3; i++) {
		printf("Test %-*s ==> 5 loops load average time cost: %.6fs\n", 20, filenames[i], avg_load_time[i]);
	}
}