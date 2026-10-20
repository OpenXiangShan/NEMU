#ifndef __RISCV64_AME_MVALUE_H__
#define __RISCV64_AME_MVALUE_H__
#include <common.h>
#include <cpu/decode.h>
#include "mreg.h"
#ifdef CONFIG_RV_AME
// Classify only nominal formats supported by the wide accumulator interface.
int ame_acc_class(mcfg_t cfg);
void ame_acc_require(int reg);
void ame_fp_commit(void);
uint32_t ame_fp_op(Decode *s, int op, uint32_t a, uint32_t b);
uint32_t ame_load_expand(Decode *s, mcfg_t cfg, uint32_t bits);
uint32_t ame_store_narrow(Decode *s, mcfg_t cfg, uint32_t bits);
uint32_t ame_int_result(int64_t value);
void ame_acc_convert(Decode *s, int reg, mcfg_t target);
void ame_ew_execute(Decode *s);
void ame_lv_memory(Decode *s);
#endif
#endif
