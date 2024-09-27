#ifndef __VREG_IMPL_H__
#define __VREG_IMPL_H__
#include "common.h"

rtlreg_t check_vsetvl(rtlreg_t vtype_req, rtlreg_t vl_req, int mode);
rtlreg_t get_mask(int reg, int idx);
void set_mask(uint32_t reg, int idx, uint64_t mask, uint64_t vsew, uint64_t vlmul);
int get_vlmax(int vsew, int vlmul);
int get_vlen_max(int vsew, int vlmul, int widening);
int get_reg(uint64_t reg, int idx, uint64_t vsew);
int get_idx(uint64_t reg, int idx, uint64_t vsew);
void get_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew, uint64_t vlmul, int is_signed, int needAlign);
void get_vreg_with_addr(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew, uint64_t vlmul, int is_signed, int needAlign, void **addr);
void set_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew, uint64_t vlmul, int needAlign);
void get_tmp_vreg(uint64_t reg, int idx, rtlreg_t *dst, uint64_t vsew);
void set_tmp_vreg(uint64_t reg, int idx, rtlreg_t src, uint64_t vsew);
void vreg_to_tmp_vreg(uint64_t reg, int idx, uint64_t vsew);
void set_vreg_tail(uint64_t reg);
void isa_misalign_vreg_check(uint64_t reg, uint64_t vlmul, int needAlign);


#endif
