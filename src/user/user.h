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

void *user_mmap(void *addr, size_t length, int prot,
    int flags, int fd, off_t offset);
int user_munmap(void *addr, size_t length);
void *user_mremap(void *old_addr, size_t old_size, size_t new_size,
    int flags, void *new_addr);

static inline uint8_t* user_to_host(word_t uaddr) {
  return (uint8_t *)(uintptr_t)uaddr;
}

static inline word_t host_to_user(void *haddr) {
  return (word_t)(uintptr_t)haddr;
}

#endif
