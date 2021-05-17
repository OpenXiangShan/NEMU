#include "user.h"
#include <isa.h>
#include <memory/host.h>
#include <stdlib.h>

#define ROUNDUP(a, sz)      ((((uintptr_t)a) + (sz) - 1) & ~((sz) - 1))
#define ROUNDDOWN(a, sz)    ((((uintptr_t)a)) & ~((sz) - 1))

word_t vaddr_read(struct Decode *s, vaddr_t addr, int len, int mmu_mode) {
  return host_read(user_to_host(addr), len);
}

void vaddr_write(struct Decode *s, vaddr_t addr, int len, word_t data, int mmu_mode) {
  host_write(user_to_host(addr), len, data);
}

word_t vaddr_ifetch(vaddr_t addr, int len) {
  return vaddr_read(NULL, addr, len, MMU_DYNAMIC);
}

word_t vaddr_read_safe(vaddr_t addr, int len) {
  return vaddr_read(NULL, addr, len, MMU_DYNAMIC);
}


typedef struct vma_t {
  uintptr_t addr;
  size_t length;
  int prot;
  int flags;
  int fd;
  off_t offset;
  struct vma_t *next;
  struct vma_t *prev;
} vma_t;

static vma_t vma_list = { };
static vma_t *dyn_start;

#define vma_foreach(p) for (p = vma_list.next; !vma_list_is_end(p); p = p->next)

static inline void vma_list_add_after(vma_t *left, vma_t *_new) {
  vma_t *right = left->next;
  _new->next = right;
  _new->prev = left;
  left->next = _new;
  right->prev = _new;
}

static inline bool vma_list_is_end(vma_t *p) {
  return (p == &vma_list);
}

static inline vma_t* vma_list_find_fix_area(uintptr_t addr, size_t length) {
  vma_t *p;
  vma_foreach(p) {
    if (p->addr == addr && p->length == length) return p;
  }
  return NULL;
}

static inline vma_t* vma_list_new_fix_area(uintptr_t addr, size_t length) {
  vma_t *candidate = NULL;
  vma_t *p;
  vma_foreach(p) {
    uintptr_t l = p->addr;
    uintptr_t r = p->addr + p->length;
    if (!((addr + length <= l) || (addr >= r))) {
      // overlap
      return NULL;
    }
    vma_t *right = p->next;
    if (p->addr < addr && addr < right->addr) { candidate = p; }
  }
  assert(candidate != NULL);
  return candidate;
}

// return the vma_t whose right is suitable
static inline vma_t* vma_list_new_dyn_area(size_t length) {
  vma_t *p = dyn_start;
  for (; !vma_list_is_end(p); p = p->next) {
    vma_t *right = p->next;
    size_t free = (uint8_t *)right->addr - ((uint8_t *)p->addr + p->length);
    if (free >= length) return p;
  }
  assert(0);
  return NULL;
}

static inline vma_t* vma_new(uintptr_t addr, size_t length, int prot,
    int flags, int fd, off_t offset) {
  vma_t *vma = (vma_t *) malloc(sizeof(vma_t));
  assert(vma);
  *vma = (vma_t) { .addr = addr, .length = length, .prot = prot,
    .flags = flags, .fd = fd, .offset = offset };
  return vma;
}

void init_mem() {
  vma_t *p = &vma_list;
  p->next = p->prev = p;

  vma_t *zero = vma_new(0ul, 0ul, 0, 0, -1, 0);
  vma_list_add_after(p, zero);

  dyn_start = vma_new(0x80000000ul, 0ul, 0, 0, -1, 0);
  vma_list_add_after(zero, dyn_start);

  vma_t *kernel = vma_new(0xc0000000ul, 0x40000000ul, 0, 0, -1, 0);
  vma_list_add_after(dyn_start, kernel);
}

void *user_mmap(void *addr, size_t length, int prot,
    int flags, int fd, off_t offset) {
  vma_t *left = NULL;
  length = ROUNDUP(length, 4096);
  if (flags & MAP_FIXED) {
    left = vma_list_new_fix_area((uintptr_t)addr, length);
    assert(left != NULL);
  } else {
    left = vma_list_new_dyn_area(length);
    addr = (uint8_t *)left->addr + left->length;
    flags |= MAP_FIXED;
  }
  vma_t *vma = vma_new((uintptr_t)addr, length, prot, flags, fd, offset);
  vma_list_add_after(left, vma);

  void *ret = mmap(addr, length, prot, flags, fd, offset);
  if (flags & MAP_FIXED) { assert(ret == addr); }
  return ret;
}

int user_munmap(void *addr, size_t length) {
  vma_t *p = vma_list_find_fix_area((uintptr_t)addr, length);
  assert(p != NULL);
  vma_t *prev = p->prev;
  vma_t *next = p->next;
  prev->next = next;
  next->prev = prev;

  int ret = munmap(addr, length);
  assert(ret == 0);
  free(p);
  return ret;
}

void *user_mremap(void *old_addr, size_t old_size, size_t new_size,
    int flags, void *new_addr) {
  vma_t *p = vma_list_find_fix_area((uintptr_t)old_addr, old_size);
  assert(p != NULL);
  assert(!(flags & MREMAP_FIXED));
  vma_t *next = p->next;
  size_t free_size_to_expand = next->addr - p->addr;
  new_size = ROUNDUP(new_size, 4096);
  if (free_size_to_expand >= new_size) {
    p->length = new_size;
    void *ret = mremap(old_addr, old_size, new_size, 0); // dont move
    if (ret != old_addr) perror("mremap");
    assert(ret == old_addr);
    return old_addr;
  } else {
    // should move
    new_addr = user_mmap(NULL, new_size, p->prot, p->flags & ~MAP_FIXED, -1, 0);
    memcpy(new_addr, old_addr, old_size);
    user_munmap(old_addr, p->length);
    return new_addr;
  }
}
