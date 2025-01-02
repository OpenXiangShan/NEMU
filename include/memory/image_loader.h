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

#ifndef __IMAGE_LOADER_H__
#define __IMAGE_LOADER_H__

#include <stddef.h>


long load_gz_img(const char *filename);

long load_zstd_img(const char *filename);

long load_img(char *img_name, const char *which_img, uint8_t* load_start, size_t img_size);

void fill_memory(const char* img_file, const char* flash_image, const char* cpt_image, int64_t* img_size, int64_t* flash_size);

#endif //  __IMAGE_LOADER_H__
