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

#ifndef __CHECKPOINT_CPT_ENV__
#define __CHECKPOINT_CPT_ENV__
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <sys/stat.h>

enum { GZ_FORMAT, ZSTD_FORMAT, RAW_FORMAT};

extern char *output_base_dir;
extern char *config_name;
extern char *workload_name;
extern char *simpoints_dir;
extern int cpt_id;
extern char *cpt_file;
extern char *restorer;
extern char compress_file_format;

extern unsigned char page_vec[0x200000];

typedef struct{
	uint32_t l;
	uint32_t r;
}seg;
extern seg segs[0x100000];
extern size_t seg_num;

extern double dump_gz_time, dump_raw_time, dump_raw_gz_time;
extern size_t total_gz_size, total_raw_size, total_raw_gz_size;

extern clock_t start, end;
extern double time_cost;

extern struct stat file_stat;

extern size_t ckpt_num;

#define INDEX(addr) ((addr - 0x100000000) / 0x1000)

#endif
