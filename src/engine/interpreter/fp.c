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
#ifndef CONFIG_FPU_NONE
#include MUXDEF(CONFIG_FPU_SOFT, "softfloat-fp.h", "host-fp.h")

#define BOX_MASK_FP16 0xFFFFFFFFFFFF0000
#define BOX_MASK_FP32 0xFFFFFFFF00000000

static inline rtlreg_t unboxf16(rtlreg_t r) {
  return MUXDEF(CONFIG_FPU_SOFT, (r & BOX_MASK_FP16) == BOX_MASK_FP16, true)
    ? (r & ~BOX_MASK_FP16) : defaultNaNF16UI;
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

def_rtl(fpcall, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2, uint32_t cmd) {
#ifndef CONFIG_FPU_NONE
  uint32_t w = FPCALL_W(cmd);
  uint32_t op = FPCALL_OP(cmd);
  isa_fp_csr_check();
  if (op < FPCALL_NEED_RM) {
    uint32_t rm = isa_fp_get_rm(s);
    softfloat_roundingMode = rm;
    isa_fp_rm_check(softfloat_roundingMode);
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
  }else if (w == FPCALL_W32) {
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
