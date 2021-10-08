#ifndef __USER_H__
#define __USER_H__

#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include <memory/vaddr.h>
#include <sys/mman.h>

typedef struct {
  word_t entry;
  word_t brk;
  word_t brk_page;
  word_t program_brk;
  word_t phdr;
  int phent;
  int phnum;
  int std_fd[3];
} user_state_t;

extern user_state_t user_state;

word_t user_mmap(word_t addr, size_t length, int prot,
    int flags, int fd, off_t offset);
int user_munmap(word_t addr, size_t length);
word_t user_mremap(word_t old_addr, size_t old_size, size_t new_size,
    int flags, word_t new_addr);

#define USER_BASE MUXDEF(CONFIG_TARGET_SHARE, 0x100000000ul, 0x0ul)

static inline void* user_to_host(word_t uaddr) {
  return (void *)(uintptr_t)(uaddr + USER_BASE);
}

static inline word_t host_to_user(void *haddr) {
  return (word_t)(uintptr_t)(haddr - USER_BASE);
}

#endif
