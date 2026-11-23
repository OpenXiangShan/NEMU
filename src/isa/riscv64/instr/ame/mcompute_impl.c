/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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
#include <stdint.h>
#include <stdlib.h>

#ifdef CONFIG_RV_AME

#include "mcompute_impl.h"
#include <cpu/cpu.h>
#include "mcommon.h"
#include <ext/amuctrl.h>

void require_matrix() {
  // if (mstatus->ms == 0) {
  //   longjmp_exception(EX_II);
  // }
  // #ifdef CONFIG_RVH
  // if (cpu.v && vsstatus->ms == 0) {
  //   longjmp_exception(EX_II);
  // }
  // #endif
}

bool is_mx_mcfg(mcfg_t cfg) {
  return cfg.table_set == 1 &&
    (cfg.type_code == MTYPECODE_MXFP4 ||
     cfg.type_code == MTYPECODE_FP8E5M2 ||
     cfg.type_code == MTYPECODE_FP8E4M3);
}

void require_mx_transport(void) {
#if defined(CONFIG_SHARE_REF) && defined(CONFIG_DIFFTEST_AMU_CTRL) && !defined(CONFIG_SHARE_CTRL)
  if (amu_snapshot_ready()) return;
#endif
#if defined(CONFIG_SHARE_REF) || defined(CONFIG_SHARE_CTRL) || defined(CONFIG_DIFFTEST_AMU_CTRL)
  // The independent snapshot path must be explicitly ABI/capacity-admitted.
  // Neither an old four-pointer lazy client nor shared controller mode may
  // silently execute an MX instruction as an unscaled operation.
  panic("MX transport unsupported: independent snapshot ABI/capacities not "
        "negotiated for this reference session");
  abort(); // Fail closed even when the host was compiled with NDEBUG.
#endif
}

uint8_t mx_compute_type(mcfg_t cfg) {
  if (!is_mx_mcfg(cfg)) return 0;
  switch (cfg.type_code) {
    case MTYPECODE_MXFP4: return 10;
    case MTYPECODE_FP8E5M2: return 8;
    case MTYPECODE_FP8E4M3: return 7;
    default: return 0;
  }
}

uint8_t get_packed_mreg4(int reg, int row, int k) {
  const uint8_t packed = mtreg_l8(reg, row, k >> 1);
  return (k & 1) ? (packed >> 4) : (packed & 0xf);
}

#endif // CONFIG_RV_AME
