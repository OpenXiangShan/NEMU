/***************************************************************************************
* Copyright (c) 2022-2024 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2024 Beijing Institute of Open Source Chip
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

/**
 * Branch Log
 * This file is organized from code scattered across various parts of NEMU.
*/

#include "common.h"
#include "utils.h"

#ifndef CONFIG_BR_LOG_SIZE
  #define CONFIG_BR_LOG_SIZE 50000000
#endif // CONFIG_BR_LOG_SIZE

struct br_info {
  uint64_t pc;
  uint64_t target;
  int taken;
  int type;
};

struct br_info br_log[CONFIG_BR_LOG_SIZE];
uint64_t br_count = 0;
int br_overflow = 0;

void br_log_commit(vaddr_t pc, vaddr_t target, int taken, int type) {
  br_log[br_count].pc = pc; // cpu.pc - 4;
  br_log[br_count].target = target;
  br_log[br_count].taken = taken;
  br_log[br_count].type = type;

  if (br_count + 1 >= CONFIG_BR_LOG_SIZE) {
    br_overflow = 1;
  }
  br_count = (br_count + 1) % CONFIG_BR_LOG_SIZE;
}

void br_log_dump() {
  int start = 0;
  int total = br_count;
  if (br_overflow) {
    start = br_count;
    total = CONFIG_BR_LOG_SIZE;
  }

  fprintf(stdout, "======== branch log ========\n");
  for (int i = 0; i < total; i++) {
    int idx = (i + start) % CONFIG_BR_LOG_SIZE;
    fprintf(stdout, FMT_WORD " %d %d " FMT_WORD "\n",
      br_log[idx].pc, br_log[idx].taken, br_log[idx].type, br_log[idx].target);
  }
  fprintf(stdout, "======== branch log end ========\n");
}

void * br_log_query() {
  return (void *) br_log;
}

uint64_t br_log_get_count() {
  return br_count;
}

void br_log_set_count(uint64_t val) {
  br_count = val;
}
