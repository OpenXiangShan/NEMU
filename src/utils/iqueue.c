#include <common.h>

#define INSTR_QUEUE_SIZE (1 << 5)
#define MAX_ILEN 16

static struct {
  vaddr_t pc;
  uint8_t instr[MAX_ILEN];
  uint8_t ilen;
} iqueue[INSTR_QUEUE_SIZE];
static uint32_t q_idx = 0;

void iqueue_commit(vaddr_t pc, uint8_t *instr_buf, uint8_t ilen) {
  iqueue[q_idx].pc = pc;
  iqueue[q_idx].ilen = ilen;
  assert(ilen < MAX_ILEN);
  memcpy(iqueue[q_idx].instr, instr_buf, ilen);
  q_idx = (q_idx + 1) % INSTR_QUEUE_SIZE;
}

void iqueue_dump() {
  int i;
  int victim_idx = (q_idx - 1) % INSTR_QUEUE_SIZE;
  printf("======== instruction queue =========\n");
  for (i = 0; i < INSTR_QUEUE_SIZE; i ++) {
    printf("%5s " FMT_WORD ": ", (i == victim_idx ? "-->" : ""), iqueue[i].pc);
#ifdef CONFIG_ITRACE
    void disassemble(char *str, int size, vaddr_t pc, uint8_t *code, int nbyte);
    char buf[80];
    disassemble(buf, sizeof(buf), iqueue[i].pc, iqueue[i].instr, iqueue[i].ilen);
    printf("%s\t\t", buf);
#endif
    int j;
    for (j = 0; j < iqueue[i].ilen; j ++) {
      printf(" %02x", iqueue[i].instr[j]);
    }
    printf("\n");
  }
  printf("======== instruction queue end =========\n");
}
