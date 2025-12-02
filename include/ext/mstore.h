#ifndef __EXT_MSTORE_H__
#define __EXT_MSTORE_H__

#include <common.h>

#ifdef CONFIG_RVMATRIX

#include <isa-def.h>

typedef struct {
  uint64_t base_vaddr;
  uint64_t stride;
  uint64_t pc;
  uint32_t row;
  uint32_t column;
  uint32_t msew;
  bool     transpose;
  
  bool     valid[MTOK];
  uint64_t mrelease[MTOK];
} mstore_info_t;

#endif // CONFIG_RVMATRIX
#endif // __EXT_MSTORE_H__

