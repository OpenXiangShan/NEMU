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

#ifndef CONFIG_SHARE
#ifndef __CHECKPOINT_CPT_ENV__
#define __CHECKPOINT_CPT_ENV__

enum { GZ_FORMAT, ZSTD_FORMAT };

extern char *output_base_dir;
extern char *config_name;
extern char *workload_name;
extern char *simpoints_dir;
extern int cpt_id;
extern char *cpt_file;
extern char *restorer;
extern char compress_file_format;

#endif
#endif // CONFIG_SHARE
