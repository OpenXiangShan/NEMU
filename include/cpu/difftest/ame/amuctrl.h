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

#define AMU_CTRL_SNAPSHOT_VERSION 2u
#define AMU_MMA_SNAPSHOT_VERSION 2u

typedef struct {
  uint32_t generation;
  uint16_t rows;
  uint16_t blocks;
  uint32_t row_pitch;
  uint8_t reg_id;
  uint8_t side;
  uint8_t valid;
  uint8_t reserved;
  uint64_t source_stride;
} amu_scale_metadata_t;

typedef struct {
  uint8_t *bytes;
  uint64_t capacity;
  amu_scale_metadata_t metadata;
} amu_scale_snapshot_t;

typedef struct {
  uint8_t *bytes;
  uint64_t capacity;
  uint32_t row_pitch;
  uint32_t reserved;
} amu_matrix_view_t;

typedef struct {
  uint32_t version;
  uint32_t struct_size;
  uint64_t epoch;
  amu_scale_snapshot_t scales[2];
  amu_matrix_view_t expected_tr;
  uint32_t raised_flags;
  uint32_t reserved;
} amu_ctrl_snapshot_t;

typedef struct {
  uint32_t version;
  uint32_t struct_size;
  const amu_ctrl_snapshot_t *scales;
  amu_matrix_view_t matrices[4];
  uint32_t raised_flags;
  uint32_t reserved;
} amu_mma_snapshot_t;

#if defined(__cplusplus)
static_assert(sizeof(amu_scale_metadata_t) == 24, "AMU scale metadata ABI changed");
static_assert(sizeof(amu_matrix_view_t) == 24, "AMU matrix view ABI changed");
static_assert(sizeof(amu_ctrl_snapshot_t) == 128, "AMU control snapshot ABI changed");
static_assert(offsetof(amu_ctrl_snapshot_t, expected_tr) == 96, "AMU expected TR offset changed");
static_assert(offsetof(amu_ctrl_snapshot_t, raised_flags) == 120, "AMU flags offset changed");
static_assert(offsetof(amu_ctrl_snapshot_t, reserved) == 124, "AMU reserved offset changed");
static_assert(sizeof(amu_mma_snapshot_t) == 120, "AMU MMA snapshot ABI changed");
#else
_Static_assert(sizeof(amu_scale_metadata_t) == 24, "AMU scale metadata ABI changed");
_Static_assert(sizeof(amu_matrix_view_t) == 24, "AMU matrix view ABI changed");
_Static_assert(sizeof(amu_ctrl_snapshot_t) == 128, "AMU control snapshot ABI changed");
_Static_assert(offsetof(amu_ctrl_snapshot_t, expected_tr) == 96, "AMU expected TR offset changed");
_Static_assert(offsetof(amu_ctrl_snapshot_t, raised_flags) == 120, "AMU flags offset changed");
_Static_assert(offsetof(amu_ctrl_snapshot_t, reserved) == 124, "AMU reserved offset changed");
_Static_assert(sizeof(amu_mma_snapshot_t) == 120, "AMU MMA snapshot ABI changed");
#endif

int check_amu_ctrl(amu_ctrl_event_t *cmp);
int check_amu_ctrl_snapshot(amu_ctrl_event_t *cmp, amu_ctrl_snapshot_t *snapshot);
amu_ctrl_event_t get_amu_ctrl_info();
int exec_amu(void *amu_ctrl, void *res);
int exec_amu_hash(void *amu_ctrl, amu_hash_t *res);
void exec_amu_lazy(void *amu_ctrl, void *res, void *src1, void *src2, void *src3);
int exec_amu_lazy_snapshot(const amu_ctrl_event_t *event, amu_mma_snapshot_t *snapshot);
int exec_amu_scaled_mma_snapshot(const amu_ctrl_event_t *event,
                                 const uint8_t *scale_a, size_t scale_a_size,
                                 const uint8_t *scale_b, size_t scale_b_size,
                                 amu_mma_snapshot_t *snapshot);
int amu_snapshot_negotiate(uint32_t version, uint32_t ctrl_size, uint32_t mma_size,
                           uint32_t event_prefix_size, uint32_t tile_bytes,
                           uint32_t acc_bytes);
bool amu_snapshot_ready(void);
void amu_snapshot_reset(void);

// Each row's first active_columns * element_bytes bytes are covered; the rest is unwritten.
// Require the covered bytes to fit in each row and storage for active_rows full rows.
// An empty region returns the ISO CRC of the all-unwritten register without
// reading its data. The register capacity remains part of the canonical stream.
amu_hash_t hash_amu_region(const amu_write_region_t *region);
#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

#endif // __AME_AMUCTRL_H__
