/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
***************************************************************************************/

#ifndef __RISCV64_MXFP_H__
#define __RISCV64_MXFP_H__

#include <stdbool.h>
#include <stdint.h>

float mxfp_e8m0_to_float(uint8_t scale);
float mxfp_decode_e2m1(uint8_t bits);
float mxfp_decode_e4m3(uint8_t bits);
float mxfp_decode_e5m2(uint8_t bits);
float mxfp_decode_element(uint8_t bits, uint8_t scale, int is_fp4, int is_e5m2);

// All 32 raw binary32 lanes are active; caller supplies e, never selected here.
// FP4: e<=251, |x|<=16*2^(e-128), strict midpoint ties toward zero, zero code=0.
// FP8: e<=254, d=2^(e-127), |x|<=max*d (E4M3:448, E5M2:57344).
// RNE, preserving both exact and rounded signed zero.
// Reject any nonfinite lane or out-of-domain block, clearing exactly 16/32
// payload bytes. FP4 nibble j is continuous K lane j (low nibble first).
// Does not publish matrix/scalar flags; preserves the SoftFloat environment.
bool mxfp_encode_supplied_block(const uint32_t values[32], uint8_t e,
  int is_fp4, int is_e5m2, uint8_t *payload);

// Dynamic FP4 scale selection for one fully active K32. An all-zero input is
// supported with e=0. For nonzero finite inputs, the integer exponent/fraction
// guard accepts E=2..253 with F=0 or F in [0x400,0x7ffc00] and returns e=E-2.
// Nonfinite or unguarded inputs return false with e=0. No flags or FP state
// are touched; the exact supplied-scale encoder remains the payload owner.
bool mxfp_dynamic_scale_e(const uint32_t values[32], uint8_t *e);

// Same numerical owner for the experimental vector instruction. Full u32 e;
// a rejected active lane returns code0 and requests scalar NV, no other flags.
uint32_t mxfp_encode_f32_lane(uint32_t value, uint32_t e, bool *invalid);

// Unpacked active lanes only (1..32). One exact block contribution is added
// to binary32 C with RNE; IEEE flags are ORed into *flags.
uint32_t mxfp_accumulate_block(uint32_t c, const uint8_t *a, const uint8_t *b,
  unsigned lanes, uint8_t scale_a, uint8_t scale_b, int is_fp4, int is_e5m2,
  uint8_t *flags);

#endif // __RISCV64_MXFP_H__
