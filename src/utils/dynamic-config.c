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

#include <common.h>

struct DynamicConfig dynamic_config = {};
void update_dynamic_config(void* config) {
  memcpy((void*)&dynamic_config, config, sizeof(struct DynamicConfig));
  // printf("NEMU dynamic_config update\n");
  // printf("  - ignore_illegal_mem_access %x\n", dynamic_config.ignore_illegal_mem_access);
  // printf("  - debug_difftest %x\n", dynamic_config.debug_difftest);
}
