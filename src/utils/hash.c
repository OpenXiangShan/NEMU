/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include <common.h>
#include <utils/hash.h>

#ifdef CONFIG_FAST_REF
#include <xxhash.h>

void difftest_hash_bytes(uint64_t *lo, uint64_t *hi, const void *data, size_t size) {
  XXH128_hash_t hash = XXH3_128bits(data, size);
  *lo = hash.low64;
  *hi = hash.high64;
}
#endif
