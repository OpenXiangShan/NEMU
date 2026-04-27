#include <common.h>
#ifdef CONFIG_RVV
#ifndef __RISCV64_VCOMMON_H__
#define __RISCV64_VCOMMON_H__

#include "cpu/exec.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"

uint8_t check_vstart_ignore(Decode *s);
void check_vstart_exception(Decode *s);
bool check_vlmul_sew_illegal(rtlreg_t vtype_req);
uint32_t fp_type_from_vsew(uint64_t vsew);
void set_NAN(rtlreg_t* fpreg, uint32_t fp_type);
bool check_isFpCanonicalNAN(rtlreg_t* fpreg, uint32_t fp_type);
void vp_set_dirty();

int get_mode(Decode *s);
void set_vtype_vl(Decode *s, int mode);
void predecode_vls(Decode *s);

#endif
#endif