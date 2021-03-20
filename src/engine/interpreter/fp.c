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

uint32_t isa_fp_get_rm(Decode *s);
void isa_fp_update_ex_flags(Decode *s, uint32_t ex_flags);

def_rtl(fpcall, rtlreg_t *dest, const rtlreg_t *src, uint32_t cmd) {
  softfloat_roundingMode = isa_fp_get_rm(s);
  int w = FPCALL_W(cmd);
  int op = FPCALL_OP(cmd);

  if (w == FPCALL_W32) {
    float32_t fsrc1 = rtlToF32(*dest);
    float32_t fsrc2 = rtlToF32(*src);
    switch (op) {
      case FPCALL_ADD: *dest = f32_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f32_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f32_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f32_div(fsrc1, fsrc2).v; break;

      case FPCALL_I32ToF: *dest = i32_to_f32(*src).v; break;
      default: panic("op = %d not supported", op);
    }
  } else if (w == FPCALL_W64) {
    float64_t fsrc1 = rtlToF64(*dest);
    float64_t fsrc2 = rtlToF64(*src);
    switch (op) {
      case FPCALL_ADD: *dest = f64_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f64_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f64_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f64_div(fsrc1, fsrc2).v; break;
      default: panic("op = %d not supported", op);
    }
  }

  if (softfloat_exceptionFlags) {
    isa_fp_update_ex_flags(s, softfloat_exceptionFlags);
    softfloat_exceptionFlags = 0;
  }
}
