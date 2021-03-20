#include <rtl/rtl.h>
#include <softfloat.h>
#include <specialize.h>
#include <internals.h>

#define BOX_MASK 0xFFFFFFFF00000000

static inline rtlreg_t unbox(rtlreg_t r) {
  if ((r & BOX_MASK) == BOX_MASK) return r & ~BOX_MASK;
  else return defaultNaNF32UI;
}

static inline float32_t rtlToF32(rtlreg_t r) {
  float32_t f = { .v = unbox(r) };
  return f;
}

static inline float64_t rtlToF64(rtlreg_t r) {
  float64_t f = { .v = r };
  return f;
}

typedef float32_t (*f32_binfun_t)(float32_t, float32_t);
static const f32_binfun_t f32_binfun[] = {
  [FPCALL_ADD] = f32_add,
  [FPCALL_SUB] = f32_sub,
  [FPCALL_MUL] = f32_mul,
  [FPCALL_DIV] = f32_div,
};

typedef float64_t (*f64_binfun_t)(float64_t, float64_t);
static const f64_binfun_t f64_binfun[] = {
  [FPCALL_ADD] = f64_add,
  [FPCALL_SUB] = f64_sub,
  [FPCALL_MUL] = f64_mul,
  [FPCALL_DIV] = f64_div,
};

uint32_t isa_fp_get_rm(Decode *s);
void isa_fp_update_ex_flags(Decode *s, uint32_t ex_flags);

def_rtl(fpcall, rtlreg_t *dest, const rtlreg_t *src, uint32_t cmd) {
  softfloat_roundingMode = isa_fp_get_rm(s);
  int w = FPCALL_W(cmd);
  int op = FPCALL_OP(cmd);

  if (w == FPCALL_W32) {
    float32_t fsrc1 = rtlToF32(*dest);
    float32_t fsrc2 = rtlToF32(*src);
    assert(op < sizeof(f32_binfun) / sizeof(f32_binfun[0]));
    *dest = f32_binfun[op](fsrc1, fsrc2).v;
  } else if (w == FPCALL_W64) {
    float64_t fsrc1 = rtlToF64(*dest);
    float64_t fsrc2 = rtlToF64(*src);
    assert(op < sizeof(f64_binfun) / sizeof(f64_binfun[0]));
    *dest = f64_binfun[op](fsrc1, fsrc2).v;
  }

  if (softfloat_exceptionFlags) {
    isa_fp_update_ex_flags(s, softfloat_exceptionFlags);
    softfloat_exceptionFlags = 0;
  }
}
