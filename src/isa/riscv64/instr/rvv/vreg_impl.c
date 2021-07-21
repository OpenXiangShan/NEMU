#include <common.h>
#ifdef CONFIG_RVV_010

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


rtlreg_t check_vsetvl(rtlreg_t vtype_req, rtlreg_t vl_req, bool max_req) {
  vtype_t vt = (vtype_t )vtype_req;
  rtlreg_t vl_group = ((VLEN >> 3) >> vt.vsew) << vt.vlmul;
  if(max_req == true) {
    return vl_group;
  }else if(vt.vsew > 3) { //check if max-len supported
    return (uint64_t)-1; //return 0 means error, including vl_req is 0, for vl_req should not be 0.
  }else 
    return vl_req <= vl_group ? vl_req : vl_group;
  // if(vl_group >= vl_req) {
  //   return vl_req;
  // } else if(vl_req >= 2 * vl_group) {
  //   return vl_group;
  // } else {
  //   return vl_req / 2 + 1;
  // }
}

rtlreg_t get_mask(int reg, int idx, uint64_t vsew, uint64_t vlmul) {
  // int sum = VLEN / ((1 << vsew) * 8) * vlmul;
  int sum = ((VLEN >> vsew) >> 3) << vlmul;
  int single = VLEN / sum; //(1 << vsew * 8) / vlmul;
  int bit_idx = idx * single;
  int idx1 = bit_idx / 64;
  int idx2 = bit_idx % 64;
  
  return (rtlreg_t)((cpu.vr[reg]._64[idx1] & (1lu << idx2)) != 0);
}

void set_mask(uint32_t reg, int idx, uint64_t mask, uint64_t vsew, uint64_t vlmul) {
  int sum = ((VLEN >> vsew) >> 3) << vlmul;
  int single = VLEN / sum; //(1 << vsew * 8) / vlmul;
  int bit_idx = idx * single;
  int idx1 = bit_idx / 64;
  int idx2 = bit_idx % 64;

  uint64_t clear_bit = 0;
  for(int i=0; i < single; i++) {
    clear_bit  = clear_bit << 1;
    clear_bit |= 1;
  } // get single-bit 1-string
  cpu.vr[(int)reg]._64[idx1] &= ~(clear_bit << idx2); // clear the dest position.
  cpu.vr[(int)reg]._64[idx1] |= (mask==0) ? 0 : (1lu << idx2);
}

void get_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew, uint64_t vlmul, int is_signed, int needAlign) {
  Assert(vlmul <= 3, "vlmul should be less than 4\n");
  Assert(vsew <= 3, "vsew should be less than 4\n");
  if(needAlign) Assert(reg % (1 << vlmul) == 0, "vreg is not aligned\n");
  int new_vlmul = 1 << vlmul;
  int width = (1 << vsew);
  int width_bit = width * 8;
  int new_reg = reg + (idx * width_bit) / SLEN % new_vlmul;
  int new_idx = (idx * width_bit) / (SLEN * new_vlmul) * (SLEN / width_bit)
                + idx % (SLEN / width_bit);
  switch (vsew) {
    case 0 : *dst = is_signed ? (char)vreg_b(new_reg, new_idx) : vreg_b(new_reg, new_idx); break;
    case 1 : *dst = is_signed ? (short)vreg_s(new_reg, new_idx) : vreg_s(new_reg, new_idx); break;
    case 2 : *dst = is_signed ? (int)vreg_i(new_reg, new_idx) : vreg_i(new_reg, new_idx); break;
    case 3 : *dst = is_signed ? (long)vreg_l(new_reg, new_idx) : vreg_l(new_reg, new_idx); break;
  }
  // printf("get_vreg: idx:%d reg:%lu new_idx:%d new_reg:%d vsew:%lu vlmul:%lu\n", 
  //         idx, reg, new_idx, new_reg, vsew, vlmul);
}

void set_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew, uint64_t vlmul, int needAlign) {
  Assert(vlmul <= 3, "vlmul should be less than 4\n");
  Assert(vsew <= 3, "vsew should be less than 4\n");
  if(needAlign) Assert(reg % (1 << vlmul) == 0, "vreg is not aligned\n");
  int new_vlmul = 1 << vlmul;
  int width = (1 << vsew);
  int width_bit = width * 8;
  int new_reg = reg + (idx * width_bit) / SLEN % new_vlmul;
  int new_idx = (idx * width_bit) / (SLEN * new_vlmul) * (SLEN / width_bit)
                + idx % (SLEN / width_bit);
  switch (vtype->vsew) {
    case 0 : vreg_b(new_reg, new_idx) = (uint8_t  )src; break;
    case 1 : vreg_s(new_reg, new_idx) = (uint16_t )src; break;
    case 2 : vreg_i(new_reg, new_idx) = (uint32_t )src; break;
    case 3 : vreg_l(new_reg, new_idx) = (uint64_t )src; break;
  }
  // printf("set_vreg: idx:%d reg:%lu new_idx:%d new_reg:%d vsew:%lu vlmul:%lu\n", 
  //         idx, reg, new_idx, new_reg, vsew, vlmul);
}

void longjmp_raise_intr(uint32_t foo) {
    assert(0);
}

#endif // CONFIG_RVV_010