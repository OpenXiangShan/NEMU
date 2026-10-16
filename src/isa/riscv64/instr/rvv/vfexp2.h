#ifndef __RISCV64_RVV_VFEXP2_H__
#define __RISCV64_RVV_VFEXP2_H__

#include <common.h>
#include <rtl/fp.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint64_t result;
  uint32_t fflags;
} vfexp2_result_t;

bool vfexp2_rm_valid(uint32_t rm);
vfexp2_result_t vfexp2_compute(uint64_t src_bits, int sew, bool is_bf16, uint32_t rm);

#endif
