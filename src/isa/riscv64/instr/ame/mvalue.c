#include <common.h>
#ifdef CONFIG_RV_AME
#include <cpu/cpu.h>
#include "mvalue.h"
#include "mcompute_impl.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include <rtl/rtl.h>
#include <limits.h>

int ame_acc_class(mcfg_t cfg) {
  if (cfg.table_set != 0) return -1;
  switch (cfg.type_code) {
    case MTYPECODE_INT8: case MTYPECODE_INT32: return 0;
    case MTYPECODE_FP16: case MTYPECODE_BF16: case MTYPECODE_FP32: return 1;
    default: return -1;
  }
}

void ame_acc_require(int reg) {
  if (reg < 4 || reg >= 8 || ame_acc_class(cpu.mcfg[reg]) < 0)
    longjmp_exception(EX_II);
}

void ame_fp_commit(void) {
#ifndef CONFIG_FPU_NONE
  mfflags->val |= ame_fp_matrix_end() & 0x1f;
  mcsr->mfflags = mfflags->val;
#endif
}

static uint32_t ame_fp_convert(Decode *s, int width, int op, uint32_t bits) {
#ifdef CONFIG_FPU_NONE
  longjmp_exception(EX_II);
  return 0;
#else
  rtlreg_t a = bits, b = 0, result = 0;
  rtl_hostcall(s, HOSTCALL_MFP, &result, &a, &b, FPCALL_CMD(op, width));
  return result;
#endif
}

uint32_t ame_fp_op(Decode *s, int op, uint32_t a, uint32_t b) {
#ifdef CONFIG_FPU_NONE
  longjmp_exception(EX_II);
  return 0;
#else
  rtlreg_t lhs = a, rhs = b, result = 0;
  rtl_hostcall(s, HOSTCALL_MFP, &result, &lhs, &rhs, FPCALL_CMD(op, FPCALL_W32));
  return result;
#endif
}

uint32_t ame_int_result(int64_t value) {
  if (msaten->val && (value > INT32_MAX || value < INT32_MIN)) {
    value = value > INT32_MAX ? INT32_MAX : INT32_MIN;
    msat->val = 1;
    mcsr->msat = 1;
  }
  return (uint32_t)value;
}

uint32_t ame_load_expand(Decode *s, mcfg_t cfg, uint32_t bits) {
  switch (cfg.type_code) {
    case MTYPECODE_INT8: return (int32_t)(int8_t)bits;
    case MTYPECODE_FP16:
      return ame_fp_convert(s, FPCALL_W16, FPCALL_F16ToF32, bits);
    case MTYPECODE_BF16:
      return ame_fp_convert(s, FPCALL_BF16, FPCALL_BF16ToF32, bits);
    default: return bits;
  }
}

uint32_t ame_store_narrow(Decode *s, mcfg_t cfg, uint32_t bits) {
  switch (cfg.type_code) {
    case MTYPECODE_INT8: {
      int32_t value = bits;
      if (msaten->val && (value < INT8_MIN || value > INT8_MAX)) {
        value = value > INT8_MAX ? INT8_MAX : INT8_MIN;
        msat->val = 1;
        mcsr->msat = 1;
      }
      return (uint8_t)value;
    }
    case MTYPECODE_FP16:
      return ame_fp_convert(s, FPCALL_W32, FPCALL_F32ToF16, bits);
    case MTYPECODE_BF16:
      return ame_fp_convert(s, FPCALL_W32, FPCALL_F32ToBF16, bits);
    default: return bits;
  }
}

void ame_acc_convert(Decode *s, int reg, mcfg_t target) {
  ame_acc_require(reg);
  int source_class = ame_acc_class(cpu.mcfg[reg]);
  int target_class = ame_acc_class(target);
  if (target_class < 0) longjmp_exception(EX_II);
  if (source_class != target_class) {
#ifdef CONFIG_FPU_NONE
    longjmp_exception(EX_II);
#else
    ame_fp_matrix_begin(mfrm->val);
    int op = target_class ? FPCALL_I32ToF : FPCALL_FToI32;
    for (int i = 0; i < ROWNUM; i++)
      for (int j = 0; j < ARENUM32; j++)
        macc_l32(reg, i, j) = ame_fp_convert(s, FPCALL_W32, op,
                                           macc_l32(reg, i, j));
    ame_fp_commit();
#endif
  }
  cpu.mcfg[reg] = target;
}
#endif
