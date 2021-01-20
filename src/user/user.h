#ifndef __USER_H__
#define __USER_H__

#include <memory/paddr.h>

typedef struct {
  word_t entry;
  word_t brk;
  word_t program_brk;
  word_t phdr;
  int phent;
  int phnum;
} user_state_t;

extern user_state_t user_state;

static inline void* user_to_host(word_t uaddr) {
  return guest_to_host(uaddr - PMEM_BASE);
}

static inline word_t host_to_user(void *haddr) {
  return host_to_guest(haddr) + PMEM_BASE;
}

#endif
