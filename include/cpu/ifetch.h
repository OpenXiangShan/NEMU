#ifndef __CPU_IFETCH_H__

#include <memory/vaddr.h>

static inline uint32_t instr_fetch(vaddr_t *pc, int len) {
  uint32_t instr = vaddr_ifetch(*pc, len);
#ifdef ENABLE_DIFFTEST_INSTR_QUEUE
  extern void add_instr(uint8_t *instr, int len);
  add_instr((void *)&instr, len);
#endif
#ifdef CONFIG_DEBUG
  uint8_t *p_instr = (void *)&instr;
  int i;
  for (i = 0; i < len; i ++) {
    int l = strlen(log_bytebuf);
    snprintf(log_bytebuf + l, sizeof(log_bytebuf) - l, "%02x ", p_instr[i]);
  }
#endif
  (*pc) += len;
  return instr;
}

#endif
