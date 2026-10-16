/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef CPU_DIFFTEST_AME_AMUHASH_MATH_H
#define CPU_DIFFTEST_AME_AMUHASH_MATH_H

#include "amuhash.h"

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

// Low word of value * (x^7+x^2+x+1), used when folding the polynomial.
static inline uint64_t gf128_fold_word(uint64_t value) {
  return value ^ (value << 1) ^ (value << 2) ^ (value << 7);
}

// Multiply by x^bits modulo x^128+x^7+x^2+x+1; callers use constant bits in 1..63.
static inline void gf128_shift(gf128_t *value, unsigned bits) {
  const uint64_t high = value->hi >> (64 - bits);
  value->hi = ((value->hi << bits) | (value->lo >> (64 - bits))) ^
              (high >> 62) ^ (high >> 57);
  value->lo = (value->lo << bits) ^ gf128_fold_word(high);
}


// Low/high halves of x^(9 * 2^k) modulo the hash polynomial, for k = 0..63.
// Entry zero is x^9; each subsequent entry is the square of the preceding entry.
static const gf128_t hash_shift_powers[64] = {
  {UINT64_C(0x0000000000000200), UINT64_C(0x0000000000000000)},
  {UINT64_C(0x0000000000040000), UINT64_C(0x0000000000000000)},
  {UINT64_C(0x0000001000000000), UINT64_C(0x0000000000000000)},
  {UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000100)},
  {UINT64_C(0x0000000000870000), UINT64_C(0x0000000000000000)},
  {UINT64_C(0x0000401500000000), UINT64_C(0x0000000000000000)},
  {UINT64_C(0x0000000000000000), UINT64_C(0x0000000010000111)},
  {UINT64_C(0x8700000000878787), UINT64_C(0x0000000000000000)},
  {UINT64_C(0x0000401540154015), UINT64_C(0x4015000000000000)},
  {UINT64_C(0x1000011110000529), UINT64_C(0x70008ff710000111)},
  {UINT64_C(0x86000000009686f0), UINT64_C(0xea000021eb0120ea)},
  {UINT64_C(0x8f5b41915c3fcf8d), UINT64_C(0xcfc8000000021cac)},
  {UINT64_C(0x11500749fec6a1be), UINT64_C(0xdb57808510014101)},
  {UINT64_C(0xd254509b3486518c), UINT64_C(0x1552eccac034d8b6)},
  {UINT64_C(0xb512d34d07e2113c), UINT64_C(0xdefbe4279499d2b0)},
  {UINT64_C(0x4f44326a343e4a0f), UINT64_C(0x59960974f577069b)},
  {UINT64_C(0x26ede4bf1a096036), UINT64_C(0xc6335d7c2449f71f)},
  {UINT64_C(0x153c512d35ebbd60), UINT64_C(0xbe7acdc28a2d87e7)},
  {UINT64_C(0xe6cf61538f91a382), UINT64_C(0x71b7cda5881fb66c)},
  {UINT64_C(0xa195ed0915630082), UINT64_C(0xbfb001978d15c5d3)},
  {UINT64_C(0xe8269bdbb8de64f6), UINT64_C(0x34091a1154f74d8b)},
  {UINT64_C(0xba1b70b74d06b741), UINT64_C(0xc7302553e599d6ca)},
  {UINT64_C(0xb487667a78509782), UINT64_C(0xffad9a4701ffb0a5)},
  {UINT64_C(0x15ec11081a3681a2), UINT64_C(0x4518b50370c06faf)},
  {UINT64_C(0xea6db51e0709b717), UINT64_C(0x79e6f2b352f602d1)},
  {UINT64_C(0xf522212d472033a4), UINT64_C(0x9e28b2166c2e58c7)},
  {UINT64_C(0x55a733a4d3ecee9c), UINT64_C(0x39bf39e75d1d8937)},
  {UINT64_C(0x8f32fa46b511d1b5), UINT64_C(0xab75343cbf69e2f3)},
  {UINT64_C(0x35304c6dff367de3), UINT64_C(0xbf04e6f1c636a287)},
  {UINT64_C(0xef3b945bcb6c8f87), UINT64_C(0x753a0d5bb6173fff)},
  {UINT64_C(0x012999b9f0554508), UINT64_C(0xb7a0bd99682fc53d)},
  {UINT64_C(0x18c22613a8e4a31e), UINT64_C(0x51c9d8623357230c)},
  {UINT64_C(0xddd3a9f95a9e2cdc), UINT64_C(0xf6eec16a12de6f1b)},
  {UINT64_C(0x94715df2437be05a), UINT64_C(0x7016d680f4cc1a9d)},
  {UINT64_C(0x335c8cf5f0fd7a45), UINT64_C(0xaa1098442c1c952f)},
  {UINT64_C(0x60b0fbc05ab13653), UINT64_C(0xfad9967126982d63)},
  {UINT64_C(0x070923c330296acf), UINT64_C(0x1cf553661823bb85)},
  {UINT64_C(0xa3c21af96c3e98a2), UINT64_C(0xae8e23bef1943669)},
  {UINT64_C(0x3ff64a26d026182a), UINT64_C(0xb388bbaa1ffc25c6)},
  {UINT64_C(0xfd0006a6159eaf41), UINT64_C(0x5cefb4f7683cfbc8)},
  {UINT64_C(0x4cd3f3cf4c2785f9), UINT64_C(0x8bca8708534f65f5)},
  {UINT64_C(0xe5c35f3424cd66d1), UINT64_C(0xf322c2f89fee719d)},
  {UINT64_C(0x6810d7f7ffb22a67), UINT64_C(0x7d884e30a3620ff1)},
  {UINT64_C(0x8bc42b182f443aa4), UINT64_C(0xd656e0c80abbce36)},
  {UINT64_C(0x27aa68629eeac123), UINT64_C(0x7d21ad57a86c9080)},
  {UINT64_C(0xbc9e11d51720c133), UINT64_C(0xc62058e0e17fe9cc)},
  {UINT64_C(0xad98c400dd6f88ea), UINT64_C(0xff3e5d5cd7eafd3a)},
  {UINT64_C(0x6c919ba24275f9c9), UINT64_C(0x4478f1e48e2fde99)},
  {UINT64_C(0xfbaa221349cc323e), UINT64_C(0x6c2a8bea6ae9e055)},
  {UINT64_C(0x5fb6dd3ca90cff95), UINT64_C(0x10f77bb9e7748ed3)},
  {UINT64_C(0xe2a0e301bed0748a), UINT64_C(0x967e64f599297f3c)},
  {UINT64_C(0x23116c4dd539e293), UINT64_C(0x196284a6305e23d7)},
  {UINT64_C(0xca19d9ef4ab67cee), UINT64_C(0xa2486f3cdc03c63f)},
  {UINT64_C(0x0ef447a6af3ef516), UINT64_C(0x8e50508b1668e6cc)},
  {UINT64_C(0xc93348bff35488c3), UINT64_C(0xebf0a218e734a74f)},
  {UINT64_C(0xf3ec824396a31e7f), UINT64_C(0xdf312e26ce5ce3be)},
  {UINT64_C(0xdab09a9eafec71a6), UINT64_C(0x492ccfd577aa0640)},
  {UINT64_C(0xa59dab8c1f65302c), UINT64_C(0x00017099da467ea9)},
  {UINT64_C(0x15816e77c58ff917), UINT64_C(0x441141dcaf64263e)},
  {UINT64_C(0xa7302427432fb581), UINT64_C(0x7961c78e64fa0b86)},
  {UINT64_C(0x745e0c89666b8f5b), UINT64_C(0x8e5869aebedaefb3)},
  {UINT64_C(0x649100b2c75d58b9), UINT64_C(0xfeb4c79e4d34b7ce)},
  {UINT64_C(0x092082024082cbbc), UINT64_C(0x149f1258baca29a0)},
  {UINT64_C(0x68f5d3da6d219950), UINT64_C(0x8f10682bc510d6e7)},
};


// Each byte is a nine-bit symbol: 0x100 | data when covered, zero when unwritten.
static inline void hash_update_byte(gf128_t *hash, uint64_t symbol) {
  gf128_shift(hash, HASH_SYMBOL_BITS);
  hash->lo ^= symbol;
}

// Seven symbols occupy 63 bits; reducing them together breaks the bytewise dependency.
static inline void hash_update_seven(gf128_t *hash, uint64_t symbols) {
  gf128_shift(hash, HASH_SYMBOL_BITS * HASH_BATCH_BYTES);
  hash->lo ^= symbols;
}

static inline uint64_t hash_pack_seven(const uint8_t *data) {
  return UINT64_C(0x4020100804020100) | (uint64_t)data[0] | ((uint64_t)data[1] << 9) |
         ((uint64_t)data[2] << 18) | ((uint64_t)data[3] << 27) |
         ((uint64_t)data[4] << 36) | ((uint64_t)data[5] << 45) | ((uint64_t)data[6] << 54);
}

static inline void hash_update_data(gf128_t *hash, const uint8_t *data, size_t bytes) {
  while (bytes >= HASH_BATCH_BYTES) {
    bytes -= HASH_BATCH_BYTES;
    hash_update_seven(hash, hash_pack_seven(data + bytes));
  }
  while (bytes != 0) hash_update_byte(hash, UINT64_C(0x100) | data[--bytes]);
}

static inline void hash_advance_unwritten(gf128_t *hash, size_t bytes) {
  while (bytes >= HASH_BATCH_BYTES) {
    hash_update_seven(hash, 0);
    bytes -= HASH_BATCH_BYTES;
  }
  while (bytes-- != 0) hash_update_byte(hash, 0);
}

#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

#endif // CPU_DIFFTEST_AME_AMUHASH_MATH_H
