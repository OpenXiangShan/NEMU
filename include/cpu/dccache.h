#ifndef __CPU_DCCACHE_H__
#define __CPU_DCCACHE_H__

#include <cpu/decode.h>

#define DCACHE_SIZE 256

typedef struct {
  vaddr_t tag;
  const void *EHelper;
  DecodeExecState s;
} dccacheEntry;

extern dccacheEntry dccache[DCACHE_SIZE];

void dccache_flush();

static inline int dccache_idx(vaddr_t pc) {
  return (pc >> 2) % DCACHE_SIZE;
}

#endif
