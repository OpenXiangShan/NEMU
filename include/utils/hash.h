/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef NEMU_UTILS_HASH_H
#define NEMU_UTILS_HASH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// XXH3-128 of the boundary CPU-state bytes, independent of the DUT store CRC.
void difftest_hash_bytes(uint64_t *lo, uint64_t *hi, const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // NEMU_UTILS_HASH_H
