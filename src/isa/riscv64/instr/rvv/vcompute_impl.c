/***************************************************************************************
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

#include "rtl/fp.h"
#include <common.h>
#include <stdint.h>
#ifdef CONFIG_RVV

#include "vcompute_impl.h"
#include <cpu/cpu.h>
#include <rtl/fp.h>
#include "vcommon.h"

#include "../rvb/rvintrin.h" // For the Zvbc clmul function reutilization
#include "../rvk/aes_common.h"
#include "../rvk/sm4_common.h"

#undef s0
#undef s1


#define s0    (&tmp_reg[0])
#define s1    (&tmp_reg[1])
#define s2    (&tmp_reg[2])
#define s3    (&tmp_reg[3])

rtlvreg_t tmp_vreg[8];

typedef __uint128_t uint128_t;
typedef __int128_t int128_t;

#define ADD(T, x, y) \
    ({T _x = (x), _y = (y); _x + _y;})

#define SUB(T, x, y) \
    ({T _x = (x), _y = (y); _x - _y;})

#define SAT_ADD(T, UT, x, y, sat) \
    ({UT ux = (T)x; UT uy = (T)y; UT res = ADD(UT, ux, uy); \
      bool _sat = false; int sh = sizeof(T) * 8 - 1; \
      ux = (ux >> sh) + (((UT)0x1 << sh) - 1); \
      if ((T) ((ux ^ uy) | ~(uy ^ res)) >= 0) { \
        res = ux; _sat = true; } \
      *(sat) = _sat; (T)res;})

#define SAT_SUB(T, UT, x, y, sat) \
    ({UT ux = (UT)x; UT uy = (UT)y; UT res = SUB(UT, ux, uy); \
      bool _sat = false; int sh = sizeof(T) * 8 - 1; \
      ux = (ux >> sh) + (((UT)0x1 << sh) - 1); \
      if ((T) ((ux ^ uy) & (ux ^ res)) < 0) { \
        res = ux; _sat = true; } \
      *(sat) = _sat; (T)res;})

#define SAT_SUBU(UT, x, y, sat) \
    ({UT res = (UT)x - (UT)y; \
      bool _sat = false; \
      _sat = !(res <= (UT)x); \
      res &= -(res <= (UT)x); \
      *(sat) = _sat; (UT)res;})

#define INT_ROUNDING(result, xrm, gb) \
  do { \
    const uint64_t lsb = 1UL << (gb); \
    const uint64_t lsb_half = lsb >> 1; \
    switch (xrm) { \
      case RNU: \
        result += lsb_half; \
        break; \
      case RNE: \
        if ((result & lsb_half) && ((result & (lsb_half - 1)) || (result & lsb))) \
          result += lsb; \
        break; \
      case RDN: \
        break; \
      case ROD: \
        if (result & (lsb - 1)) \
          result |= lsb; \
        break; \
    } \
  } while (0)

#define ROUNDING(op, vd, vs2, vs1, signed) \
    ({ \
        uint128_t res = signed ? ((int128_t)(int64_t)vs2) op ((int128_t)(int64_t)vs1) : (uint128_t)(vs2) op (uint128_t)(vs1); \
        INT_ROUNDING(res, vxrm->val, 1); \
        vd = res >> 1; \
    })

uint32_t vf_allowed_e16[] = {
  FWCVT_FXU,      // vfwcvt_fxuv
  FWCVT_FX,       // vfwcvt_fxv
  FNCVT_XUF,      // vfncvt_xufw
  FNCVT_XF,       // vfncvt_xfw
  FNCVT_RTZ_XUF,  // vfncvt_rtz_xufw
  FNCVT_RTZ_XF,   // vfncvt_rtz_xfw

#ifdef CONFIG_RV_ZVFH_MIN
  FWCVT_FF,       // vfwcvt_ffv
  FNCVT_FF,       // vfncvt_ffw
#endif

#ifdef CONFIG_RV_ZVFH
  //33.13
  FADD,
  FSUB,
  FRSUB,
  //FWADD_W,//fwadd use fadd
  //FWSUB_W,//fwsub use fsub
  FMUL,
  FDIV,
  FRDIV,
  FWMUL,
  FMACC,
  FNMACC,
  FMSAC,
  FNMSAC,
  FMADD,
  FNMADD,
  FMSUB,
  FNMSUB,
  FWMACC,
  FWNMACC,
  FWMSAC,
  FWNMSAC,
  FSQRT,
  FRSQRT7,
  FREC7,
  FMIN,
  FMAX,
  FSGNJ,
  FSGNJN,
  FSGNJX,
  MFEQ,
  MFNE,
  MFLT,
  MFLE,
  MFGT,
  MFGE,
  FCLASS,
  FMERGE,
  //FMV_V_F == FMERGE,
  FCVT_XUF,
  FCVT_XF,
  FCVT_RTZ_XUF,
  FCVT_RTZ_XF,
  FCVT_FXU,
  FCVT_FX,
  FWCVT_XUF,
  FWCVT_XF,
  FWCVT_RTZ_XUF,
  FWCVT_RTZ_XF,
  FWCVT_FXU,
  FWCVT_FX,
  FWCVT_FF,
  FNCVT_XUF,
  FNCVT_XF,
  FNCVT_RTZ_XUF,
  FNCVT_RTZ_XF,
  FNCVT_FXU,
  FNCVT_FX,
  FNCVT_FF,
  FNCVT_ROD_FF,
  FREDOSUM,
  FREDUSUM,
  FREDMAX,
  FREDMIN,
  FWREDOSUM,
  FWREDUSUM,
  FSLIDE1UP, 
  FSLIDE1DOWN,
#endif

#ifdef CONFIG_RV_ZVFBF_MIN
  FWCVT_BF16_FF,  // vfwcvtbf16_ffv
  FNCVT_BF16_FF,  // vfncvtbf16_ffw
#endif
#ifdef CONFIG_RV_ZVFBF_WMA
  FWMACCBF16,     // vfwmaccbf16.vv/.vf
#endif
};

# ifdef CONFIG_RV_ZVFH

uint32_t vf_allowed_e8[] = {
  FWCVT_FXU,
  FWCVT_FX,
  FNCVT_XUF,
  FNCVT_XF,
  FNCVT_RTZ_XUF,
  FNCVT_RTZ_XF,
};
#endif

static bool is_vf_allowed_e16(uint32_t opcode) {
  int len = sizeof(vf_allowed_e16) / sizeof(vf_allowed_e16[0]);
  for (int i = 0; i < len; i++) {
    if (vf_allowed_e16[i] == opcode) {
      return true;
    }
  }
  return false;
}

static bool is_vf_allowed_e8(uint32_t opcode) {
  int len = sizeof(vf_allowed_e8) / sizeof(vf_allowed_e8[0]);
  for (int i = 0; i < len; i++) {
    if (vf_allowed_e8[i] == opcode) {
      return true;
    }
  }
  return false;
}

#if defined(CONFIG_RV_ZVFBF_MIN) || defined(CONFIG_RV_ZVFBF_WMA)
static bool is_bf16_opcode(uint32_t opcode) {
  switch (opcode) {
#ifdef CONFIG_RV_ZVFBF_MIN
    case FWCVT_BF16_FF:
    case FNCVT_BF16_FF:
      return true;
#endif
#ifdef CONFIG_RV_ZVFBF_WMA
    case FWMACCBF16:
      return true;
#endif
    default:
      return false;
  }
}
#endif

static inline void update_vcsr() {
  vcsr->val = (vxrm->val) << 1 | vxsat->val;
}

static inline void reverse_byte_bits(uint64_t *val) {
  uint64_t tmp = *val;
  tmp = ((tmp & 0xaaaaaaaaaaaaaaaaLLU) >> 1) | ((tmp & 0x5555555555555555LLU) << 1);
  tmp = ((tmp & 0xccccccccccccccccLLU) >> 2) | ((tmp & 0x3333333333333333LLU) << 2);
  tmp = ((tmp & 0xf0f0f0f0f0f0f0f0LLU) >> 4) | ((tmp & 0x0f0f0f0f0f0f0f0fLLU) << 4);
  *val = tmp;
}

static inline void reverse_nbytes(uint64_t *val, int sew) {
  uint64_t tmp = *val;
  if (sew >= 1) tmp = ((tmp & 0xff00ff00ff00ff00LLU) >> 8) | ((tmp & 0x00ff00ff00ff00ffLLU) << 8);
  if (sew >= 2) tmp = ((tmp & 0xffff0000ffff0000LLU) >> 16) | ((tmp & 0x0000ffff0000ffffLLU) << 16);
  if (sew >= 3) tmp = ((tmp & 0xffffffff00000000LLU) >> 32) | ((tmp & 0x00000000ffffffffLLU) << 32);
  *val = tmp;
}

#ifdef CONFIG_RV_ZVKG

static inline uint128_t brev8_128(uint128_t value) {
  uint64_t lo = value;
  uint64_t hi = value >> 64;

  reverse_byte_bits(&lo);
  reverse_byte_bits(&hi);
  return ((uint128_t)hi << 64) | lo;
}

static inline uint128_t get_velem128(int reg, int group) {
  uint128_t value = 0;
  rtlreg_t lane_value;
  int base = group * 4;

  for (int lane = 0; lane < 4; lane++) {
    get_vreg(reg, base + lane, &lane_value, 2, vtype->vlmul, 0, 1);
    value |= (uint128_t)(lane_value & UINT32_MAX) << (lane * 32);
  }
  return value;
}

static inline void set_velem128(int reg, int group, uint128_t value) {
  int base = group * 4;

  for (int lane = 0; lane < 4; lane++) {
    set_vreg(reg, base + lane, value >> (lane * 32),
             vtype->vsew, vtype->vlmul, 1);
  }
}

static inline uint128_t ghash_multiply(uint128_t y, uint128_t h) {
  uint128_t z = 0;

  y = brev8_128(y);
  h = brev8_128(h);
  for (int bit = 0; bit < 128; bit++) {
    uint128_t y_mask = (uint128_t)0 - (y & 1);
    uint128_t reduce_mask = (uint128_t)0 - (h >> 127);

    z ^= h & y_mask;
    h <<= 1;
    h ^= (uint128_t)0x87 & reduce_mask;
    y >>= 1;
  }

  return brev8_128(z);
}

static void vgcm_instr(int opcode, Decode *s) {
  int width = 8 << vtype->vsew;
  double vflmul = compute_vflmul();
  if ((vl->val & 3) != 0 || (vstart->val & 3) != 0 || vflmul * VLEN < 4 * width) {
    longjmp_exception(EX_II);
  }

  check_vstart_exception(s);
  if (check_vstart_ignore(s)) {
    vp_set_dirty();
    return;
  }

  int eg_len = vl->val / 4;
  int eg_start = vstart->val / 4;
  for (int group = eg_start; group < eg_len; group++) {
    int base = group * 4;
    bool active = s->vm != 0;
    if (!active) {
      for (int lane = 0; lane < 4; lane++) {
        active |= get_mask(0, base + lane) != 0;
      }
      if (!active) {
        if (RVV_AGNOSTIC && vtype->vma) {
          for (int lane = 0; lane < 4; lane++) {
            set_vreg(id_dest->reg, base + lane, UINT64_MAX,
                     vtype->vsew, vtype->vlmul, 1);
          }
        }
        continue;
      }
    }

    uint128_t y = get_velem128(id_dest->reg, group);
    uint128_t h = get_velem128(id_src2->reg, group);

    if (opcode == VGHSH) {
      y ^= get_velem128(id_src->reg, group);
    } else {
      y = get_velem128(id_src2->reg, group);
      h = get_velem128(id_dest->reg, group);
    }
    uint128_t result = ghash_multiply(y, h);
    for (int lane = 0; lane < 4; lane++) {
      if (s->vm || get_mask(0, base + lane)) {
        set_vreg(id_dest->reg, base + lane,
                 result >> (lane * 32), vtype->vsew, vtype->vlmul, 1);
      } else if (RVV_AGNOSTIC && vtype->vma) {
        set_vreg(id_dest->reg, base + lane, UINT64_MAX,
                 vtype->vsew, vtype->vlmul, 1);
      }
    }
  }

  if (RVV_AGNOSTIC && vtype->vta) {
    int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
    for (int idx = eg_len * 4; idx < vlmax; idx++) {
      set_vreg(id_dest->reg, idx, UINT64_MAX,
               vtype->vsew, vtype->vlmul, 1);
    }
  }

  vstart->val = 0;
  vp_set_dirty();
}

#endif // CONFIG_RV_ZVKG

#if defined(CONFIG_RV_ZVKNHA) || defined(CONFIG_RV_ZVKNHB)

static inline uint64_t vsha2_mask(uint64_t value, int width) {
  return width == 32 ? (uint32_t)value : value;
}

static inline uint64_t vsha2_rotr(uint64_t value, int amount, int width) {
  value = vsha2_mask(value, width);
  return vsha2_mask((value >> amount) | (value << (width - amount)), width);
}

static inline uint64_t vsha2_sig0(uint64_t value, int width) {
  if (width == 32) {
    return vsha2_rotr(value, 7, width) ^ vsha2_rotr(value, 18, width) ^ (value >> 3);
  }
  return vsha2_rotr(value, 1, width) ^ vsha2_rotr(value, 8, width) ^ (value >> 7);
}

static inline uint64_t vsha2_sig1(uint64_t value, int width) {
  if (width == 32) {
    return vsha2_rotr(value, 17, width) ^ vsha2_rotr(value, 19, width) ^ (value >> 10);
  }
  return vsha2_rotr(value, 19, width) ^ vsha2_rotr(value, 61, width) ^ (value >> 6);
}

static inline uint64_t vsha2_sum0(uint64_t value, int width) {
  if (width == 32) {
    return vsha2_rotr(value, 2, width) ^ vsha2_rotr(value, 13, width) ^ vsha2_rotr(value, 22, width);
  }
  return vsha2_rotr(value, 28, width) ^ vsha2_rotr(value, 34, width) ^ vsha2_rotr(value, 39, width);
}

static inline uint64_t vsha2_sum1(uint64_t value, int width) {
  if (width == 32) {
    return vsha2_rotr(value, 6, width) ^ vsha2_rotr(value, 11, width) ^ vsha2_rotr(value, 25, width);
  }
  return vsha2_rotr(value, 14, width) ^ vsha2_rotr(value, 18, width) ^ vsha2_rotr(value, 41, width);
}

static inline uint64_t vsha2_ch(uint64_t x, uint64_t y, uint64_t z) {
  return (x & y) ^ (~x & z);
}

static inline uint64_t vsha2_maj(uint64_t x, uint64_t y, uint64_t z) {
  return (x & y) ^ (x & z) ^ (y & z);
}

static inline void vsha2_compress(const uint64_t dest[4], const uint64_t src1[4],
                                  const uint64_t src2[4], int word_base,
                                  int width, uint64_t result[4]) {
  uint64_t a = src2[3];
  uint64_t b = src2[2];
  uint64_t c = dest[3];
  uint64_t d = dest[2];
  uint64_t e = src2[1];
  uint64_t f = src2[0];
  uint64_t g = dest[1];
  uint64_t h = dest[0];

  for (int round = 0; round < 2; round++) {
    uint64_t t1 = vsha2_mask(h + vsha2_sum1(e, width) + vsha2_ch(e, f, g) +
                              src1[word_base + round], width);
    uint64_t t2 = vsha2_mask(vsha2_sum0(a, width) + vsha2_maj(a, b, c), width);
    h = g;
    g = f;
    f = e;
    e = vsha2_mask(d + t1, width);
    d = c;
    c = b;
    b = a;
    a = vsha2_mask(t1 + t2, width);
  }

  result[0] = f;
  result[1] = e;
  result[2] = b;
  result[3] = a;
}

static inline void vsha2_load_group(int reg, int base, uint64_t group[4]) {
  rtlreg_t value;
  for (int lane = 0; lane < 4; lane++) {
    get_vreg(reg, base + lane, &value, vtype->vsew, vtype->vlmul, 0, 1);
    group[lane] = value;
  }
}

void vsha2_exec(vsha2_op_t op, Decode *s) {
  require_vector(true);

  int width = 8 << vtype->vsew;
  double vflmul = compute_vflmul();
  if ((vl->val & 3) != 0 || (vstart->val & 3) != 0 || vflmul * VLEN < 4 * width) {
    longjmp_exception(EX_II);
  }

  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src->reg, vflmul);
  require_aligned(id_src2->reg, vflmul);

  int register_group_size = vflmul < 1 ? 1 : vflmul;
  require_noover(id_dest->reg, register_group_size, id_src->reg, register_group_size);
  require_noover(id_dest->reg, register_group_size, id_src2->reg, register_group_size);

  if (check_vstart_ignore(s)) {
    vp_set_dirty();
    return;
  }

  int egroups = vl->val / 4;
  for (int group = vstart->val / 4; group < egroups; group++) {
    int base = group * 4;
    uint64_t dest[4], src1[4], src2[4], result[4];
    vsha2_load_group(id_dest->reg, base, dest);
    vsha2_load_group(id_src->reg, base, src1);
    vsha2_load_group(id_src2->reg, base, src2);

    if (op == VSHA2_OP_MS) {
      result[0] = vsha2_mask(vsha2_sig1(src1[2], width) + src2[1] +
                              vsha2_sig0(dest[1], width) + dest[0], width);
      result[1] = vsha2_mask(vsha2_sig1(src1[3], width) + src2[2] +
                              vsha2_sig0(dest[2], width) + dest[1], width);
      result[2] = vsha2_mask(vsha2_sig1(result[0], width) + src2[3] +
                              vsha2_sig0(dest[3], width) + dest[2], width);
      result[3] = vsha2_mask(vsha2_sig1(result[1], width) + src1[0] +
                              vsha2_sig0(src2[0], width) + dest[3], width);
    } else {
      int word_base = op == VSHA2_OP_CH ? 2 : 0;
      vsha2_compress(dest, src1, src2, word_base, width, result);
    }

    for (int lane = 0; lane < 4; lane++) {
      set_vreg(id_dest->reg, base + lane, result[lane],
               vtype->vsew, vtype->vlmul, 1);
    }
  }

  if (RVV_AGNOSTIC && vtype->vta) {
    int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
    for (int idx = egroups * 4; idx < vlmax; idx++) {
      set_vreg(id_dest->reg, idx, UINT64_MAX,
               vtype->vsew, vtype->vlmul, 1);
    }
  }

  vstart->val = 0;
  vp_set_dirty();
}

#endif // CONFIG_RV_ZVKNHA || CONFIG_RV_ZVKNHB

static inline void require_vector_vs() {
  if (mstatus->vs == 0) {
    longjmp_exception(EX_II);
  }
  #ifdef CONFIG_RVH
  if (cpu.v && vsstatus->vs == 0) {
    longjmp_exception(EX_II);
  }
  #endif
}

void require_float() {
  if (mstatus->fs == 0) {
    longjmp_exception(EX_II);
  }
#ifdef CONFIG_RVH
  if (cpu.v && vsstatus->fs == 0) {
    longjmp_exception(EX_II);
  }
#endif
}

static inline bool is_overlapped(const int astart, int asize, const int bstart, int bsize) {
  asize = asize == 0 ? 1 : asize;
  bsize = bsize == 0 ? 1 : bsize;

  const int aend = astart + asize;
  const int bend = bstart + bsize;

  const int min_start = astart < bstart ? astart : bstart;
  const int max_end = aend > bend ? aend : bend;

  return max_end - min_start < asize + bsize;
}

static inline bool is_overlapped_widen(const int astart, int asize, const int bstart, int bsize) {
  asize = asize == 0 ? 1 : asize;
  bsize = bsize == 0 ? 1 : bsize;

  const int aend = astart + asize;
  const int bend = bstart + bsize;

  const int min_start = astart < bstart ? astart : bstart;
  const int max_end = aend > bend ? aend : bend;

  if (astart < bstart &&
      is_overlapped(astart, asize, bstart, bsize) &&
      !is_overlapped(astart, asize, bstart + bsize, bsize)) {
    return false;
  } else  {
    return max_end - min_start < asize + bsize;
  }
}

void require_noover(const int astart, int asize, const int bstart, int bsize) {
  if (is_overlapped(astart, asize, bstart, bsize)) {
    longjmp_exception(EX_II);
  }
}

void require_noover_widen(const int astart, int asize, const int bstart, int bsize) {
  if (is_overlapped_widen(astart, asize, bstart, bsize)) {
    longjmp_exception(EX_II);
  }
}

void require_vm(Decode *s) {
  if (s->vm == 0 && id_dest->reg == 0) {
    longjmp_exception(EX_II);
  }
}

static inline bool is_aligned(const unsigned val, const unsigned pos) {
  return pos ? (val & (pos - 1)) == 0 : true;
}

void require_aligned(const unsigned val, const unsigned pos) {
  if (!is_aligned(val, pos)) {
    longjmp_exception(EX_II);
  }
}

double compute_vflmul() {
  double vflmul = 1.0;
  if (vtype->vlmul < 4) {
    vflmul = 1.0 * (1 << vtype->vlmul);
  } else if (vtype->vlmul > 4) {
    vflmul = 1.0 / (1 << (8 - vtype->vlmul));
  }
  return vflmul;
}

void require_vector(bool is_require_vtype) {
  require_vector_vs();
  if (is_require_vtype && vtype->vill != 0) {
    longjmp_exception(EX_II);
  }
}

void vector_narrow_check(Decode *s) {
  require_vector(true);
  double vflmul = compute_vflmul();
  if (vflmul > 4) {
    longjmp_exception(EX_II);
  }
  if (vtype->vsew > 2) {
    longjmp_exception(EX_II);
  }
  require_aligned(id_src2->reg, vflmul * 2);
  require_aligned(id_dest->reg, vflmul);
  require_vm(s);
}

void vector_wide_check(Decode *s) {
  require_vector(true);
  double vflmul = compute_vflmul();
  if (vflmul > 4) {
    longjmp_exception(EX_II);
  }
  if (vtype->vsew > 2) {
    longjmp_exception(EX_II);
  }
  require_aligned(id_dest->reg, vflmul * 2);
  require_vm(s);
}

void vector_mvv_check(Decode *s, bool is_vs1) {
  require_vector(true);
  double vflmul = compute_vflmul();
  if (id_dest->reg != id_src2->reg) {
    require_noover(id_dest->reg, 1, id_src2->reg, vflmul);
  }
  require_aligned(id_src2->reg, vflmul);
  if (is_vs1) {
    if (id_dest->reg != id_src->reg) {
      require_noover(id_dest->reg, 1, id_src->reg, vflmul);
    }
    require_aligned(id_src->reg, vflmul);
  }
}

void vector_vvv_check(Decode *s, bool is_vs1) {
  require_vector(true);
  require_vm(s);
  double vflmul = compute_vflmul();
  if (vflmul > 1) {
    require_aligned(id_dest->reg, vflmul);
    require_aligned(id_src2->reg, vflmul);
    if (is_vs1) {
      require_aligned(id_src->reg, vflmul);
    }
  }
}

void vector_wvv_check(Decode *s, bool is_vs1) {
  vector_wide_check(s);
  double vflmul = compute_vflmul();
  require_aligned(id_src2->reg, vflmul);
  if (vflmul < 1) {
    require_noover(id_dest->reg, vflmul * 2, id_src2->reg, vflmul);
  } else {
    require_noover_widen(id_dest->reg, vflmul * 2, id_src2->reg, vflmul);
  }
  if (is_vs1) {
    require_aligned(id_src->reg, vflmul);
    if (vflmul < 1) {
      require_noover(id_dest->reg, vflmul * 2, id_src->reg, vflmul);
    } else {
      require_noover_widen(id_dest->reg, vflmul * 2, id_src->reg, vflmul);
    }
  }
}

void vector_wwv_check(Decode *s, bool is_vs1) {
  vector_wide_check(s);
  double vflmul = compute_vflmul();
  require_aligned(id_src2->reg, vflmul * 2);
  if (is_vs1) {
    require_aligned(id_src->reg, vflmul);
    if (vflmul < 1) {
      require_noover(id_dest->reg, vflmul * 2, id_src->reg, vflmul);
    } else {
      require_noover_widen(id_dest->reg, vflmul * 2, id_src->reg, vflmul);
    }
  }
}

void vector_vwv_check(Decode *s, bool is_vs1) {
  vector_narrow_check(s);
  double vflmul = compute_vflmul();
  if (id_dest->reg != id_src2->reg) {
    require_noover(id_dest->reg, vflmul, id_src2->reg, vflmul * 2);
  }
  if (is_vs1) {
    require_aligned(id_src->reg, vflmul);
  }
}

void vector_reduction_check(Decode *s, bool is_wide) {
  require_vector(true);
  if (is_wide) {
    if (vtype->vsew > 2) {
      longjmp_exception(EX_II);
    }
  }
  double vflmul = compute_vflmul();
  require_aligned(id_src2->reg, vflmul);
}

void vector_slide_check(Decode *s, bool is_over) {
  require_vector(true);
  double vflmul = compute_vflmul();
  require_aligned(id_src2->reg, vflmul);
  require_aligned(id_dest->reg, vflmul);
  require_vm(s);
  if (is_over) {
    if (id_dest->reg == id_src2->reg) {
      longjmp_exception(EX_II);
    }
  }
}

#ifdef CONFIG_RV_ZVKNED

static inline void aes_subbytes(uint32_t state[4], bool encrypt) {
  const uint8_t *sbox = encrypt ? AES_ENC_SBOX : AES_DEC_SBOX;
  for (int c = 0; c < 4; c++) {
    uint32_t col = state[c];
    state[c] = ((uint32_t)sbox[(col >>  0) & 0xFF] <<  0) |
               ((uint32_t)sbox[(col >>  8) & 0xFF] <<  8) |
               ((uint32_t)sbox[(col >> 16) & 0xFF] << 16) |
               ((uint32_t)sbox[(col >> 24) & 0xFF] << 24);
  }
}

static inline void aes_shiftrows(uint32_t state[4]) {
  uint8_t b[4][4];
  for (int c = 0; c < 4; c++)
    for (int r = 0; r < 4; r++)
      b[r][c] = (state[c] >> (8*r)) & 0xFF;

  uint8_t tmp;
  tmp = b[1][0]; b[1][0] = b[1][1]; b[1][1] = b[1][2]; b[1][2] = b[1][3]; b[1][3] = tmp;
  tmp = b[2][0]; b[2][0] = b[2][2]; b[2][2] = tmp;
  tmp = b[2][1]; b[2][1] = b[2][3]; b[2][3] = tmp;
  tmp = b[3][3]; b[3][3] = b[3][2]; b[3][2] = b[3][1]; b[3][1] = b[3][0]; b[3][0] = tmp;

  for (int c = 0; c < 4; c++)
    state[c] = ((uint32_t)b[0][c] <<  0) | ((uint32_t)b[1][c] <<  8) |
               ((uint32_t)b[2][c] << 16) | ((uint32_t)b[3][c] << 24);
}

static inline void aes_invshiftrows(uint32_t state[4]) {
  uint8_t b[4][4];
  for (int c = 0; c < 4; c++)
    for (int r = 0; r < 4; r++)
      b[r][c] = (state[c] >> (8*r)) & 0xFF;

  uint8_t tmp;
  tmp = b[1][3]; b[1][3] = b[1][2]; b[1][2] = b[1][1]; b[1][1] = b[1][0]; b[1][0] = tmp;
  tmp = b[2][0]; b[2][0] = b[2][2]; b[2][2] = tmp;
  tmp = b[2][1]; b[2][1] = b[2][3]; b[2][3] = tmp;
  tmp = b[3][0]; b[3][0] = b[3][1]; b[3][1] = b[3][2]; b[3][2] = b[3][3]; b[3][3] = tmp;

  for (int c = 0; c < 4; c++)
    state[c] = ((uint32_t)b[0][c] <<  0) | ((uint32_t)b[1][c] <<  8) |
               ((uint32_t)b[2][c] << 16) | ((uint32_t)b[3][c] << 24);
}

static inline void aes_mixcolumns(uint32_t state[4], bool inverse) {
  for (int c = 0; c < 4; c++) {
    state[c] = inverse ? AES_INVMIXCOLUMN(state[c]) : AES_MIXCOLUMN(state[c]);
  }
}

static inline void aes_addroundkey(uint32_t state[4], const uint32_t rk[4]) {
  for (int c = 0; c < 4; c++)
    state[c] ^= rk[c];
}

#endif // CONFIG_RV_ZVKNED

void arithmetic_instr(int opcode, int is_signed, int widening, int narrow, int dest_mask, Decode *s) {
  require_vector(true);
  int vlmax = get_vlmax(vtype->vsew, vtype->vlmul);
  uint64_t carry;
  bool sat = false;
  bool overflow;
  int shift;
  unsigned narrow_shift;
  uint128_t u128_result;
  int128_t i128_result;
  int sew = 8 << vtype->vsew;
  int64_t int_max = ((uint64_t) INT64_MAX) >> (64 - sew);
  int64_t int_min = ((int64_t) INT64_MIN) >> (64 - sew);
  uint64_t uint_max = ((uint64_t) UINT64_MAX) >> (64 - sew);
  uint64_t sign_mask = ((uint64_t) UINT64_MAX) << sew;
  uint64_t lshift = 0;
  uint64_t rshift = 0;
  uint64_t shift_mask = -1;
  int i = 0;
  if (widening == 0 && narrow == 0 && dest_mask == 0) {
    if (s->src_vmode == SRC_VV) {
      vector_vvv_check(s, true);
    } else {
      vector_vvv_check(s, false);
    }
  } else if (dest_mask == 1) {
    if (s->src_vmode == SRC_VV) {
      vector_mvv_check(s, true);
    } else {
      vector_mvv_check(s, false);
    }
  } else if (widening == 1 && narrow == 1) {
    // e.g. vwadd_wv
    if (s->src_vmode == SRC_VV) {
      vector_wwv_check(s, true);
    } else {
      vector_wwv_check(s, false);
    }
  } else if (widening == 1) {
    // e.g. vwadd_vv
    if (s->src_vmode == SRC_VV) {
      vector_wvv_check(s, true);
    } else {
      vector_wvv_check(s, false);
    }
  } else if (narrow == 1) {
    if (s->src_vmode == SRC_VV) {
      vector_vwv_check(s, true);
    } else {
      vector_vwv_check(s, false);
    }
  } else if (narrow < 0) {
    if (vtype->vsew + narrow < 0) {
      longjmp_exception(EX_II);
    }
  }
#ifdef CONFIG_RV_ZVKG
  if (opcode == VGHSH || opcode == VGMUL) {
    vgcm_instr(opcode, s);
    return;
  }
#endif
  check_vstart_exception(s);
  if(check_vstart_ignore(s)) {
    vp_set_dirty();
    return;
  }
  for(word_t idx = vstart->val; idx < vl->val; idx ++) {
    // mask
    rtlreg_t mask = get_mask(0, idx);
    carry = 0;
    if(s->vm == 0) {
      carry = mask;
      // merge instr will exec no matter mask or not
      // masked and mask off exec will left dest unmodified.
      if(opcode != MERGE \
        && opcode != ADC \
        && opcode != MADC \
        && opcode != SBC \
        && opcode != MSBC \
        && opcode != SLIDEUP \
        && mask==0) {
        if (RVV_AGNOSTIC) {
            if (dest_mask == 1) {
              if (vtype->vma) {
                set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
              }
              continue;
            }
            if (vtype->vma) {
              *s1 = (uint64_t) -1;
              set_vreg(id_dest->reg, idx, *s1, vtype->vsew+widening, vtype->vlmul, 1);
            }
          }
          continue;
        }

    }
    if(s->vm == 1 && opcode == MERGE) {
      mask = 1; // merge(mv) get the first operand (s1, rs1, imm);
    }

    int eew;
    int emul;
    /*
    The vector integer extension instructions zero- or sign-extend a source vector
    integer operand with EEW less than SEW to ll SEW-sized elements in the
    destination. The EEW of the source is 1/2, 1/4, or 1/8 of SEW, while EMUL of the
    source is (EEW/SEW)*LMUL. The destination has EEW equal to SEW and EMUL equal to
    LMUL.
    */
    switch (opcode) {
      case VEXT:
        eew = vtype->vsew + narrow;
        emul = vtype->vlmul + narrow;
        break;
      default:
        eew = vtype->vsew + narrow;
        emul = vtype->vlmul;
        break;
    }
    // operand - vs2
    get_vreg(id_src2->reg, idx, s0, eew, emul, is_signed, 1);
    if(is_signed) rtl_sext(s, s0, s0, 1 << (vtype->vsew+narrow));

    // operand - s1 / rs1 / imm
    switch (s->src_vmode) {
      case SRC_VV :
        /*
        The vrgather.vv form uses SEW/LMUL for both the data and indices. The
        vrgatherei16.vv form uses SEW/LMUL for the data in vs2 but EEW=16 and EMUL =
        (16/SEW)*LMUL for the indices in vs1.
        */
        switch (opcode) {
          case RGATHEREI16:
            eew = 1;
            emul = vtype->vlmul - (vtype->vsew - 1);
            break;
          default:
            eew = vtype->vsew;
            emul = vtype->vlmul;
            break;
        }
        get_vreg(id_src->reg, idx, s1, eew, emul, is_signed, 1);
        if(is_signed) rtl_sext(s, s1, s1, 1 << vtype->vsew);
        break;
      case SRC_VX :   
        rtl_lr(s, &(id_src->val), id_src1->reg, 4);
        rtl_mv(s, s1, &id_src->val); 
        if(opcode != RGATHER && opcode != RGATHEREI16 && opcode != SLIDEUP && opcode != SLIDEDOWN) {
          switch (vtype->vsew) {
            case 0 : *s1 = *s1 & 0xff; break;
            case 1 : *s1 = *s1 & 0xffff; break;
            case 2 : *s1 = *s1 & 0xffffffff; break;
            case 3 : *s1 = *s1 & 0xffffffffffffffff; break;
          }
          if(is_signed) rtl_sext(s, s1, s1, 1 << vtype->vsew);
        }
        break;
      case SRC_VI :
        shift_mask = 0x1f;
        if(is_signed) {
          if (opcode == NCLIP) {
            // vnclip use unsigned imm, signed vs2
            rtl_li(s, s1, s->isa.instr.v_opimm.v_imm5);
          } else {
            rtl_li(s, s1, s->isa.instr.v_opsimm.v_simm5);
          }
        } else {
          if (opcode == MSLEU || opcode == MSGTU || opcode == SADDU) {
            rtl_li(s, s1, s->isa.instr.v_opsimm.v_simm5);
            switch (vtype->vsew) {
              case 0 : *s1 = *s1 & 0xff; break;
              case 1 : *s1 = *s1 & 0xffff; break;
              case 2 : *s1 = *s1 & 0xffffffff; break;
              case 3 : *s1 = *s1 & 0xffffffffffffffff; break;
            }
          } else if (opcode == ROR) {
            // imm for vror_v.vi has 6 bits
            rtl_li(s, s1, s->isa.instr.v_opimm.v_imm5);
            rtl_li(s, s2, s->isa.instr.v_opimm.v_i);
            * s1 |= *s2 << 5;
          }
          else
            rtl_li(s, s1, s->isa.instr.v_opimm.v_imm5);
        }       
        break;
    }

    shift = *s1 & (sew - 1) & shift_mask;
    narrow_shift = *s1 & (sew * 2 - 1);
    lshift = *s1 & (sew - 1);
    rshift = (-lshift) & (sew - 1);

    if (opcode == SLIDEUP) {
      if(s->vm == 0 && mask == 0 && (uint64_t)idx >= (uint64_t)*s1) {
        if (RVV_AGNOSTIC && vtype->vma) {
          *s2 = (uint64_t) -1;
          set_vreg(id_dest->reg, idx, *s2, vtype->vsew+widening, vtype->vlmul, 1);
        }
        continue;
      }
    }

    // op
    switch (opcode) {
      case ADD : rtl_add(s, s1, s0, s1); break;
      case SUB : rtl_sub(s, s1, s0, s1); break;
      case RSUB: rtl_sub(s, s1, s1, s0); break;
      case AND : rtl_and(s, s1, s0, s1); break;
      case OR  : rtl_or(s, s1, s0, s1); break;
      case XOR : rtl_xor(s, s1, s0, s1); break;
      case MIN : rtl_min(s, s1, s0, s1); break;
      case MAX : rtl_max(s, s1, s0, s1); break;
      case MINU: rtl_minu(s, s1, s0, s1); break;
      case MAXU: rtl_maxu(s, s1, s0, s1); break;
      case VEXT: rtl_mv(s, s1, s0); break;
      case ADC : 
        rtl_add(s, s1, s0, s1);
        rtl_li(s, s2, mask);
        rtl_add(s, s1, s1, s2); break;
      case SBC : 
        rtl_sub(s, s1, s0, s1);
        rtl_li(s, s2, mask);
        rtl_sub(s, s1, s1, s2); break;
      case MADC:
        for (i = 0; i < (8 << vtype->vsew); i++) {
            carry = (((*s0 >> i) & 1) + ((*s1 >> i) & 1) + carry) >> 1;
            carry &= 1;
        }
        rtl_li(s, s1, carry);
        break;
      case MSBC:
        for (i = 0; i < (8 << vtype->vsew); i++) {
          carry = ((~(*s0 >> i) & 1) + ((*s1 >> i) & 1) + carry) >> 1;
          carry &= 1;
        }
        rtl_li(s, s1, carry);
        break;
      case SLL :
        if (widening)
            rtl_andi(s, s1, s1, (16 << vtype->vsew)-1); //low lg2(SEW*2)
        else
            rtl_andi(s, s1, s1, (8 << vtype->vsew)-1); //low lg2(SEW)
        rtl_shl(s, s1, s0, s1); break;
      case SRL :
        if (narrow)
            rtl_andi(s, s1, s1, (16 << vtype->vsew)-1); //low lg2(SEW*2)
        else
            rtl_andi(s, s1, s1, (8 << vtype->vsew)-1); //low lg2(SEW)
        rtl_shr(s, s1, s0, s1); break;
      case SRA :
        if (narrow) {
            rtl_andi(s, s1, s1, (16 << vtype->vsew)-1); //low lg2(SEW*2)
            rtl_sext(s, s0, s0, 2 << vtype->vsew);
        }
        else {
            rtl_andi(s, s1, s1, (8 << vtype->vsew)-1); //low lg2(SEW)
            rtl_sext(s, s0, s0, 1 << vtype->vsew);
        }
        rtl_sar(s, s1, s0, s1); break;
      case MULHU : 
        *s1 = (uint64_t)(((__uint128_t)(*s0) * (__uint128_t)(*s1))>>(8 << vtype->vsew));
        break;
      case MUL : rtl_mulu_lo(s, s1, s0, s1); break;
      case MULSU : 
        rtl_sext(s, s0, s0, 1 << vtype->vsew);
        rtl_mulu_lo(s, s1, s0, s1); break;
      case MULHSU :
        rtl_sext(s, t0, s0, 1 << vtype->vsew);
        rtl_sari(s, t0, t0, (8 << vtype->vsew)-1);
        rtl_and(s, t0, s1, t0);
        *s1 = (uint64_t)(((__uint128_t)(*s0) * (__uint128_t)(*s1))>>(8 << vtype->vsew));
        rtl_sub(s, s1, s1, t0);
        break;
      case MULH :
        rtl_sext(s, s0, s0, 1 << vtype->vsew);
        rtl_sext(s, s1, s1, 1 << vtype->vsew);
        *s1 = (uint64_t)(((__int128_t)(sword_t)(*s0) * (__int128_t)(sword_t)(*s1))>>(8 << vtype->vsew));
        break;
      case MACC : 
        rtl_mulu_lo(s, s1, s0, s1);
        get_vreg(id_dest->reg, idx, s0, vtype->vsew+widening, vtype->vlmul, is_signed, 1);
        rtl_add(s, s1, s1, s0);
        break;
      case MACCSU :
        rtl_sext(s, s1, s1, 1 << vtype->vsew);
        rtl_mulu_lo(s, s1, s0, s1);
        get_vreg(id_dest->reg, idx, s0, vtype->vsew+widening, vtype->vlmul, is_signed, 1);
        rtl_add(s, s1, s1, s0);
        break;
      case MACCUS :
        rtl_sext(s, s0, s0, 1 << vtype->vsew);
        rtl_mulu_lo(s, s1, s0, s1);
        get_vreg(id_dest->reg, idx, s0, vtype->vsew+widening, vtype->vlmul, is_signed, 1);
        rtl_add(s, s1, s1, s0);
        break;
      case NMSAC :
        rtl_mulu_lo(s, s1, s0, s1);
        get_vreg(id_dest->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
        rtl_sub(s, s1, s0, s1);
        break;
      case MADD :
        get_vreg(id_dest->reg, idx, s2, vtype->vsew, vtype->vlmul, is_signed, 1);
        rtl_mulu_lo(s, s1, s1, s2);
        rtl_add(s, s1, s1, s0);
        break;
      case NMSUB :
        get_vreg(id_dest->reg, idx, s2, vtype->vsew, vtype->vlmul, is_signed, 1);
        rtl_mulu_lo(s, s1, s2, s1);
        rtl_sub(s, s1, s0, s1);
        break;
      case DIVU :
        if(*s1 == 0) rtl_li(s, s1, ~0lu);
        else rtl_divu_q(s, s1, s0, s1);
        break;
      case DIV :
        rtl_sext(s, s0, s0, 1 << vtype->vsew);
        rtl_sext(s, s1, s1, 1 << vtype->vsew);
        if(*s1 == 0) rtl_li(s, s1, ~0lu);
        else if(*s0 == 0x8000000000000000LL && *s1 == (word_t)-1) //may be error
          rtl_mv(s, s1, s0);
        else rtl_divs_q(s, s1, s0, s1);
        break;
      case REMU :
        if (*s1 == 0) rtl_mv(s, s1, s0);
        else rtl_divu_r(s, s1, s0, s1);
        break;
      case REM :
        rtl_sext(s, s0, s0, 1 << vtype->vsew);
        rtl_sext(s, s1, s1, 1 << vtype->vsew);
        if(*s1 == 0) rtl_mv(s, s1, s0);
        else if(*s1 == 0x8000000000000000LL && *s1 == (word_t)-1) //may be error
          rtl_li(s, s1, 0);
        else rtl_divs_r(s, s1, s0, s1);
        break;
      case MERGE : rtl_mux(s, s1, &mask, s1, s0); break;
      case MSEQ  : rtl_setrelop(s, RELOP_EQ,  s1, s0, s1); break;
      case MSNE  : rtl_setrelop(s, RELOP_NE,  s1, s0, s1); break;
      case MSLTU : rtl_setrelop(s, RELOP_LTU, s1, s0, s1); break;
      case MSLT  : rtl_setrelop(s, RELOP_LT,  s1, s0, s1); break;
      case MSLEU : rtl_setrelop(s, RELOP_LEU, s1, s0, s1); break;
      case MSLE  : rtl_setrelop(s, RELOP_LE,  s1, s0, s1); break;
      case MSGTU : rtl_setrelop(s, RELOP_GTU, s1, s0, s1); break;
      case MSGT  : rtl_setrelop(s, RELOP_GT,  s1, s0, s1); break;
      case SLIDEUP :
        if ((uint64_t)idx >= (uint64_t)*s1) get_vreg(id_src2->reg, idx - *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        else get_vreg(id_dest->reg, idx, s1, vtype->vsew, vtype->vlmul, 0, 1);
        break;
      case SLIDEDOWN :// idx + s1 should be prevented from overflowing and thus failing the judgment
        // Data manipulation is forbidden when vl is 0
        if (vl->val != 0) {
          if ((uint128_t)idx + (uint128_t)(uint64_t)*s1 < (uint128_t)vlmax)
            get_vreg(id_src2->reg, idx + *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
          else
            rtl_li(s, s1, 0);
        }
        break;
      case SLIDE1UP :
        if (idx > 0) get_vreg(id_src2->reg, idx - 1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        break;
      case SLIDE1DOWN :
        if (idx < vl->val - 1) get_vreg(id_src2->reg, idx + 1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        break;
      case RGATHER :
        if ((uint64_t)*s1 < (uint64_t)vlmax) get_vreg(id_src2->reg, *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        else rtl_li(s, s1, 0);
        break;
      case RGATHEREI16 :
        /*
        The vrgather.vv form uses SEW/LMUL for both the data and indices. The
        vrgatherei16.vv form uses SEW/LMUL for the data in vs2 but EEW=16 and EMUL =
        (16/SEW)*LMUL for the indices in vs1.
        */
        eew = 1;
        emul = vtype->vlmul-(vtype->vsew-1);
        get_vreg(id_src1->reg, idx, s1, eew, emul, 0, 1);
        *s1 = *s1 & 0xffff;
        if ((uint64_t)*s1 < (uint64_t)vlmax) get_vreg(id_src2->reg, *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        else rtl_li(s, s1, 0);
        break;
      case SADDU :
        *s2 = *s1;
        rtl_add(s, s1, s0, s1);
        sat = 0;
        carry = 0;
        for (i = 0; i < (8 << vtype->vsew); i++) {
            carry = (((*s0 >> i) & 1) + ((*s2 >> i) & 1) + carry) >> 1;
            carry &= 1;
        }
        if (carry) {rtl_li(s, s1, ~0lu); sat = 1;}
        vxsat->val |= sat;
        break;
      case SSUBU :
        switch (vtype->vsew) {
          case 0 : *s1 = SAT_SUBU(uint8_t, *s0, *s1, &sat); break;
          case 1 : *s1 = SAT_SUBU(uint16_t, *s0, *s1, &sat); break;
          case 2 : *s1 = SAT_SUBU(uint32_t, *s0, *s1, &sat); break;
          case 3 : *s1 = SAT_SUBU(uint64_t, *s0, *s1, &sat); break;
        }
        vxsat->val |= sat;
        break;
      case SADD :
        switch (vtype->vsew) {
          case 0 : *s1 = SAT_ADD(int8_t, uint8_t, *s0, *s1, &sat); break;
          case 1 : *s1 = SAT_ADD(int16_t, uint16_t, *s0, *s1, &sat); break;
          case 2 : *s1 = SAT_ADD(int32_t, uint32_t, *s0, *s1, &sat); break;
          case 3 : *s1 = SAT_ADD(int64_t, uint64_t, *s0, *s1, &sat); break;
        }
        vxsat->val |= sat;
        break;
      case SSUB :
        switch (vtype->vsew) {
          case 0 : *s1 = SAT_SUB(int8_t, uint8_t, *s0, *s1, &sat); break;
          case 1 : *s1 = SAT_SUB(int16_t, uint16_t, *s0, *s1, &sat); break;
          case 2 : *s1 = SAT_SUB(int32_t, uint32_t, *s0, *s1, &sat); break;
          case 3 : *s1 = SAT_SUB(int64_t, uint64_t, *s0, *s1, &sat); break;
        }
        vxsat->val |= sat;
        break;
      case AADD : ROUNDING(+, *s1, *s0, *s1, is_signed); break;
      case ASUB : ROUNDING(-, *s1, *s0, *s1, is_signed); break;
      case SMUL :
        overflow = *s1 == *s0 && *s1 == (word_t)int_min;
        i128_result = (int128_t)(int64_t)*s1 * (int128_t)(int64_t)*s0;
        INT_ROUNDING(i128_result, vxrm->val, sew - 1);
        i128_result = i128_result >> (sew - 1);
        if (overflow) { *s1 = int_max; vxsat->val |= 1; }
        else *s1 = (int64_t)i128_result;
        break;
      case SSRL :
        u128_result = *s0;
        INT_ROUNDING(u128_result, vxrm->val, shift);
        *s1 = (uint64_t)(u128_result >> shift);
        break;
      case SSRA :
        i128_result = (int128_t)(int64_t)*s0;
        INT_ROUNDING(i128_result, vxrm->val, shift);
        *s1 = (uint64_t)(i128_result >> shift);
        break;
      case NCLIP :
        i128_result = (int128_t)(int64_t)*s0;
        INT_ROUNDING(i128_result, vxrm->val, narrow_shift);
        i128_result >>= narrow_shift;
        if (i128_result > int_max) { i128_result = int_max; vxsat->val |= 1; }
        else if (i128_result < int_min) { i128_result = int_min; vxsat->val |= 1; }
        *s1 = (int64_t)i128_result;
        break;
      case NCLIPU :
        u128_result = *s0;
        INT_ROUNDING(u128_result, vxrm->val, narrow_shift);
        u128_result >>= narrow_shift;
        if (u128_result & sign_mask) {
          u128_result = uint_max;
          vxsat->val |= 1;
        }
        *s1 = (uint64_t) u128_result;
        break;
      case ANDN :
        rtl_not(s, s1, s1);
        rtl_and(s, s1, s0, s1);
        break;
      case BREV_V :
        reverse_byte_bits(s0);
        reverse_nbytes(s0, vtype->vsew);
        *s1 = *s0;
        break;
      case BREV8_V :
        reverse_byte_bits(s0);
        *s1 = *s0;
        break;
      case REV8_V :
        reverse_nbytes(s0, vtype->vsew);
        *s1 = *s0;
        break;
      case CLZ_V :
        i = 0;
        for (; i < sew; i++) {
          if ((*s0 >> (sew - 1 - i)) & 1) break;
        }
        *s1 = i;
        break;
      case CTZ_V :
        i = 0;
        for (; i < sew; i++) {
          if ((*s0 >> i) & 1) break;
        }
        *s1 = i;
        break;
      case CPOP_V :
        *s1 = 0;
        for (i = 0; i < sew; i++) {
          if ((*s0 >> i) & 1) (*s1)++;
        }
        break;
      case ROL :
        *s1 = (*s0 << lshift) | (*s0 >> rshift);
        break;
      case ROR :
        *s1 = (*s0 >> lshift) | (*s0 << rshift);
        break;
#ifdef CONFIG_RV_ZVBC
      case VCLMUL :
        *s1 = _rv_clmul(*s0, *s1);
        break;
      case VCLMULH :
        *s1 = _rv_clmulh(*s0, *s1);
        break;
    }
    update_vcsr();
    // store to vrf
    if(dest_mask == 1) 
      set_mask(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul);
    else
      set_vreg(id_dest->reg, idx, *s1, vtype->vsew+widening, vtype->vlmul, 1);
  }

  if (RVV_AGNOSTIC) {
    if(vtype->vta) {
      int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, widening);
      for(int idx = vl->val; idx < vlmax; idx++) {
        if (dest_mask == 1)
          continue;
        *s1 = (uint64_t) -1;
        set_vreg(id_dest->reg, idx, *s1, vtype->vsew+widening, vtype->vlmul, 1);
      }
    }
    if(dest_mask) {
      for (int idx = vl->val; idx < VLEN; idx++) {
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
      }
    }
  }

  rtl_li(s, s0, 0);
  vcsr_write(IDXVSTART, s0);
  vp_set_dirty();
}

#ifdef CONFIG_RV_ZVKNED

void vaes_exec(vaes_op_t op, Decode *s) {
  require_vector(true);
  uint8_t rnum = 0;
  if (op == VAES_OP_VAESKF1 || op == VAES_OP_VAESKF2) {
    rnum = s->isa.instr.v_opv.v_vs1 & 0xF;
  }

  double vflmul = compute_vflmul();
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vflmul);

  check_vstart_exception(s);
  if (check_vstart_ignore(s)) {
    vp_set_dirty();
    return;
  }

  int egroups = vl->val / 4;

  for (int g = vstart->val / 4; g < egroups; g++) {
    int base = g * 4;

    bool all_masked = true;
    if (s->vm == 0) {
      for (int k = 0; k < 4; k++) {
        if (get_mask(0, base + k)) { all_masked = false; break; }
      }
      if (all_masked) continue;
    }

    bool is_vs = (s->isa.instr.v_opv.v_funct6 & 1)
                 && (op != VAES_OP_VAESKF1 && op != VAES_OP_VAESKF2);

    uint32_t state[4], rk[4];

    if (op == VAES_OP_VAESKF1) {
      for (int k = 0; k < 4; k++) {
        get_vreg(id_src2->reg, base + k, s0, vtype->vsew, vtype->vlmul, 0, 1);
        state[k] = (uint32_t)*s0;
      }
    } else if (op == VAES_OP_VAESKF2) {
      for (int k = 0; k < 4; k++) {
        get_vreg(id_dest->reg, base + k, s0, vtype->vsew, vtype->vlmul, 0, 1);
        state[k] = (uint32_t)*s0;
      }
      for (int k = 0; k < 4; k++) {

    get_vreg(id_src2->reg, base + k, s1, vtype->vsew, vtype->vlmul, 0, 1);
        rk[k] = (uint32_t)*s1;
      }
    } else {
      for (int k = 0; k < 4; k++) {
        get_vreg(id_dest->reg, base + k, s0, vtype->vsew, vtype->vlmul, 0, 1);
        state[k] = (uint32_t)*s0;
      }
      for (int k = 0; k < 4; k++) {
        get_vreg(id_src2->reg, is_vs ? k : base + k, s1, vtype->vsew, vtype->vlmul, 0, 1);
        rk[k] = (uint32_t)*s1;
      }
    }

    switch (op) {
      case VAES_OP_VAESZ:
        // just XOR state with round key
        aes_addroundkey(state, rk);
        break;

      case VAES_OP_VAESEF:
        // ShiftRows + SubBytes + AddRoundKey
        aes_shiftrows(state);
        aes_subbytes(state, true);
        aes_addroundkey(state, rk);
        break;

      case VAES_OP_VAESEM:
        // ShiftRows + SubBytes + MixColumns + AddRoundKey
        aes_shiftrows(state);
        aes_subbytes(state, true);
        aes_mixcolumns(state, false);
        aes_addroundkey(state, rk);
        break;

      case VAES_OP_VAESDF: {
        // InvShiftRows + InvSubBytes + AddRoundKey
        aes_invshiftrows(state);
        aes_subbytes(state, false);
        aes_addroundkey(state, rk);
        break;
      }

      case VAES_OP_VAESDM:
        // InvShiftRows + InvSubBytes + AddRoundKey + InvMixColumns
        aes_invshiftrows(state);
        aes_subbytes(state, false);
        aes_addroundkey(state, rk);
        aes_mixcolumns(state, true);
        break;

      case VAES_OP_VAESKF1: {
        static const uint8_t rcon[10] = {
          0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
        };
        uint8_t rnd = rnum;
        if (rnd == 0 || rnd > 10) {
          rnd ^= 0x8;  // invert bit 3 per spec
        }
        uint8_t rcon_idx = rnd - 1;

        uint32_t tmp = state[3];
        tmp = (tmp >> 8) | (tmp << 24);
        tmp = ((uint32_t)AES_ENC_SBOX[(tmp >>  0) & 0xFF] <<  0) |
              ((uint32_t)AES_ENC_SBOX[(tmp >>  8) & 0xFF] <<  8) |
              ((uint32_t)AES_ENC_SBOX[(tmp >> 16) & 0xFF] << 16) |
              ((uint32_t)AES_ENC_SBOX[(tmp >> 24) & 0xFF] << 24);
        tmp ^= rcon[rcon_idx];

        uint32_t new_rk[4];
        new_rk[0] = state[0] ^ tmp;
        new_rk[1] = state[1] ^ new_rk[0];
        new_rk[2] = state[2] ^ new_rk[1];
        new_rk[3] = state[3] ^ new_rk[2];
        for (int k = 0; k < 4; k++) state[k] = new_rk[k];
        break;
      }
      case VAES_OP_VAESKF2: {
        static const uint8_t rcon[10] = {
          0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
        };

        uint8_t rnd = rnum & 0xF;

        if (rnd < 2 || rnd > 14) {
          rnd ^= 0x8;
        }

        uint32_t tmp = rk[3];


        if ((rnd & 0x1) == 1) {
          // Even round: SubWord only (NO RotWord, NO Rcon)
          tmp = ((uint32_t)AES_ENC_SBOX[(tmp >>  0) & 0xFF] <<  0) |
                ((uint32_t)AES_ENC_SBOX[(tmp >>  8) & 0xFF] <<  8) |
                ((uint32_t)AES_ENC_SBOX[(tmp >> 16) & 0xFF] << 16) |
                ((uint32_t)AES_ENC_SBOX[(tmp >> 24) & 0xFF] << 24);
        } else {
          // Odd round: RotWord + SubWord + Rcon
          uint8_t rcon_idx = (rnd / 2) - 1;

          tmp = (tmp >> 8) | (tmp << 24);

          // SubWord
          tmp = ((uint32_t)AES_ENC_SBOX[(tmp >>  0) & 0xFF] <<  0) |
                ((uint32_t)AES_ENC_SBOX[(tmp >>  8) & 0xFF] <<  8) |
                ((uint32_t)AES_ENC_SBOX[(tmp >> 16) & 0xFF] << 16) |
                ((uint32_t)AES_ENC_SBOX[(tmp >> 24) & 0xFF] << 24);

          tmp ^= rcon[rcon_idx];
        }

        uint32_t new_rk[4];
        new_rk[0] = state[0] ^ tmp;
        new_rk[1] = state[1] ^ new_rk[0];
        new_rk[2] = state[2] ^ new_rk[1];
        new_rk[3] = state[3] ^ new_rk[2];

        for (int k = 0; k < 4; k++) {
          state[k] = new_rk[k];
        }
        break;
      }
    }

    for (int k = 0; k < 4; k++) {
      if (s->vm == 0 && get_mask(0, base + k) == 0) {
        if (RVV_AGNOSTIC && vtype->vma) {
          *s1 = (uint64_t)-1;
          set_vreg(id_dest->reg, base + k, *s1, vtype->vsew, vtype->vlmul, 1);
        }
        continue;
      }
      *s1 = state[k];
      set_vreg(id_dest->reg, base + k, *s1, vtype->vsew, vtype->vlmul, 1);
    }
  }

  if (RVV_AGNOSTIC && vtype->vta) {
    int vlmax_tail = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
    for (int idx = egroups * 4; idx < vlmax_tail; idx++) {
      *s1 = (uint64_t)-1;
      set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
    }
  }

  vstart->val = 0;
  vp_set_dirty();
}

#endif // CONFIG_RV_ZVKNED

#ifdef CONFIG_RV_ZVKSED

uint32_t sm4_subword(uint32_t B) {
  return (sm4_sbox[B >> 24] << 24 |
          sm4_sbox[(B >> 16) & 0xFF] << 16 |
          sm4_sbox[(B >> 8) & 0xFF] << 8 |
          sm4_sbox[B & 0xFF]);
}

uint32_t rol32(uint32_t x, uint32_t n) {
  return (x << n) | (x >> (32 - n));
}

uint32_t sm4_round_key(uint32_t x, uint32_t S) {
  return ((x) ^ (S ^ rol32(S, 13) ^ rol32(S, 23)));
}

uint32_t sm4_round(uint32_t x, uint32_t S) {
  return ((x) ^ ((S) ^ rol32(S, 2) ^ rol32(S, 10) ^ rol32(S, 18) ^ rol32(S, 24)));
}

void vsm_exec(vsm_op_t op, Decode *s) {
  require_vector(true);

  uint8_t rnum = 0;
  if (op == VSM_OP_VSM4K) {
    rnum = s->isa.instr.v_opv.v_vs1 & 0x7;
  }

  double vflmul = compute_vflmul();
  require_aligned(id_dest->reg, vflmul);
  require_aligned(id_src2->reg, vflmul);

  check_vstart_exception(s);
  if (check_vstart_ignore(s)) {
    vp_set_dirty();
    return;
  }

  int egroups = vl->val / 4;

  for (int g = vstart->val / 4; g < egroups; g++) {
    int base = g * 4;

    bool all_masked = true;
    if (s->vm == 0) {
      for (int k = 0; k < 4; k++) {
        if (get_mask(0, base + k)) { all_masked = false; break; }
      }
      if (all_masked) continue;
    }

    uint32_t state[8] = {0};
    uint32_t rk[8] = {0};

    if (op == VSM_OP_VSM4R) {
      for (int k = 0; k < 4; k++) {
        get_vreg(id_src2->reg, base + k, s0, vtype->vsew, vtype->vlmul, 0, 1);
        get_vreg(id_dest->reg, base + k, s1, vtype->vsew, vtype->vlmul, 0, 1);

        rk[k] = (uint32_t)*s0;
        state[k] = (uint32_t)*s1;
      }
    } else if (op == VSM_OP_VSM4K) {
      for (int k = 0; k < 4; k++) {
        get_vreg(id_src2->reg, base + k, s0, vtype->vsew, vtype->vlmul, 0, 1);

        rk[k] = (uint32_t)*s0;
      }

    } else {

    }

    switch (op) {
      case VSM_OP_VSM4R:
        for (int i = 0; i < 4; i++) {
          uint32_t B = state[i + 1] ^ state[i + 2] ^ state[i + 3] ^ rk[i];
          uint32_t S = sm4_subword(B);
          state[i + 4] = sm4_round(state[i], S);
        }
        break;
      case VSM_OP_VSM4K:
        for (int i = 0; i < 4; i++) {
          uint32_t B = rk[i + 1] ^ rk[i + 2] ^ rk[i + 3] ^ ck[4 * rnum + i];
          uint32_t S = sm4_subword(B);
          rk[i+4] = sm4_round_key(rk[i], S);
        }
        break;
    }


    for (int k = 0; k < 4; k++) {
      if (s->vm == 0 && get_mask(0, base + k) == 0) {
        if (RVV_AGNOSTIC && vtype->vma) {
          *s1 = (uint64_t)-1;
          set_vreg(id_dest->reg, base + k, *s1, vtype->vsew, vtype->vlmul, 1);
        }
        continue;
      }
      *s1 = op == VSM_OP_VSM4R ? state[k + 4] : rk[k + 4];
      set_vreg(id_dest->reg, base + k, *s1, vtype->vsew, vtype->vlmul, 1);
    }
  }

  if (RVV_AGNOSTIC && vtype->vta) {
    int vlmax_tail = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
    for (int idx = egroups * 4; idx < vlmax_tail; idx++) {
      *s1 = (uint64_t)-1;
      set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
    }
  }

  vstart->val = 0;
  vp_set_dirty();
}


#endif // CONFIG_RV_ZVKSED

void permutaion_instr(int opcode, Decode *s) {
  check_vstart_exception(s);
  if(check_vstart_ignore(s)) return;
  int vlmax = get_vlmax(vtype->vsew, vtype->vlmul);
  for(word_t idx = vstart->val; idx < vl->val; idx ++) {
    // mask
    rtlreg_t mask = get_mask(0, idx);
    if(s->vm == 0) {
      // merge instr will exec no matter mask or not
      // masked and mask off exec will left dest unmodified.
      if(opcode != SLIDEUP \
        && mask==0) {
        if (RVV_AGNOSTIC) {
            if (vtype->vma) {
              *s1 = (uint64_t) -1;
              set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
            }
          }
          continue;
        }

    }
    if(s->vm == 1 && opcode == MERGE) {
      mask = 1; // merge(mv) get the first operand (s1, rs1, imm);
    }

    int eew;
    int emul;
    /*
    The vector integer extension instructions zero- or sign-extend a source vector
    integer operand with EEW less than SEW to ll SEW-sized elements in the
    destination. The EEW of the source is 1/2, 1/4, or 1/8 of SEW, while EMUL of the
    source is (EEW/SEW)*LMUL. The destination has EEW equal to SEW and EMUL equal to
    LMUL.
    */
    eew = vtype->vsew;
    emul = vtype->vlmul;
    // operand - vs2
    get_vreg(id_src2->reg, idx, s0, eew, emul, 0, 1);

    // operand - s1 / rs1 / imm
    switch (s->src_vmode) {
      case SRC_VV :
        /*
        The vrgather.vv form uses SEW/LMUL for both the data and indices. The
        vrgatherei16.vv form uses SEW/LMUL for the data in vs2 but EEW=16 and EMUL =
        (16/SEW)*LMUL for the indices in vs1.
        */
        switch (opcode) {
          case RGATHEREI16:
            eew = 1;
            emul = vtype->vlmul - (vtype->vsew - 1);
            break;
          default:
            eew = vtype->vsew;
            emul = vtype->vlmul;
            break;
        }
        get_vreg(id_src->reg, idx, s1, eew, emul, 0, 1);
        break;
      case SRC_VX :   
        rtl_lr(s, &(id_src->val), id_src1->reg, 4);
        rtl_mv(s, s1, &id_src->val); 
        if(opcode != RGATHER && opcode != RGATHEREI16 && opcode != SLIDEUP && opcode != SLIDEDOWN) {
          switch (vtype->vsew) {
            case 0 : *s1 = *s1 & 0xff; break;
            case 1 : *s1 = *s1 & 0xffff; break;
            case 2 : *s1 = *s1 & 0xffffffff; break;
            case 3 : *s1 = *s1 & 0xffffffffffffffff; break;
          }
        }
        break;
      case SRC_VI :
        rtl_li(s, s1, s->isa.instr.v_opimm.v_imm5);       
        break;
    }

    if (opcode == SLIDEUP) {
      if(s->vm == 0 && mask == 0 && (uint64_t)idx >= (uint64_t)*s1) {
        if (RVV_AGNOSTIC && vtype->vma) {
          *s2 = (uint64_t) -1;
          set_vreg(id_dest->reg, idx, *s2, vtype->vsew, vtype->vlmul, 1);
        }
        continue;
      }
    }

    // op
    switch (opcode) {
      case SLIDEUP :
        if ((uint64_t)idx >= (uint64_t)*s1) get_vreg(id_src2->reg, idx - *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        else get_vreg(id_dest->reg, idx, s1, vtype->vsew, vtype->vlmul, 0, 1);
        break;
      case SLIDEDOWN :// idx + s1 should be prevented from overflowing and thus failing the judgment
        // Data manipulation is forbidden when vl is 0
        if (vl->val != 0) {
          if ((uint128_t)idx + (uint128_t)(uint64_t)*s1 < (uint128_t)vlmax)
            get_vreg(id_src2->reg, idx + *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
          else
            rtl_li(s, s1, 0);
        }
        break;
      case SLIDE1UP :
        if (idx > 0) get_vreg(id_src2->reg, idx - 1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        break;
      case SLIDE1DOWN :
        if (idx < vl->val - 1) get_vreg(id_src2->reg, idx + 1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        break;
      case RGATHER :
        if ((uint64_t)*s1 < (uint64_t)vlmax) get_vreg(id_src2->reg, *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        else rtl_li(s, s1, 0);
        break;
      case RGATHEREI16 :
        /*
        The vrgather.vv form uses SEW/LMUL for both the data and indices. The
        vrgatherei16.vv form uses SEW/LMUL for the data in vs2 but EEW=16 and EMUL =
        (16/SEW)*LMUL for the indices in vs1.
        */
        eew = 1;
        emul = vtype->vlmul-(vtype->vsew-1);
        get_vreg(id_src1->reg, idx, s1, eew, emul, 0, 1);
        *s1 = *s1 & 0xffff;
        if ((uint64_t)*s1 < (uint64_t)vlmax) get_vreg(id_src2->reg, *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        else rtl_li(s, s1, 0);
        break;
    }
    set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
  }

  if (RVV_AGNOSTIC) {
    if(vtype->vta) {
      int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
      for(int idx = vl->val; idx < vlmax; idx++) {
        *s1 = (uint64_t) -1;
        set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
      }
    }
  }

  rtl_li(s, s0, 0);
  vcsr_write(IDXVSTART, s0);
  vp_set_dirty();
}

void floating_arithmetic_instr(int opcode, int is_signed, int widening, int dest_mask, Decode *s) {
  require_float();
  require_vector(true);
  uint32_t rm = isa_fp_get_frm();
  isa_fp_rm_check(rm);

  if (dest_mask) {
    if (s->src_vmode == SRC_VV) {
      vector_mvv_check(s, true);
    } else {
      vector_mvv_check(s, false);
    }
  } else if (widening == noWidening) {
    if (s->src_vmode == SRC_VV) {
      vector_vvv_check(s, true);
    } else {
      vector_vvv_check(s, false);
    }
  } else if (widening == noCheck) {
    widening = noWidening;
  } else if (widening == vdWidening || widening == vsdWidening) {
    if (s->src_vmode == SRC_VV) {
      vector_wvv_check(s, true);
    } else {
      vector_wvv_check(s, false);
    }
  } else if (widening == vdNarrow) {
    if (s->src_vmode == SRC_VV) {
      vector_vwv_check(s, true);
    } else {
      vector_vwv_check(s, false);
    }
  } else if (widening == vsWidening) {
    if (s->src_vmode == SRC_VV) {
      vector_wwv_check(s, true);
    } else {
      vector_wwv_check(s, false);
    }
  }

  // check whether the fp16 instruction is supported
#ifdef CONFIG_RV_ZVFH
  if ((vtype->vsew == 1 && !is_vf_allowed_e16(opcode)) || (vtype->vsew == 0 && !is_vf_allowed_e8(opcode))) {
    longjmp_exception(EX_II);
  }
#else
  if (vtype->vsew == 1 && !is_vf_allowed_e16(opcode)) {
    Loge("ZVFH extension is not enabled, please make menuconfig!");
    longjmp_exception(EX_II);
  }
#endif
#if defined(CONFIG_RV_ZVFBF_MIN) || defined(CONFIG_RV_ZVFBF_WMA)
  if (is_bf16_opcode(opcode) && vtype->vsew != 1) {
    longjmp_exception(EX_II);
  }
#endif
  word_t FPCALL_TYPE = FPCALL_W64;
  // fpcall type
  switch (vtype->vsew) {
#ifdef CONFIG_RV_ZVFH
    case 0 :
      switch (widening) {
        case vdWidening  : FPCALL_TYPE = FPCALL_W8; break;
        case vdNarrow    : FPCALL_TYPE = FPCALL_W16; break;
      }
      break;   
#else
    case 0 : Loge("f8 not supported"); longjmp_exception(EX_II); break;
#endif

#ifdef CONFIG_RV_ZVFH
    case 1 :
      switch (widening) {
        case noWidening  :
        case vdWidening  : FPCALL_TYPE = FPCALL_W16; break;
        case vdNarrow    : FPCALL_TYPE = FPCALL_W32; break;
        case vsdWidening : FPCALL_TYPE = FPCALL_W16_to_32; break;//fwadd fwsub
        case vsWidening  : FPCALL_TYPE = FPCALL_SRC2_W16_to_32; break;
      }
      break;

#else
    case 1 :
      switch (widening) {
        case vdWidening  : FPCALL_TYPE = FPCALL_W16; break;
        case vdNarrow    : FPCALL_TYPE = FPCALL_W32; break;
      }
      break;
#endif
    case 2 : 
      switch (widening) {
        case vsdWidening : FPCALL_TYPE = FPCALL_W32_to_64; break;
        case vsWidening  : FPCALL_TYPE = FPCALL_SRC2_W32_to_64; break;
        case vdNarrow    : FPCALL_TYPE = FPCALL_W64; break;
        case vdWidening  :
        case noWidening  : FPCALL_TYPE = FPCALL_W32; break;
      }
      break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
    default: Loge("other fp type not supported"); longjmp_exception(EX_II); break;
  }
  uint32_t fp_box_type = fp_type_from_vsew(vtype->vsew);
  if (opcode == FWMACCBF16) {
    fp_box_type = FPCALL_BF16;
  }
  check_vstart_exception(s);
  if(check_vstart_ignore(s)) {
    if (opcode != FCLASS && opcode != FMERGE && opcode != FSLIDE1UP && opcode != FSLIDE1DOWN) {
      fp_set_dirty();
    }
    vp_set_dirty();
    return;
  }
  for(word_t idx = vstart->val; idx < vl->val; idx ++) {
    // mask
    rtlreg_t mask = get_mask(0, idx);
    if(s->vm == 0) {
      // merge instr will exec no matter mask or not
      // masked and mask off exec will left dest unmodified.
      if(opcode != FMERGE \
        && mask==0) {
        if (RVV_AGNOSTIC) {
          if (dest_mask == 1) {
            if (vtype->vma) {
              set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
            }
            continue;
          }
          if (vtype->vma) {
            *s1 = (uint64_t) -1;
            if (widening == vsdWidening || widening == vdWidening || widening == vsWidening)
              set_vreg(id_dest->reg, idx, *s1, vtype->vsew+1, vtype->vlmul, 1);
            else
              set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
          }
        }
        continue;
      }

    }
    if(s->vm == 1 && opcode == FMERGE) {
      mask = 1; // merge(mv) get the first operand (s1, rs1, vs2);
    }

    // operand - vs2
    if (widening == vsWidening || widening == vdNarrow)
      get_vreg(id_src2->reg, idx, s0, vtype->vsew+1, vtype->vlmul, is_signed, 1);
    else
      get_vreg(id_src2->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);

    // operand - s1 / rs1 / imm
    switch (s->src_vmode) {
      case SRC_VV : 
        get_vreg(id_src->reg, idx, s1, vtype->vsew, vtype->vlmul, 0, 0);
        break;
      case SRC_VF :   
        rtl_mv(s, s1, &fpreg_l(id_src1->reg)); // f[rs1]
        check_isFpCanonicalNAN(s1, fp_box_type);
        switch (vtype->vsew) {
          case 0 : *s1 = *s1 & 0xff; break;
          case 1 : *s1 = *s1 & 0xffff; break;
          case 2 : *s1 = *s1 & 0xffffffff; break;
          case 3 : *s1 = *s1 & 0xffffffffffffffff; break;
        }
        break;
    }

    switch (opcode) {
      case FMACC :
      case FNMACC :
      case FMSAC :
      case FNMSAC :
      case FMADD :
      case FNMADD :
      case FMSUB : 
      case FNMSUB :
      case FWMACCBF16 :
        if (widening == vsdWidening) get_vreg(id_dest->reg, idx, s2, vtype->vsew+1, vtype->vlmul, 0, 1);
        else get_vreg(id_dest->reg, idx, s2, vtype->vsew, vtype->vlmul, 0, 1);
        break;
    }

    // op
    switch (opcode) {
      case FADD : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_ADD, FPCALL_TYPE)); break;
      case FSUB : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_SUB, FPCALL_TYPE)); break;
      case FRSUB: rtl_hostcall(s, HOSTCALL_VFP, s1, s1, s0, FPCALL_CMD(FPCALL_SUB, FPCALL_TYPE)); break;
      case FMUL : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_MUL, FPCALL_TYPE)); break;
      case FDIV : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DIV, FPCALL_TYPE)); break;
      case FRDIV: rtl_hostcall(s, HOSTCALL_VFP, s1, s1, s0, FPCALL_CMD(FPCALL_DIV, FPCALL_TYPE)); break;
      case FMIN : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_MIN, FPCALL_TYPE)); break;
      case FMAX : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_MAX, FPCALL_TYPE)); break;
      case FMACC : rtl_hostcall(s, HOSTCALL_VFP, s2, s1, s0, FPCALL_CMD(FPCALL_MACC, FPCALL_TYPE)); rtl_mv(s, s1, s2); break;
      case FNMACC : rtl_hostcall(s, HOSTCALL_VFP, s2, s1, s0, FPCALL_CMD(FPCALL_NMACC, FPCALL_TYPE)); rtl_mv(s, s1, s2); break;
      case FMSAC : rtl_hostcall(s, HOSTCALL_VFP, s2, s1, s0, FPCALL_CMD(FPCALL_MSAC, FPCALL_TYPE)); rtl_mv(s, s1, s2); break;
      case FNMSAC : rtl_hostcall(s, HOSTCALL_VFP, s2, s1, s0, FPCALL_CMD(FPCALL_NMSAC, FPCALL_TYPE)); rtl_mv(s, s1, s2); break;
      case FMADD : rtl_hostcall(s, HOSTCALL_VFP, s2, s1, s0, FPCALL_CMD(FPCALL_MADD, FPCALL_TYPE)); rtl_mv(s, s1, s2); break;
      case FNMADD : rtl_hostcall(s, HOSTCALL_VFP, s2, s1, s0, FPCALL_CMD(FPCALL_NMADD, FPCALL_TYPE)); rtl_mv(s, s1, s2); break;
      case FMSUB : rtl_hostcall(s, HOSTCALL_VFP, s2, s1, s0, FPCALL_CMD(FPCALL_MSUB, FPCALL_TYPE)); rtl_mv(s, s1, s2); break;
      case FNMSUB : rtl_hostcall(s, HOSTCALL_VFP, s2, s1, s0, FPCALL_CMD(FPCALL_NMSUB, FPCALL_TYPE)); rtl_mv(s, s1, s2); break;
      case FSQRT : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_SQRT, FPCALL_TYPE)); break;
      case FRSQRT7 : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_RSQRT7, FPCALL_TYPE)); break;
      case FREC7 : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_REC7, FPCALL_TYPE)); break;
      case FCLASS : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_CLASS, FPCALL_TYPE)); break;
      case FMERGE : isa_fp_rm_check(rm); rtl_mux(s, s1, &mask, s1, s0); break;
      case MFEQ : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_EQ, FPCALL_TYPE)); break;
      case MFNE : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_NE, FPCALL_TYPE)); break;
      case MFLT : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_LT, FPCALL_TYPE)); break;
      case MFLE : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_LE, FPCALL_TYPE)); break;
      case MFGT : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_GT, FPCALL_TYPE)); break;
      case MFGE : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_GE, FPCALL_TYPE)); break;
      case FSGNJ : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_SGNJ, FPCALL_TYPE)); break;
      case FSGNJN : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_SGNJN, FPCALL_TYPE)); break;
      case FSGNJX : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_SGNJX, FPCALL_TYPE)); break;
      case FCVT_XUF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_FToU, FPCALL_TYPE)); break;
      case FCVT_XF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_FToS, FPCALL_TYPE)); break;
      case FCVT_RTZ_XUF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_FToUT, FPCALL_TYPE)); break;
      case FCVT_RTZ_XF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_FToST, FPCALL_TYPE)); break;
      case FCVT_FXU : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_UToF, FPCALL_TYPE)); break;
      case FCVT_FX : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_SToF, FPCALL_TYPE)); break;
      case FWCVT_XUF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_FToDU, FPCALL_TYPE)); break;
      case FWCVT_XF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_FToDS, FPCALL_TYPE)); break;
      case FWCVT_RTZ_XUF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_FToDUT, FPCALL_TYPE)); break;
      case FWCVT_RTZ_XF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_FToDST, FPCALL_TYPE)); break;
      case FWCVT_FXU : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_UToDF, FPCALL_TYPE)); break;
      case FWCVT_FX : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_SToDF, FPCALL_TYPE)); break;
      case FWCVT_FF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_FToDF, FPCALL_TYPE)); break;
      case FNCVT_XUF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DFToU, FPCALL_TYPE)); break;
      case FNCVT_XF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DFToS, FPCALL_TYPE)); break;
      case FNCVT_RTZ_XUF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DFToUT, FPCALL_TYPE)); break;
      case FNCVT_RTZ_XF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DFToST, FPCALL_TYPE)); break;
      case FNCVT_FXU : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DUToF, FPCALL_TYPE)); break;
      case FNCVT_FX : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DSToF, FPCALL_TYPE)); break;
      case FNCVT_FF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DFToF, FPCALL_TYPE)); break;
      case FWCVT_BF16_FF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_BF16ToF32, FPCALL_BF16)); break;
      case FNCVT_BF16_FF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_F32ToBF16, FPCALL_W32)); break;
      case FWMACCBF16 :
        rtl_hostcall(s, HOSTCALL_VFP, s0, s0, s0, FPCALL_CMD(FPCALL_BF16ToF32, FPCALL_BF16));
        rtl_hostcall(s, HOSTCALL_VFP, s1, s1, s1, FPCALL_CMD(FPCALL_BF16ToF32, FPCALL_BF16));
        rtl_hostcall(s, HOSTCALL_VFP, s2, s1, s0, FPCALL_CMD(FPCALL_MACC, FPCALL_W32));
        rtl_mv(s, s1, s2);
        break;
      case FNCVT_ROD_FF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DFToF_ODD, FPCALL_TYPE)); break;
      case FSLIDE1UP :
        isa_fp_rm_check(rm);
        if (idx > 0) get_vreg(id_src2->reg, idx - 1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        break;
      case FSLIDE1DOWN :
        isa_fp_rm_check(rm);
        if (idx < vl->val - 1) get_vreg(id_src2->reg, idx + 1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        break;
    }

    if(dest_mask == 1) 
      set_mask(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul);
    else if (widening == vsdWidening || widening == vdWidening || widening == vsWidening)
      set_vreg(id_dest->reg, idx, *s1, vtype->vsew+1, vtype->vlmul, 1);
    else
      set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
  }

  if (RVV_AGNOSTIC) {
    if(vtype->vta) {
      int vlmax = 0;
      if (widening == vsdWidening || widening == vdWidening || widening == vsWidening)
        vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, 1);
      else
        vlmax = get_vlen_max(vtype->vsew, vtype->vlmul, 0);
      for(int idx = vl->val; idx < vlmax; idx++) {
        if (dest_mask == 1)
          continue;
        *s1 = (uint64_t) -1;
        if (widening == vsdWidening || widening == vdWidening || widening == vsWidening)
          set_vreg(id_dest->reg, idx, *s1, vtype->vsew+1, vtype->vlmul, 1);
        else
          set_vreg(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul, 1);
      }
    }
    if(dest_mask) {
      for (int idx = vl->val; idx < VLEN; idx++) {
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
      }
    }
  }

  rtl_li(s, s0, 0);
  vcsr_write(IDXVSTART, s0);
  if (opcode != FCLASS && opcode != FMERGE && opcode != FSLIDE1UP && opcode != FSLIDE1DOWN) {
    fp_set_dirty();
  }
  vp_set_dirty();
}

void mask_instr(int opcode, Decode *s) {
  require_vector(true);
  if (s->vm == 0) {
    longjmp_exception(EX_II);
  }
  check_vstart_exception(s);
  if(check_vstart_ignore(s)) return;
  for(word_t idx = vstart->val; idx < vl->val; idx++) {
    // operand - vs2
    *s0 = get_mask(id_src2->reg, idx); // unproper usage of s0
    *s0 &= 1; // only LSB

    // operand - s1
    *s1 = get_mask(id_src->reg, idx); // unproper usage of s1
    *s1 &= 1; // only LSB

    // op
    switch (opcode) {
      case MAND    : rtl_and(s, s1, s0, s1); break;
      case MNAND   : rtl_and(s, s1, s0, s1);
                     *s1 = !(*s1); break;
      case MANDNOT : *s1 = !(*s1); // unproper usage of not
                     rtl_and(s, s1, s0, s1); break;
      case MXOR    : rtl_xor(s, s1, s0, s1); break;
      case MOR     : rtl_or(s, s1, s0, s1); break;
      case MNOR    : rtl_or(s, s1, s0, s1);
                     *s1 = !(*s1); break;
      case MORNOT  : *s1 = !(*s1);
                     rtl_or(s, s1, s0, s1); break;
      case MXNOR   : rtl_xor(s, s1, s0, s1);
                     *s1 = !(*s1); break;
      default      : assert(0);
    }
    // store to vrf
    *s1 &= 1; // make sure the LSB
    set_mask(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul);
  }
  rtl_li(s, s0, 0);
  vcsr_write(IDXVSTART, s0);
  vp_set_dirty();

  if (RVV_AGNOSTIC) {
    for (int idx = vl->val; idx < VLEN; idx++) {
      set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
    }
  }
}


/*
Vector reduction operations take a vector register group of elements and a
scalar held in element 0 of a vector register, and perform a reduction using
some binary operator, to produce a scalar result in element 0 of a vector
register.The scalar input and output operands are held in element 0 of a single
vector register, not a vector register group, so any vector register can be the
scalar source or destination of a vector reduction regardless of LMUL setting.
*/
void reduction_instr(int opcode, int is_signed, int wide, Decode *s) {
  vector_reduction_check(s, wide);
  check_vstart_exception(s);
  if(check_vstart_ignore(s)) {
    vp_set_dirty();
    return;
  }
  // operand - vs1
  get_vreg(id_src->reg, 0, s1, vtype->vsew+wide, vtype->vlmul, is_signed, 0);
  if(is_signed) rtl_sext(s, s1, s1, 1 << (vtype->vsew+wide));
  for(word_t idx = vstart->val; idx < vl->val; idx ++) {
    // get mask
    rtlreg_t mask = get_mask(0, idx);
    if(s->vm == 0 && mask==0) {
      continue;
    }
    // operand - vs2
    get_vreg(id_src2->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 0);
    if(is_signed) rtl_sext(s, s0, s0, 1 << vtype->vsew);

    // op
    switch (opcode) {
      case REDSUM : rtl_add(s, s1, s0, s1); break;
      case REDOR  : rtl_or(s, s1, s0, s1); break;
      case REDAND : rtl_and(s, s1, s0, s1); break;
      case REDXOR : rtl_xor(s, s1, s0, s1); break;
      case REDMIN : rtl_min(s, s1, s0, s1); break;
      case REDMAX : rtl_max(s, s1, s0, s1); break;
      case REDMINU: rtl_minu(s, s1, s0, s1); break;
      case REDMAXU: rtl_maxu(s, s1, s0, s1); break;
      //  case MIN : 
      // MINU is hard to achieve parallel
    }

  }
  if (RVV_AGNOSTIC) {
    if(vtype->vta && vl->val != 0) set_vreg_tail(id_dest->reg);
  }
  // No write when vl is 0
  if (vl->val != 0) {
    set_vreg(id_dest->reg, 0, *s1, vtype->vsew+wide, vtype->vlmul, 0);
  }
  vp_set_dirty();
  vstart->val = 0;
}

void float_reduction_instr(int opcode, int widening, Decode *s) {
  isa_fp_rm_check(isa_fp_get_frm());
  require_float();

  vector_reduction_check(s, widening);
  if (widening)
    get_vreg(id_src->reg, 0, s1, vtype->vsew+1, vtype->vlmul, 0, 0);
  else
    get_vreg(id_src->reg, 0, s1, vtype->vsew, vtype->vlmul, 0, 0);
  // store vs1's value
  *s2 = *s1;

  word_t FPCALL_TYPE = FPCALL_W64;
  uint64_t active_num = 0;

  // fpcall type
  switch (vtype->vsew) {
    case 0 : Loge("f8 not supported"); longjmp_exception(EX_II); break;
#ifdef CONFIG_RV_ZVFH
    case 1 :
      switch (widening) {
        case vsWidening : FPCALL_TYPE = FPCALL_SRC1_W16_to_32; break;
        case noWidening : FPCALL_TYPE = FPCALL_W16; break;
      }
      break;
#else
    case 1 : Loge("ZVFH extension is not enabled, please make menuconfig!"); longjmp_exception(EX_II); break;
#endif
    case 2 : 
      switch (widening) {
        case vsWidening : FPCALL_TYPE = FPCALL_SRC1_W32_to_64; break;
        case noWidening : FPCALL_TYPE = FPCALL_W32; break;
      }
      break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
    default: Loge("other fp type not supported"); longjmp_exception(EX_II); break;
  }

  check_vstart_exception(s);
  if(check_vstart_ignore(s)) {
    fp_set_dirty();
    vp_set_dirty();
    return;
  }

  for(word_t idx = vstart->val; idx < vl->val; idx ++) {
    rtlreg_t mask = get_mask(0, idx);
    if(s->vm == 0 && mask==0) {
      continue;
    }
    active_num++;
    // operand - vs2
    get_vreg(id_src2->reg, idx, s0, vtype->vsew, vtype->vlmul, 0, 1);

    // op
    switch (opcode) {
      case FREDUSUM : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_ADD, FPCALL_TYPE)); break;
      case FREDOSUM : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_ADD, FPCALL_TYPE)); break;
      case FREDMIN : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_MIN, FPCALL_TYPE)); break;
      case FREDMAX : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_MAX, FPCALL_TYPE)); break;
      //  case MIN : 
      // MINU is hard to achieve parallel
    }

  }

  if (active_num == 0) {
    // If no elements are active, no operations are performed, so the scalar in vs1[0] is simply copied to the destination register, without
    // canonicalizing NaN values and without setting any exception flags
    *s1 = *s2;
  }

  if (RVV_AGNOSTIC) {
    if(vtype->vta && vl->val != 0) set_vreg_tail(id_dest->reg);
  }
  // No write when vl is 0
  if (vl->val != 0) {
    if (widening)
      set_vreg(id_dest->reg, 0, *s1, vtype->vsew+1, vtype->vlmul, 0);
    else
      set_vreg(id_dest->reg, 0, *s1, vtype->vsew, vtype->vlmul, 0);
  }
  fp_set_dirty();
  vp_set_dirty();
  vstart->val = 0;
}

static void init_tmp_vreg(Decode *s, int vsew) {
  *s0 = 0;
  // init each element with negative zero
  for (int i = 0; i < 8; i++) {
    switch (vtype->vsew) {
      case 1 :
        rtl_hostcall(s, HOSTCALL_VFP, s0, s0, s0, FPCALL_CMD(FPCALL_GenNegZero, FPCALL_W16));
        for (int j = 0; j < VLEN / 16; j++) {
          if (isa_fp_get_frm() == FPCALL_RM_RDN) {
            tmp_vreg[i]._16[j] = 0;
          }
          else {
            tmp_vreg[i]._16[j] = *s0;
          }
        }
        break;
      case 2 :
        rtl_hostcall(s, HOSTCALL_VFP, s0, s0, s0, FPCALL_CMD(FPCALL_GenNegZero, FPCALL_W32));
        for (int j = 0; j < VLEN / 32; j++) {
          if (isa_fp_get_frm() == FPCALL_RM_RDN) {
            tmp_vreg[i]._32[j] = 0;
          }
          else {
            tmp_vreg[i]._32[j] = *s0;
          }
        }
        break;
      case 3 :
        rtl_hostcall(s, HOSTCALL_VFP, s0, s0, s0, FPCALL_CMD(FPCALL_GenNegZero, FPCALL_W64));
        for (int j = 0; j < VLEN / 64; j++) {
          if (isa_fp_get_frm() == FPCALL_RM_RDN) {
            tmp_vreg[i]._64[j] = 0;
          }
          else {
            tmp_vreg[i]._64[j] = *s0;
          }
        }
        break;
      default: Loge("other fp type not supported"); longjmp_exception(EX_II); break;
    }
  }
}

void float_reduction_step2(uint64_t src, Decode *s) {
  word_t FPCALL_TYPE = FPCALL_W64;

  // fpcall type
  switch (vtype->vsew) {
    case 0 : Loge("f8 not supported"); longjmp_exception(EX_II); break;
#ifdef CONFIG_RV_ZVFH
    case 1 : FPCALL_TYPE = FPCALL_W16; break;
#else
    case 1 : Loge("ZVFH extension is not enabled, please make menuconfig!"); longjmp_exception(EX_II); break;
#endif
    case 2 : FPCALL_TYPE = FPCALL_W32; break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
    default: Loge("other fp type not supported"); longjmp_exception(EX_II); break;
  }

  int element_num = VLEN >> (3 + vtype->vsew);

  while (element_num != 1) {
    for (int i = 0; i < element_num / 2; i++) {
      get_tmp_vreg(src, i, s1, vtype->vsew);
      get_tmp_vreg(src, i + element_num / 2, s0, vtype->vsew);
      rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_ADD, FPCALL_TYPE));
      set_tmp_vreg(src, i, *s1, vtype->vsew);
    }
    element_num >>= 1;
  }
}

void float_reduction_step1(uint64_t src1, uint64_t src2, Decode *s) {
  word_t FPCALL_TYPE = FPCALL_W64;

  // fpcall type
  switch (vtype->vsew) {
    case 0 : Loge("f8 not supported"); longjmp_exception(EX_II); break;
#ifdef CONFIG_RV_ZVFH
    case 1 : FPCALL_TYPE = FPCALL_W16; break;
#else
    case 1 : Loge("ZVFH extension is not enabled, please make menuconfig!"); longjmp_exception(EX_II); break;
#endif
    case 2 : FPCALL_TYPE = FPCALL_W32; break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
    default: Loge("other fp type not supported"); longjmp_exception(EX_II); break;
  }

  int element_num = VLEN >> (3 + vtype->vsew);

  for (int i = 0; i < element_num; i++) {
    get_tmp_vreg(src1, i, s1, vtype->vsew);
    get_tmp_vreg(src2, i, s0, vtype->vsew);
    rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_ADD, FPCALL_TYPE));
    set_tmp_vreg(src1, i, *s1, vtype->vsew);
  }
}

void float_reduction_computing(Decode *s) {
  isa_fp_rm_check(isa_fp_get_frm());
  require_float();
  vector_reduction_check(s, false);
  word_t FPCALL_TYPE = FPCALL_W64;
  uint64_t active_num = 0;

  // fpcall type
  switch (vtype->vsew) {
    case 0 : Loge("f8 not supported"); longjmp_exception(EX_II); break;
#ifdef CONFIG_RV_ZVFH
    case 1 : FPCALL_TYPE = FPCALL_W16; break;
#else
    case 1 : Loge("ZVFH extension is not enabled, please make menuconfig!"); longjmp_exception(EX_II); break;
#endif
    case 2 : FPCALL_TYPE = FPCALL_W32; break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
    default: Loge("other fp type not supported"); longjmp_exception(EX_II); break;
  }

  check_vstart_exception(s);
  if(check_vstart_ignore(s)) {
    fp_set_dirty();
    vp_set_dirty();
    return;
  }

  // copy the vector register to the temp register
  init_tmp_vreg(s, vtype->vsew);
  for(word_t idx = vstart->val; idx < vl->val; idx ++) {
    rtlreg_t mask = get_mask(0, idx);
    if(s->vm == 0 && mask==0) {
      continue;
    }
    active_num++;
    vreg_to_tmp_vreg(id_src2->reg, idx, vtype->vsew);
  }

  // computing the reduction result
  switch (vtype->vlmul) {
    case 5 :
    case 6 :
    case 7 :
    case 0 : 
      float_reduction_step2(0, s);
      break;
    case 1 : 
      float_reduction_step1(0, 1, s);
      float_reduction_step2(0, s);
      break;
    case 2 :
      float_reduction_step1(0, 1, s);
      float_reduction_step1(2, 3, s);
      float_reduction_step1(0, 2, s);
      float_reduction_step2(0, s);
      break;
    case 3 :
      float_reduction_step1(0, 1, s);
      float_reduction_step1(2, 3, s);
      float_reduction_step1(4, 5, s);
      float_reduction_step1(6, 7, s);
      float_reduction_step1(0, 2, s);
      float_reduction_step1(4, 6, s);
      float_reduction_step1(0, 4, s);
      float_reduction_step2(0, s);
      break;
    default: Loge("lmul = 4 is reserved"); longjmp_exception(EX_II); break;
  }

  get_vreg(id_src->reg, 0, s1, vtype->vsew, vtype->vlmul, 0, 0);
  get_tmp_vreg(0, 0, s0, vtype->vsew);

  if (active_num != 0) {
    // If no elements are active, no operations are performed, so the scalar in vs1[0] is simply copied to the destination register, without
    // canonicalizing NaN values and without setting any exception flags
    rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_ADD, FPCALL_TYPE));
  }

  if (RVV_AGNOSTIC) {
    if(vtype->vta && vl->val != 0) set_vreg_tail(id_dest->reg);
  }

  // No write when vl is 0
  if (vl->val != 0) {
    set_vreg(id_dest->reg, 0, *s1, vtype->vsew, vtype->vlmul, 0);
  }
  fp_set_dirty();
  vp_set_dirty();
  vstart->val = 0;
}

// dirty job here
#undef s0
#undef s1
#define s0 &ls0
#define s1 &ls1

#endif // CONFIG_RVV
