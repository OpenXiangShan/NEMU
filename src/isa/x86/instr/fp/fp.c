#include "../local-include/intr.h"
#include <rtl/fp.h>
#include <cpu/decode.h>
#include <cpu/cpu.h>

static uint32_t g_rm = FPCALL_RM_RNE;

void x86_fp_set_rm(uint32_t rm_x86) {
  switch (rm_x86) {
    case 0b00: g_rm = FPCALL_RM_RNE; break;
    case 0b01: g_rm = FPCALL_RM_RDN; break;
    case 0b10: g_rm = FPCALL_RM_RUP; break;
    case 0b11: g_rm = FPCALL_RM_RTZ; break;
  }
}

uint32_t x86_fp_get_rm() {
  switch (g_rm) {
    case FPCALL_RM_RNE: return 0b00;
    case FPCALL_RM_RDN: return 0b01;
    case FPCALL_RM_RUP: return 0b10;
    case FPCALL_RM_RTZ: return 0b11;
  }
  return 0;
}

uint32_t isa_fp_get_rm(Decode *s) {
  return g_rm;
}

void isa_fp_set_ex(uint32_t ex) {
  assert(0);
}
