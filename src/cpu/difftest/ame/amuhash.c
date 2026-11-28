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
#include "amuhash_math.h"

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

AMU_HASH_TARGET_BASELINE
amu_hash_t hash_amu_region(const amu_write_region_t *region) {
  const size_t active_row_bytes = amu_active_row_bytes(region);
  uint64_t hash = 0;
  if (!region->element_bytes && (region->active_columns & 1)) {
    for (size_t r = region->active_rows; r-- > 0;) {
      if (r + 1 != region->active_rows)
        hash = hash_advance_unwritten(hash, region->reg_row_bytes - active_row_bytes);
      const uint8_t *row = region->reg_data + r * region->reg_row_bytes;
      hash = gf32_shift(hash, 9) ^ (0x10 | (row[active_row_bytes - 1] & 15));
      hash = hash_update_data(hash, row, active_row_bytes - 1);
    }
  } else if (region->active_rows != 0 && active_row_bytes != 0) {
#ifdef AMU_HASH_X86
    if (!hash_amu_x86_try(region->reg_data, region->reg_row_bytes,
                          region->active_rows, active_row_bytes, &hash))
#endif
      hash = hash_amu_portable(region->reg_data, region->reg_row_bytes,
                               region->active_rows, active_row_bytes);
  }
  return (amu_hash_t){.lo = hash_finalize(hash, region->reg_size_bytes), .hi = 0,
                      .bytes = region->active_rows * active_row_bytes};
}

#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
