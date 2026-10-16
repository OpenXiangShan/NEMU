/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <cpu/difftest/ame/amuctrl.h>
#include "amuhash.h"

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

AMU_HASH_TARGET_BASELINE
amu_hash_t hash_amu_region(const amu_write_region_t *region) {
  const size_t active_row_bytes = region->active_columns * region->element_bytes;
  if (region->active_rows == 0 || active_row_bytes == 0) return (amu_hash_t){0};

  gf128_t hash;
#ifdef AMU_HASH_X86
  if (!hash_amu_x86_try(region->reg_data, region->reg_row_bytes,
                        region->active_rows, active_row_bytes, &hash))
#endif
    hash = hash_amu_portable(region->reg_data, region->reg_row_bytes,
                             region->active_rows, active_row_bytes);
  return (amu_hash_t){.lo = hash.lo, .hi = hash.hi,
                      .bytes = region->active_rows * active_row_bytes};
}

#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
