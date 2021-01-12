#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include <common.h>

#ifdef _SHARE
    #define PMEM_SIZE (8 * 1024 * 1024 * 1024UL)
#else
    #define PMEM_SIZE (256 * 1024 * 1024UL) 
#endif

void init_mem();

/* convert the guest physical address in the guest program to host virtual address in NEMU */
void* guest_to_host(paddr_t addr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
paddr_t host_to_guest(void *addr);

word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, word_t data, int len);
bool is_sfence_safe(paddr_t addr, int len);

#if _SHARE
#define DIFFTEST_STORE_COMMIT
#endif

#ifdef DIFFTEST_STORE_COMMIT

#define STORE_QUEUE_SIZE 48
typedef struct {
    uint64_t addr;
    uint64_t data;
    uint8_t  mask;
    uint8_t  valid;
} store_commit_t;
extern store_commit_t store_commit_queue[STORE_QUEUE_SIZE];

void store_commit_queue_push(uint64_t addr, uint64_t data, int len);
store_commit_t *store_commit_queue_pop();
int check_store_commit(uint64_t *addr, uint64_t *data, uint8_t *mask);
#endif

#endif
