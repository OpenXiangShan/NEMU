#ifndef NEMU_CUTEST_H
#define NEMU_CUTEST_H

#include <common.h>
#include <ext/amuctrl.h>

#ifdef CONFIG_SHARE_CTRL

void cutest_mma_emplace(uint8_t md, bool sat, bool isfp, bool issigned,
  uint8_t ms1, uint8_t ms2,
  uint16_t mtilem, uint16_t mtilen, uint16_t mtilek,
  uint8_t types, uint8_t typed);

void cutest_mls_emplace(uint8_t ms, bool ls, bool transpose, bool isacc,
  uint64_t base, uint64_t stride,
  uint16_t row, uint16_t column, uint8_t msew);

void cutest_mrelease_emplace(uint8_t tokenRd);

void cutest_mzero_emplace(bool isacc, uint8_t md);

#endif // CONFIG_SHARE_CTRL
#endif // NEMU_CUTEST_H