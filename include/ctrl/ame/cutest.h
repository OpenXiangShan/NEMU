#ifndef __CTRL_AME_CUTEST_H__
#define __CTRL_AME_CUTEST_H__

#include <common.h>

#ifdef CONFIG_SHARE_CTRL
#include <ame/event.h>

void cutest_mma_emplace(uint8_t md, bool sat, bool isfp,
  uint8_t ms1, uint8_t ms2,
  uint16_t mtilem, uint16_t mtilen, uint16_t mtilek,
  uint8_t types1, uint8_t types2, uint8_t typed);

void cutest_mls_emplace(uint8_t ms, bool ls, bool transpose, bool isacc,
  bool isA, uint64_t base, uint64_t stride,
  uint16_t row, uint16_t column, uint8_t msew);

void cutest_mrelease_emplace(uint8_t msyncRd);

void cutest_mzero_emplace(bool isacc, uint8_t md);

#endif // CONFIG_SHARE_CTRL
#endif // __CTRL_AME_CUTEST_H__
