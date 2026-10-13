/***************************************************************************************
 * Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
 * Copyright (c) 2021-2026 Beijing Institute of Open Source Chip
 *
 * NEMU is licensed under Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 ***************************************************************************************/

#include <utils.h>

// Store hash: CRC-64/6sub8x, G(x) = x^64 + x^7 + x^6 + x^5 + x^4 + 1.
// Source: https://users.ece.cmu.edu/~koopman/crc/crc64.html (explicit polynomial 0x100000000000000f1).
// Absorb addr64 || maskedData64 || mask8, MSB first, seed 0, no reflection or xorout.
// Carryless multiplication by x^7 + x^6 + x^5 + x^4 + 1, truncated to 64 bits.
static inline uint64_t store_hash_polynomial(uint64_t v) {
  return v ^ (v << 4) ^ (v << 5) ^ (v << 6) ^ (v << 7);
}

static inline uint64_t store_hash_word(uint64_t v) {
  // x^64 = 0xf1 (mod G). The first product's overflow has at most seven bits.
  uint64_t high = (v >> 57) ^ (v >> 58) ^ (v >> 59) ^ (v >> 60);
  return store_hash_polynomial(v) ^ store_hash_polynomial(high);
}

uint64_t difftest_store_hash_update(uint64_t crc, uint64_t addr, uint64_t data, uint8_t mask) {
  // Spread each mask bit into the low bit of its byte, then expand to 0xff.
  uint64_t bytes = mask;
  bytes = (bytes | (bytes << 28)) & UINT64_C(0x0000000f0000000f);
  bytes = (bytes | (bytes << 14)) & UINT64_C(0x0003000300030003);
  bytes = (bytes | (bytes << 7)) & UINT64_C(0x0101010101010101);
  crc = store_hash_word(crc ^ addr);
  crc = store_hash_word(crc ^ (data & (bytes * 0xff)));
  return (crc << 8) ^ store_hash_polynomial((crc >> 56) ^ mask);
}
