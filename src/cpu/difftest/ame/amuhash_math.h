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
#include <string.h>

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

// Normal Castagnoli polynomial tables; P = x^32 + 0x1edc6f41.
static const uint32_t hash_shift_powers[64] = {
  UINT32_C(0x00000200), UINT32_C(0x00040000), UINT32_C(0xf31a9b51), UINT32_C(0x6bed1038),
  UINT32_C(0xc555c054), UINT32_C(0xb8caa439), UINT32_C(0xa6955f31), UINT32_C(0xcf8f7aa7),
  UINT32_C(0x04f40d48), UINT32_C(0xe5609971), UINT32_C(0x4afde2f2), UINT32_C(0x1cc813f5),
  UINT32_C(0x6b617a6f), UINT32_C(0xba093473), UINT32_C(0xc02835ea), UINT32_C(0x4cda9daf),
  UINT32_C(0x544e32d5), UINT32_C(0x478b2e43), UINT32_C(0xd770d206), UINT32_C(0xc083b71e),
  UINT32_C(0xb1b6b567), UINT32_C(0x0b7f2108), UINT32_C(0xe5989fef), UINT32_C(0x10b0e75d),
  UINT32_C(0x0012a4de), UINT32_C(0xa03e2f8f), UINT32_C(0xec44eab0), UINT32_C(0x9e81ea7f),
  UINT32_C(0x7b2a652e), UINT32_C(0xd0af0743), UINT32_C(0x54f63754), UINT32_C(0x00000200),
  UINT32_C(0x00040000), UINT32_C(0xf31a9b51), UINT32_C(0x6bed1038), UINT32_C(0xc555c054),
  UINT32_C(0xb8caa439), UINT32_C(0xa6955f31), UINT32_C(0xcf8f7aa7), UINT32_C(0x04f40d48),
  UINT32_C(0xe5609971), UINT32_C(0x4afde2f2), UINT32_C(0x1cc813f5), UINT32_C(0x6b617a6f),
  UINT32_C(0xba093473), UINT32_C(0xc02835ea), UINT32_C(0x4cda9daf), UINT32_C(0x544e32d5),
  UINT32_C(0x478b2e43), UINT32_C(0xd770d206), UINT32_C(0xc083b71e), UINT32_C(0xb1b6b567),
  UINT32_C(0x0b7f2108), UINT32_C(0xe5989fef), UINT32_C(0x10b0e75d), UINT32_C(0x0012a4de),
  UINT32_C(0xa03e2f8f), UINT32_C(0xec44eab0), UINT32_C(0x9e81ea7f), UINT32_C(0x7b2a652e),
  UINT32_C(0xd0af0743), UINT32_C(0x54f63754), UINT32_C(0x00000200), UINT32_C(0x00040000),
};

static const uint32_t hash_initial_powers[64] = {
  UINT32_C(0xea6a836a), UINT32_C(0xedfd3b42), UINT32_C(0xdc0df191), UINT32_C(0x3ae95822),
  UINT32_C(0xdb4c5b6c), UINT32_C(0xb49b9ba0), UINT32_C(0x2c05b807), UINT32_C(0x597b27c9),
  UINT32_C(0x8f2ad203), UINT32_C(0xd6359d9c), UINT32_C(0x89112215), UINT32_C(0xd39c53c8),
  UINT32_C(0x5f578c77), UINT32_C(0xd7add622), UINT32_C(0x9a35944f), UINT32_C(0x1427bf3d),
  UINT32_C(0x7a0f92b4), UINT32_C(0x244f1a88), UINT32_C(0x3af8eee0), UINT32_C(0x051ed2bb),
  UINT32_C(0x7241e3fe), UINT32_C(0x73146487), UINT32_C(0x5eb921b2), UINT32_C(0x1fcc0ca7),
  UINT32_C(0x40b8432e), UINT32_C(0xca42ea2c), UINT32_C(0x7e1c337f), UINT32_C(0x768a2a4f),
  UINT32_C(0x9e4d5323), UINT32_C(0x0a79198d), UINT32_C(0x36811a94), UINT32_C(0xea6a836a),
  UINT32_C(0xedfd3b42), UINT32_C(0xdc0df191), UINT32_C(0x3ae95822), UINT32_C(0xdb4c5b6c),
  UINT32_C(0xb49b9ba0), UINT32_C(0x2c05b807), UINT32_C(0x597b27c9), UINT32_C(0x8f2ad203),
  UINT32_C(0xd6359d9c), UINT32_C(0x89112215), UINT32_C(0xd39c53c8), UINT32_C(0x5f578c77),
  UINT32_C(0xd7add622), UINT32_C(0x9a35944f), UINT32_C(0x1427bf3d), UINT32_C(0x7a0f92b4),
  UINT32_C(0x244f1a88), UINT32_C(0x3af8eee0), UINT32_C(0x051ed2bb), UINT32_C(0x7241e3fe),
  UINT32_C(0x73146487), UINT32_C(0x5eb921b2), UINT32_C(0x1fcc0ca7), UINT32_C(0x40b8432e),
  UINT32_C(0xca42ea2c), UINT32_C(0x7e1c337f), UINT32_C(0x768a2a4f), UINT32_C(0x9e4d5323),
  UINT32_C(0x0a79198d), UINT32_C(0x36811a94), UINT32_C(0xea6a836a), UINT32_C(0xedfd3b42),
};

static const uint32_t hash_reduce_table[256] = {
  UINT32_C(0x00000000), UINT32_C(0x1edc6f41), UINT32_C(0x3db8de82), UINT32_C(0x2364b1c3),
  UINT32_C(0x7b71bd04), UINT32_C(0x65add245), UINT32_C(0x46c96386), UINT32_C(0x58150cc7),
  UINT32_C(0xf6e37a08), UINT32_C(0xe83f1549), UINT32_C(0xcb5ba48a), UINT32_C(0xd587cbcb),
  UINT32_C(0x8d92c70c), UINT32_C(0x934ea84d), UINT32_C(0xb02a198e), UINT32_C(0xaef676cf),
  UINT32_C(0xf31a9b51), UINT32_C(0xedc6f410), UINT32_C(0xcea245d3), UINT32_C(0xd07e2a92),
  UINT32_C(0x886b2655), UINT32_C(0x96b74914), UINT32_C(0xb5d3f8d7), UINT32_C(0xab0f9796),
  UINT32_C(0x05f9e159), UINT32_C(0x1b258e18), UINT32_C(0x38413fdb), UINT32_C(0x269d509a),
  UINT32_C(0x7e885c5d), UINT32_C(0x6054331c), UINT32_C(0x433082df), UINT32_C(0x5deced9e),
  UINT32_C(0xf8e959e3), UINT32_C(0xe63536a2), UINT32_C(0xc5518761), UINT32_C(0xdb8de820),
  UINT32_C(0x8398e4e7), UINT32_C(0x9d448ba6), UINT32_C(0xbe203a65), UINT32_C(0xa0fc5524),
  UINT32_C(0x0e0a23eb), UINT32_C(0x10d64caa), UINT32_C(0x33b2fd69), UINT32_C(0x2d6e9228),
  UINT32_C(0x757b9eef), UINT32_C(0x6ba7f1ae), UINT32_C(0x48c3406d), UINT32_C(0x561f2f2c),
  UINT32_C(0x0bf3c2b2), UINT32_C(0x152fadf3), UINT32_C(0x364b1c30), UINT32_C(0x28977371),
  UINT32_C(0x70827fb6), UINT32_C(0x6e5e10f7), UINT32_C(0x4d3aa134), UINT32_C(0x53e6ce75),
  UINT32_C(0xfd10b8ba), UINT32_C(0xe3ccd7fb), UINT32_C(0xc0a86638), UINT32_C(0xde740979),
  UINT32_C(0x866105be), UINT32_C(0x98bd6aff), UINT32_C(0xbbd9db3c), UINT32_C(0xa505b47d),
  UINT32_C(0xef0edc87), UINT32_C(0xf1d2b3c6), UINT32_C(0xd2b60205), UINT32_C(0xcc6a6d44),
  UINT32_C(0x947f6183), UINT32_C(0x8aa30ec2), UINT32_C(0xa9c7bf01), UINT32_C(0xb71bd040),
  UINT32_C(0x19eda68f), UINT32_C(0x0731c9ce), UINT32_C(0x2455780d), UINT32_C(0x3a89174c),
  UINT32_C(0x629c1b8b), UINT32_C(0x7c4074ca), UINT32_C(0x5f24c509), UINT32_C(0x41f8aa48),
  UINT32_C(0x1c1447d6), UINT32_C(0x02c82897), UINT32_C(0x21ac9954), UINT32_C(0x3f70f615),
  UINT32_C(0x6765fad2), UINT32_C(0x79b99593), UINT32_C(0x5add2450), UINT32_C(0x44014b11),
  UINT32_C(0xeaf73dde), UINT32_C(0xf42b529f), UINT32_C(0xd74fe35c), UINT32_C(0xc9938c1d),
  UINT32_C(0x918680da), UINT32_C(0x8f5aef9b), UINT32_C(0xac3e5e58), UINT32_C(0xb2e23119),
  UINT32_C(0x17e78564), UINT32_C(0x093bea25), UINT32_C(0x2a5f5be6), UINT32_C(0x348334a7),
  UINT32_C(0x6c963860), UINT32_C(0x724a5721), UINT32_C(0x512ee6e2), UINT32_C(0x4ff289a3),
  UINT32_C(0xe104ff6c), UINT32_C(0xffd8902d), UINT32_C(0xdcbc21ee), UINT32_C(0xc2604eaf),
  UINT32_C(0x9a754268), UINT32_C(0x84a92d29), UINT32_C(0xa7cd9cea), UINT32_C(0xb911f3ab),
  UINT32_C(0xe4fd1e35), UINT32_C(0xfa217174), UINT32_C(0xd945c0b7), UINT32_C(0xc799aff6),
  UINT32_C(0x9f8ca331), UINT32_C(0x8150cc70), UINT32_C(0xa2347db3), UINT32_C(0xbce812f2),
  UINT32_C(0x121e643d), UINT32_C(0x0cc20b7c), UINT32_C(0x2fa6babf), UINT32_C(0x317ad5fe),
  UINT32_C(0x696fd939), UINT32_C(0x77b3b678), UINT32_C(0x54d707bb), UINT32_C(0x4a0b68fa),
  UINT32_C(0xc0c1d64f), UINT32_C(0xde1db90e), UINT32_C(0xfd7908cd), UINT32_C(0xe3a5678c),
  UINT32_C(0xbbb06b4b), UINT32_C(0xa56c040a), UINT32_C(0x8608b5c9), UINT32_C(0x98d4da88),
  UINT32_C(0x3622ac47), UINT32_C(0x28fec306), UINT32_C(0x0b9a72c5), UINT32_C(0x15461d84),
  UINT32_C(0x4d531143), UINT32_C(0x538f7e02), UINT32_C(0x70ebcfc1), UINT32_C(0x6e37a080),
  UINT32_C(0x33db4d1e), UINT32_C(0x2d07225f), UINT32_C(0x0e63939c), UINT32_C(0x10bffcdd),
  UINT32_C(0x48aaf01a), UINT32_C(0x56769f5b), UINT32_C(0x75122e98), UINT32_C(0x6bce41d9),
  UINT32_C(0xc5383716), UINT32_C(0xdbe45857), UINT32_C(0xf880e994), UINT32_C(0xe65c86d5),
  UINT32_C(0xbe498a12), UINT32_C(0xa095e553), UINT32_C(0x83f15490), UINT32_C(0x9d2d3bd1),
  UINT32_C(0x38288fac), UINT32_C(0x26f4e0ed), UINT32_C(0x0590512e), UINT32_C(0x1b4c3e6f),
  UINT32_C(0x435932a8), UINT32_C(0x5d855de9), UINT32_C(0x7ee1ec2a), UINT32_C(0x603d836b),
  UINT32_C(0xcecbf5a4), UINT32_C(0xd0179ae5), UINT32_C(0xf3732b26), UINT32_C(0xedaf4467),
  UINT32_C(0xb5ba48a0), UINT32_C(0xab6627e1), UINT32_C(0x88029622), UINT32_C(0x96def963),
  UINT32_C(0xcb3214fd), UINT32_C(0xd5ee7bbc), UINT32_C(0xf68aca7f), UINT32_C(0xe856a53e),
  UINT32_C(0xb043a9f9), UINT32_C(0xae9fc6b8), UINT32_C(0x8dfb777b), UINT32_C(0x9327183a),
  UINT32_C(0x3dd16ef5), UINT32_C(0x230d01b4), UINT32_C(0x0069b077), UINT32_C(0x1eb5df36),
  UINT32_C(0x46a0d3f1), UINT32_C(0x587cbcb0), UINT32_C(0x7b180d73), UINT32_C(0x65c46232),
  UINT32_C(0x2fcf0ac8), UINT32_C(0x31136589), UINT32_C(0x1277d44a), UINT32_C(0x0cabbb0b),
  UINT32_C(0x54beb7cc), UINT32_C(0x4a62d88d), UINT32_C(0x6906694e), UINT32_C(0x77da060f),
  UINT32_C(0xd92c70c0), UINT32_C(0xc7f01f81), UINT32_C(0xe494ae42), UINT32_C(0xfa48c103),
  UINT32_C(0xa25dcdc4), UINT32_C(0xbc81a285), UINT32_C(0x9fe51346), UINT32_C(0x81397c07),
  UINT32_C(0xdcd59199), UINT32_C(0xc209fed8), UINT32_C(0xe16d4f1b), UINT32_C(0xffb1205a),
  UINT32_C(0xa7a42c9d), UINT32_C(0xb97843dc), UINT32_C(0x9a1cf21f), UINT32_C(0x84c09d5e),
  UINT32_C(0x2a36eb91), UINT32_C(0x34ea84d0), UINT32_C(0x178e3513), UINT32_C(0x09525a52),
  UINT32_C(0x51475695), UINT32_C(0x4f9b39d4), UINT32_C(0x6cff8817), UINT32_C(0x7223e756),
  UINT32_C(0xd726532b), UINT32_C(0xc9fa3c6a), UINT32_C(0xea9e8da9), UINT32_C(0xf442e2e8),
  UINT32_C(0xac57ee2f), UINT32_C(0xb28b816e), UINT32_C(0x91ef30ad), UINT32_C(0x8f335fec),
  UINT32_C(0x21c52923), UINT32_C(0x3f194662), UINT32_C(0x1c7df7a1), UINT32_C(0x02a198e0),
  UINT32_C(0x5ab49427), UINT32_C(0x4468fb66), UINT32_C(0x670c4aa5), UINT32_C(0x79d025e4),
  UINT32_C(0x243cc87a), UINT32_C(0x3ae0a73b), UINT32_C(0x198416f8), UINT32_C(0x075879b9),
  UINT32_C(0x5f4d757e), UINT32_C(0x41911a3f), UINT32_C(0x62f5abfc), UINT32_C(0x7c29c4bd),
  UINT32_C(0xd2dfb272), UINT32_C(0xcc03dd33), UINT32_C(0xef676cf0), UINT32_C(0xf1bb03b1),
  UINT32_C(0xa9ae0f76), UINT32_C(0xb7726037), UINT32_C(0x9416d1f4), UINT32_C(0x8acabeb5),
};

// A 64-bit value may be an unreduced representative modulo the degree-32 P.
// Reduce its high four bytes with the normal CRC table, then include its low word.
static inline uint32_t gf32_reduce(uint64_t value) {
  uint32_t high = value >> 32;
  for (unsigned i = 0; i < 4; ++i)
    high = (high << 8) ^ hash_reduce_table[high >> 24];
  return high ^ (uint32_t)value;
}

// rem(high * x^64, P); used to collapse an unreduced 128-bit product.
static inline uint64_t gf32_fold(uint64_t high) {
  uint32_t crc = gf32_reduce(high);
  for (unsigned i = 0; i < 8; ++i)
    crc = (crc << 8) ^ hash_reduce_table[crc >> 24];
  return crc;
}

static inline uint64_t gf32_shift(uint64_t value, unsigned bits) {
  return (value << bits) ^ gf32_fold(value >> (64 - bits));
}

static inline uint64_t gf32_multiply(uint64_t a, uint64_t b) {
  a = gf32_reduce(a);
  b = gf32_reduce(b);
  uint64_t result = 0;
  while (b) {
    result ^= (UINT64_C(0) - (b & 1)) & a;
    a = (uint32_t)(a << 1) ^ ((UINT64_C(0) - (a >> 31)) & UINT64_C(0x1edc6f41));
    b >>= 1;
  }
  return result;
}

typedef struct { uint64_t multiples[16]; } gf32_multiplier_t;

static inline void gf32_prepare(gf32_multiplier_t *table, uint64_t factor) {
  table->multiples[0] = 0;
  for (unsigned bit = 1; bit < 16; bit <<= 1) {
    table->multiples[bit] = factor;
    for (unsigned i = 1; i < bit; ++i)
      table->multiples[bit + i] = factor ^ table->multiples[i];
    factor = gf32_shift(factor, 1);
  }
}

static inline uint64_t gf32_multiply_prepared(uint64_t value, const gf32_multiplier_t *table) {
  uint64_t result = 0;
  for (unsigned nibble = 0; nibble < 16; ++nibble, value <<= 4)
    result = gf32_shift(result, 4) ^ table->multiples[value >> 60];
  return result;
}

static inline uint64_t hash_shift_factor(size_t bytes) {
  uint64_t factor = 1;
  for (unsigned bit = 0; bytes; ++bit, bytes >>= 1) {
    if (bytes & 1) factor = factor == 1 ? hash_shift_powers[bit] :
                           gf32_multiply(factor, hash_shift_powers[bit]);
  }
  return factor;
}

static inline uint64_t hash_reverse(uint64_t value) {
  value = ((value & UINT64_C(0x5555555555555555)) << 1) |
          ((value >> 1) & UINT64_C(0x5555555555555555));
  value = ((value & UINT64_C(0x3333333333333333)) << 2) |
          ((value >> 2) & UINT64_C(0x3333333333333333));
  value = ((value & UINT64_C(0x0f0f0f0f0f0f0f0f)) << 4) |
          ((value >> 4) & UINT64_C(0x0f0f0f0f0f0f0f0f));
  return __builtin_bswap64(value);
}

static inline uint64_t hash_finalize(uint64_t value, size_t reg_bytes) {
  uint64_t initial;
  if (reg_bytes && !(reg_bytes & (reg_bytes - 1))) {
    initial = hash_initial_powers[__builtin_ctzll(reg_bytes)];
  } else {
    initial = gf32_multiply(UINT32_MAX, hash_shift_factor(reg_bytes));
  }
  const uint32_t state = gf32_reduce((uint64_t)gf32_reduce(value) << 32) ^ initial;
  return (uint32_t)~(hash_reverse(state) >> 32);
}

static inline uint64_t hash_pack_seven(const uint8_t *data) {
  return UINT64_C(0x4020100804020100) | (uint64_t)data[0] | ((uint64_t)data[1] << 9) |
         ((uint64_t)data[2] << 18) | ((uint64_t)data[3] << 27) |
         ((uint64_t)data[4] << 36) | ((uint64_t)data[5] << 45) | ((uint64_t)data[6] << 54);
}

static inline uint64_t hash_load_le64(const uint8_t *data) {
  uint64_t value;
  memcpy(&value, data, sizeof(value));
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  value = __builtin_bswap64(value);
#endif
  return value;
}

static inline uint64_t hash_expand_seven(uint64_t packed) {
  packed &= UINT64_C(0x00ffffffffffffff);
  packed = (packed & UINT64_C(0xffffffff)) | ((packed >> 32) << 36);
  uint64_t moved = packed & UINT64_C(0x0ff00000ffff0000);
  packed = (packed & UINT64_C(0x000ffff00000ffff)) | (moved << 2);
  moved = packed & UINT64_C(0x000ff003fc00ff00);
  return (packed & ~UINT64_C(0x000ff003fc00ff00)) | (moved << 1) |
         UINT64_C(0x4020100804020100);
}

static inline uint64_t hash_update_data(uint64_t hash, const uint8_t *data, size_t bytes) {
  const size_t row_bytes = bytes;
  while (bytes >= 7) {
    bytes -= 7;
    uint64_t symbols;
    // The eight-byte load must stay inside the active row, including its first group.
    if (bytes) symbols = hash_expand_seven(hash_load_le64(data + bytes - 1) >> 8);
    else if (row_bytes >= 8) symbols = hash_expand_seven(hash_load_le64(data));
    else symbols = hash_pack_seven(data);
    hash = gf32_shift(hash, 63) ^ symbols;
  }
  while (bytes) hash = gf32_shift(hash, 9) ^ UINT64_C(0x100) ^ data[--bytes];
  return hash;
}

static inline uint64_t hash_advance_unwritten(uint64_t hash, size_t bytes) {
  while (bytes >= 7) { hash = gf32_shift(hash, 63); bytes -= 7; }
  while (bytes--) hash = gf32_shift(hash, 9);
  return hash;
}

#endif
#endif
