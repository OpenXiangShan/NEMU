#ifndef __RISCV64_MXFP_H__
#define __RISCV64_MXFP_H__

#include <stdbool.h>
#include <stdint.h>

float mxfp_e8m0_to_float(uint8_t scale);
float mxfp_decode_e2m1(uint8_t bits);
float mxfp_decode_e4m3(uint8_t bits);
float mxfp_decode_e5m2(uint8_t bits);
float mxfp_decode_element(uint8_t bits, uint8_t scale, int is_fp4, int is_e5m2);
bool mxfp_encode_supplied_block(const uint32_t values[32], uint8_t e,
  int is_fp4, int is_e5m2, uint8_t *payload);
bool mxfp_dynamic_scale_e(const uint32_t values[32], uint8_t *e);
uint32_t mxfp_encode_f32_lane(uint32_t value, uint32_t e, bool *invalid);
uint32_t mxfp_accumulate_block(uint32_t c, const uint8_t *a, const uint8_t *b,
  unsigned lanes, uint8_t scale_a, uint8_t scale_b, int is_fp4, int is_e5m2,
  uint8_t *flags);

#endif
