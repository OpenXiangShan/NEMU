#include "user.h"
#include <stdlib.h>

typedef struct vma_t {
  void *addr;
  size_t length;
  int prot;
  int flags;
  int fd;
  off_t offset;
  struct vma_t *next;
} vma_t;

static vma_t vma_list = {};

#define vma_foreach(p) for (p = vma_list.next; p != NULL; p = p->next)

static inline bool vma_list_check(void *addr, size_t length) {
  vma_t *p;
  vma_foreach(p) {
    void *l = p->addr;
    void *r = p->addr + p->length;
    if (!((addr + length <= l) || (addr >= r))) {
      // overlap
      return false;
    }
  }
  return true;
}

static inline vma_t* vma_list_find_last_big(void *addr) {
  vma_t *p;
  vma_foreach(p) {
    if (p->next == NULL) return p;
    if (addr < p->next->addr) return p;
  }
  return &vma_list;
}

static inline vma_t* vma_list_find_first_fit(size_t length) {
  vma_t *p;
  vma_foreach(p) {
    if (p->next == NULL) return p;
    size_t free = p->addr - (p->next->addr + p->next->length);
    if (free >= length) return p;
  }
  assert(0);
}

static inline vma_t* vma_new(void *addr, size_t length, int prot,
    int flags, int fd, off_t offset) {
  vma_t *vma = malloc(sizeof(vma_t));
  assert(vma);
  *vma = (vma_t) { .addr = addr, .length = length, .prot = prot,
    .flags = flags, .fd = fd, .offset = offset };
  return vma;
}

void *user_mmap(void *addr, size_t length, int prot,
    int flags, int fd, off_t offset) {
  vma_t *left = NULL;
  if (flags & MAP_FIXED) {
    int ok = vma_list_check(addr, length);
    assert(ok);
    left = vma_list_find_last_big(addr);
  } else {
    left = vma_list_find_first_fit(length);
    assert(left->addr > NULL + length);
    addr = left->addr - length;
    flags |= MAP_FIXED;
  }
  vma_t *vma = vma_new(addr, length, prot, flags, fd, offset);
  vma->next = left->next;
  left->next = vma;

  void *ret = mmap(addr, length, prot, flags, fd, offset);
  if (flags & MAP_FIXED) {
    assert(ret == addr);
  }
  return ret;
}

int user_munmap(void *addr, size_t length) {
  vma_t *p;
  bool ok = false;
  vma_t *last = &vma_list;
  vma_foreach(p) {
    if (p->addr == addr && p->length == length) {
      ok = true;
      break;
    }
    last = p;
  }
  assert(ok);
  last->next = p->next;

  int ret = munmap(addr, length);
  assert(ret == 0);
  free(p);
  return ret;
}
