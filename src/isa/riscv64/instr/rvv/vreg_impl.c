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
#include <stdio.h>
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
  vtype_t vt = (vtype_t )vtype_req;
  rtlreg_t VLMAX = get_vlmax(vt.vsew, vt.vlmul);

  if (mode == 1) {
    return VLMAX;
  } else if (mode == 2) {
    return old_vl;
  } else {
    if (vt.vsew > 3) { //check if max-len supported
      return (uint64_t)-1; //return 0 means error, including vl_req is 0, for vl_req should not be 0.
    }
    if (vl_req <= VLMAX) {
        return vl_req;
    } else if (vl_req < 2 *VLMAX) {
        return vl_req / 2 + 1;
    } else {
        return VLMAX;
    }
  }
}

rtlreg_t get_mask(int reg, int idx, uint64_t vsew, uint64_t vlmul) {
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
  if (vlmul > 4) vlmul -= 8;
  return VLEN >> (3 + vsew - vlmul);
}

int get_vlen_max(int vsew, int vlmul) {
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

void get_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew, uint64_t vlmul, int is_signed, int needAlign) {
  // printf("get_vreg: reg = %lu, idx = %d, vsew = %lu, vlmul = %lu, is_signed = %d, needAlign = %d\n", reg, idx, vsew, vlmul, is_signed, needAlign);
  Assert(vlmul != 4, "vlmul = 4 is reserved\n");
  Assert(vsew <= 3, "vsew should be less than 4\n");
  if(needAlign && vlmul < 4) Assert(reg % (1 << vlmul) == 0, "vreg is not aligned\n");
  int new_reg = get_reg(reg, idx, vsew);
  int new_idx = get_idx(reg, idx, vsew);
  switch (vsew) {
    case 0 : *dst = is_signed ? (char)vreg_b(new_reg, new_idx) : vreg_b(new_reg, new_idx); break;
    case 1 : *dst = is_signed ? (short)vreg_s(new_reg, new_idx) : vreg_s(new_reg, new_idx); break;
    case 2 : *dst = is_signed ? (int)vreg_i(new_reg, new_idx) : vreg_i(new_reg, new_idx); break;
    case 3 : *dst = is_signed ? (long)vreg_l(new_reg, new_idx) : vreg_l(new_reg, new_idx); break;
  }
  // printf("get_reg: %lu idx: %d new_reg: %d new_idx: %d src: %lx\n", reg, idx, new_reg, new_idx, *dst);
}

void set_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew, uint64_t vlmul, int needAlign) {
  // printf("set_vreg: reg = %lu, idx = %d, vsew = %lu, vlmul = %lu, needAlign = %d\n", reg, idx, vsew, vlmul, needAlign);
  Assert(vlmul != 4, "vlmul = 4 is reserved\n");
  Assert(vsew <= 3, "vsew should be less than 4\n");
  if(needAlign && vlmul < 4) Assert(reg % (1 << vlmul) == 0, "vreg is not aligned\n");
  int new_reg = get_reg(reg, idx, vsew);
  int new_idx = get_idx(reg, idx, vsew);

  switch (vsew) {
    case 0 : src = src & 0xff; break;
    case 1 : src = src & 0xffff; break;
    case 2 : src = src & 0xffffffff; break;
    case 3 : src = src & 0xffffffffffffffff; break;
  }
  //printf("set_reg: %lu idx: %d new_reg: %d new_idx: %d src: %lx vsew: %lu\n", reg, idx, new_reg, new_idx, src, vsew);
  switch (vsew) {
    case 0 : vreg_b(new_reg, new_idx) = (uint8_t  )src; break;
    case 1 : vreg_s(new_reg, new_idx) = (uint16_t )src; break;
    case 2 : vreg_i(new_reg, new_idx) = (uint32_t )src; break;
    case 3 : vreg_l(new_reg, new_idx) = (uint64_t )src; break;
  }
}

void init_tmp_vreg() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < VLEN / 64; j++) {
      tmp_vreg[i]._64[j] = 0;
    }
  }
}

void get_tmp_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew) {
  // printf("get_vreg: reg = %lu, idx = %d, vsew = %lu\n", reg, idx, vsew);
  Assert(vsew <= 3, "vsew should be less than 4\n");
  switch (vsew) {
    case 0 : *dst = tmp_vreg[reg]._8[idx];  break;
    case 1 : *dst = tmp_vreg[reg]._16[idx]; break;
    case 2 : *dst = tmp_vreg[reg]._32[idx]; break;
    case 3 : *dst = tmp_vreg[reg]._64[idx]; break;
  }
  //printf("get_reg: %lu idx: %d new_reg: %d new_idx: %d src: %lx\n", reg, idx, new_reg, new_idx, *dst);
}

void set_tmp_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew) {
  // printf("set_vreg: reg = %lu, idx = %d, vsew = %lu\n", reg, idx, vsew);
  Assert(vsew <= 3, "vsew should be less than 4\n");

  switch (vsew) {
    case 0 : src = src & 0xff; break;
    case 1 : src = src & 0xffff; break;
    case 2 : src = src & 0xffffffff; break;
    case 3 : src = src & 0xffffffffffffffff; break;
  }
  //printf("set_reg: %lu idx: %d new_reg: %d new_idx: %d src: %lx vsew: %lu\n", reg, idx, new_reg, new_idx, src, vsew);
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

void longjmp_raise_intr(uint32_t foo) {
    assert(0);
}

#endif // CONFIG_RVV