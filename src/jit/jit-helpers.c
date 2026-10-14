/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

// Helpers called from translated code. Their results match the
// interpreter's instruction helpers bit for bit.

#include <cpu/cpu.h>
#include <memory/vaddr.h>
#include "jit-internal.h"

sljit_sw SLJIT_FUNC jit_helper_load(Decode *d, sljit_uw addr, sljit_s32 width_sign) {
  int width = width_sign & 0xff;
  word_t v = vaddr_read(d, addr, width, MMU_TRANSLATE);
  if (width_sign & 0x100) {
    switch (width) {
      case 1: return (int8_t)v;
      case 2: return (int16_t)v;
      case 4: return (int32_t)v;
      default: return v;
    }
  }
  return v;
}

void SLJIT_FUNC jit_helper_store(Decode *d, sljit_uw addr, sljit_uw data, sljit_s32 width) {
  vaddr_write(d, addr, width, data, MMU_TRANSLATE);
}

#define SEXT32(x) ((sljit_sw)(int32_t)(x))

sljit_sw SLJIT_FUNC jit_helper_div(sljit_sw a, sljit_sw b) {
  if (b == 0) return -1;
  if (a == INT64_MIN && b == -1) return a;
  return a / b;
}

sljit_sw SLJIT_FUNC jit_helper_divu(sljit_sw a, sljit_sw b) {
  if (b == 0) return -1;
  return (sljit_uw)a / (sljit_uw)b;
}

sljit_sw SLJIT_FUNC jit_helper_rem(sljit_sw a, sljit_sw b) {
  if (b == 0) return a;
  if (a == INT64_MIN && b == -1) return 0;
  return a % b;
}

sljit_sw SLJIT_FUNC jit_helper_remu(sljit_sw a, sljit_sw b) {
  if (b == 0) return a;
  return (sljit_uw)a % (sljit_uw)b;
}

sljit_sw SLJIT_FUNC jit_helper_divw(sljit_sw a, sljit_sw b) {
  int32_t x = a, y = b;
  if (y == 0) return -1;
  if (x == INT32_MIN && y == -1) return SEXT32(x);
  return SEXT32(x / y);
}

sljit_sw SLJIT_FUNC jit_helper_divuw(sljit_sw a, sljit_sw b) {
  uint32_t x = a, y = b;
  if (y == 0) return -1;
  return SEXT32(x / y);
}

sljit_sw SLJIT_FUNC jit_helper_remw(sljit_sw a, sljit_sw b) {
  int32_t x = a, y = b;
  if (y == 0) return SEXT32(x);
  if (x == INT32_MIN && y == -1) return 0;
  return SEXT32(x % y);
}

sljit_sw SLJIT_FUNC jit_helper_remuw(sljit_sw a, sljit_sw b) {
  uint32_t x = a, y = b;
  if (y == 0) return SEXT32(x);
  return SEXT32(x % y);
}

sljit_sw SLJIT_FUNC jit_helper_mulh(sljit_sw a, sljit_sw b) {
  return ((__int128_t)a * (__int128_t)b) >> 64;
}

sljit_sw SLJIT_FUNC jit_helper_mulhu(sljit_sw a, sljit_sw b) {
  return ((__uint128_t)(sljit_uw)a * (__uint128_t)(sljit_uw)b) >> 64;
}

sljit_sw SLJIT_FUNC jit_helper_mulhsu(sljit_sw a, sljit_sw b) {
  sljit_uw hi = ((__uint128_t)(sljit_uw)a * (__uint128_t)(sljit_uw)b) >> 64;
  return hi - (a < 0 ? (sljit_uw)b : 0);
}

#ifdef CONFIG_JIT_TIER2
// Loads and stores of the second tier. An exception is caught before any
// trap state changes and reported instead: the region leaves before the
// access, and the interpreter executes it again and takes the trap. Neither
// reads nor writes guest registers.
bool SLJIT_FUNC jit_t2_load(Decode *d, sljit_uw addr, sljit_s32 width_sign, sljit_uw *out) {
  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) { pop_context(); return false; }
  *out = jit_helper_load(d, addr, width_sign);
  pop_context();
  return true;
}

bool SLJIT_FUNC jit_t2_store(Decode *d, sljit_uw addr, sljit_uw data, sljit_s32 width) {
  int cause;
  PUSH_CONTEXT(&cause);
  if (cause) { pop_context(); return false; }
  jit_helper_store(d, addr, data, width);
  pop_context();
  return true;
}
#endif
