/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
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

#include <rtl/rtl.h>
#include <string.h>
#ifndef CONFIG_FPU_NONE
#include MUXDEF(CONFIG_FPU_SOFT, "softfloat-fp.h", "host-fp.h")

#define BOX_MASK_FP16 0xFFFFFFFFFFFF0000
#define BOX_MASK_FP32 0xFFFFFFFF00000000

static inline rtlreg_t unboxf16(rtlreg_t r) {
  return MUXDEF(CONFIG_FPU_SOFT, (r & BOX_MASK_FP16) == BOX_MASK_FP16, true)
    ? (r & ~BOX_MASK_FP16) : defaultNaNF16UI;
}

static inline rtlreg_t unboxbf16(rtlreg_t r) {
  return MUXDEF(CONFIG_FPU_SOFT, (r & BOX_MASK_FP16) == BOX_MASK_FP16, true)
    ? (r & ~BOX_MASK_FP16) : defaultNaNBF16UI;
}

static inline rtlreg_t unboxf32(rtlreg_t r) {
  return MUXDEF(CONFIG_FPU_SOFT, (r & BOX_MASK_FP32) == BOX_MASK_FP32, true)
    ? (r & ~BOX_MASK_FP32) : defaultNaNF32UI;
}

static inline float16_t rtlToF16(rtlreg_t r) {
  float16_t f = { .v = (uint16_t)unboxf16(r) };
  return f;
}

static inline float16_t rtlToVF16(rtlreg_t r) {
  float16_t f = { .v = r };
  return f;
}

static inline bfloat16_t rtlToBF16(rtlreg_t r) {
  bfloat16_t f = { .v = (uint16_t)unboxbf16(r) };
  return f;
}

static inline bfloat16_t rtlToVBF16(rtlreg_t r) {
  bfloat16_t f = { .v = r };
  return f;
}

static inline float32_t rtlToVF32(rtlreg_t r) {
  float32_t f = { .v = (uint32_t)r };
  return f;
}

static inline float32_t rtlToF32(rtlreg_t r) {
  float32_t f = { .v = (uint32_t)unboxf32(r) };
  return f;
}

static inline float64_t rtlToF64(rtlreg_t r) {
  float64_t f = { .v = r };
  return f;
}

uint32_t isa_fp_get_rm(Decode *s);
void isa_fp_set_ex(uint32_t ex);
void isa_fp_csr_check();
void isa_fp_rm_check(uint32_t rm);
uint32_t isa_fp_get_frm();
#endif // CONFIG_FPU_NONE

#ifdef CONFIG_FPU_SOFT_HOST_FASTPATH
bool isa_fp_inexact_set(void);
bool isa_fp_underflow_set(void);
bool fpcall_host_relaxed(uint32_t op, uint32_t w, rtlreg_t *dest,
                         const rtlreg_t *src1, const rtlreg_t *src2);

// Operands and results whose magnitude lies in [2^-lo, 2^hi) cannot raise
// underflow or overflow in the operations below, and their exact rounding
// errors are representable, so host IEEE arithmetic rounds exactly like
// softfloat. Zero is allowed as an operand but not as a result.
bool fp_host_fastpath_enabled = true;

static inline bool f64_ok(uint64_t v) {
  uint32_t e = (v >> 52) & 0x7ff;
  return e >= 1023 - 900 && e <= 1023 + 900;
}
static inline bool f64_ok_or_zero(uint64_t v) {
  return (v << 1) == 0 || f64_ok(v);
}
static inline bool f32_ok(uint32_t v) {
  uint32_t e = (v >> 23) & 0xff;
  return e >= 127 - 100 && e <= 127 + 100;
}
static inline bool f32_ok_or_zero(uint32_t v) {
  return (v << 1) == 0 || f32_ok(v);
}

static inline double as_f64(uint64_t v) { double d; memcpy(&d, &v, 8); return d; }
static inline uint64_t f64_bits(double d) { uint64_t v; memcpy(&v, &d, 8); return v; }
static inline float as_f32(uint32_t v) { float f; memcpy(&f, &v, 4); return f; }
static inline uint32_t f32_bits(float f) { uint32_t v; memcpy(&v, &f, 4); return v; }

// Returns true when the result was produced here. Only reached with round to
// nearest even and with the inexact flag already set, so the only flag the
// operation could raise is already present.
static bool fpcall_host_fastpath(uint32_t op, uint32_t w, rtlreg_t *dest,
                                 const rtlreg_t *src1, const rtlreg_t *src2) {
  if (w == FPCALL_W64) {
    uint64_t a = *src1, b = *src2, c = *dest, r;
    switch (op) {
      case FPCALL_ADD:
        if (!f64_ok_or_zero(a) || !f64_ok_or_zero(b)) return false;
        r = f64_bits(as_f64(a) + as_f64(b)); break;
      case FPCALL_SUB:
        if (!f64_ok_or_zero(a) || !f64_ok_or_zero(b)) return false;
        r = f64_bits(as_f64(a) - as_f64(b)); break;
      case FPCALL_MUL:
        if (!f64_ok(a) || !f64_ok(b)) return false;
        r = f64_bits(as_f64(a) * as_f64(b)); break;
      case FPCALL_DIV:
        if (!f64_ok(a) || !f64_ok(b)) return false;
        r = f64_bits(as_f64(a) / as_f64(b)); break;
      case FPCALL_SQRT:
        if (!f64_ok(a) || (a >> 63)) return false;
        r = f64_bits(__builtin_sqrt(as_f64(a))); break;
      case FPCALL_MADD:
        if (!f64_ok(a) || !f64_ok(b) || !f64_ok_or_zero(c)) return false;
        r = f64_bits(__builtin_fma(as_f64(a), as_f64(b), as_f64(c))); break;
      default: return false;
    }
    // Sums of window operands (or zeros) are exact or normal and cannot
    // overflow; other results must stay inside the window.
    if (op != FPCALL_ADD && op != FPCALL_SUB && !f64_ok(r)) return false;
    *dest = r;
    return true;
  }
  if (w == FPCALL_W32) {
    // Operands must be properly NaN-boxed; anything else takes softfloat.
    if ((*src1 >> 32) != 0xffffffffu) return false;
    uint32_t a = *src1, b = *src2, c = *dest, r;
    switch (op) {
      case FPCALL_ADD: case FPCALL_SUB: case FPCALL_MUL: case FPCALL_DIV: case FPCALL_MADD:
        if ((*src2 >> 32) != 0xffffffffu) return false;
        break;
    }
    switch (op) {
      case FPCALL_ADD:
        if (!f32_ok_or_zero(a) || !f32_ok_or_zero(b)) return false;
        r = f32_bits(as_f32(a) + as_f32(b)); break;
      case FPCALL_SUB:
        if (!f32_ok_or_zero(a) || !f32_ok_or_zero(b)) return false;
        r = f32_bits(as_f32(a) - as_f32(b)); break;
      case FPCALL_MUL:
        if (!f32_ok(a) || !f32_ok(b)) return false;
        r = f32_bits(as_f32(a) * as_f32(b)); break;
      case FPCALL_DIV:
        if (!f32_ok(a) || !f32_ok(b)) return false;
        r = f32_bits(as_f32(a) / as_f32(b)); break;
      case FPCALL_SQRT:
        if (!f32_ok(a) || (a >> 31)) return false;
        r = f32_bits(__builtin_sqrtf(as_f32(a))); break;
      case FPCALL_MADD:
        if ((*dest >> 32) != 0xffffffffu) return false;
        if (!f32_ok(a) || !f32_ok(b) || !f32_ok_or_zero(c)) return false;
        r = f32_bits(__builtin_fmaf(as_f32(a), as_f32(b), as_f32(c))); break;
      default: return false;
    }
    if (op != FPCALL_ADD && op != FPCALL_SUB && !f32_ok(r)) return false;
    *dest = r;
    return true;
  }
  return false;
}

// When inexact and underflow are both already set, host arithmetic on finite
// operands matches softfloat for every finite result: subnormal results are
// rounded correctly by the host, and the tininess rule only decides the
// underflow flag. Overflow, invalid operations and division by zero are
// excluded by requiring finite operands and results and a non-zero divisor.
static inline bool fin64(uint64_t v) { return ((v >> 52) & 0x7ff) != 0x7ff; }
static inline bool fin32(uint32_t v) { return ((v >> 23) & 0xff) != 0xff; }

bool fpcall_host_relaxed(uint32_t op, uint32_t w, rtlreg_t *dest,
                         const rtlreg_t *src1, const rtlreg_t *src2) {
  if (w == FPCALL_W64) {
    uint64_t a = *src1, b = *src2, c = *dest, r;
    switch (op) {
      case FPCALL_ADD: if (!fin64(a) || !fin64(b)) return false; r = f64_bits(as_f64(a) + as_f64(b)); break;
      case FPCALL_SUB: if (!fin64(a) || !fin64(b)) return false; r = f64_bits(as_f64(a) - as_f64(b)); break;
      case FPCALL_MUL: if (!fin64(a) || !fin64(b)) return false; r = f64_bits(as_f64(a) * as_f64(b)); break;
      case FPCALL_DIV:
        if (!fin64(a) || !fin64(b) || (b << 1) == 0) return false;
        r = f64_bits(as_f64(a) / as_f64(b)); break;
      case FPCALL_SQRT:
        if (!fin64(a) || ((a >> 63) && (a << 1) != 0)) return false;
        r = f64_bits(__builtin_sqrt(as_f64(a))); break;
      case FPCALL_MADD:
        if (!fin64(a) || !fin64(b) || !fin64(c)) return false;
        r = f64_bits(__builtin_fma(as_f64(a), as_f64(b), as_f64(c))); break;
      default: return false;
    }
    if (!fin64(r)) return false;
    *dest = r;
    return true;
  }
  if (w == FPCALL_W32) {
    if ((*src1 >> 32) != 0xffffffffu) return false;
    if (op != FPCALL_SQRT && (*src2 >> 32) != 0xffffffffu) return false;
    if (op == FPCALL_MADD && (*dest >> 32) != 0xffffffffu) return false;
    uint32_t a = *src1, b = *src2, c = *dest, r;
    switch (op) {
      case FPCALL_ADD: if (!fin32(a) || !fin32(b)) return false; r = f32_bits(as_f32(a) + as_f32(b)); break;
      case FPCALL_SUB: if (!fin32(a) || !fin32(b)) return false; r = f32_bits(as_f32(a) - as_f32(b)); break;
      case FPCALL_MUL: if (!fin32(a) || !fin32(b)) return false; r = f32_bits(as_f32(a) * as_f32(b)); break;
      case FPCALL_DIV:
        if (!fin32(a) || !fin32(b) || (b << 1) == 0) return false;
        r = f32_bits(as_f32(a) / as_f32(b)); break;
      case FPCALL_SQRT:
        if (!fin32(a) || ((a >> 31) && (a << 1) != 0)) return false;
        r = f32_bits(__builtin_sqrtf(as_f32(a))); break;
      case FPCALL_MADD:
        if (!fin32(a) || !fin32(b) || !fin32(c)) return false;
        r = f32_bits(__builtin_fmaf(as_f32(a), as_f32(b), as_f32(c))); break;
      default: return false;
    }
    if (!fin32(r)) return false;
    *dest = 0xffffffff00000000ull | r;
    return true;
  }
  return false;
}

// Compare the fast path with softfloat on random operands biased toward the
// boundaries of the fast-path window. Returns the number of mismatches.
static uint64_t rng_state = 0x243f6a8885a308d3ull;
static uint64_t rng_next(void) {
  rng_state ^= rng_state << 13; rng_state ^= rng_state >> 7; rng_state ^= rng_state << 17;
  return rng_state;
}

static uint64_t random_f64(void) {
  uint64_t r = rng_next();
  uint64_t sign = r & (1ull << 63);
  uint64_t frac = rng_next() & ((1ull << 52) - 1);
  uint64_t e;
  switch (r & 7) {
    case 0: e = 0; break;                                    // zero or subnormal
    case 1: e = 1023 - 900 + (int64_t)(r >> 8) % 3 - 1; break; // lower window edge
    case 2: e = 1023 + 900 + (int64_t)(r >> 8) % 3 - 1; break; // upper window edge
    case 3: e = 0x7ff; break;                                // inf or NaN
    case 4: frac &= ~((1ull << 30) - 1); /* short mantissa */ /* fall through */
    default: e = 1023 + (int64_t)((r >> 8) % 120) - 60; break;
  }
  return sign | (e << 52) | frac;
}

static uint32_t random_f32(void) {
  uint64_t r = rng_next();
  uint32_t sign = r & (1u << 31);
  uint32_t frac = rng_next() & ((1u << 23) - 1);
  uint32_t e;
  switch (r & 7) {
    case 0: e = 0; break;
    case 1: e = 127 - 100 + (int)((r >> 8) % 3) - 1; break;
    case 2: e = 127 + 100 + (int)((r >> 8) % 3) - 1; break;
    case 3: e = 0xff; break;
    case 4: frac &= ~((1u << 12) - 1); /* fall through */
    default: e = 127 + (int)((r >> 8) % 60) - 30; break;
  }
  return sign | (e << 23) | frac;
}

uint64_t fp_fastpath_selftest(uint64_t n) {
  static const uint32_t ops[] = { FPCALL_ADD, FPCALL_SUB, FPCALL_MUL, FPCALL_DIV, FPCALL_SQRT, FPCALL_MADD };
  uint64_t bad = 0, fast = 0;
  softfloat_roundingMode = softfloat_round_near_even;
  for (uint64_t i = 0; i < n; i++) {
    bool w64 = rng_next() & 1;
    uint32_t op = ops[rng_next() % ARRLEN(ops)];
    uint32_t w = w64 ? FPCALL_W64 : FPCALL_W32;
    rtlreg_t a, b, c;
    if (w64) { a = random_f64(); b = random_f64(); c = random_f64(); }
    else {
      a = 0xffffffff00000000ull | random_f32();
      b = 0xffffffff00000000ull | random_f32();
      c = 0xffffffff00000000ull | random_f32();
    }
    rtlreg_t fast_r = c;
    bool relaxed = false;
    if (!fpcall_host_fastpath(op, w, &fast_r, &a, &b)) {
      fast_r = c;
      if (!fpcall_host_relaxed(op, w, &fast_r, &a, &b)) continue;
      relaxed = true;
      if (!w64) fast_r = (uint32_t)fast_r;
    }
    fast++;
    softfloat_exceptionFlags = 0;
    rtlreg_t ref;
    if (w64) {
      float64_t fa = { a }, fb = { b }, fc = { c };
      switch (op) {
        case FPCALL_ADD: ref = f64_add(fa, fb).v; break;
        case FPCALL_SUB: ref = f64_sub(fa, fb).v; break;
        case FPCALL_MUL: ref = f64_mul(fa, fb).v; break;
        case FPCALL_DIV: ref = f64_div(fa, fb).v; break;
        case FPCALL_SQRT: ref = f64_sqrt(fa).v; break;
        default: ref = f64_mulAdd(fa, fb, fc).v; break;
      }
    } else {
      float32_t fa = { (uint32_t)a }, fb = { (uint32_t)b }, fc = { (uint32_t)c };
      switch (op) {
        case FPCALL_ADD: ref = f32_add(fa, fb).v; break;
        case FPCALL_SUB: ref = f32_sub(fa, fb).v; break;
        case FPCALL_MUL: ref = f32_mul(fa, fb).v; break;
        case FPCALL_DIV: ref = f32_div(fa, fb).v; break;
        case FPCALL_SQRT: ref = f32_sqrt(fa).v; break;
        default: ref = f32_mulAdd(fa, fb, fc).v; break;
      }
    }
    // The fast path may only omit flags that are already set: inexact, and
    // underflow for the relaxed path.
    uint32_t allowed = softfloat_flag_inexact | (relaxed ? softfloat_flag_underflow : 0);
    bool flags_ok = (softfloat_exceptionFlags & ~allowed) == 0;
    if (ref != fast_r || !flags_ok) {
      if (bad < 10) {
        printf("fp fast path mismatch: op %u w %u a %016lx b %016lx c %016lx fast %016lx ref %016lx flags %x\n",
               op, w, a, b, c, fast_r, ref, softfloat_exceptionFlags);
      }
      bad++;
    }
  }
  softfloat_exceptionFlags = 0;
  printf("fp fast path self-test: %lu cases, %lu on the fast path, %lu mismatches\n", n, fast, bad);
  return bad;
}
#endif // CONFIG_FPU_SOFT_HOST_FASTPATH

def_rtl(fpcall, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2, uint32_t cmd) {
#ifndef CONFIG_FPU_NONE
  uint32_t w = FPCALL_W(cmd);
  uint32_t op = FPCALL_OP(cmd);
  isa_fp_csr_check();
  if (op < FPCALL_NEED_RM) {
    uint32_t rm = isa_fp_get_rm(s);
    softfloat_roundingMode = rm;
    isa_fp_rm_check(softfloat_roundingMode);
#ifdef CONFIG_FPU_SOFT_HOST_FASTPATH
    if (fp_host_fastpath_enabled && rm == FPCALL_RM_RNE && isa_fp_inexact_set()) {
      if (fpcall_host_fastpath(op, w, dest, src1, src2)) return;
      if (isa_fp_underflow_set() && fpcall_host_relaxed(op, w, dest, src1, src2)) return;
    }
#endif
  }
  if (w == FPCALL_W16) {
    float16_t fsrc1 = rtlToF16(*src1);
    float16_t fsrc2 = rtlToF16(*src2);
    switch(op){
      case FPCALL_ADD: *dest = f16_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f16_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f16_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f16_div(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f16_min(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f16_max(fsrc1, fsrc2).v; break;

      case FPCALL_FLI:  *dest = f16_fli(*src1).v; break;
      case FPCALL_MINM: *dest = f16_minm(fsrc1, fsrc2).v; break;
      case FPCALL_MAXM: *dest = f16_maxm(fsrc1, fsrc2).v; break;
      case FPCALL_FROUND:   *dest = f16_roundToInt(fsrc1, softfloat_roundingMode, false).v; break;
      case FPCALL_FROUNDNX: *dest = f16_roundToInt(fsrc1, softfloat_roundingMode,  true).v; break;
      case FPCALL_FLEQ: *dest = f16_le_quiet(fsrc1, fsrc2); break;
      case FPCALL_FLTQ: *dest = f16_lt_quiet(fsrc1, fsrc2); break; 

      case FPCALL_SQRT: *dest = f16_sqrt(fsrc1).v; break;
      case FPCALL_MADD: *dest = f16_mulAdd(fsrc1, fsrc2, rtlToF16(*dest)).v; break;

      case FPCALL_LE: *dest = f16_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f16_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f16_eq(fsrc1, fsrc2); break;

      case FPCALL_SGNJ:  *dest = fsgnj16(fsrc1, fsrc2, false, false); break;
      case FPCALL_SGNJN: *dest = fsgnj16(fsrc1, fsrc2, true, false); break;
      case FPCALL_SGNJX: *dest = fsgnj16(fsrc1, fsrc2, false, true); break;

      case FPCALL_I32ToF: *dest = my_i32_to_f16 (*src1).v; break;
      case FPCALL_U32ToF: *dest = my_ui32_to_f16(*src1).v; break;
      case FPCALL_I64ToF: *dest = my_i64_to_f16 (*src1).v; break;
      case FPCALL_U64ToF: *dest = my_ui64_to_f16(*src1).v; break;

      case FPCALL_FToI32: *dest = my_f16_to_i32 (fsrc1); break;
      case FPCALL_FToU32: *dest = my_f16_to_ui32(fsrc1); break;
      case FPCALL_FToI64: *dest = my_f16_to_i64 (fsrc1); break;
      case FPCALL_FToU64: *dest = my_f16_to_ui64(fsrc1); break;

      case FPCALL_F16ToF32: *dest = my_f16_to_f32(rtlToF16(*src1)).v; break;
      case FPCALL_F16ToF64: *dest = my_f16_to_f64(rtlToF16(*src1)).v; break;
      default: panic("op = %d not supported", op);
    }
  } else if (w == FPCALL_BF16) {
    bfloat16_t fsrc1 = rtlToBF16(*src1);
    switch(op) {
      case FPCALL_BF16ToF32: *dest = my_bf16_to_f32(fsrc1).v; break;
      default: panic("op = %d not supported for BF16 width", op);
    }
  } else if (w == FPCALL_W32) {
    float32_t fsrc1 = rtlToF32(*src1);
    float32_t fsrc2 = rtlToF32(*src2);
    switch (op) {
      case FPCALL_ADD: *dest = f32_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f32_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f32_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f32_div(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f32_min(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f32_max(fsrc1, fsrc2).v; break;

      case FPCALL_FLI:  *dest = f32_fli(*src1).v; break;
      case FPCALL_MINM: *dest = f32_minm(fsrc1, fsrc2).v; break;
      case FPCALL_MAXM: *dest = f32_maxm(fsrc1, fsrc2).v; break;
      case FPCALL_FROUND:   *dest = f32_roundToInt(fsrc1, softfloat_roundingMode, false).v; break;
      case FPCALL_FROUNDNX: *dest = f32_roundToInt(fsrc1, softfloat_roundingMode,  true).v; break;
      case FPCALL_FLEQ: *dest = f32_le_quiet(fsrc1, fsrc2); break;
      case FPCALL_FLTQ: *dest = f32_lt_quiet(fsrc1, fsrc2); break;

      case FPCALL_SQRT: *dest = f32_sqrt(fsrc1).v; break;

      case FPCALL_MADD: *dest = f32_mulAdd(fsrc1, fsrc2, rtlToF32(*dest)).v; break;

      case FPCALL_LE: *dest = f32_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f32_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f32_eq(fsrc1, fsrc2); break;

      case FPCALL_I32ToF: *dest = i32_to_f32 (*src1).v; break;
      case FPCALL_U32ToF: *dest = ui32_to_f32(*src1).v; break;
      case FPCALL_I64ToF: *dest = i64_to_f32 (*src1).v; break;
      case FPCALL_U64ToF: *dest = ui64_to_f32(*src1).v; break;

      case FPCALL_FToI32: *dest = my_f32_to_i32 (fsrc1); break;
      case FPCALL_FToU32: *dest = my_f32_to_ui32(fsrc1); break;
      case FPCALL_FToI64: *dest = my_f32_to_i64 (fsrc1); break;
      case FPCALL_FToU64: *dest = my_f32_to_ui64(fsrc1); break;

      case FPCALL_F32ToF16: *dest = my_f32_to_f16(rtlToF32(*src1)).v; break;
      case FPCALL_F32ToBF16: *dest = my_f32_to_bf16(fsrc1).v; break;
      default: panic("op = %d not supported", op);
    }
  } else if (w == FPCALL_W64) {
    float64_t fsrc1 = rtlToF64(*src1);
    float64_t fsrc2 = rtlToF64(*src2);
    switch (op) {
      case FPCALL_ADD: *dest = f64_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f64_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f64_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f64_div(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f64_max(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f64_min(fsrc1, fsrc2).v; break;

      case FPCALL_FLI:  *dest = f64_fli(*src1).v; break;
      case FPCALL_MINM: *dest = f64_minm(fsrc1, fsrc2).v; break;
      case FPCALL_MAXM: *dest = f64_maxm(fsrc1, fsrc2).v; break;
      case FPCALL_FROUND:   *dest = f64_roundToInt(fsrc1, softfloat_roundingMode, false).v; break;
      case FPCALL_FROUNDNX: *dest = f64_roundToInt(fsrc1, softfloat_roundingMode,  true).v; break;
      case FPCALL_FLEQ: *dest = f64_le_quiet(fsrc1, fsrc2); break;
      case FPCALL_FLTQ: *dest = f64_lt_quiet(fsrc1, fsrc2); break;
      case FPCALL_FCVTMOD: *dest = f64_fcvtmod(fsrc1); break;

      case FPCALL_SQRT: *dest = f64_sqrt(fsrc1).v; break;

      case FPCALL_MADD: *dest = f64_mulAdd(fsrc1, fsrc2, rtlToF64(*dest)).v; break;

      case FPCALL_LE: *dest = f64_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f64_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f64_eq(fsrc1, fsrc2); break;

      case FPCALL_I32ToF: *dest = i32_to_f64 (*src1).v; break;
      case FPCALL_U32ToF: *dest = ui32_to_f64(*src1).v; break;
      case FPCALL_I64ToF: *dest = i64_to_f64 (*src1).v; break;
      case FPCALL_U64ToF: *dest = ui64_to_f64(*src1).v; break;

      case FPCALL_FToI32: *dest = my_f64_to_i32 (fsrc1); break;
      case FPCALL_FToU32: *dest = my_f64_to_ui32(fsrc1); break;
      case FPCALL_FToI64: *dest = my_f64_to_i64 (fsrc1); break;
      case FPCALL_FToU64: *dest = my_f64_to_ui64(fsrc1); break;

      case FPCALL_F32ToF64: *dest = f32_to_f64(rtlToF32(*src1)).v; break;
      case FPCALL_F64ToF32: *dest = f64_to_f32(fsrc1).v; break;

      case FPCALL_F64ToF16: *dest = my_f64_to_f16(fsrc1).v; break;
      default: panic("op = %d not supported", op);
    }
  }

  uint32_t ex = fp_get_exception();
  if (ex) {
    isa_fp_set_ex(ex);
    fp_clear_exception();
  }
#endif // CONFIG_FPU_NONE
}

def_rtl(vfpcall, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2, uint32_t cmd) {
#ifndef CONFIG_FPU_NONE
  uint32_t w = FPCALL_W(cmd);
  uint32_t op = FPCALL_OP(cmd);
  isa_fp_csr_check();
  softfloat_roundingMode = isa_fp_get_frm();

  if (w == FPCALL_W8) {
    // w8 only can hold int/uint
    switch (op) {
      case FPCALL_SToDF: *dest = i32_to_f16((int32_t)(int8_t)*src1).v; break;
      case FPCALL_UToDF: *dest = ui32_to_f16(*src1).v; break;
    }
  } else if (w == FPCALL_W16) {
    float16_t fsrc1 = rtlToVF16(*src1);
    float16_t fsrc2 = rtlToVF16(*src2);
    switch (op) {
      case FPCALL_ADD: *dest = f16_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f16_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f16_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f16_div(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f16_min(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f16_max(fsrc1, fsrc2).v; break;

      case FPCALL_SQRT:   *dest = f16_sqrt(fsrc1).v; break;
      case FPCALL_RSQRT7: *dest = f16_rsqrte7(fsrc1).v; break;
      case FPCALL_REC7:   *dest = f16_recip7(fsrc1).v; break;
      case FPCALL_CLASS:  *dest = f16_classify(fsrc1); break;

      case FPCALL_MADD:  *dest = f16_mulAdd(rtlToVF16(*dest), fsrc1, fsrc2).v; break;
      case FPCALL_NMADD: *dest = f16_mulAdd(f16_neg(rtlToVF16(*dest)), fsrc1, f16_neg(fsrc2)).v; break;
      case FPCALL_MSUB:  *dest = f16_mulAdd(rtlToVF16(*dest), fsrc1, f16_neg(fsrc2)).v; break;
      case FPCALL_NMSUB: *dest = f16_mulAdd(f16_neg(rtlToVF16(*dest)), fsrc1, fsrc2).v; break;
      case FPCALL_MACC:  *dest = f16_mulAdd(fsrc1, fsrc2, rtlToVF16(*dest)).v; break;
      case FPCALL_NMACC: *dest = f16_mulAdd(f16_neg(fsrc2), fsrc1, f16_neg(rtlToVF16(*dest))).v; break;
      case FPCALL_MSAC:  *dest = f16_mulAdd(fsrc1, fsrc2, f16_neg(rtlToVF16(*dest))).v; break;
      case FPCALL_NMSAC: *dest = f16_mulAdd(f16_neg(fsrc1), fsrc2, rtlToVF16(*dest)).v; break;
      
      case FPCALL_LE: *dest = f16_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f16_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f16_eq(fsrc1, fsrc2); break;
      case FPCALL_GE: *dest = f16_le(fsrc2, fsrc1); break;
      case FPCALL_GT: *dest = f16_lt(fsrc2, fsrc1); break;
      case FPCALL_NE: *dest = !f16_eq(fsrc1, fsrc2); break;

      case FPCALL_SGNJ:  *dest = fsgnj16(fsrc1, fsrc2, false, false); break;
      case FPCALL_SGNJN: *dest = fsgnj16(fsrc1, fsrc2, true, false); break;
      case FPCALL_SGNJX: *dest = fsgnj16(fsrc1, fsrc2, false, true); break;

      case FPCALL_FToU:  *dest = f16_to_ui16(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToS:  *dest = f16_to_i16(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToUT: *dest = f16_to_ui16(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_FToST: *dest = f16_to_i16(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_UToF:  *dest = ui32_to_f16(*src1).v; break;
      case FPCALL_SToF:  *dest = i32_to_f16(*src1).v; break;

      case FPCALL_FToDU:  *dest = f16_to_ui32(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToDS:  *dest = f16_to_i32(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToDUT: *dest = f16_to_ui32(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_FToDST: *dest = f16_to_i32(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_UToDF:  *dest = ui32_to_f32(*src1).v; break;
      case FPCALL_SToDF:  *dest = i32_to_f32((int32_t)(int16_t)*src1).v; break;
      case FPCALL_FToDF:  *dest = f16_to_f32(fsrc1).v; break;

      case FPCALL_DFToU:  *dest = f16_to_ui8(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_DFToS:  *dest = f16_to_i8(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_DFToUT: *dest = f16_to_ui8(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_DFToST: *dest = f16_to_i8(fsrc1, softfloat_round_minMag, true); break;

      case FPCALL_GenNegZero: *dest = f16_neg_zero().v; break;

      default: panic("op = %d not supported", op);
    }
  } else if (w == FPCALL_BF16) {
    bfloat16_t fsrc1 = rtlToVBF16(*src1);
    switch (op) {
      case FPCALL_BF16ToF32: *dest = bf16_to_f32(fsrc1).v; break;
      default: panic("op = %d not supported", op);
    }
  } else if (w == FPCALL_W32 || w == FPCALL_W16_to_32 || w == FPCALL_SRC1_W16_to_32 || w == FPCALL_SRC2_W16_to_32) {
    float32_t fsrc1;
    float32_t fsrc2;
    if (w == FPCALL_W32) {
      fsrc1 = rtlToVF32(*src1);
      fsrc2 = rtlToVF32(*src2);
    } else if (w == FPCALL_SRC1_W16_to_32) {
      fsrc1 = f16_to_f32(rtlToVF16(*src1));
      fsrc2 = rtlToVF32(*src2);
    } else if (w == FPCALL_SRC2_W16_to_32) {
      fsrc1 = rtlToVF32(*src1);
      fsrc2 = f16_to_f32(rtlToVF16(*src2));
    } else {
      fsrc1 = f16_to_f32(rtlToVF16(*src1));
      fsrc2 = f16_to_f32(rtlToVF16(*src2));
    }

    switch (op) {
      case FPCALL_ADD: *dest = f32_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f32_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f32_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f32_div(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f32_min(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f32_max(fsrc1, fsrc2).v; break;

      case FPCALL_SQRT:   *dest = f32_sqrt(fsrc1).v; break;
      case FPCALL_RSQRT7: *dest = f32_rsqrte7(fsrc1).v; break;
      case FPCALL_REC7:   *dest = f32_recip7(fsrc1).v; break;
      case FPCALL_CLASS:  *dest = f32_classify(fsrc1); break;

      case FPCALL_MADD:  *dest = f32_mulAdd(rtlToVF32(*dest), fsrc1, fsrc2).v; break;
      case FPCALL_NMADD: *dest = f32_mulAdd(f32_neg(rtlToVF32(*dest)), fsrc1, f32_neg(fsrc2)).v; break;
      case FPCALL_MSUB:  *dest = f32_mulAdd(rtlToVF32(*dest), fsrc1, f32_neg(fsrc2)).v; break;
      case FPCALL_NMSUB: *dest = f32_mulAdd(f32_neg(rtlToVF32(*dest)), fsrc1, fsrc2).v; break;
      case FPCALL_MACC:  *dest = f32_mulAdd(fsrc1, fsrc2, rtlToVF32(*dest)).v; break;
      case FPCALL_NMACC: *dest = f32_mulAdd(f32_neg(fsrc2), fsrc1, f32_neg(rtlToVF32(*dest))).v; break;
      case FPCALL_MSAC:  *dest = f32_mulAdd(fsrc1, fsrc2, f32_neg(rtlToVF32(*dest))).v; break;
      case FPCALL_NMSAC: *dest = f32_mulAdd(f32_neg(fsrc1), fsrc2, rtlToVF32(*dest)).v; break;

      case FPCALL_LE: *dest = f32_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f32_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f32_eq(fsrc1, fsrc2); break;
      case FPCALL_GE: *dest = f32_le(fsrc2, fsrc1); break;
      case FPCALL_GT: *dest = f32_lt(fsrc2, fsrc1); break;
      case FPCALL_NE: *dest = !f32_eq(fsrc1, fsrc2); break;

      case FPCALL_SGNJ:  *dest = fsgnj32(fsrc1, fsrc2, false, false); break;
      case FPCALL_SGNJN: *dest = fsgnj32(fsrc1, fsrc2, true, false); break;
      case FPCALL_SGNJX: *dest = fsgnj32(fsrc1, fsrc2, false, true); break;

      case FPCALL_FToU:  *dest = f32_to_ui32(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToS:  *dest = f32_to_i32(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToUT: *dest = f32_to_ui32(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_FToST: *dest = f32_to_i32(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_UToF:  *dest = ui32_to_f32(*src1).v; break;
      case FPCALL_SToF:  *dest = i32_to_f32(*src1).v; break;

      case FPCALL_FToDU:  *dest = f32_to_ui64(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToDS:  *dest = f32_to_i64(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToDUT: *dest = f32_to_ui64(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_FToDST: *dest = f32_to_i64(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_UToDF:  *dest = ui32_to_f64(*src1).v; break;
      case FPCALL_SToDF:  *dest = i32_to_f64(*src1).v; break;
      case FPCALL_FToDF:  *dest = f32_to_f64(fsrc1).v; break;

      case FPCALL_DFToU:  *dest = f32_to_ui16(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_DFToS:  *dest = f32_to_i16(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_DFToUT: *dest = f32_to_ui16(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_DFToST: *dest = f32_to_i16(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_DUToF:  *dest = ui32_to_f16(*src1).v; break;
      case FPCALL_DSToF:  *dest = i32_to_f16(*src1).v; break;
      case FPCALL_DFToF:  *dest = f32_to_f16(fsrc1).v; break;
      case FPCALL_F32ToBF16: *dest = f32_to_bf16(fsrc1).v; break;
      case FPCALL_DFToF_ODD:
        softfloat_roundingMode = softfloat_round_odd;
        *dest = f32_to_f16(fsrc1).v;
        break;
      case FPCALL_GenNegZero: *dest = f32_neg_zero().v; break;

      default: panic("op = %d not supported", op);
    }
  } else if (w == FPCALL_W64 || w == FPCALL_W32_to_64 || w == FPCALL_SRC1_W32_to_64 || w == FPCALL_SRC2_W32_to_64) {
    float64_t fsrc1;
    float64_t fsrc2;
    if (w == FPCALL_W64) {
      fsrc1 = rtlToF64(*src1);
      fsrc2 = rtlToF64(*src2);
    } else if (w == FPCALL_SRC1_W32_to_64) {
      fsrc1 = f32_to_f64(rtlToVF32(*src1));
      fsrc2 = rtlToF64(*src2);
    } else if (w == FPCALL_SRC2_W32_to_64) {
      fsrc1 = rtlToF64(*src1);
      fsrc2 = f32_to_f64(rtlToVF32(*src2));
    } else {
      fsrc1 = f32_to_f64(rtlToVF32(*src1));
      fsrc2 = f32_to_f64(rtlToVF32(*src2));
    }

    switch (op) {
      case FPCALL_ADD: *dest = f64_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f64_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f64_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f64_div(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f64_max(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f64_min(fsrc1, fsrc2).v; break;

      case FPCALL_SQRT:   *dest = f64_sqrt(fsrc1).v; break;
      case FPCALL_RSQRT7: *dest = f64_rsqrte7(fsrc1).v; break;
      case FPCALL_REC7:   *dest = f64_recip7(fsrc1).v; break;
      case FPCALL_CLASS:  *dest = f64_classify(fsrc1); break;

      case FPCALL_MADD:  *dest = f64_mulAdd(rtlToF64(*dest), fsrc1, fsrc2).v; break;
      case FPCALL_NMADD: *dest = f64_mulAdd(f64_neg(rtlToF64(*dest)), fsrc1, f64_neg(fsrc2)).v; break;
      case FPCALL_MSUB:  *dest = f64_mulAdd(rtlToF64(*dest), fsrc1, f64_neg(fsrc2)).v; break;
      case FPCALL_NMSUB: *dest = f64_mulAdd(f64_neg(rtlToF64(*dest)), fsrc1, fsrc2).v; break;
      case FPCALL_MACC:  *dest = f64_mulAdd(fsrc1, fsrc2, rtlToF64(*dest)).v; break;
      case FPCALL_NMACC: *dest = f64_mulAdd(f64_neg(fsrc2), fsrc1, f64_neg(rtlToF64(*dest))).v; break;
      case FPCALL_MSAC:  *dest = f64_mulAdd(fsrc1, fsrc2, f64_neg(rtlToF64(*dest))).v; break;
      case FPCALL_NMSAC: *dest = f64_mulAdd(f64_neg(fsrc1), fsrc2, rtlToF64(*dest)).v; break;

      case FPCALL_LE: *dest = f64_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f64_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f64_eq(fsrc1, fsrc2); break;
      case FPCALL_GE: *dest = f64_le(fsrc2, fsrc1); break;
      case FPCALL_GT: *dest = f64_lt(fsrc2, fsrc1); break;
      case FPCALL_NE: *dest = !f64_eq(fsrc1, fsrc2); break;

      case FPCALL_SGNJ:  *dest = fsgnj64(fsrc1, fsrc2, false, false); break;
      case FPCALL_SGNJN: *dest = fsgnj64(fsrc1, fsrc2, true, false); break;
      case FPCALL_SGNJX: *dest = fsgnj64(fsrc1, fsrc2, false, true); break;

      case FPCALL_FToU:  *dest = f64_to_ui64(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToS:  *dest = f64_to_i64(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToUT: *dest = f64_to_ui64(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_FToST: *dest = f64_to_i64(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_UToF:  *dest = ui64_to_f64(*src1).v; break;
      case FPCALL_SToF:  *dest = i64_to_f64(*src1).v; break;

      case FPCALL_DFToU:  *dest = f64_to_ui32(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_DFToS:  *dest = f64_to_i32(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_DFToUT: *dest = f64_to_ui32(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_DFToST: *dest = f64_to_i32(fsrc1, softfloat_round_minMag, true); break;
      case FPCALL_DUToF:  *dest = ui64_to_f32(*src1).v; break;
      case FPCALL_DSToF:  *dest = i64_to_f32(*src1).v; break;
      case FPCALL_DFToF:  *dest = f64_to_f32(fsrc1).v; break;
      case FPCALL_DFToF_ODD:
        softfloat_roundingMode = softfloat_round_odd;
        *dest = f64_to_f32(fsrc1).v;
        break;
      case FPCALL_GenNegZero: *dest = f64_neg_zero().v; break;

      default: panic("op = %d not supported", op);
    } 
  }

  uint32_t ex = fp_get_exception();
  if (ex) {
    isa_fp_set_ex(ex);
    fp_clear_exception();
  }
#endif // CONFIG_FPU_NONE
}

// fpcall for matrix extension
def_rtl(mfpcall, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2, uint32_t cmd) {
#ifndef CONFIG_FPU_NONE
  uint32_t w = FPCALL_W(cmd);
  uint32_t op = FPCALL_OP(cmd);
  
  if (w == FPCALL_W16) {
    float16_t fsrc1 = rtlToVF16(*src1);
    float16_t fsrc2 = rtlToVF16(*src2);
    switch(op){
      case FPCALL_ADD: *dest = f16_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f16_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f16_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f16_div(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f16_min(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f16_max(fsrc1, fsrc2).v; break;

      case FPCALL_FLI:  *dest = f16_fli(*src1).v; break;
      case FPCALL_MINM: *dest = f16_minm(fsrc1, fsrc2).v; break;
      case FPCALL_MAXM: *dest = f16_maxm(fsrc1, fsrc2).v; break;
      case FPCALL_FROUND:   *dest = f16_roundToInt(fsrc1, softfloat_roundingMode, false).v; break;
      case FPCALL_FROUNDNX: *dest = f16_roundToInt(fsrc1, softfloat_roundingMode,  true).v; break;
      case FPCALL_FLEQ: *dest = f16_le_quiet(fsrc1, fsrc2); break;
      case FPCALL_FLTQ: *dest = f16_lt_quiet(fsrc1, fsrc2); break; 

      case FPCALL_SQRT: *dest = f16_sqrt(fsrc1).v; break;
      case FPCALL_MADD: *dest = f16_mulAdd(fsrc1, fsrc2, rtlToVF16(*dest)).v; break;

      case FPCALL_LE: *dest = f16_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f16_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f16_eq(fsrc1, fsrc2); break;

      case FPCALL_I32ToF: *dest = my_i32_to_f16 (*src1).v; break;
      case FPCALL_U32ToF: *dest = my_ui32_to_f16(*src1).v; break;
      case FPCALL_I64ToF: *dest = my_i64_to_f16 (*src1).v; break;
      case FPCALL_U64ToF: *dest = my_ui64_to_f16(*src1).v; break;

      case FPCALL_FToI32: *dest = my_f16_to_i32 (fsrc1); break;
      case FPCALL_FToU32: *dest = my_f16_to_ui32(fsrc1); break;
      case FPCALL_FToI64: *dest = my_f16_to_i64 (fsrc1); break;
      case FPCALL_FToU64: *dest = my_f16_to_ui64(fsrc1); break;

      case FPCALL_F16ToF32: *dest = my_f16_to_f32(rtlToF16(*src1)).v; break;
      case FPCALL_F16ToF64: *dest = my_f16_to_f64(rtlToF16(*src1)).v; break;
      default: panic("op = %d not supported", op);
    }
  } else if (w == FPCALL_BF16) {
    bfloat16_t fsrc1 = rtlToVBF16(*src1);
    bfloat16_t fsrc2 = rtlToVBF16(*src2);
    switch (op) {
      /* Matrix registers hold raw BF16 lanes, unlike boxed scalar operands. */
      case FPCALL_MADD:
        *dest = f32_mulAdd(my_bf16_to_f32(fsrc1), my_bf16_to_f32(fsrc2),
                           rtlToVF32(*dest)).v;
        break;
      default:
        panic("op = %d not supported for BF16 matrix operands", op);
    }
  } else if (w == FPCALL_W32 || w == FPCALL_W16_to_32) {
    float32_t fsrc1;
    float32_t fsrc2;
    if (w == FPCALL_W16_to_32) {
      fsrc1 = f16_to_f32(rtlToVF16(*src1));
      fsrc2 = f16_to_f32(rtlToVF16(*src2));
    } else {
      fsrc1 = rtlToVF32(*src1);
      fsrc2 = rtlToVF32(*src2);
    }
    switch (op) {
      case FPCALL_ADD: *dest = f32_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f32_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f32_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f32_div(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f32_min(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f32_max(fsrc1, fsrc2).v; break;

      case FPCALL_FLI:  *dest = f32_fli(*src1).v; break;
      case FPCALL_MINM: *dest = f32_minm(fsrc1, fsrc2).v; break;
      case FPCALL_MAXM: *dest = f32_maxm(fsrc1, fsrc2).v; break;
      case FPCALL_FROUND:   *dest = f32_roundToInt(fsrc1, softfloat_roundingMode, false).v; break;
      case FPCALL_FROUNDNX: *dest = f32_roundToInt(fsrc1, softfloat_roundingMode,  true).v; break;
      case FPCALL_FLEQ: *dest = f32_le_quiet(fsrc1, fsrc2); break;
      case FPCALL_FLTQ: *dest = f32_lt_quiet(fsrc1, fsrc2); break;

      case FPCALL_SQRT: *dest = f32_sqrt(fsrc1).v; break;

      case FPCALL_MADD: *dest = f32_mulAdd(fsrc1, fsrc2, rtlToVF32(*dest)).v; break;

      case FPCALL_LE: *dest = f32_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f32_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f32_eq(fsrc1, fsrc2); break;

      case FPCALL_I32ToF: *dest = i32_to_f32 (*src1).v; break;
      case FPCALL_U32ToF: *dest = ui32_to_f32(*src1).v; break;
      case FPCALL_I64ToF: *dest = i64_to_f32 (*src1).v; break;
      case FPCALL_U64ToF: *dest = ui64_to_f32(*src1).v; break;

      case FPCALL_FToI32: *dest = my_f32_to_i32 (fsrc1); break;
      case FPCALL_FToU32: *dest = my_f32_to_ui32(fsrc1); break;
      case FPCALL_FToI64: *dest = my_f32_to_i64 (fsrc1); break;
      case FPCALL_FToU64: *dest = my_f32_to_ui64(fsrc1); break;

      case FPCALL_F32ToF16: *dest = my_f32_to_f16(rtlToF32(*src1)).v; break;
      default: panic("op = %d not supported", op);
    }
  } else if (w == FPCALL_W64 || w == FPCALL_W32_to_64) {
    float64_t fsrc1;
    float64_t fsrc2;
    if (w == FPCALL_W32_to_64) {
      fsrc1 = f32_to_f64(rtlToVF32(*src1));
      fsrc2 = f32_to_f64(rtlToVF32(*src2));
    } else {
      fsrc1 = rtlToF64(*src1);
      fsrc2 = rtlToF64(*src2);
    }
    switch (op) {
      case FPCALL_ADD: *dest = f64_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f64_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f64_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f64_div(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f64_max(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f64_min(fsrc1, fsrc2).v; break;

      case FPCALL_FLI:  *dest = f64_fli(*src1).v; break;
      case FPCALL_MINM: *dest = f64_minm(fsrc1, fsrc2).v; break;
      case FPCALL_MAXM: *dest = f64_maxm(fsrc1, fsrc2).v; break;
      case FPCALL_FROUND:   *dest = f64_roundToInt(fsrc1, softfloat_roundingMode, false).v; break;
      case FPCALL_FROUNDNX: *dest = f64_roundToInt(fsrc1, softfloat_roundingMode,  true).v; break;
      case FPCALL_FLEQ: *dest = f64_le_quiet(fsrc1, fsrc2); break;
      case FPCALL_FLTQ: *dest = f64_lt_quiet(fsrc1, fsrc2); break;
      case FPCALL_FCVTMOD: *dest = f64_fcvtmod(fsrc1); break;

      case FPCALL_SQRT: *dest = f64_sqrt(fsrc1).v; break;

      case FPCALL_MADD: *dest = f64_mulAdd(fsrc1, fsrc2, rtlToF64(*dest)).v; break;

      case FPCALL_LE: *dest = f64_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f64_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f64_eq(fsrc1, fsrc2); break;

      case FPCALL_I32ToF: *dest = i32_to_f64 (*src1).v; break;
      case FPCALL_U32ToF: *dest = ui32_to_f64(*src1).v; break;
      case FPCALL_I64ToF: *dest = i64_to_f64 (*src1).v; break;
      case FPCALL_U64ToF: *dest = ui64_to_f64(*src1).v; break;

      case FPCALL_FToI32: *dest = my_f64_to_i32 (fsrc1); break;
      case FPCALL_FToU32: *dest = my_f64_to_ui32(fsrc1); break;
      case FPCALL_FToI64: *dest = my_f64_to_i64 (fsrc1); break;
      case FPCALL_FToU64: *dest = my_f64_to_ui64(fsrc1); break;

      case FPCALL_F32ToF64: *dest = f32_to_f64(rtlToF32(*src1)).v; break;
      case FPCALL_F64ToF32: *dest = f64_to_f32(fsrc1).v; break;

      case FPCALL_F64ToF16: *dest = my_f64_to_f16(fsrc1).v; break;
      default: panic("op = %d not supported", op);
    }
  }
#endif // CONFIG_FPU_NONE
}

def_rtl(fclass, rtlreg_t *fdest, rtlreg_t *src, int width) {
#ifndef CONFIG_FPU_NONE
  if(width == FPCALL_W16){
    *fdest = f16_classify(rtlToF16(*src));
  }else if (width == FPCALL_W32) {
    *fdest = f32_classify(rtlToF32(*src));
  } else if (width == FPCALL_W64) {
    *fdest = f64_classify(rtlToF64(*src));
  } else assert(0);
#endif // CONFIG_FPU_NONE
}
