#ifndef __X86_MMU_H__
#define __X86_MMU_H__

#include <common.h>

/* 32bit x86 uses 4KB page size */
#define NR_PDE						1024
#define NR_PTE						1024
#define PT_SIZE						((NR_PTE) * (PAGE_SIZE))

/* the 32bit Page Directory(first level page table) data structure */
typedef union PageDirectoryEntry {
  struct {
    uint32_t present             : 1;
    uint32_t read_write          : 1; 
    uint32_t user_supervisor     : 1;
    uint32_t page_write_through  : 1;
    uint32_t page_cache_disable  : 1;
    uint32_t accessed            : 1;
    uint32_t pad0                : 6;
    uint32_t page_frame          : 20;
  };
  uint32_t val;
} PDE;

/* the 32bit Page Table Entry(second level page table) data structure */
typedef union PageTableEntry {
  struct {
    uint32_t present             : 1;
    uint32_t read_write          : 1;
    uint32_t user_supervisor     : 1;
    uint32_t page_write_through  : 1;
    uint32_t page_cache_disable  : 1;
    uint32_t accessed            : 1;
    uint32_t dirty               : 1;
    uint32_t pad0                : 1;
    uint32_t global              : 1;
    uint32_t pad1                : 3;
    uint32_t page_frame          : 20;
  };
  uint32_t val;
} PTE;

typedef PTE (*PT) [NR_PTE];

typedef union GateDescriptor {
  struct {
    uint32_t offset_15_0      : 16;
    uint32_t dont_care0       : 16;
    uint32_t dont_care1       : 15;
    uint32_t present          : 1;
    uint32_t offset_31_16     : 16;
  };
  uint32_t val;
} GateDesc;

#endif
