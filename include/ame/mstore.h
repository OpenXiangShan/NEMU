#ifndef __AME_MSTORE_H__
#define __AME_MSTORE_H__

#include <common.h>

#ifdef CONFIG_RV_AME

#include <isa-def.h>

#ifdef CONFIG_RV_AME_FP4
enum { MSEW_FP4 = 7 };
#endif

static inline size_t matrix_row_bytes(size_t columns, int msew) {
#ifdef CONFIG_RV_AME_FP4
  if (msew == MSEW_FP4) return (columns + 1) / 2;
#endif
  return columns << msew;
}

typedef struct {
  uint64_t base_vaddr;
  uint64_t stride;
  uint64_t pc;
  uint32_t row;
  uint32_t column;
  uint32_t msew;
  bool     transpose;

  bool     valid[MSYNC];
  uint64_t mrelease[MSYNC];
} mstore_info_t;

#endif // CONFIG_RV_AME
#endif // __AME_MSTORE_H__

