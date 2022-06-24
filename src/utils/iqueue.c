/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#define INSTR_QUEUE_SIZE (1 << 5)

static struct {
  vaddr_t pc;
  uint8_t instr[20];
  uint8_t ilen;
} iqueue[INSTR_QUEUE_SIZE];
static uint32_t q_idx = 0;

void iqueue_commit(vaddr_t pc, uint8_t *instr_buf, uint8_t ilen) {
  iqueue[q_idx].pc = pc;
  iqueue[q_idx].ilen = ilen;
  assert(ilen < 20);
  memcpy(iqueue[q_idx].instr, instr_buf, ilen);
  q_idx = (q_idx + 1) % INSTR_QUEUE_SIZE;
}

void iqueue_dump() {
  int i;
  int victim_idx = (q_idx - 1) % INSTR_QUEUE_SIZE;
  printf("======== instruction queue =========\n");
  for (i = 0; i < INSTR_QUEUE_SIZE; i ++) {
    printf("%5s " FMT_WORD ": ", (i == victim_idx ? "-->" : ""), iqueue[i].pc);
    int j;
    for (j = 0; j < iqueue[i].ilen; j ++) {
      printf("%02x ", iqueue[i].instr[j]);
    }
    printf("\n");
  }
  printf("======== instruction queue end =========\n");
}
