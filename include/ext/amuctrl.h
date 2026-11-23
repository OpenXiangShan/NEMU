#ifndef __EXT_AMUCTRL_H__
#define __EXT_AMUCTRL_H__

#include <common.h>

#ifdef CONFIG_RV_AME

// Attention: the order of fields in this class must be the same as the order
//            in the corresponding struct in XiangShan(or XSAI)'s diffstate.h.
typedef struct  {
  uint8_t  valid;
  uint8_t  op;
  uint8_t  rm;
  uint8_t  md;       // also used as ms
  uint8_t  sat;      // also used as ls
  uint8_t  ms1;
  uint8_t  ms2;
  uint16_t mtilem;   // also used as row
  uint16_t mtilen;   // also used as column
  uint16_t mtilek;
  uint8_t  types1;   // also used as isacc
  uint8_t  types2;
  uint8_t  typed;    // also used as widths
  uint8_t  isfp;     // also used as transpose
  uint64_t base;
  uint64_t stride;
  uint64_t pc;
  // Append-only; mirrored after pc in difftest/AmuCtrlEvent's schema.
  uint8_t computeType;
  uint8_t isScale;
  uint8_t isConvertF32;
  uint8_t isDynamicMxfpQuant;
} amu_ctrl_event_t;

// Versioned extension of the existing control-pop API. The semantic event
// prefix above is unchanged. Callers own the destination buffers; queue entries
// own the immutable source bytes, independently of later reloads and MINIT.
#define AMU_CTRL_SNAPSHOT_VERSION 2u
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

// The same bounded matrix view is used by the owned control result and the
// existing lazy MMA seam. No pointer returned here may alias live REF state.
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
  amu_scale_snapshot_t scales[2]; // side0=A, side1=B; not architectural reg IDs.
  amu_matrix_view_t expected_tr; // dynamic: compact 8 rows x 32 packed bytes.
  uint32_t raised_flags; // producer-local status, never sticky flags or XOR.
  uint32_t reserved;
} amu_ctrl_snapshot_t;

// Bounded logical views for the existing lazy execution seam. A/B rows are
// packed consecutive-K bytes; C/expected rows are binary32. This API has no
// DUT-result input: REF computes and advances only its own expected result.
#define AMU_MMA_SNAPSHOT_VERSION 2u

typedef struct {
  uint32_t version;
  uint32_t struct_size;
  const amu_ctrl_snapshot_t *scales;
  amu_matrix_view_t matrices[4]; // A, B, C input, independently computed expected.
  uint32_t raised_flags;
  uint32_t reserved;
} amu_mma_snapshot_t;

// Host-session admission at the existing snapshot seam. No guest/environment
// switch: an old or incompatible client must retain the shared-MX refusal.
#ifdef __cplusplus
extern "C" {
#endif
int amu_snapshot_negotiate(uint32_t version, uint32_t ctrl_size, uint32_t mma_size,
                           uint32_t event_prefix_size, uint32_t tile_bytes, uint32_t acc_bytes);
bool amu_snapshot_ready(void);
void amu_snapshot_reset(void);

int check_amu_ctrl(amu_ctrl_event_t *cmp);
// -2: incompatible descriptor/insufficient capacity; do not consume the queue.
int check_amu_ctrl_snapshot(amu_ctrl_event_t *cmp, amu_ctrl_snapshot_t *snapshot);
int exec_amu_lazy_snapshot(const amu_ctrl_event_t *event, amu_mma_snapshot_t *snapshot);
amu_ctrl_event_t get_amu_ctrl_info();
int exec_amu(void *amu_ctrl, void *res);
void exec_amu_lazy(void *amu_ctrl, void *res, void *src1, void *src2, void *src3);
#ifdef __cplusplus
}
#endif

#endif // CONFIG_RV_AME
#endif // __EXT_AMUCTRL_H__
