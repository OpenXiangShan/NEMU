#ifndef __NONEFP_H__

#define defaultNaNF32UI 0x7FC00000

typedef union { uint32_t v; float f; } float32_t;
static inline void bad_info() {
  panic("Floating point operation is not supported without FPU Emulation.\n"
        "Enable FPU Emulation in menuconfig if floating point operation is needed.");
}

static inline float32_t f32_bad_op() {
  bad_info();
  float32_t ret = { .v = 0 };
  return ret;
}

static inline bool bool_bad_op() {
  bad_info();
  return false;
}

static inline uint32_t u32_bad_op() {
  bad_info();
  return 0;
}

static inline float32_t f32_add(float32_t a, float32_t b) { return f32_bad_op(); }
static inline float32_t f32_sub(float32_t a, float32_t b) { return f32_bad_op(); }
static inline float32_t f32_mul(float32_t a, float32_t b) { return f32_bad_op(); }
static inline float32_t f32_div(float32_t a, float32_t b) { return f32_bad_op(); }
static inline float32_t f32_sqrt(float32_t a) { return f32_bad_op(); }
static inline float32_t f32_mulAdd(float32_t a, float32_t b, float32_t c) { return f32_bad_op(); }
static inline float32_t f32_min(float32_t a, float32_t b) { return f32_bad_op(); }
static inline float32_t f32_max(float32_t a, float32_t b) { return f32_bad_op(); }
static inline bool f32_le(float32_t a, float32_t b) { return bool_bad_op(); }
static inline bool f32_lt(float32_t a, float32_t b) { return bool_bad_op(); }
static inline bool f32_eq(float32_t a, float32_t b) { return bool_bad_op(); }
static inline float32_t i32_to_f32 (rtlreg_t a) { return f32_bad_op(); }
static inline float32_t ui32_to_f32(rtlreg_t a) { return f32_bad_op(); }
static inline float32_t i64_to_f32 (rtlreg_t a) { return f32_bad_op(); }
static inline float32_t ui64_to_f32(rtlreg_t a) { return f32_bad_op(); }
static inline int32_t  my_f32_to_i32 (float32_t a) { return u32_bad_op(); }
static inline uint32_t my_f32_to_ui32(float32_t a) { return u32_bad_op(); }
static inline int64_t  my_f32_to_i64 (float32_t a) { return u32_bad_op(); }
static inline uint64_t my_f32_to_ui64(float32_t a) { return u32_bad_op(); }


typedef union { uint64_t v; double f; } float64_t;

static inline float64_t f64_bad_op() {
  bad_info();
  float64_t ret = { .v = 0 };
  return ret;
}

static inline float64_t f64_add(float64_t a, float64_t b) { return f64_bad_op(); }
static inline float64_t f64_sub(float64_t a, float64_t b) { return f64_bad_op(); }
static inline float64_t f64_mul(float64_t a, float64_t b) { return f64_bad_op(); }
static inline float64_t f64_div(float64_t a, float64_t b) { return f64_bad_op(); }
static inline float64_t f64_sqrt(float64_t a) { return f64_bad_op(); }
static inline float64_t f64_mulAdd(float64_t a, float64_t b, float64_t c) { return f64_bad_op(); }
static inline float64_t f64_min(float64_t a, float64_t b) { return f64_bad_op(); }
static inline float64_t f64_max(float64_t a, float64_t b) { return f64_bad_op(); }
static inline bool f64_le(float64_t a, float64_t b) { return bool_bad_op(); }
static inline bool f64_lt(float64_t a, float64_t b) { return bool_bad_op(); }
static inline bool f64_eq(float64_t a, float64_t b) { return bool_bad_op(); }
static inline float64_t i32_to_f64 (rtlreg_t a) { return f64_bad_op(); }
static inline float64_t ui32_to_f64(rtlreg_t a) { return f64_bad_op(); }
static inline float64_t i64_to_f64 (rtlreg_t a) { return f64_bad_op(); }
static inline float64_t ui64_to_f64(rtlreg_t a) { return f64_bad_op(); }
static inline int32_t  my_f64_to_i32 (float64_t a) { return u32_bad_op(); }
static inline uint32_t my_f64_to_ui32(float64_t a) { return u32_bad_op(); }
static inline int64_t  my_f64_to_i64 (float64_t a) { return u32_bad_op(); }
static inline uint64_t my_f64_to_ui64(float64_t a) { return u32_bad_op(); }

static inline float64_t f32_to_f64(float32_t a) { return f64_bad_op(); }
static inline float32_t f64_to_f32(float64_t a) { return f32_bad_op(); }

static inline float64_t fpcall_f64_roundToInt(float64_t a) { return f64_bad_op(); }
static inline float64_t fpcall_f64_pow2(float64_t a) { return f64_bad_op(); }
static inline float64_t fpcall_f64_log2(float64_t a) { return f64_bad_op(); }
static inline float64_t fpcall_f64_mod(float64_t a, float64_t b) { return f64_bad_op(); }
static inline float64_t fpcall_f64_atan(float64_t a, float64_t b) { return f64_bad_op(); }

static inline void fp_set_rm(int rm) { bad_info(); }
static inline uint32_t fp_get_exception() { return u32_bad_op(); }
static inline void fp_clear_exception() { bad_info(); }

#endif
