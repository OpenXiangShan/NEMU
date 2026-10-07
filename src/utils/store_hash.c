/***************************************************************************************
 * Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
 *
 * NEMU is licensed under Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 ***************************************************************************************/

#include <utils.h>

// Store hash: CRC-64/6sub8x, G(x) = x^64 + x^7 + x^6 + x^5 + x^4 + 1.
// Source: https://users.ece.cmu.edu/~koopman/crc/crc64.html (explicit polynomial 0x100000000000000f1).
// Absorb addr64 || maskedData64 || mask8, MSB first, seed 0, no reflection or xorout.
static inline uint64_t difftest_store_hash_byte(uint64_t crc, uint8_t byte) {
  // table[i] = rem(i * x^64, G); two 4-bit steps avoid a 136-bit software loop.
  static const uint64_t table[16] = {UINT64_C(0x0),   UINT64_C(0xf1),  UINT64_C(0x1e2), UINT64_C(0x113),
                                     UINT64_C(0x3c4), UINT64_C(0x335), UINT64_C(0x226), UINT64_C(0x2d7),
                                     UINT64_C(0x788), UINT64_C(0x779), UINT64_C(0x66a), UINT64_C(0x69b),
                                     UINT64_C(0x44c), UINT64_C(0x4bd), UINT64_C(0x5ae), UINT64_C(0x55f)};
  crc ^= (uint64_t)byte << 56;
  crc = (crc << 4) ^ table[crc >> 60];
  return (crc << 4) ^ table[crc >> 60];
}

uint64_t difftest_store_hash_update(uint64_t crc, uint64_t addr, uint64_t data, uint8_t mask) {
  for (int byte = 7; byte >= 0; byte--) {
    crc = difftest_store_hash_byte(crc, (uint8_t)(addr >> (byte * 8)));
  }
  for (int byte = 7; byte >= 0; byte--) {
    const uint8_t value = (mask & (1u << byte)) ? (uint8_t)(data >> (byte * 8)) : 0;
    crc = difftest_store_hash_byte(crc, value);
  }
  return difftest_store_hash_byte(crc, mask);
}
