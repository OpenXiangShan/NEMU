#ifndef __HOSTFP_H__
#include <math.h>
#include <fenv.h>

#define defaultNaNF32UI 0x7FC00000
#define I32MAX 0x7fffffff
#define I32MIN 0x80000000
#define U32MAX 0xffffffff
#define I64MAX 0x7fffffffffffffffll
#define I64MIN 0x8000000000000000ll
#define U64MAX 0xffffffffffffffffull

static inline void fp_set_exception(int excep);
static inline void fp_clear_exception();

typedef union { uint32_t v; float  f; } float32_t;
static inline float32_t float32(float f) { float32_t r = { .f = f }; return r; }

static inline float32_t f32_add(float32_t a, float32_t b) { return float32(a.f + b.f); }
static inline float32_t f32_sub(float32_t a, float32_t b) { return float32(a.f - b.f); }
static inline float32_t f32_mul(float32_t a, float32_t b) { return float32(a.f * b.f); }
static inline float32_t f32_div(float32_t a, float32_t b) { return float32(a.f / b.f); }
static inline float32_t f32_sqrt(float32_t a) { return float32(sqrtf(a.f)); }
static inline float32_t f32_mulAdd(float32_t a, float32_t b,
    float32_t c) { return float32(fmaf(a.f, b.f, c.f)); }
static inline float32_t f32_min(float32_t a, float32_t b) { return float32(a.f < b.f ? a.f : b.f); }
static inline float32_t f32_max(float32_t a, float32_t b) { return float32(a.f > b.f ? a.f : b.f); }
static inline bool f32_le(float32_t a, float32_t b) { return a.f <= b.f; }
static inline bool f32_lt(float32_t a, float32_t b) { return a.f <  b.f; }
static inline bool f32_eq(float32_t a, float32_t b) { return a.f == b.f; }
static inline float32_t i32_to_f32 (rtlreg_t a) { return float32((int32_t)a); }
static inline float32_t ui32_to_f32(rtlreg_t a) { return float32((uint32_t)a); }
static inline float32_t i64_to_f32 (uint64_t a) { return float32((int64_t)a); }
static inline float32_t ui64_to_f32(uint64_t a) { return float32((uint64_t)a); }

static inline int32_t check_f32_i32(bool* succ, float32_t a){
  int sign = a.v >> 31;
  int exp = BITS(a.v, 30, 23);
  int mant = BITS(a.v, 22, 0);
  if(exp == 0 && mant == 0) {
    *succ = 1;
    return llrintf(a.f);
  }
  exp -= 127;
  if(exp > 31 || (exp == 31 && (!sign || mant))){
    fp_set_exception(FE_INVALID);
    *succ = 1;
    return sign ? I32MIN : I32MAX;
  }
  return 0;
}

static inline uint32_t check_f32_ui32(bool* succ, float32_t a){
  int sign = a.v >> 31;
  int exp = BITS(a.v, 30, 23);
  int mant = BITS(a.v, 22, 0);
  if(exp == 0 && mant == 0){
    *succ = 1;
    return llrintf(a.f);
  }
  exp -= 127;
  if(sign && (exp >= 0 || (mant >> exp) != 0)){
    fp_set_exception(FE_INVALID);
    *succ = 1;
    return 0;
  }
  if(!sign && exp >= 32){
    *succ = 1;
    fp_set_exception(FE_INVALID);
    return U32MAX;
  }
  return 0;
}

static inline int64_t check_f32_i64(bool* succ, float32_t a){
  int sign = a.v >> 31;
  int exp = BITS(a.v, 30, 23);
  int mant = BITS(a.v, 22, 0);
  if(exp == 0 && mant == 0) {
    *succ = 1;
    return llrintf(a.f);
  }
  exp -= 127;
  if(exp > 63 || (exp == 63 && (!sign || mant))){
    fp_set_exception(FE_INVALID);
    *succ = 1;
    return sign ? I64MIN : I64MAX;
  }
  return 0;
}

static inline uint64_t check_f32_ui64(bool* succ, float32_t a){
  int sign = a.v >> 31;
  int exp = BITS(a.v, 30, 23);
  int mant = BITS(a.v, 22, 0);
  if(exp == 0 && mant == 0){
    *succ = 1;
    return llrintf(a.f);
  }
  exp -= 127;
  if(sign && (exp >= 0 || (mant >> exp) != 0)){
    fp_set_exception(FE_INVALID);
    *succ = 1;
    return 0;
  }
  if(!sign && exp >= 64){
    *succ = 1;
    fp_set_exception(FE_INVALID);
    return U64MAX;
  }
  if(exp == 63){
    *succ = 1;
    a.v = (189 << 23) | mant;
    uint64_t ret = (uint64_t)llrintf(a.f) << 1;
    fp_clear_exception();
    return ret;
  }
  return 0;
}

static inline int32_t my_f32_to_i32 (float32_t a) {
  bool succ = 0;
  int32_t ret = check_f32_i32(&succ, a);
  if(succ) return ret;
  return llrintf(a.f);
}

static inline uint32_t my_f32_to_ui32(float32_t a) {
  bool succ = 0;
  uint32_t ret = check_f32_ui32(&succ, a);
  if(succ) return ret;
  return llrintf(a.f);
}

static inline int64_t my_f32_to_i64 (float32_t a) {
  bool succ = 0;
  int64_t ret = check_f32_i64(&succ, a);
  if(succ) return ret;
  return llrintf(a.f);
}

static inline uint64_t my_f32_to_ui64(float32_t a) {
  bool succ = 0;
  uint64_t ret = check_f32_ui64(&succ, a);
  if(succ) return ret;
  return llrintf(a.f);
}

static inline int32_t  my_f32_to_i32_rmm (float32_t a) {
  bool succ = 0;
  int32_t ret = check_f32_i32(&succ, a);
  if(succ) return ret;
  return llroundf(a.f);
}

static inline uint32_t my_f32_to_ui32_rmm(float32_t a) {
  bool succ = 0;
  uint32_t ret = check_f32_ui32(&succ, a);
  if(succ) return ret;
  return llroundf(a.f);
}

static inline int64_t  my_f32_to_i64_rmm (float32_t a) {
  bool succ = 0;
  int64_t ret = check_f32_i64(&succ, a);
  if(succ) return ret;
  return llroundf(a.f);
}
static inline uint64_t my_f32_to_ui64_rmm(float32_t a) {
  bool succ = 0;
  uint64_t ret = check_f32_ui64(&succ, a);
  if(succ) return ret;
  return llroundf(a.f);
}


typedef union { uint64_t v; double f; } float64_t;
static inline float64_t float64(double f) { float64_t r = { .f = f }; return r; }

static inline float64_t f64_add(float64_t a, float64_t b) { return float64(a.f + b.f); }
static inline float64_t f64_sub(float64_t a, float64_t b) { return float64(a.f - b.f); }
static inline float64_t f64_mul(float64_t a, float64_t b) { return float64(a.f * b.f); }
static inline float64_t f64_div(float64_t a, float64_t b) { return float64(a.f / b.f); }
static inline float64_t f64_sqrt(float64_t a) { return float64(sqrt(a.f)); }
static inline float64_t f64_mulAdd(float64_t a, float64_t b,
    float64_t c) { return float64(fma(a.f, b.f, c.f)); }
static inline float64_t f64_min(float64_t a, float64_t b) { return float64(a.f < b.f ? a.f : b.f); }
static inline float64_t f64_max(float64_t a, float64_t b) { return float64(a.f > b.f ? a.f : b.f); }
static inline bool f64_le(float64_t a, float64_t b) { return a.f <= b.f; }
static inline bool f64_lt(float64_t a, float64_t b) { return a.f <  b.f; }
static inline bool f64_eq(float64_t a, float64_t b) { return a.f == b.f; }
static inline float64_t i32_to_f64 (rtlreg_t a) { return float64((int32_t)a); }
static inline float64_t ui32_to_f64(rtlreg_t a) { return float64((uint32_t)a); }
static inline float64_t i64_to_f64 (uint64_t a) { return float64((int64_t)a); }
static inline float64_t ui64_to_f64(uint64_t a) { float64_t ret = float64(a); if(a == 0) ret.v=0; return ret; }

static inline int32_t check_f64_i32(bool* succ, float64_t a){
  int64_t sign = a.v >> 63;
  int64_t exp = BITS(a.v, 62, 52);
  int64_t mant = BITS(a.v, 51, 0);
  if(exp == 0 && mant == 0) {
    *succ = 1;
    return llrint(a.f);
  }
  exp -= 1023;
  if(exp > 31 || (exp == 31 && (!sign || (mant >> 21)))){
    fp_set_exception(FE_INVALID);
    *succ = 1;
    return sign ? I32MIN : I32MAX;
  }
  return 0;
}

static inline uint32_t check_f64_ui32(bool* succ, float64_t a){
  int64_t sign = a.v >> 63;
  int64_t exp = BITS(a.v, 62, 52);
  int64_t mant = BITS(a.v, 51, 0);
  if(exp == 0 && mant == 0){
    *succ = 1;
    return llrint(a.f);
  }
  exp -= 1023;
  if(sign && (exp >= 0 || (mant >> exp) != 0)){
    fp_set_exception(FE_INVALID);
    *succ = 1;
    return 0;
  }
  if(!sign && exp >= 32){
    *succ = 1;
    fp_set_exception(FE_INVALID);
    return U32MAX;
  }
  return 0;
}

static inline int64_t check_f64_i64(bool* succ, float64_t a){
  int64_t sign = a.v >> 63;
  int64_t exp = BITS(a.v, 62, 52);
  int64_t mant = BITS(a.v, 51, 0);
  if(exp == 0 && mant == 0) {
    *succ = 1;
    return llrint(a.f);
  }
  exp -= 1023;
  if(exp > 63 || (exp == 63 && (!sign || mant))){
    fp_set_exception(FE_INVALID);
    *succ = 1;
    return sign ? I64MIN : I64MAX;
  }
  return 0;
}

static inline uint64_t check_f64_ui64(bool* succ, float64_t a){
  int64_t sign = a.v >> 63;
  int64_t exp = BITS(a.v, 62, 52);
  int64_t mant = BITS(a.v, 51, 0);
  if(exp == 0 && mant == 0){
    *succ = 1;
    return llrint(a.f);
  }
  exp -= 1023;
  if(sign && (exp >= 0 || (mant >> exp) != 0)){
    fp_set_exception(FE_INVALID);
    *succ = 1;
    return 0;
  }
  if(!sign && exp >= 64){
    *succ = 1;
    fp_set_exception(FE_INVALID);
    return U64MAX;
  }
  if(exp == 63){  // out of long long bound
    *succ = 1;
    a.v = ((uint64_t)1085 << 52) | mant;
    uint64_t ret = (uint64_t)llrint(a.f) << 1;
    fp_clear_exception();
    return ret;
  }
  return 0;
}

static inline int32_t  my_f64_to_i32 (float64_t a) {
  bool succ = 0;
  int32_t ret = check_f64_i32(&succ, a);
  if(succ) return ret;
  return llrint(a.f);
}

static inline uint32_t my_f64_to_ui32(float64_t a) {
  bool succ = 0;
  uint32_t ret = check_f64_ui32(&succ, a);
  if(succ) return ret;
  return llrint(a.f);
}
static inline int64_t  my_f64_to_i64 (float64_t a) {
  bool succ = 0;
  int64_t ret = check_f64_i64(&succ, a);
  if(succ) return ret;
  return llrint(a.f);
}
static inline uint64_t my_f64_to_ui64(float64_t a) {
  bool succ = 0;
  uint64_t ret = check_f64_ui64(&succ, a);
  if(succ) return ret;
  return llrint(a.f);
}
static inline int32_t  my_f64_to_i32_rmm (float64_t a) {
  bool succ = 0;
  int32_t ret = check_f64_i32(&succ, a);
  if(succ) return ret;
  return llround(a.f);
}
static inline uint32_t my_f64_to_ui32_rmm(float64_t a) {
  bool succ = 0;
  uint32_t ret = check_f64_ui32(&succ, a);
  if(succ) return ret;
  return llround(a.f);
}
static inline int64_t  my_f64_to_i64_rmm (float64_t a) {
  bool succ = 0;
  int64_t ret = check_f64_i64(&succ, a);
  if(succ) return ret;
  return llround(a.f);
}
static inline uint64_t my_f64_to_ui64_rmm(float64_t a) {
  bool succ = 0;
  uint64_t ret = check_f64_ui64(&succ, a);
  if(succ) return ret;
  return llround(a.f);
}

static inline float64_t f32_to_f64(float32_t a) { return float64(a.f); }
static inline float32_t f64_to_f32(float64_t a) { return float32(a.f); }

static inline float64_t fpcall_f64_roundToInt(float64_t a) {
  return float64(nearbyint(a.f));
}

static inline float64_t fpcall_f64_pow2(float64_t a) {
  return float64(pow(2.0, a.f));
}

static inline float64_t fpcall_f64_log2(float64_t a) {
  return float64(log2(a.f));
}

static inline float64_t fpcall_f64_mod(float64_t a, float64_t b) {
  return float64(fmod(a.f, b.f));
}

static inline float64_t fpcall_f64_atan(float64_t a, float64_t b) {
  return float64(atan2(a.f, b.f));
}

static inline void fp_set_rm_internal(int rm) {
  switch (rm) {
    case FPCALL_RM_RNE: rm = FE_TONEAREST; break;
    case FPCALL_RM_RTZ: rm = FE_TOWARDZERO; break;
    case FPCALL_RM_RDN: rm = FE_DOWNWARD; break;
    case FPCALL_RM_RUP: rm = FE_UPWARD; break;
    case FPCALL_RM_RMM: rm = FE_TONEAREST; break; // x86 does not support RMM
    default: assert(0);
  }
  fesetround(rm);
}

static inline uint32_t fp_get_exception() {
  uint32_t ex = 0;
#if 1
  uint32_t host_ex = fetestexcept(FE_ALL_EXCEPT);
  if (host_ex & FE_INEXACT  ) ex |= FPCALL_EX_NX;
  if (host_ex & FE_UNDERFLOW) ex |= FPCALL_EX_UF;
  if (host_ex & FE_OVERFLOW ) ex |= FPCALL_EX_OF;
  if (host_ex & FE_DIVBYZERO) ex |= FPCALL_EX_DZ;
  if (host_ex & FE_INVALID  ) ex |= FPCALL_EX_NV;
#endif
  return ex;
}

static inline void fp_clear_exception() {
  feclearexcept(FE_ALL_EXCEPT);
}

static inline void fp_set_exception(int excep){
  fp_clear_exception();
  feraiseexcept(excep);
}

#endif
