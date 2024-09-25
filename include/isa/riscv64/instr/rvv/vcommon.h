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
void set_NAN(rtlreg_t* fpreg, uint64_t vsew);
bool check_isFpCanonicalNAN(rtlreg_t* fpreg, uint64_t vsew);
void vp_set_dirty();

#endif
#endif