#ifndef __AME_AMUCTRL_H__
#define __AME_AMUCTRL_H__

#include <stddef.h>
#include <stdint.h>

#include <ame/event.h>

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
typedef struct {
  // Full architectural register layout, independent of this instruction's tile.
  const uint8_t *reg_data;
  size_t reg_size_bytes;
  size_t reg_row_bytes;
  size_t active_rows;
  size_t active_columns;
  size_t element_bytes;
} amu_write_region_t;

int check_amu_ctrl(amu_ctrl_event_t *cmp);
amu_ctrl_event_t get_amu_ctrl_info();
int exec_amu(void *amu_ctrl, void *res);
int exec_amu_hash(void *amu_ctrl, amu_hash_t *res);
void exec_amu_lazy(void *amu_ctrl, void *res, void *src1, void *src2, void *src3);

// Each row's first active_columns * element_bytes bytes are covered; the rest is unwritten.
// Require the covered bytes to fit in each row and storage for active_rows full rows.
// An empty region returns zero without reading its data.
amu_hash_t hash_amu_region(const amu_write_region_t *region);
#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

#endif // __AME_AMUCTRL_H__
