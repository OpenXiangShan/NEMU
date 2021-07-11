#include "../local-include/intr.h"
#include <rtl/fp.h>
#include <cpu/decode.h>
#include <cpu/cpu.h>

static uint32_t g_rm = FPCALL_RM_RNE;

void fp_set_rm(uint32_t rm) {
  g_rm = rm;
}

uint32_t isa_fp_get_rm(Decode *s) {
  return g_rm;
}

void isa_fp_set_ex(uint32_t ex) {
  assert(0);
}
