/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
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

#ifndef __C_OP_H__
#define __C_OP_H__

#include <common.h>

#define c_shift_mask MUXDEF(CONFIG_ISA64, 0x3f, 0x1f)

#define c_add(a, b) ((a) + (b))
#define c_sub(a, b) ((a) - (b))
#define c_and(a, b) ((a) & (b))
#define c_or(a, b)  ((a) | (b))
#define c_xor(a, b) ((a) ^ (b))
#define c_shl(a, b) ((a) << ((b) & c_shift_mask))
#define c_shr(a, b) ((a) >> ((b) & c_shift_mask))
#define c_sar(a, b) ((sword_t)(a) >> ((b) & c_shift_mask))
#define c_min(a, b) ((int64_t)(a) < (int64_t)(b) ? (a) : (b))
#define c_max(a, b) ((int64_t)(a) > (int64_t)(b) ? (a) : (b))
#define c_minu(a, b) ((uint64_t)(a) < (uint64_t)(b) ? (a) : (b))
#define c_maxu(a, b) ((uint64_t)(a) > (uint64_t)(b) ? (a) : (b))

#ifdef CONFIG_ISA64
#define c_sext32to64(a) ((int64_t)(int32_t)(a))
#define c_addw(a, b) c_sext32to64((a) + (b))
#define c_subw(a, b) c_sext32to64((a) - (b))
#define c_shlw(a, b) c_sext32to64((uint32_t)(a) << ((b) & 0x1f))
#define c_shrw(a, b) c_sext32to64((uint32_t)(a) >> ((b) & 0x1f))
#define c_sarw(a, b) c_sext32to64(( int32_t)(a) >> ((b) & 0x1f))
#endif

#define c_mulu_lo(a, b) ((a) * (b))
#ifdef CONFIG_ISA64
# define c_mulu_hi(a, b) (((__uint128_t)(a) * (__uint128_t)(b)) >> 64)
# define c_muls_hi(a, b) (((__int128_t)(sword_t)(a) * (__int128_t)(sword_t)(b)) >> 64)
# define c_mulw(a, b) c_sext32to64((a) * (b))
# define c_divw(a, b)  c_sext32to64(( int32_t)(a) / ( int32_t)(b))
# define c_divuw(a, b) c_sext32to64((uint32_t)(a) / (uint32_t)(b))
# define c_remw(a, b)  c_sext32to64(( int32_t)(a) % ( int32_t)(b))
# define c_remuw(a, b) c_sext32to64((uint32_t)(a) % (uint32_t)(b))
#else
#define c_mulu_hi(a, b) (((uint64_t)(a) * (uint64_t)(b)) >> 32)
#define c_muls_hi(a, b) (((int64_t)(sword_t)(a) * (int64_t)(sword_t)(b)) >> 32)
#endif

#define c_divu_q(a, b) ((a) / (b))
#define c_divu_r(a, b)  ((a) % (b))
#define c_divs_q(a, b) ((sword_t)(a) / (sword_t)(b))
#define c_divs_r(a, b)  ((sword_t)(a) % (sword_t)(b))

static inline bool interpret_relop(uint32_t relop, const rtlreg_t src1, const rtlreg_t src2) {
  word_t sc1 = src1, sc2 = src2;
#ifdef CONFIG_SIM32
  if (src1 & 0x80000000) {
    sc1 = src1 | 0xffffffff00000000;
  }
  if (src2 & 0x80000000) {
    sc2 = src2 | 0xffffffff00000000;
  }
#else
  sc1 = src1;
  sc2 = src2;
#endif
  switch (relop) {
    case RELOP_FALSE: return false;
    case RELOP_TRUE: return true;
    case RELOP_EQ: return sc1 == sc2;
    case RELOP_NE: return sc1 != sc2;
    case RELOP_LT: return (sword_t)sc1 <  (sword_t)sc2;
    case RELOP_LE: return (sword_t)sc1 <= (sword_t)sc2;
    case RELOP_GT: return (sword_t)sc1 >  (sword_t)sc2;
    case RELOP_GE: return (sword_t)sc1 >= (sword_t)sc2;
    case RELOP_LTU: return sc1 < sc2;
    case RELOP_GTU: return sc1 > sc2;
    case RELOP_GEU: return sc1 >= sc2;
    default: panic("unsupported relop = %d", relop);
  }
}

#endif
