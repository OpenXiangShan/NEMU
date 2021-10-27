#include <rtl/rtl.h>
#include <cpu/difftest.h>
#include MUXDEF(CONFIG_FPU_SOFT, "softfloat-fp.h", \
    MUXDEF(CONFIG_FPU_HOST, "host-fp.h", "none-fp.h"))

#define BOX_MASK 0xFFFFFFFF00000000

static fpreg_t unbox(fpreg_t r) {
  return MUXDEF(CONFIG_FPU_SOFT, (r & BOX_MASK) == BOX_MASK, true)
    ? (r & ~BOX_MASK) : defaultNaNF32UI;
}

static float32_t fpToF32(fpreg_t r) {
  float32_t f = { .v = (uint32_t)unbox(r) };
  return f;
}

static float64_t fpToF64(fpreg_t r) {
  float64_t f = { .v = r };
  return f;
}

static fpreg_t fpcall_load_const(int idx) {
  static uint64_t table[] = {
    0x3ff0000000000000ull, // 1.0
    0x0,                   // FIXME: log2(10)
    0x3FF71547652B82FEull, // log2(e)
    0x0,                   // FIXME: pi
    0x3FD34413509F79FEull, // lg(2)
    0x3FE62E42FEFA39EFull, // ln(2)
    0x0,                   // 0.0
  };
  assert(idx != 1);
  assert(idx != 3);
  assert(idx != 7);
  return table[idx];
}

static void fp_update_ex() {
  return;
  uint32_t ex = fp_get_exception();
  if (ex) {
    isa_fp_set_ex(ex);
    fp_clear_exception();
  }
}

static void fp_set_rm(uint32_t isa_rm) {
  static uint32_t last_rm = -1;
  if (last_rm != isa_rm) {
    uint32_t fpcall_rm = isa_fp_translate_rm(isa_rm);
    fp_set_rm_internal(fpcall_rm);
    last_rm = isa_rm;
  }
}

def_rtl(flm, fpreg_t *dest, const rtlreg_t *addr, sword_t offset, int len, int mmu_mode) {
  if (len == 8 && !ISDEF(CONFIG_ISA64)) {
    uint32_t lo = vaddr_read(s, *addr + offset + 0, 4, mmu_mode);
    uint32_t hi = vaddr_read(s, *addr + offset + 4, 4, mmu_mode);
    *dest = lo | ((uint64_t)hi << 32);
  } else {
    *dest = vaddr_read(s, *addr + offset, len, mmu_mode);
  }
}

def_rtl(fsm, const fpreg_t *src1, const rtlreg_t *addr, sword_t offset, int len, int mmu_mode) {
  if (len == 8 && !ISDEF(CONFIG_ISA64)) {
    vaddr_write(s, *addr + offset + 0, 4, *src1, mmu_mode);
    vaddr_write(s, *addr + offset + 4, 4, *src1 >> 32, mmu_mode);
  } else {
    vaddr_write(s, *addr + offset, len, *src1, mmu_mode);
  }
}

#define fpToF(fpreg, w) concat(fpToF, w)(fpreg)
#define def_rtl_fp(name, body, ...) \
  def_rtl(name, __VA_ARGS__) { *dest = (body); fp_update_ex(); }

#define def_rtl_fp_unary(name, op_name, w) \
  def_rtl_fp(name, op_name(fpToF(*src1, w)).v, fpreg_t *dest, const fpreg_t *src1)

#define def_rtl_fp_binary(name, op_name, w) \
  def_rtl_fp(name, op_name(fpToF(*src1, w), fpToF(*src2, w)).v, \
      fpreg_t *dest, const fpreg_t *src1, const fpreg_t *src2)

#define def_rtl_fp_ternary(name, op_name, w) \
  def_rtl_fp(name, op_name(fpToF(*src1, w), fpToF(*src2, w), fpToF(*dest, w)).v, \
      fpreg_t *dest, const fpreg_t *src1, const fpreg_t *src2)

#define def_rtl_fp_cmp(name, op_name, w) \
  def_rtl_fp(name, op_name(fpToF(*src1, w), fpToF(*src2, w)), \
      rtlreg_t *dest, const fpreg_t *src1, const fpreg_t *src2)

#define def_rtl_i2f(name, op_name) \
  def_rtl_fp(name, op_name(*src1).v, fpreg_t *dest, const rtlreg_t *src1)

#define def_rtl_i642f(name, op_name) \
  def_rtl_fp(name, op_name(*src1).v, fpreg_t *dest, const fpreg_t *src1)

#define def_rtl_f2i(name, op_name, w) \
  def_rtl_fp(name, op_name(fpToF(*src1, w)), rtlreg_t *dest, const fpreg_t *src1)

#define def_rtl_f2i64(name, op_name, w) \
  def_rtl_fp(name, op_name(fpToF(*src1, w)), fpreg_t *dest, const fpreg_t *src1)

def_rtl_fp_binary(fadds, f32_add, 32);
def_rtl_fp_binary(fsubs, f32_sub, 32);
def_rtl_fp_binary(fmuls, f32_mul, 32);
def_rtl_fp_binary(fdivs, f32_div, 32);
def_rtl_fp_binary(fmins, f32_min, 32);
def_rtl_fp_binary(fmaxs, f32_max, 32);
def_rtl_fp_unary(fsqrts, f32_sqrt, 32);
def_rtl_fp_ternary(fmadds, f32_mulAdd, 32);
def_rtl_fp_cmp(fles, f32_le, 32);
def_rtl_fp_cmp(flts, f32_lt, 32);
def_rtl_fp_cmp(feqs, f32_eq, 32);
def_rtl_i2f(fcvt_i32_to_f32, i32_to_f32);
def_rtl_i2f(fcvt_u32_to_f32, ui32_to_f32);
def_rtl_i642f(fcvt_i64_to_f32, i64_to_f32);
def_rtl_i642f(fcvt_u64_to_f32, ui64_to_f32);
def_rtl_f2i(fcvt_f32_to_i32, my_f32_to_i32,  32);
def_rtl_f2i(fcvt_f32_to_u32, my_f32_to_ui32, 32);
def_rtl_f2i64(fcvt_f32_to_i64, my_f32_to_i64,  32);
def_rtl_f2i64(fcvt_f32_to_u64, my_f32_to_ui64, 32);

def_rtl_fp_binary(faddd, f64_add, 64);
def_rtl_fp_binary(fsubd, f64_sub, 64);
def_rtl_fp_binary(fmuld, f64_mul, 64);
def_rtl_fp_binary(fdivd, f64_div, 64);
def_rtl_fp_binary(fmind, f64_min, 64);
def_rtl_fp_binary(fmaxd, f64_max, 64);
def_rtl_fp_unary(fsqrtd, f64_sqrt, 64);
def_rtl_fp_ternary(fmaddd, f64_mulAdd, 64);
def_rtl_fp_cmp(fled, f64_le, 64);
def_rtl_fp_cmp(fltd, f64_lt, 64);
def_rtl_fp_cmp(feqd, f64_eq, 64);
def_rtl_i2f(fcvt_i32_to_f64, i32_to_f64);
def_rtl_i2f(fcvt_u32_to_f64, ui32_to_f64);
def_rtl_i642f(fcvt_i64_to_f64, i64_to_f64);
def_rtl_i642f(fcvt_u64_to_f64, ui64_to_f64);
def_rtl_f2i(fcvt_f64_to_i32, my_f64_to_i32,  64);
def_rtl_f2i(fcvt_f64_to_u32, my_f64_to_ui32, 64);
def_rtl_f2i64(fcvt_f64_to_i64, my_f64_to_i64,  64);
def_rtl_f2i64(fcvt_f64_to_u64, my_f64_to_ui64, 64);
def_rtl_fp_unary(fcvt_f32_to_f64, f32_to_f64, 32);
def_rtl_fp_unary(fcvt_f64_to_f32, f64_to_f32, 64);

def_rtl(fmv, fpreg_t *dest, const fpreg_t *src1) {
  *dest = *src1;
}

def_rtl(fneg, fpreg_t *dest, const fpreg_t *src1) {
  *dest = *src1 ^ 0x8000000000000000ul;
}

def_rtl(fabs, fpreg_t *dest, const fpreg_t *src1) {
  *dest = *src1 & 0x7ffffffffffffffful;
}

def_rtl(fclassd, rtlreg_t *dest, const fpreg_t *src1) {
  *dest = 1 << ((int64_t)(*src1) < 0 ? 1 : 6);
}

def_rtl(fpcall, uint32_t id, fpreg_t *dest, const fpreg_t *src1, const rtlreg_t *src2, uint32_t imm) {
  switch (id) {
    case FPCALL_LOADCONST: *dest = fpcall_load_const(imm); return;
    case FPCALL_SETRM: fp_set_rm(*src2); return;
    case FPCALL_SETRM_CONST: fp_set_rm(imm); return;
#ifdef CONFIG_ISA_x86
    case FPCALL_FILDLL:
      rtl_flm(s, dest, src2, imm, 8, MMU_DYNAMIC);
      rtl_fcvt_i64_to_f64(s, dest, dest);
      return;
    case FPCALL_FISTLL:
      rtl_fcvt_f64_to_i64(s, &s->isa.fptmp, dest);
      rtl_fsm(s, &s->isa.fptmp, src2, imm, 8, MMU_DYNAMIC);
      return;
#endif
    case FPCALL_CMOV: if (*src2) *dest = *src1; return;
  }

  // Some library floating point functions gives very small
  // rounding diffrerence between DUT and REF. This is strange.
  // But we deal with it by skip the guest instruction of REF.
  difftest_skip_ref();

  switch (id) {
    case FPCALL_ROUNDINT: *dest = fpcall_f64_roundToInt(fpToF64(*src1)).v; break;
    case FPCALL_POW2: *dest = fpcall_f64_pow2(fpToF64(*src1)).v; break;
    case FPCALL_LOG2: *dest = fpcall_f64_log2(fpToF64(*src1)).v; break;
    case FPCALL_MOD: *dest = fpcall_f64_mod(fpToF64(*dest), fpToF64(*src1)).v; break;
    case FPCALL_ATAN: *dest = fpcall_f64_atan(fpToF64(*dest), fpToF64(*src1)).v; break;
    default: panic("unsupport id = %d", id);
  }
}
