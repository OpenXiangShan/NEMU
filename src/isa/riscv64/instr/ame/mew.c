#include <common.h>
#ifdef CONFIG_RV_AME
#include <cpu/cpu.h>
#include "mvalue.h"
#include "mcompute_impl.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/reg.h"
#include "../rvv/vfexp2.h"
#include <memory/vaddr.h>
#include <rtl/fp.h>

static uint32_t integer_ew(int op, int32_t a, int32_t b) {
  switch (op) {
    case 0: return ame_int_result((int64_t)a * b);
    case 1: return ame_int_result((int64_t)a + b);
    case 5: return ame_int_result((int64_t)a - b);
    case 2: return a > b ? a : b;
    case 6: return a < b ? a : b;
    default: longjmp_exception(EX_II); return 0;
  }
}

static uint32_t float_ew(Decode *s, int op, uint32_t a, uint32_t b) {
  switch (op) {
    case 0: return ame_fp_op(s, FPCALL_MUL, a, b);
    case 1: return ame_fp_op(s, FPCALL_ADD, a, b);
    case 5: return ame_fp_op(s, FPCALL_SUB, a, b);
    case 2: return ame_fp_op(s, FPCALL_MAX, a, b);
    case 6: return ame_fp_op(s, FPCALL_MIN, a, b);
    default: longjmp_exception(EX_II); return 0;
  }
}

void ame_ew_execute(Decode *s) {
  require_matrix();
  uint32_t inst = s->isa.instr.val;
  int fmt = BITS(inst, 31, 30), op = BITS(inst, 29, 27);
  int control = BITS(inst, 26, 25);
  int src = BITS(inst, 19, 15), src2 = BITS(inst, 24, 20);
  int dest = BITS(inst, 11, 7);
  uint64_t m = mtilem->val, n = mtilen->val;
  ame_acc_require(src);
  int fp = ame_acc_class(cpu.mcfg[src]);
  if (m > ROWNUM || n > ARENUM32 || n > ROWNUM)
    longjmp_exception(EX_II);
  if (fmt == 0) {
    if (op != 0 && op != 1 && op != 5 && op != 2 && op != 6)
      longjmp_exception(EX_II);
    ame_acc_require(dest);
    if (ame_acc_class(cpu.mcfg[dest]) != fp) longjmp_exception(EX_II);
    if ((control == 1 && n > LVLEN) || (control == 2 && m > LVLEN))
      longjmp_exception(EX_II);
    if (control == 0) {
      ame_acc_require(src2);
      if (ame_acc_class(cpu.mcfg[src2]) != fp) longjmp_exception(EX_II);
    }
  } else if (fmt == 1) {
    if (op > 2 || control || src2) longjmp_exception(EX_II);
    ame_acc_require(dest);
    if (ame_acc_class(cpu.mcfg[dest]) != fp || (op != 0 && !fp))
      longjmp_exception(EX_II);
  } else if (fmt == 2) {
    if (op > 1 || src2 || ((control & 1) ? n : m) > LVLEN)
      longjmp_exception(EX_II);
  } else {
    longjmp_exception(EX_II);
  }
#ifdef CONFIG_FPU_NONE
  if (fp) longjmp_exception(EX_II);
#endif
  mp_set_dirty();
  if (fmt == 2) {
    bool axis = control & 1, accumulate = control & 2;
    uint64_t outputs = axis ? n : m, inputs = axis ? m : n;
    if (inputs == 0) {
      if (!accumulate)
        for (uint64_t k = 0; k < outputs; k++) cpu.mlv[dest][k] = 0;
      return;
    }
#ifndef CONFIG_FPU_NONE
    if (fp && outputs) ame_fp_matrix_begin(mfrm->val);
#endif
    for (uint64_t k = 0; k < outputs; k++) {
      // Read the old destination once. Integer SUM narrows only after all terms.
      int64_t isum = 0;
      uint32_t result = 0;
      bool have = accumulate;
      if (accumulate) {
        result = cpu.mlv[dest][k];
        isum = (int32_t)result;
      }
      for (uint64_t x = 0; x < inputs; x++) {
        uint32_t a = macc_l32(src, axis ? x : k, axis ? k : x);
        if (!fp && op == 0) {
          isum += (int32_t)a;
        } else if (!have) {
          // A singleton still quiets signaling NaNs and reports NV.
          result = fp ? ame_fp_op(s, FPCALL_MAX, a, a) : a;
        } else if (fp) {
          result = ame_fp_op(s, op == 0 ? FPCALL_ADD : FPCALL_MAX, result, a);
        } else {
          result = (int32_t)result > (int32_t)a ? result : a;
        }
        have = true;
      }
      cpu.mlv[dest][k] = !fp && op == 0 ? ame_int_result(isum) : result;
    }
    if (fp && outputs) ame_fp_commit();
    return;
  }
  if (m == 0 || n == 0) return;
  bool do_fp = fp && !(fmt == 1 && op == 0);
#ifndef CONFIG_FPU_NONE
  if (do_fp) ame_fp_matrix_begin(mfrm->val);
#endif
#ifdef CONFIG_AME_TILEREG_UB_CHECK
  ame_matrix_region_check_read(src, m, n * 4, s->pc);
  if (fmt == 0 && control == 0)
    ame_matrix_region_check_read(src2, m, n * 4, s->pc);
#endif
  for (uint64_t i = 0; i < m; i++) {
    for (uint64_t j = 0; j < n; j++) {
      uint32_t a = macc_l32(src, i, j), result;
      if (fmt == 1) {
        if (op == 0) {
          result = fp ? a & 0x7fffffff : ame_int_result((int32_t)a < 0 ? -(int64_t)(int32_t)a : (int32_t)a);
        } else if (op == 1) {
          vfexp2_result_t exp2 = vfexp2_compute(a, 2, false, mfrm->val);
          result = exp2.result;
          mfflags->val |= exp2.fflags;
          mcsr->mfflags = mfflags->val;
        } else {
          result = ame_fp_op(s, FPCALL_REC7, a, 0);
        }
      } else {
        uint32_t b = control == 0 ? macc_l32(src2, i, j) :
                     control == 1 ? cpu.mlv[src2][j] :
                     control == 2 ? cpu.mlv[src2][i] : (uint32_t)reg_l(src2);
        result = fp ? float_ew(s, op, a, b) : integer_ew(op, a, b);
      }
      macc_l32(dest, i, j) = result;
    }
  }
  if (do_fp) ame_fp_commit();
#ifdef CONFIG_AME_TILEREG_UB_CHECK
  ame_matrix_region_mark_write(dest, m, n * 4, s->pc);
#endif
}

void ame_lv_memory(Decode *s) {
  require_matrix();
  uint32_t inst = s->isa.instr.val;
  int kind = BITS(inst, 31, 28), reg = BITS(inst, 11, 7);
  int rs1 = BITS(inst, 19, 15), rs2 = BITS(inst, 24, 20);
  bool store = BITS(inst, 25, 25);
  uint64_t length = kind == 11 ? LVLEN : kind == 3 ? mtilen->val : mtilem->val;
  if (length > LVLEN || (kind == 11 && rs2)) longjmp_exception(EX_II);
  uint64_t base = reg_l(rs1), stride = kind == 11 ? 4 : reg_l(rs2);
  if (!store && length) mp_set_dirty();
  for (uint64_t k = 0; k < length; k++) {
    uint64_t address = base + k * stride;
    if (store) vaddr_write(s, address, 4, cpu.mlv[reg][k], MMU_TRANSLATE);
    else cpu.mlv[reg][k] = vaddr_read(s, address, 4, MMU_TRANSLATE);
  }
}
#endif
