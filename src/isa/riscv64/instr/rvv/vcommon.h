#include <common.h>
#ifdef CONFIG_RVV
#ifndef __RISCV64_VCOMMON_H__
#define __RISCV64_VCOMMON_H__

#include "cpu/exec.h"
#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include "../local-include/rtl.h"

uint8_t check_vstart_ignore(Decode *s);
bool check_vlmul_sew_illegal(rtlreg_t vtype_req);

#endif
#endif