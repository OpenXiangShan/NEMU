#ifndef __RISCV64_VFNCVTXX8_H__
#define __RISCV64_VFNCVTXX8_H__

#include "vcompute_impl.h"
#include "vfncvtxx8_impl.h"

void isa_fp_set_ex(uint32_t ex);

enum {
  VFNCVTXX8_FORMAT_INT8,
  VFNCVTXX8_FORMAT_E4M3,
  VFNCVTXX8_FORMAT_E5M2,
};

static inline int vfncvtxx8_lmul_exponent(uint32_t vlmul) {
  return vlmul < 4 ? (int)vlmul : (int)vlmul - 8;
}

static inline int vfncvtxx8_group_registers(int lmul_exponent) {
  return lmul_exponent > 0 ? 1 << lmul_exponent : 1;
}

static inline uint32_t vfncvtxx8_encode_lmul(int lmul_exponent) {
  return lmul_exponent >= 0 ? lmul_exponent : lmul_exponent + 8;
}

static inline void vfncvtxx8_check(Decode *s) {
  require_float();
  require_vector(true);
  check_vstart_exception(s);

  const uint32_t rm = isa_fp_get_frm();
  isa_fp_rm_check(rm);
  if (vtype->vsew != 2) {
    longjmp_exception(EX_II);
  }

  const int source_lmul = vfncvtxx8_lmul_exponent(vtype->vlmul);
  const int destination_lmul = source_lmul - 2;
  if (source_lmul < -1) {
    longjmp_exception(EX_II);
  }

  const int source_registers = vfncvtxx8_group_registers(source_lmul);
  const int destination_registers = vfncvtxx8_group_registers(destination_lmul);
  if ((source_lmul > 0 && (id_src2->reg & (source_registers - 1))) ||
      (destination_lmul > 0 && (id_dest->reg & (destination_registers - 1))) ||
      id_src2->reg + source_registers > 32 ||
      id_dest->reg + destination_registers > 32) {
    longjmp_exception(EX_II);
  }

  const bool groups_overlap =
      id_dest->reg < id_src2->reg + source_registers &&
      id_src2->reg < id_dest->reg + destination_registers;
  if ((groups_overlap && id_dest->reg != id_src2->reg) ||
      (s->vm == 0 && id_dest->reg == 0)) {
    longjmp_exception(EX_II);
  }
}

static inline void vfncvtxx8_execute(Decode *s, int format) {
  vfncvtxx8_check(s);
  const uint32_t rm = isa_fp_get_frm();
  const int source_lmul = vfncvtxx8_lmul_exponent(vtype->vlmul);
  const uint32_t destination_vlmul = vfncvtxx8_encode_lmul(source_lmul - 2);
  const int vlmax = get_vlmax(vtype->vsew, vtype->vlmul);
  uint32_t accumulated_flags = 0;

  if (vl->val != 0) {
    for (word_t index = 0; index < vl->val; index++) {
      if (s->vm == 0 && get_mask(0, index) == 0) {
        if (vtype->vma) {
          set_vreg(id_dest->reg, index, 0xff, 0, destination_vlmul, 1);
        }
        continue;
      }

      rtlreg_t source;
      get_vreg(id_src2->reg, index, &source, 2, vtype->vlmul, 0, 1);
      vfncvtxx8_result_t converted;
      if (format == VFNCVTXX8_FORMAT_INT8) {
        converted = vfncvtxx8_convert_int8((uint32_t)source, rm);
      } else if (format == VFNCVTXX8_FORMAT_E4M3) {
        converted = vfncvtxx8_convert_e4m3((uint32_t)source, rm);
      } else {
        converted = vfncvtxx8_convert_e5m2((uint32_t)source, rm);
      }
      set_vreg(id_dest->reg, index, converted.value, 0, destination_vlmul, 1);
      accumulated_flags |= converted.flags;
    }

    if (vtype->vta) {
      for (word_t index = vl->val; index < (word_t)vlmax; index++) {
        set_vreg(id_dest->reg, index, 0xff, 0, destination_vlmul, 1);
      }
    }
  }

  if (accumulated_flags != 0) {
    isa_fp_set_ex(accumulated_flags);
  } else {
    fp_set_dirty();
  }
  vp_set_dirty();
}

def_EHelper(vfncvtxx8_int8) {
  vfncvtxx8_execute(s, VFNCVTXX8_FORMAT_INT8);
}

def_EHelper(vfncvtxx8_e4m3) {
  vfncvtxx8_execute(s, VFNCVTXX8_FORMAT_E4M3);
}

def_EHelper(vfncvtxx8_e5m2) {
  vfncvtxx8_execute(s, VFNCVTXX8_FORMAT_E5M2);
}

#endif
