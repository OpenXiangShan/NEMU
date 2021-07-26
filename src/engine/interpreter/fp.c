#include <rtl/rtl.h>
#include MUXDEF(CONFIG_FPU_SOFT, "softfloat-fp.h", "host-fp.h")

#define BOX_MASK 0xFFFFFFFF00000000

static inline rtlreg_t unbox(rtlreg_t r) {
  return MUXDEF(CONFIG_FPU_SOFT, (r & BOX_MASK) == BOX_MASK, true)
    ? (r & ~BOX_MASK) : defaultNaNF32UI;
}

static inline float32_t rtlToF32(rtlreg_t r) {
  float32_t f = { .v = (uint32_t)unbox(r) };
  return f;
}

static inline float64_t rtlToF64(rtlreg_t r) {
  float64_t f = { .v = r };
  return f;
}

uint32_t isa_fp_get_rm(Decode *s);
void isa_fp_set_ex(uint32_t ex);
void isa_fp_csr_check();

def_rtl(fpcall, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2, uint32_t cmd) {
  uint32_t w = FPCALL_W(cmd);
  uint32_t op = FPCALL_OP(cmd);
  isa_fp_csr_check();
  if (op < FPCALL_NEED_RM) {
    static uint32_t last_rm = -1;
    uint32_t rm = isa_fp_get_rm(s);
    if (unlikely(rm != last_rm)) {
      fp_set_rm(rm);
      last_rm = rm;
    }
  }

  if (w == FPCALL_W32) {
    float32_t fsrc1 = rtlToF32(*src1);
    float32_t fsrc2 = rtlToF32(*src2);
    switch (op) {
      case FPCALL_ADD: *dest = f32_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f32_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f32_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f32_div(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f32_min(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f32_max(fsrc1, fsrc2).v; break;

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
      default: panic("op = %d not supported", op);
    }
  }

  uint32_t ex = fp_get_exception();
  if (ex) {
    isa_fp_set_ex(ex);
    fp_clear_exception();
  }
}

def_rtl(fclass, rtlreg_t *fdest, rtlreg_t *src, int width) {
  if (width == FPCALL_W32) {
    *fdest = f32_classify(rtlToF32(*src));
  } else if (width == FPCALL_W64) {
    *fdest = f64_classify(rtlToF64(*src));
  } else assert(0);
}
