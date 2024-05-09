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

#include <ctime>
#include <stddef.h>
#include <stdio.h>
#include <checkpoint/cpt_env.h>

char *output_base_dir = NULL;
char *config_name = NULL;
char *workload_name = NULL;
char *simpoints_dir = NULL;
int cpt_id = -1;
char *cpt_file = NULL;
char *restorer = NULL;

unsigned char page_vec[0x200000];

seg segs[0x100000];
size_t seg_num;

double dump_gz_time, dump_raw_time, dump_raw_gz_time;
size_t total_gz_size, total_raw_size, total_raw_gz_size;

clock_t start, end;
double time_cost;

struct stat file_stat;

size_t ckpt_num;

extern "C" {
void init_segs()
{
	long start = -1, end = -1;
	size_t i = 0, j = 0;
	for (; i < 0x200000; i++) {
		if (page_vec[i] == 1) {
			if(start == -1) {
				start = i;
			}
		} else {
			if(start != -1) {
				end = i - 1;
				segs[j].l = start;
				segs[j].r = end;
				start = -1;
				j++;
			}
		}
	}
	if(start != -1) {
		end = i - 1;
		segs[j].l = start;
		segs[j].r = end;
		j++;
	}
	seg_num = j;
}

void dump_ckpt_info() 
{
	double size = 0;
	printf("==========   gz   ==========\n");
	printf("Generate %lu checkpoint files\n", ckpt_num);
	printf("total dump time: %fs     average dump time: %fs\n", dump_gz_time, dump_gz_time / ckpt_num);
	size = (double)total_gz_size / 0x100000;
	printf("total ckpt size：%fMB   average ckpt size: %fMB\n", size, size / ckpt_num );

	printf("==========   raw   ==========\n");
	printf("Generate %lu checkpoint files\n", ckpt_num);
	printf("total dump time: %fs     average dump time: %fs\n", dump_raw_time, dump_raw_time / ckpt_num);
	size = (double)total_raw_size / 0x100000;
	printf("total ckpt size：%fMB   average ckpt size: %fMB\n", size, size / ckpt_num );

	printf("==========   raw-gz   ==========\n");
	printf("Generate %lu checkpoint files\n", ckpt_num);
	printf("total dump time: %fs     average dump time: %fs\n", dump_raw_gz_time, dump_raw_gz_time / ckpt_num);
	size  = (double)total_raw_gz_size / 0x100000;
	printf("total ckpt size：%fMB   average ckpt size: %fMB\n", size, size / ckpt_num );
}

}
