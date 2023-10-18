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

#include <common.h>
#ifdef CONFIG_RVV

#include "vcompute_impl.h"

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
        if (signed && vd == 0x8000000000000000) vd = 0; \
    })

static inline void update_vcsr() {
  vcsr->val = (vxrm->val) << 1 | vxsat->val;
}

void arthimetic_instr(int opcode, int is_signed, int widening, int narrow, int dest_mask, Decode *s) {
  int vlmax = get_vlmax(vtype->vsew, vtype->vlmul);
  int idx;
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
  for(idx = vstart->val; idx < vl->val; idx ++) {
    // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
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
        if (AGNOSTIC == 1) {
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

    // operand - vs2
    get_vreg(id_src2->reg, idx, s0, vtype->vsew+narrow, vtype->vlmul, is_signed, 1);
    if(is_signed) rtl_sext(s, s0, s0, 1 << (vtype->vsew+narrow));

    // operand - s1 / rs1 / imm
    switch (s->src_vmode) {
      case SRC_VV : 
        get_vreg(id_src->reg, idx, s1, vtype->vsew, vtype->vlmul, is_signed, 1);
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
        if(is_signed) rtl_li(s, s1, s->isa.instr.v_opv2.v_simm5);
        else          rtl_li(s, s1, s->isa.instr.v_opv3.v_imm5 );
        break;
    }

    shift = *s1 & (sew - 1);
    narrow_shift = *s1 & (sew * 2 - 1);

    if (opcode == SLIDEUP) {
      if(s->vm == 0 && mask == 0 && vtype->vma && (uint64_t)idx >= (uint64_t)*s1) {
        if (AGNOSTIC == 1) {
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
        for (int i = 0; i < (8 << vtype->vsew); i++) {
            carry = (((*s0 >> i) & 1) + ((*s1 >> i) & 1) + carry) >> 1;
            carry &= 1;
        }
        rtl_li(s, s1, carry);
        break;
      case MSBC:
        for (int i = 0; i < (8 << vtype->vsew); i++) {
          carry = ((~(*s0 >> i) & 1) + ((*s1 >> i) & 1) + carry) >> 1;
          carry &= 1;
        }
        rtl_li(s, s1, carry);
        break;
      case SLL :
        rtl_andi(s, s1, s1, s->v_width*8-1); //low lg2(SEW) is valid
        //rtl_sext(s0, s0, 8 - (1 << vtype->vsew)); //sext first
        rtl_shl(s, s1, s0, s1); break;
      case SRL :
        if (narrow)
            rtl_andi(s, s1, s1, s->v_width*16-1); //low lg2(SEW)
        else
            rtl_andi(s, s1, s1, s->v_width*8-1); //low lg2(SEW)
        rtl_shr(s, s1, s0, s1); break;
      case SRA :
        if (narrow) {
            rtl_andi(s, s1, s1, s->v_width*16-1); //low lg2(SEW)
            rtl_sext(s, s0, s0, s->v_width*2);
        }
        else {
            rtl_andi(s, s1, s1, s->v_width*8-1); //low lg2(SEW)
            rtl_sext(s, s0, s0, s->v_width);
        }
        rtl_sar(s, s1, s0, s1); break;
      case MULHU : 
        *s1 = (uint64_t)(((__uint128_t)(*s0) * (__uint128_t)(*s1))>>(s->v_width*8));
        break;
      case MUL : rtl_mulu_lo(s, s1, s0, s1); break;
      case MULSU : 
        rtl_sext(s, s0, s0, 1 << vtype->vsew);
        rtl_mulu_lo(s, s1, s0, s1); break;
      case MULHSU :
        rtl_sext(s, t0, s0, s->v_width);
        rtl_sari(s, t0, t0, s->v_width*8-1);
        rtl_and(s, t0, s1, t0);
        *s1 = (uint64_t)(((__uint128_t)(*s0) * (__uint128_t)(*s1))>>(s->v_width*8));
        rtl_sub(s, s1, s1, t0);
        break;
      case MULH :
        rtl_sext(s, s0, s0, s->v_width);
        rtl_sext(s, s1, s1, s->v_width);
        *s1 = (uint64_t)(((__int128_t)(sword_t)(*s0) * (__int128_t)(sword_t)(*s1))>>(s->v_width*8));
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
        rtl_sext(s, s0, s0, s->v_width);
        rtl_sext(s, s1, s1, s->v_width);
        if(*s1 == 0) rtl_li(s, s1, ~0lu);
        else if(*s0 == 0x8000000000000000LL && *s1 == -1) //may be error
          rtl_mv(s, s1, s0);
        else rtl_divs_q(s, s1, s0, s1);
        break;
      case REMU :
        if (*s1 == 0) rtl_mv(s, s1, s0);
        else rtl_divu_r(s, s1, s0, s1);
        break;
      case REM :
        rtl_sext(s, s0, s0, s->v_width);
        rtl_sext(s, s1, s1, s->v_width);
        if(*s1 == 0) rtl_mv(s, s1, s0);
        else if(*s1 == 0x8000000000000000LL && *s1 == -1) //may be error
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
      case SLIDEDOWN :
        if ((uint64_t)idx + (uint64_t)*s1 < (uint64_t)vlmax) get_vreg(id_src2->reg, idx + *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        else rtl_li(s, s1, 0);
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
        get_vreg(id_src1->reg, idx, s1, 1, vtype->vlmul, 0, 1);
        *s1 = *s1 & 0xffff;
        if ((uint64_t)*s1 < (uint64_t)vlmax) get_vreg(id_src2->reg, *s1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        else rtl_li(s, s1, 0);
        break;
      case SADDU :
        *s2 = *s1;
        rtl_add(s, s1, s0, s1);
        sat = 0;
        for (int i = 0; i < (8 << vtype->vsew); i++) {
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
        overflow = *s1 == *s0 && *s1 == int_min;
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
    }
    update_vcsr();
    // store to vrf
    if(dest_mask == 1) 
      set_mask(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul);
    else
      set_vreg(id_dest->reg, idx, *s1, vtype->vsew+widening, vtype->vlmul, 1);
  }

  if (AGNOSTIC == 1) {
    if(vtype->vta) {
      int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
      for(idx = vl->val; idx < vlmax; idx++) {
        if (dest_mask == 1)
          continue;
        *s1 = (uint64_t) -1;
        set_vreg(id_dest->reg, idx, *s1, vtype->vsew+widening, vtype->vlmul, 1);
      }
    }
    if(dest_mask) {
      for (idx = vl->val; idx < VLEN; idx++) {
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
      }
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  rtl_li(s, s0, 0);
  vcsr_write(IDXVSTART, s0);
  set_mstatus_dirt();
}

void floating_arthimetic_instr(int opcode, int is_signed, int widening, int dest_mask, Decode *s) {
  int idx;
  word_t FPCALL_TYPE;
  // fpcall type
  switch (vtype->vsew) {
    case 0 : panic("f8 not supported"); break;
    case 1 : 
      switch (widening) {
        case vsdWidening : FPCALL_TYPE = FPCALL_W16_to_32; break;
        case vsWidening : FPCALL_TYPE = FPCALL_SRC2_W16_to_32; break;
        case vdNarrow : FPCALL_TYPE = FPCALL_W32; break;
        case vdWidening :
        case noWidening : FPCALL_TYPE = FPCALL_W16; break;
      }
      break;
    case 2 : 
      switch (widening) {
        case vsdWidening : FPCALL_TYPE = FPCALL_W32_to_64; break;
        case vsWidening : FPCALL_TYPE = FPCALL_SRC2_W32_to_64; break;
        case vdNarrow : FPCALL_TYPE = FPCALL_W64; break;
        case vdWidening :
        case noWidening : FPCALL_TYPE = FPCALL_W32; break;
      }
      break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
  }
  for(idx = vstart->val; idx < vl->val; idx ++) {
    // mask
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if(s->vm == 0) {
      // merge instr will exec no matter mask or not
      // masked and mask off exec will left dest unmodified.
      if(opcode != FMERGE \
        && mask==0) {
        if (AGNOSTIC == 1) {
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
      case FMERGE : rtl_mux(s, s1, &mask, s1, s0); break;
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
      case FNCVT_ROD_FF : rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_DFToFR, FPCALL_TYPE)); break;
      case FSLIDE1UP :
        if (idx > 0) get_vreg(id_src2->reg, idx - 1, s1, vtype->vsew, vtype->vlmul, 0, 1);
        break;
      case FSLIDE1DOWN :
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

  if (AGNOSTIC == 1) {
    if(vtype->vta) {
      int vlmax = get_vlen_max(vtype->vsew, vtype->vlmul);
      for(idx = vl->val; idx < vlmax; idx++) {
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
      for (idx = vl->val; idx < VLEN; idx++) {
        set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
      }
    }
  }

  // TODO: the idx larger than vl need reset to zero.
  rtl_li(s, s0, 0);
  vcsr_write(IDXVSTART, s0);
}

void mask_instr(int opcode, Decode *s) {
  int idx;
  for(idx = vstart->val; idx < vl->val; idx++) {
    // operand - vs2
    *s0 = get_mask(id_src2->reg, idx, vtype->vsew, vtype->vlmul); // unproper usage of s0
    *s0 &= 1; // only LSB

    // operand - s1
    *s1 = get_mask(id_src->reg, idx, vtype->vsew, vtype->vlmul); // unproper usage of s1
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
      default      : longjmp_raise_intr(EX_II);
    }
    // store to vrf
    *s1 &= 1; // make sure the LSB
    set_mask(id_dest->reg, idx, *s1, vtype->vsew, vtype->vlmul);
  }
  set_mstatus_dirt();

  if (AGNOSTIC == 1) {
    for (idx = vl->val; idx < VLEN; idx++) {
      set_mask(id_dest->reg, idx, 1, vtype->vsew, vtype->vlmul);
    }
  }
}


void reduction_instr(int opcode, int is_signed, int wide, Decode *s) {
  // TODO: check here: does not need align??
  get_vreg(id_src->reg, 0, s1, vtype->vsew+wide, vtype->vlmul, is_signed, 1);
  if(is_signed) rtl_sext(s, s1, s1, 1 << (vtype->vsew+wide));

  int idx;
  for(idx = vstart->val; idx < vl->val; idx ++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if(s->vm == 0 && mask==0) {
      continue;
    }
    // operand - vs2
    get_vreg(id_src2->reg, idx, s0, vtype->vsew, vtype->vlmul, is_signed, 1);
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
  if (AGNOSTIC == 1) {
    if(vtype->vta) set_vreg_tail(id_dest->reg);
  }
  set_vreg(id_dest->reg, 0, *s1, vtype->vsew+wide, vtype->vlmul, 0);
  set_mstatus_dirt();
}

void float_reduction_instr(int opcode, int widening, Decode *s) {
  if (widening)
    get_vreg(id_src->reg, 0, s1, vtype->vsew+1, vtype->vlmul, 0, 1);
  else
    get_vreg(id_src->reg, 0, s1, vtype->vsew, vtype->vlmul, 0, 1);

  int idx;
  word_t FPCALL_TYPE;

  // fpcall type
  switch (vtype->vsew) {
    case 0 : panic("f8 not supported"); break;
    case 1 : 
      switch (widening) {
        case vsWidening : FPCALL_TYPE = FPCALL_SRC1_W16_to_32; break;
        case noWidening : FPCALL_TYPE = FPCALL_W16; break;
      }
      break;
    case 2 : 
      switch (widening) {
        case vsWidening : FPCALL_TYPE = FPCALL_SRC1_W32_to_64; break;
        case noWidening : FPCALL_TYPE = FPCALL_W32; break;
      }
      break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
  }

  for(idx = vstart->val; idx < vl->val; idx ++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if(s->vm == 0 && mask==0) {
      continue;
    }
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
  if (AGNOSTIC == 1) {
    if(vtype->vta) set_vreg_tail(id_dest->reg);
  }
  if (widening)
    set_vreg(id_dest->reg, 0, *s1, vtype->vsew+1, vtype->vlmul, 0);
  else
    set_vreg(id_dest->reg, 0, *s1, vtype->vsew, vtype->vlmul, 0);
}

void float_reduction_step2(uint64_t src, Decode *s) {
  word_t FPCALL_TYPE;

  // fpcall type
  switch (vtype->vsew) {
    case 0 : panic("f8 not supported"); break;
    case 1 : FPCALL_TYPE = FPCALL_W16; break;
    case 2 : FPCALL_TYPE = FPCALL_W32; break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
  }

  int element_num = VLEN >> (3 + vtype->vsew);

  while (element_num != 1) {
    for (int i = 0; i < element_num / 2; i += 2) {
      get_tmp_vreg(src, i, s1, vtype->vsew);
      get_tmp_vreg(src, i + element_num / 2, s0, vtype->vsew);
      rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_ADD, FPCALL_TYPE));
      set_tmp_vreg(src, i, *s1, vtype->vsew);
    }
    element_num >>= 1;
  }
}

void float_reduction_step1(uint64_t src1, uint64_t src2, Decode *s) {
  word_t FPCALL_TYPE;

  // fpcall type
  switch (vtype->vsew) {
    case 0 : panic("f8 not supported"); break;
    case 1 : FPCALL_TYPE = FPCALL_W16; break;
    case 2 : FPCALL_TYPE = FPCALL_W32; break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
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
  word_t FPCALL_TYPE;
  int idx;

  // fpcall type
  switch (vtype->vsew) {
    case 0 : panic("f8 not supported"); break;
    case 1 : FPCALL_TYPE = FPCALL_W16; break;
    case 2 : FPCALL_TYPE = FPCALL_W32; break;
    case 3 : FPCALL_TYPE = FPCALL_W64; break;
  }

  // copy the vector register to the temp register
  init_tmp_vreg();
  for(idx = vstart->val; idx < vl->val; idx ++) {
    rtlreg_t mask = get_mask(0, idx, vtype->vsew, vtype->vlmul);
    if(s->vm == 0 && mask==0) {
      continue;
    }
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
    default : break;
  }

  get_vreg(id_src->reg, 0, s1, vtype->vsew, vtype->vlmul, 0, 1);
  get_tmp_vreg(0, 0, s0, vtype->vsew);
  rtl_hostcall(s, HOSTCALL_VFP, s1, s0, s1, FPCALL_CMD(FPCALL_ADD, FPCALL_TYPE));

  if (AGNOSTIC == 1) {
    if(vtype->vta) set_vreg_tail(id_dest->reg);
  }
  set_vreg(id_dest->reg, 0, *s1, vtype->vsew, vtype->vlmul, 0);
}

// dirty job here
#undef s0
#undef s1
#define s0 &ls0
#define s1 &ls1

#endif // CONFIG_RVV