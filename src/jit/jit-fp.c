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

// Direct helpers for scalar floating-point arithmetic called from translated
// code. The common case is computed with the host FPU under the same
// conditions as the interpreter's host fast path; every other case runs the
// interpreter's own instruction helper, so results are identical.

#include <isa-all-instr.h>
#include <string.h>
#include "jit-internal.h"

#if defined(CONFIG_FPU_SOFT_HOST_FASTPATH) && !defined(CONFIG_FPU_NONE)

bool isa_fp_host_ok(Decode *s);
void fp_set_dirty(void);

#define F64_SIGN (1ull << 63)
#define F32_SIGN (1u << 31)
#define BOX32 0xffffffff00000000ull

static inline bool f64_ok(uint64_t v) {
  uint32_t e = (v >> 52) & 0x7ff;
  return e >= 1023 - 900 && e <= 1023 + 900;
}
static inline bool f64_ok_or_zero(uint64_t v) { return (v << 1) == 0 || f64_ok(v); }
static inline bool f32_ok(uint32_t v) {
  uint32_t e = (v >> 23) & 0xff;
  return e >= 127 - 100 && e <= 127 + 100;
}
static inline bool f32_ok_or_zero(uint32_t v) { return (v << 1) == 0 || f32_ok(v); }
static inline bool boxed(uint64_t v) { return (v >> 32) == 0xffffffffu; }

static inline double d_of(uint64_t v) { double d; memcpy(&d, &v, 8); return d; }
static inline uint64_t bits_d(double d) { uint64_t v; memcpy(&v, &d, 8); return v; }
static inline float f_of(uint32_t v) { float f; memcpy(&f, &v, 4); return f; }
static inline uint32_t bits_f(float f) { uint32_t v; memcpy(&v, &f, 4); return v; }

static inline uint64_t rs3_of(Decode *d) { return cpu.fpr[d->isa.instr.fp.funct5]._64; }

bool isa_fp_underflow_set(void);
bool fpcall_host_relaxed(uint32_t op, uint32_t w, rtlreg_t *dest,
                         const rtlreg_t *src1, const rtlreg_t *src2);

// Second chance before the interpreter's helper: with inexact and underflow
// both set, any finite operation on finite operands matches softfloat.
static inline bool relaxed(Decode *d, uint32_t op, uint32_t w, uint64_t a, uint64_t b,
                           uint64_t c, uint64_t *r) {
  if (!isa_fp_host_ok(d) || !isa_fp_underflow_set()) return false;
  rtlreg_t x = a, y = b, z = c;
  if (!fpcall_host_relaxed(op, w, &z, &x, &y)) return false;
  *r = z;
  return true;
}

#define FALLBACK(name) do { jit_eh_lookup(concat(EXEC_ID_, name))(d); return; } while (0)
#define WRITE_D(r) do { *d->dest.preg = (r); fp_set_dirty(); return; } while (0)
#define WRITE_F(r) do { *d->dest.preg = BOX32 | (r); fp_set_dirty(); return; } while (0)

// Sums of window operands (or zeros) are exact or normal and cannot
// overflow, so `sum` skips the result check.
#define DEF_D_BIN(name, expr, okfn, sum, op)                                    \
  void SLJIT_FUNC concat(jit_fp_, name)(Decode *d) {                           \
    uint64_t a = *d->src1.preg, b = *d->src2.preg;                             \
    if (isa_fp_host_ok(d) && okfn(a) && okfn(b)) {                             \
      uint64_t r = bits_d(expr);                                               \
      if ((sum) || f64_ok(r)) WRITE_D(r);                                      \
    }                                                                          \
    uint64_t rr;                                                               \
    if (relaxed(d, op, FPCALL_W64, a, b, 0, &rr)) WRITE_D(rr);                 \
    FALLBACK(name);                                                            \
  }

DEF_D_BIN(faddd, d_of(a) + d_of(b), f64_ok_or_zero, true, FPCALL_ADD)
DEF_D_BIN(fsubd, d_of(a) - d_of(b), f64_ok_or_zero, true, FPCALL_SUB)
DEF_D_BIN(fdivd, d_of(a) / d_of(b), f64_ok, false, FPCALL_DIV)

// A product with a zero factor and a finite other factor is an exact
// signed zero, which raises no flag.
void SLJIT_FUNC jit_fp_fmuld(Decode *d) {
  uint64_t a = *d->src1.preg, b = *d->src2.preg;
  if (isa_fp_host_ok(d) && f64_ok_or_zero(a) && f64_ok_or_zero(b)) {
    uint64_t r = bits_d(d_of(a) * d_of(b));
    if (f64_ok(r) || (a << 1) == 0 || (b << 1) == 0) WRITE_D(r);
  }
  uint64_t rr;
  if (relaxed(d, FPCALL_MUL, FPCALL_W64, a, b, 0, &rr)) WRITE_D(rr);
  FALLBACK(fmuld);
}

void SLJIT_FUNC jit_fp_fsqrtd(Decode *d) {
  uint64_t a = *d->src1.preg;
  if (isa_fp_host_ok(d) && f64_ok(a) && !(a >> 63)) {
    uint64_t r = bits_d(__builtin_sqrt(d_of(a)));
    if (f64_ok(r)) WRITE_D(r);
  }
  FALLBACK(fsqrtd);
}

// fmadd/fmsub/fnmsub/fnmadd: (+-a) * b + (+-c)
#define DEF_D_FMA(name, na, nc)                                                \
  void SLJIT_FUNC concat(jit_fp_, name)(Decode *d) {                           \
    uint64_t a = *d->src1.preg ^ ((na) ? F64_SIGN : 0);                        \
    uint64_t b = *d->src2.preg;                                                \
    uint64_t c = rs3_of(d) ^ ((nc) ? F64_SIGN : 0);                            \
    if (isa_fp_host_ok(d) && f64_ok_or_zero(a) && f64_ok_or_zero(b) &&         \
        f64_ok_or_zero(c)) {                                                   \
      uint64_t r = bits_d(__builtin_fma(d_of(a), d_of(b), d_of(c)));           \
      /* A zero factor makes the result exactly c (or a signed zero). */       \
      bool exact = (a << 1) == 0 || (b << 1) == 0;                             \
      if (f64_ok(r) || exact) WRITE_D(r);                                      \
    }                                                                          \
    uint64_t rr;                                                               \
    if (relaxed(d, FPCALL_MADD, FPCALL_W64, a, b, c, &rr)) WRITE_D(rr);        \
    FALLBACK(name);                                                            \
  }

DEF_D_FMA(fmaddd, 0, 0)
DEF_D_FMA(fmsubd, 0, 1)
DEF_D_FMA(fnmsubd, 1, 0)
DEF_D_FMA(fnmaddd, 1, 1)

// Integer to double conversions: 32-bit sources are always exact; 64-bit
// sources are exact below 2^53 in magnitude.
void SLJIT_FUNC jit_fp_fcvt_d_w(Decode *d) {
  WRITE_D(bits_d((double)(int32_t)*d->src1.preg));
}
void SLJIT_FUNC jit_fp_fcvt_d_wu(Decode *d) {
  WRITE_D(bits_d((double)(uint32_t)*d->src1.preg));
}
void SLJIT_FUNC jit_fp_fcvt_d_l(Decode *d) {
  int64_t x = *d->src1.preg;
  if (x > -(1ll << 53) && x < (1ll << 53)) WRITE_D(bits_d((double)x));
  FALLBACK(fcvt_d_l);
}
void SLJIT_FUNC jit_fp_fcvt_d_lu(Decode *d) {
  uint64_t x = *d->src1.preg;
  if (x < (1ull << 53)) WRITE_D(bits_d((double)x));
  FALLBACK(fcvt_d_lu);
}

// Double to integer with rounding toward zero (the C conversion) for values
// well inside the destination range. Inexact is only raised when the value
// has a fraction; it must already be set.
void fp_set_dirty(void);
bool isa_fp_inexact_set(void);
uint32_t isa_fp_get_rm(Decode *s);

static inline bool rtz_ok(Decode *d, double v, double lim) {
  return isa_fp_get_rm(d) == FPCALL_RM_RTZ && v > -lim && v < lim &&
         (isa_fp_inexact_set() || v == (double)(int64_t)v);
}
void SLJIT_FUNC jit_fp_fcvt_w_d(Decode *d) {
  double v = d_of(*d->src1.preg);
  if (rtz_ok(d, v, 2147483648.0)) {
    if (d->dest.preg != NULL) *d->dest.preg = (int64_t)(int32_t)v;
    fp_set_dirty();
    return;
  }
  FALLBACK(fcvt_w_d);
}
void SLJIT_FUNC jit_fp_fcvt_l_d(Decode *d) {
  double v = d_of(*d->src1.preg);
  if (rtz_ok(d, v, 9223372036854775808.0)) {
    *d->dest.preg = (int64_t)v;
    fp_set_dirty();
    return;
  }
  FALLBACK(fcvt_l_d);
}

// Quiet comparisons of non-NaN operands raise no flag.
static inline bool not_nan64(uint64_t v) { return ((v >> 52) & 0x7ff) != 0x7ff; }
#define DEF_D_CMP(name, op)                                                    \
  void SLJIT_FUNC concat(jit_fp_, name)(Decode *d) {                           \
    uint64_t a = *d->src1.preg, b = *d->src2.preg;                             \
    if (not_nan64(a) && not_nan64(b)) {                                        \
      *d->dest.preg = d_of(a) op d_of(b);                                      \
      fp_set_dirty();                                                          \
      return;                                                                  \
    }                                                                          \
    FALLBACK(name);                                                            \
  }
DEF_D_CMP(fled, <=)
DEF_D_CMP(fltd, <)
DEF_D_CMP(feqd, ==)

#define DEF_F_BIN(name, expr, okfn, sum)                                       \
  void SLJIT_FUNC concat(jit_fp_, name)(Decode *d) {                           \
    uint64_t A = *d->src1.preg, B = *d->src2.preg;                             \
    uint32_t a = A, b = B;                                                     \
    if (boxed(A) && boxed(B) && isa_fp_host_ok(d) && okfn(a) && okfn(b)) {     \
      uint32_t r = bits_f(expr);                                               \
      if ((sum) || f32_ok(r)) WRITE_F(r);                                      \
    }                                                                          \
    FALLBACK(name);                                                            \
  }

DEF_F_BIN(fadds, f_of(a) + f_of(b), f32_ok_or_zero, true)
DEF_F_BIN(fsubs, f_of(a) - f_of(b), f32_ok_or_zero, true)
DEF_F_BIN(fmuls, f_of(a) * f_of(b), f32_ok, false)
DEF_F_BIN(fdivs, f_of(a) / f_of(b), f32_ok, false)

void SLJIT_FUNC jit_fp_fsqrts(Decode *d) {
  uint64_t A = *d->src1.preg;
  uint32_t a = A;
  if (boxed(A) && isa_fp_host_ok(d) && f32_ok(a) && !(a >> 31)) {
    uint32_t r = bits_f(__builtin_sqrtf(f_of(a)));
    if (f32_ok(r)) WRITE_F(r);
  }
  FALLBACK(fsqrts);
}

#define DEF_F_FMA(name, na, nc)                                                \
  void SLJIT_FUNC concat(jit_fp_, name)(Decode *d) {                           \
    uint64_t A = *d->src1.preg, B = *d->src2.preg, C = rs3_of(d);              \
    uint32_t a = (uint32_t)A ^ ((na) ? F32_SIGN : 0);                          \
    uint32_t b = B;                                                            \
    uint32_t c = (uint32_t)C ^ ((nc) ? F32_SIGN : 0);                          \
    if (boxed(A) && boxed(B) && boxed(C) && isa_fp_host_ok(d) &&               \
        f32_ok(a) && f32_ok(b) && f32_ok_or_zero(c)) {                         \
      uint32_t r = bits_f(__builtin_fmaf(f_of(a), f_of(b), f_of(c)));          \
      if (f32_ok(r)) WRITE_F(r);                                               \
    }                                                                          \
    FALLBACK(name);                                                            \
  }

DEF_F_FMA(fmadds, 0, 0)
DEF_F_FMA(fmsubs, 0, 1)
DEF_F_FMA(fnmsubs, 1, 0)
DEF_F_FMA(fnmadds, 1, 1)

#define JIT_FP_ENTRY(name) [concat(EXEC_ID_, name)] = concat(jit_fp_, name),
static void (SLJIT_FUNC *const fp_table[TOTAL_INSTR])(Decode *) = {
  JIT_FP_ENTRY(faddd) JIT_FP_ENTRY(fsubd) JIT_FP_ENTRY(fmuld) JIT_FP_ENTRY(fdivd)
  JIT_FP_ENTRY(fsqrtd) JIT_FP_ENTRY(fmaddd) JIT_FP_ENTRY(fmsubd)
  JIT_FP_ENTRY(fnmsubd) JIT_FP_ENTRY(fnmaddd)
  JIT_FP_ENTRY(fadds) JIT_FP_ENTRY(fsubs) JIT_FP_ENTRY(fmuls) JIT_FP_ENTRY(fdivs)
  JIT_FP_ENTRY(fsqrts) JIT_FP_ENTRY(fmadds) JIT_FP_ENTRY(fmsubs)
  JIT_FP_ENTRY(fnmsubs) JIT_FP_ENTRY(fnmadds)
  JIT_FP_ENTRY(fcvt_d_w) JIT_FP_ENTRY(fcvt_d_wu) JIT_FP_ENTRY(fcvt_d_l) JIT_FP_ENTRY(fcvt_d_lu)
  JIT_FP_ENTRY(fcvt_w_d) JIT_FP_ENTRY(fcvt_l_d)
  JIT_FP_ENTRY(fled) JIT_FP_ENTRY(fltd) JIT_FP_ENTRY(feqd)
};

void (SLJIT_FUNC *jit_fp_lookup(int exec_id))(Decode *) {
  return fp_table[exec_id];
}

#else

void (SLJIT_FUNC *jit_fp_lookup(int exec_id))(Decode *) {
  return NULL;
}

#endif
