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

#include "vreg.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include <stdio.h>
#include <cpu/cpu.h>
#include "isa.h"

const char * vregsl[] = {
  "v0 ", "v1 ", "v2 ", "v3 ", "v4 ", "v5 ", "v6 ", "v7 ",
  "v8 ", "v9 ", "v10", "v11", "v12", "v13", "v14", "v15",
  "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
  "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31",
};


rtlreg_t check_vsetvl(rtlreg_t vtype_req, rtlreg_t vl_req, int mode) {
  rtlreg_t old_vl;
  vcsr_read(IDXVL, &old_vl);
  vtype_t vt;
  vt.val = vtype_req;
  rtlreg_t VLMAX = get_vlmax(vt.vsew, vt.vlmul);

  if (mode == 1) {
    return VLMAX;
  } else if (mode == 2) {
    return old_vl < VLMAX ? old_vl : VLMAX;
  } else {
    if (vt.vsew > 3) { //check if max-len supported
      return (uint64_t)-1; //return 0 means error, including vl_req is 0, for vl_req should not be 0.
    }
    if (vl_req <= VLMAX) {
        return vl_req;
    } else if (vl_req < 2 * VLMAX) {
        return VLMAX;
    } else {
        return VLMAX;
    }
  }
}

rtlreg_t get_mask(int reg, int idx) {
  int idx1 = idx / 64;
  int idx2 = idx % 64;
  
  return (rtlreg_t)((cpu.vr[reg]._64[idx1] & (1lu << idx2)) != 0);
}

void set_mask(uint32_t reg, int idx, uint64_t mask, uint64_t vsew, uint64_t vlmul) {
  int idx1 = idx / 64;
  int idx2 = idx % 64;
  //printf("set_mask: idx1 = %d, idx2 = %d, mask = %ld\n", idx1, idx2, mask);
  
  if (mask) {
    cpu.vr[reg]._64[idx1] |= (1lu << idx2);
  } else {
    cpu.vr[reg]._64[idx1] &= ~(1lu << idx2);
  }
}

int get_vlmax(int vsew, int vlmul) {
  if (vlmul > 4) {
    int sew = 8 << vsew;
    switch(vlmul) {
      case 5: return VLEN / sew / 8;
      case 6: return VLEN / sew / 4;
      case 7: return VLEN / sew / 2;
      default: panic("Unexpected vlmul\n");
    }
  } else if (vlmul < 4) {
    int sew = 8 << vsew;
    switch(vlmul) {
      case 0: return VLEN / sew;
      case 1: return VLEN / sew * 2;
      case 2: return VLEN / sew * 4;
      case 3: return VLEN / sew * 8;
      default: panic("Unexpected vlmul\n");
    }
  } else {
    Loge("vlmul = 4 is reserved\n");
    return -1;
  }
}

int get_vlen_max(int vsew, int vlmul, int widening) {
  if (vlmul > 4 && widening) {
    return VLEN >> (4 + vsew);
  }
  if (vlmul > 4) vlmul = 0;
  return VLEN >> (3 + vsew - vlmul);
}

int get_reg(uint64_t reg, int idx, uint64_t vsew) {
  int elem_num = VLEN >> (3 + vsew);
  int reg_off = idx / elem_num;
  return reg + reg_off;
}

int get_idx(uint64_t reg, int idx, uint64_t vsew) {
  int elem_num = VLEN >> (3 + vsew);
  int elem_idx = idx % elem_num;
  return elem_idx;
}

void isa_misalign_vreg_check(uint64_t reg, uint64_t vlmul, int needAlign) {
  if (needAlign && vlmul < 4) {
    if (reg % (1 << vlmul) != 0) {
      Loge("vector register group misaligned happen: reg:x%lu vlmul:0x%lx needAlign:%d", reg, vlmul, needAlign);
      longjmp_exception(EX_II);
    }
  }
}

void get_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew, uint64_t vlmul, int is_signed, int needAlign) {
  Assert(vlmul != 4, "vlmul = 4 is reserved\n");
  Assert(vsew <= 3, "vsew should be less than 4\n");
  isa_misalign_vreg_check(reg, vlmul, needAlign);
  int new_reg = get_reg(reg, idx, vsew);
  int new_idx = get_idx(reg, idx, vsew);
  switch (vsew) {
    case 0 : *dst = is_signed ? (int64_t)(int8_t )vreg_b(new_reg, new_idx) : vreg_b(new_reg, new_idx); break;
    case 1 : *dst = is_signed ? (int64_t)(int16_t)vreg_s(new_reg, new_idx) : vreg_s(new_reg, new_idx); break;
    case 2 : *dst = is_signed ? (int64_t)(int32_t)vreg_i(new_reg, new_idx) : vreg_i(new_reg, new_idx); break;
    case 3 : *dst = is_signed ? (int64_t)         vreg_l(new_reg, new_idx) : vreg_l(new_reg, new_idx); break;
  }
}

void get_vreg_with_addr(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew, uint64_t vlmul, int is_signed, int needAlign, void **addr) {
  Assert(vlmul != 4, "vlmul = 4 is reserved\n");
  Assert(vsew <= 3, "vsew should be less than 4\n");
  isa_misalign_vreg_check(reg, vlmul, needAlign);
  int new_reg = get_reg(reg, idx, vsew);
  int new_idx = get_idx(reg, idx, vsew);
  switch (vsew) {
    case 0 :
    *dst = is_signed ? (int64_t)(int8_t )vreg_b(new_reg, new_idx) : vreg_b(new_reg, new_idx);
    *addr = (void *) &vreg_b(new_reg, new_idx);
    break;

    case 1 :
    *dst = is_signed ? (int64_t)(int16_t)vreg_s(new_reg, new_idx) : vreg_s(new_reg, new_idx);
    *addr = (void *) &vreg_s(new_reg, new_idx);
    break;

    case 2 :
    *dst = is_signed ? (int64_t)(int32_t)vreg_i(new_reg, new_idx) : vreg_i(new_reg, new_idx);
    *addr = (void *) &vreg_i(new_reg, new_idx);
    break;

    case 3 :
    *dst = is_signed ? (int64_t)         vreg_l(new_reg, new_idx) : vreg_l(new_reg, new_idx);
    *addr = (void *) &vreg_l(new_reg, new_idx);
    break;
  }
}

void set_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew, uint64_t vlmul, int needAlign) {
  Assert(vlmul != 4, "vlmul = 4 is reserved\n");
  Assert(vsew <= 3, "vsew should be less than 4\n");
  isa_misalign_vreg_check(reg, vlmul, needAlign);
  int new_reg = get_reg(reg, idx, vsew);
  int new_idx = get_idx(reg, idx, vsew);

  switch (vsew) {
    case 0 : src = src & 0xff; break;
    case 1 : src = src & 0xffff; break;
    case 2 : src = src & 0xffffffff; break;
    case 3 : src = src & 0xffffffffffffffff; break;
  }
  switch (vsew) {
    case 0 : vreg_b(new_reg, new_idx) = (uint8_t  )src; break;
    case 1 : vreg_s(new_reg, new_idx) = (uint16_t )src; break;
    case 2 : vreg_i(new_reg, new_idx) = (uint32_t )src; break;
    case 3 : vreg_l(new_reg, new_idx) = (uint64_t )src; break;
  }
}

#ifdef CONFIG_MULTICORE_DIFF
union {
  uint64_t _64[VENUM64];
  uint32_t _32[VENUM32];
  uint16_t _16[VENUM16];
  uint8_t  _8[VENUM8];
} vec_dual_difftest_reg[8];

void set_vec_dual_difftest_reg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew) {
  Assert(vsew <= 3, "vsew should be less than 4\n");
  switch (vsew) {
    case 0 : src = src & 0xff; break;
    case 1 : src = src & 0xffff; break;
    case 2 : src = src & 0xffffffff; break;
    case 3 : src = src & 0xffffffffffffffff; break;
  }
  switch (vsew) {
    case 0 : vec_dual_difftest_reg[reg]._8 [idx] = (uint8_t  )src; break;
    case 1 : vec_dual_difftest_reg[reg]._16[idx] = (uint16_t )src; break;
    case 2 : vec_dual_difftest_reg[reg]._32[idx] = (uint32_t )src; break;
    case 3 : vec_dual_difftest_reg[reg]._64[idx] = (uint64_t )src; break;
  }
}

void set_vec_dual_difftest_reg_idx(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew) {
  Assert(vsew <= 3, "vsew should be less than 4\n");
  int new_reg = get_reg(reg, idx, vsew);
  int new_idx = get_idx(reg, idx, vsew);

  switch (vsew) {
    case 0 : src = src & 0xff; break;
    case 1 : src = src & 0xffff; break;
    case 2 : src = src & 0xffffffff; break;
    case 3 : src = src & 0xffffffffffffffff; break;
  }
  switch (vsew) {
    case 0 : vec_difftest_reg_b(new_reg, new_idx) = (uint8_t  )src; break;
    case 1 : vec_difftest_reg_s(new_reg, new_idx) = (uint16_t )src; break;
    case 2 : vec_difftest_reg_i(new_reg, new_idx) = (uint32_t )src; break;
    case 3 : vec_difftest_reg_l(new_reg, new_idx) = (uint64_t )src; break;
  }
}

void  *get_vec_dual_reg() {
  return vec_dual_difftest_reg;
}
#endif // CONFIG_MULTICORE_DIFF

void get_tmp_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew) {
  Assert(vsew <= 3, "vsew should be less than 4\n");
  switch (vsew) {
    case 0 : *dst = tmp_vreg[reg]._8[idx];  break;
    case 1 : *dst = tmp_vreg[reg]._16[idx]; break;
    case 2 : *dst = tmp_vreg[reg]._32[idx]; break;
    case 3 : *dst = tmp_vreg[reg]._64[idx]; break;
  }
}

void set_tmp_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew) {
  Assert(vsew <= 3, "vsew should be less than 4\n");

  switch (vsew) {
    case 0 : src = src & 0xff; break;
    case 1 : src = src & 0xffff; break;
    case 2 : src = src & 0xffffffff; break;
    case 3 : src = src & 0xffffffffffffffff; break;
  }
  switch (vsew) {
    case 0 : tmp_vreg[reg]._8[idx]  = (uint8_t  )src; break;
    case 1 : tmp_vreg[reg]._16[idx] = (uint16_t )src; break;
    case 2 : tmp_vreg[reg]._32[idx] = (uint32_t )src; break;
    case 3 : tmp_vreg[reg]._64[idx] = (uint64_t )src; break;
  }
}

void vreg_to_tmp_vreg(uint64_t reg, int idx, uint64_t vsew) {
  int new_reg = get_reg(reg, idx, vsew);
  int new_idx = get_idx(reg, idx, vsew);

  switch (vsew) {
    case 0 : tmp_vreg[new_reg - reg]._8[new_idx] = vreg_b(new_reg, new_idx); break;
    case 1 : tmp_vreg[new_reg - reg]._16[new_idx] = vreg_s(new_reg, new_idx); break;
    case 2 : tmp_vreg[new_reg - reg]._32[new_idx] = vreg_i(new_reg, new_idx); break;
    case 3 : tmp_vreg[new_reg - reg]._64[new_idx] = vreg_l(new_reg, new_idx); break;
  } 
}

void set_vreg_tail(uint64_t reg) {
  for (int i = 0; i < VLEN / 64; i++) {
    vreg_l(reg, i) = 0xffffffffffffffff;
  }
}

#endif // CONFIG_RVV