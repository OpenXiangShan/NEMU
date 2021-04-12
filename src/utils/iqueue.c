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
