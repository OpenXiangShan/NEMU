#include <ext/amuctrl.h>
#include <ext/amu_ctrl_queue_wrapper.h>
#include <ext/mstore_queue_wrapper.h>
#include <isa.h>
#include <memory/host.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef CONFIG_RV_AME
_Static_assert(offsetof(amu_ctrl_event_t, computeType) == 48 &&
  offsetof(amu_ctrl_event_t, isScale) == 49 &&
  offsetof(amu_ctrl_event_t, isConvertF32) == 50 &&
  offsetof(amu_ctrl_event_t, isDynamicMxfpQuant) == 51 &&
  sizeof(amu_ctrl_event_t) == 56, "AMU event v2 semantic prefix layout");
_Static_assert(sizeof(amu_matrix_view_t) == 24 && sizeof(amu_ctrl_snapshot_t) == 128 &&
  offsetof(amu_ctrl_snapshot_t, expected_tr) == 96 &&
  offsetof(amu_ctrl_snapshot_t, raised_flags) == 120 &&
  offsetof(amu_ctrl_snapshot_t, reserved) == 124 &&
  sizeof(amu_mma_snapshot_t) == 120, "AMU snapshot v2 layout");
#include "../isa/riscv64/instr/ame/mxfp.h"
#include "../isa/riscv64/instr/ame/mcompute_impl.h"
#include "../isa/riscv64/local-include/csr.h"

#ifndef MATRIX_DIFF_PRINT_CNT
#define MATRIX_DIFF_PRINT_CNT 32
#endif

amu_ctrl_event_t amu_ctrl_event_data;
static bool snapshot_admitted = false;

bool amu_snapshot_ready(void) {
  return snapshot_admitted;
}

void amu_snapshot_reset(void) {
  snapshot_admitted = false;
}

int amu_snapshot_negotiate(uint32_t version, uint32_t ctrl_size, uint32_t mma_size,
                           uint32_t event_prefix_size, uint32_t tile_bytes, uint32_t acc_bytes) {
  snapshot_admitted = version == AMU_CTRL_SNAPSHOT_VERSION &&
    version == AMU_MMA_SNAPSHOT_VERSION &&
    ctrl_size == sizeof(amu_ctrl_snapshot_t) && mma_size == sizeof(amu_mma_snapshot_t) &&
    event_prefix_size == sizeof(amu_ctrl_event_t) &&
    tile_bytes == TLEN / 8 && acc_bytes == ALEN / 8;
  return snapshot_admitted ? 0 : -2;
}

static void require_legacy_amu_verification(const amu_ctrl_event_t *event) {
  if (event->isScale || event->computeType || event->isConvertF32 || event->isDynamicMxfpQuant) {
    panic("MX verification unsupported: MmaVerifier has no scale snapshots "
          "(computeType=%u, isScale=%u)", event->computeType, event->isScale);
    abort();
  }
}

amu_ctrl_event_t get_amu_ctrl_info() {
  return amu_ctrl_event_data;
}

__attribute__((unused))
static void print_amu_ctrl_event(amu_ctrl_event_t *event) {
  fprintf(stderr, "[NEMU] debug: amu_ctrl_event@pc: %016lx, op = %d\n", event->pc, event->op);
  fprintf(stderr, "  computeType: %u, isScale: %u\n", event->computeType, event->isScale);
  fprintf(stderr, "  isConvertF32: %u, isDynamicMxfpQuant: %u\n",
    event->isConvertF32, event->isDynamicMxfpQuant);
  switch (event->op) {
    case 0:
      fprintf(stderr, "  md: %d, sat: %d, isfp: %d, rm: %d, ms1: %d, ms2: %d\n"
                      "  mtilem: %d, mtilen: %d, mtilek: %d, types1: %d, types2: %d, typed: %d\n",
                      event->md, event->sat, event->isfp, event->rm, event->ms1, event->ms2,
                      event->mtilem, event->mtilen, event->mtilek, event->types1, event->types2, event->typed);
      break;
    case 1:
      fprintf(stderr, "  ms: %d, ls: %d, transpose: %d, isacc: %d\n"
                      "  base: %016lx, stride: %016lx, row: %d, column: %d, msew: %d\n",
                      event->md, event->sat, event->isfp, event->types1,
                      event->base, event->stride, event->mtilem, event->mtilen, event->typed);
      break;
    case 2:
      fprintf(stderr, "  msyncRd: %d\n", event->mtilem);
      break;
    case 3:
      fprintf(stderr, "  md: %d, opType: %016lx\n", event->md, event->base);
      break;
    default:
      fprintf(stderr, "  unknown op!\n");
      break;
  }
}

static bool cmp_amu_ctrl(amu_ctrl_event_t *l, amu_ctrl_event_t *r) {
  bool cmp_mma = l->op == 0 && r->op == 0 && l->rm == r->rm && l->md == r->md
                 && l->sat == r->sat && l->isfp == r->isfp
                 && l->ms1 == r->ms1 && l->ms2 == r->ms2
                 && l->mtilem == r->mtilem && l->mtilen == r->mtilen && l->mtilek == r->mtilek
                 && l->types1 == r->types1 && l->types2 == r->types2 && l->typed == r->typed;
  bool cmp_mls = l->op == 1 && r->op == 1 && l->md == r->md && l->sat == r->sat
                 && l->isfp == r->isfp
                 && l->types1 == r->types1 && l->types2 == r->types2
                 && l->base == r->base && l->stride == r->stride 
                 && l->mtilem == r->mtilem && l->mtilen == r->mtilen
                 && l->typed == r->typed;
  bool cmp_mrelease = l->op == 2 && r->op == 2 && l->mtilem == r->mtilem;
  bool cmp_mzero = l->op == 3 && r->op == 3 && l->md == r->md && l->base == r->base;
  bool cmp_pc = l->pc == r->pc;
  bool cmp_metadata = l->computeType == r->computeType && l->isScale == r->isScale &&
    l->isConvertF32 == r->isConvertF32 && l->isDynamicMxfpQuant == r->isDynamicMxfpQuant;
  return !((cmp_mma || cmp_mls || cmp_mrelease || cmp_mzero) && cmp_pc && cmp_metadata);
}

int check_amu_ctrl(amu_ctrl_event_t *cmp) {
  int result = 0;
  if (amu_ctrl_queue_empty()) {
    Log("NEMU does not commit any AMU ctrl signals.");
    cmp->op = -1;
    cmp->rm = 0;
    cmp->md = 0;
    cmp->sat = 0;
    cmp->isfp = 0;
    cmp->ms1 = 0;
    cmp->ms2 = 0;
    cmp->mtilem = 0;
    cmp->mtilen = 0;
    cmp->mtilek = 0;
    cmp->types1 = 0;
    cmp->types2 = 0;
    cmp->typed = 0;
    cmp->base = 0;
    cmp->stride = 0;
    cmp->pc = 0;
    cmp->computeType = 0;
    cmp->isScale = 0;
    cmp->isConvertF32 = 0;
    cmp->isDynamicMxfpQuant = 0;
    result = -1;
  } else {
    amu_ctrl_event_data = amu_ctrl_queue_front();
    if (cmp_amu_ctrl(&amu_ctrl_event_data, cmp)) {
      // There're differences between NEMU and DUT
      // replace them with NEMU's data
      cmp->op = amu_ctrl_event_data.op;
      cmp->md = amu_ctrl_event_data.md;
      cmp->sat = amu_ctrl_event_data.sat;
      cmp->isfp = amu_ctrl_event_data.isfp;
      cmp->mtilem = amu_ctrl_event_data.mtilem;
      cmp->mtilen = amu_ctrl_event_data.mtilen;
      cmp->pc = amu_ctrl_event_data.pc;
      cmp->computeType = amu_ctrl_event_data.computeType;
      cmp->isScale = amu_ctrl_event_data.isScale;
      cmp->isConvertF32 = amu_ctrl_event_data.isConvertF32;
      cmp->isDynamicMxfpQuant = amu_ctrl_event_data.isDynamicMxfpQuant;
      switch (amu_ctrl_event_data.op) {
        case 0:
          // case MMA
          cmp->rm = amu_ctrl_event_data.rm;
          cmp->ms1 = amu_ctrl_event_data.ms1;
          cmp->ms2 = amu_ctrl_event_data.ms2;
          cmp->mtilek = amu_ctrl_event_data.mtilek;
          cmp->types1 = amu_ctrl_event_data.types1;
          cmp->types2 = amu_ctrl_event_data.types2;
          cmp->typed = amu_ctrl_event_data.typed;
          break;
        case 1:
          // case Matrix load/store
          cmp->types1 = amu_ctrl_event_data.types1;
          cmp->types2 = amu_ctrl_event_data.types2;
          cmp->base = amu_ctrl_event_data.base;
          cmp->stride = amu_ctrl_event_data.stride;
          cmp->typed = amu_ctrl_event_data.typed;
          break;
        case 2:
          // case Mrelease
          break;
        case 3:
          // case Marith
          cmp->base = amu_ctrl_event_data.base;
          break;
        default:
          Log("invalid AMU ctrl op");
          break;
      }
      result = 1;
    }
    if (result == 0) amu_ctrl_queue_complete_pending();
    else amu_ctrl_queue_pop();
  }
  return result;
}

int check_amu_ctrl_snapshot(amu_ctrl_event_t *cmp, amu_ctrl_snapshot_t *snapshot) {
#ifdef CONFIG_SHARE
  if (!amu_snapshot_ready()) return -2;
#endif
  if (amu_ctrl_queue_empty()) return -1;
  const int result = amu_ctrl_queue_copy_snapshot(snapshot);
  if (result != 0) return result;
  return check_amu_ctrl(cmp);
}

static bool amu_view_fits(const amu_matrix_view_t *view, unsigned rows, unsigned bytes) {
  return view->bytes && !view->reserved && view->row_pitch >= bytes &&
    (uint64_t)(rows - 1) * view->row_pitch + bytes <= view->capacity;
}

int exec_amu_lazy_snapshot(const amu_ctrl_event_t *event, amu_mma_snapshot_t *snapshot) {
#ifdef CONFIG_SHARE
  if (!amu_snapshot_ready()) return -2;
#endif
  if (event && event->isDynamicMxfpQuant) {
    require_mx_transport();
    return amu_ctrl_queue_apply_dynamic_snapshot(event, snapshot);
  }
  if (!event || !snapshot || snapshot->version != AMU_MMA_SNAPSHOT_VERSION ||
      snapshot->struct_size != sizeof(*snapshot) || snapshot->reserved ||
      !snapshot->scales || snapshot->scales->version != AMU_CTRL_SNAPSHOT_VERSION ||
      snapshot->scales->struct_size != sizeof(*snapshot->scales) ||
      snapshot->scales->reserved) return -2;
  const bool fp4 = event->computeType == 10;
  const bool e5m2 = event->computeType == 8;
  const unsigned m = event->mtilem, n = event->mtilen, k = event->mtilek;
  const unsigned row_bytes = fp4 ? (k + 1) / 2 : k;
  const unsigned blocks = (k + 31) / 32;
  const unsigned type = fp4 ? 7 : e5m2 ? 0 : 4;
  if (event->op != 0 || !event->isfp || event->rm || event->typed != 2 ||
      (event->computeType != 7 && event->computeType != 8 && !fp4) ||
      event->types1 != type || event->types2 != type || event->ms1 >= 4 ||
      event->ms2 >= 4 || event->md < 4 || event->md >= 8 ||
      !m || m > ROWNUM || !n || n > ROWNUM || !k || row_bytes > TRENUM8 ||
      n * 4 > ARENUM8 || blocks > AME_MAX_SCALE_BLOCKS ||
      !amu_view_fits(&snapshot->matrices[0], m, row_bytes) ||
      !amu_view_fits(&snapshot->matrices[1], n, row_bytes) ||
      !amu_view_fits(&snapshot->matrices[2], m, n * 4) ||
      !amu_view_fits(&snapshot->matrices[3], m, n * 4)) return -2;
  for (unsigned side = 0; side < 2; ++side) {
    const amu_scale_snapshot_t *scale = &snapshot->scales->scales[side];
    const amu_scale_metadata_t *meta = &scale->metadata;
    if (!scale->bytes || !meta->valid || !meta->generation || meta->reserved ||
        meta->side != side || meta->reg_id != (side == 0 ? event->ms1 : event->ms2) ||
        meta->rows < (side == 0 ? m : n) || meta->rows > ROWNUM ||
        meta->blocks < blocks || meta->blocks > AME_MAX_SCALE_BLOCKS ||
        meta->row_pitch != meta->blocks || meta->source_stride < meta->blocks ||
        (uint64_t)meta->rows * meta->row_pitch > scale->capacity) return -2;
  }
  // Validate every view and both frozen scale sides before any output/state
  // write. Read physical REF rows directly into bounded logical views; never
  // memcpy full TLEN/ALEN into a compact caller allocation.
  for (unsigned row = 0; row < m; ++row) {
    memcpy(snapshot->matrices[0].bytes + row * snapshot->matrices[0].row_pitch,
      cpu.mtr[event->ms1][row]._8, row_bytes);
    memcpy(snapshot->matrices[2].bytes + row * snapshot->matrices[2].row_pitch,
      cpu.macc[event->md - 4][row]._8, n * 4);
  }
  for (unsigned row = 0; row < n; ++row) {
    memcpy(snapshot->matrices[1].bytes + row * snapshot->matrices[1].row_pitch,
      cpu.mtr[event->ms2][row]._8, row_bytes);
  }
  uint8_t flags = 0;
  for (unsigned i = 0; i < m; ++i) {
    for (unsigned j = 0; j < n; ++j) {
      uint32_t acc;
      memcpy(&acc, snapshot->matrices[2].bytes + i * snapshot->matrices[2].row_pitch + j * 4, 4);
      for (unsigned base = 0; base < k; base += 32) {
        const unsigned lanes = k - base < 32 ? k - base : 32;
        uint8_t a[32], b[32];
        for (unsigned lane = 0; lane < lanes; ++lane) {
          const unsigned index = base + lane;
          const uint8_t av = snapshot->matrices[0].bytes[
            i * snapshot->matrices[0].row_pitch + (fp4 ? index / 2 : index)];
          const uint8_t bv = snapshot->matrices[1].bytes[
            j * snapshot->matrices[1].row_pitch + (fp4 ? index / 2 : index)];
          a[lane] = fp4 ? (av >> ((index & 1) * 4)) & 15 : av;
          b[lane] = fp4 ? (bv >> ((index & 1) * 4)) & 15 : bv;
        }
        const amu_scale_snapshot_t *sa = &snapshot->scales->scales[0];
        const amu_scale_snapshot_t *sb = &snapshot->scales->scales[1];
        acc = mxfp_accumulate_block(acc, a, b, lanes,
          sa->bytes[i * sa->metadata.row_pitch + base / 32],
          sb->bytes[j * sb->metadata.row_pitch + base / 32], fp4, e5m2, &flags);
      }
      memcpy(snapshot->matrices[3].bytes + i * snapshot->matrices[3].row_pitch + j * 4, &acc, 4);
      cpu.macc[event->md - 4][i]._32[j] = acc;
    }
  }
  snapshot->raised_flags = flags;
  mfflags->val |= flags;
  mcsr->mfflags = mfflags->val;
  return 0;
}

static void exec_amu_load_store(void *amu_ctrl, void *res) {
  uint8_t ms = ((amu_ctrl_event_t *)amu_ctrl)->md;
  uint8_t ls = ((amu_ctrl_event_t *)amu_ctrl)->sat;
  uint64_t base = ((amu_ctrl_event_t *)amu_ctrl)->base;
  uint64_t stride = ((amu_ctrl_event_t *)amu_ctrl)->stride;
  uint32_t row = ((amu_ctrl_event_t *)amu_ctrl)->mtilem;
  uint32_t column = ((amu_ctrl_event_t *)amu_ctrl)->mtilen;
  uint32_t msew = ((amu_ctrl_event_t *)amu_ctrl)->typed;
  bool transpose = ((amu_ctrl_event_t *)amu_ctrl)->isfp;
  char m_name = ((amu_ctrl_event_t *)amu_ctrl)->types1 ? 'c' : (((amu_ctrl_event_t *)amu_ctrl)->types2 ? 'a' : 'b');
  if (m_name == 'b') {
    uint32_t tmp = row;
    row = column;
    column = tmp;
  }
  if (msew == 7) {
    // Packed FP4 MLS retains the logical K in its control event. The existing
    // physical host loader reads consecutive packed bytes at SWROB head.
    Assert(ls == 0 && ms < 4 && !transpose && m_name != 'c' &&
           row <= ROWNUM && column <= 2 * TRENUM8, "unsupported packed FP4 MLS");
    for (unsigned r = 0; r < row; ++r) {
      for (unsigned k = 0; k < column; ++k) {
        const uint8_t packed = host_read(guest_to_host(base + r * stride + k / 2), 1);
        set_mtreg_nibble(ms, r, k, (packed >> ((k & 1) * 4)) & 0x0f);
      }
    }
    return;
  }
  if (ls == 0) { // matrix load
    host_read_matrix(base, stride, row, column, msew, transpose, m_name, ms);
  } else { // matrix store
    Assert(ls == 1, "AmuCtrl.ls should be 0 or 1!");
    host_write_matrix(base, stride, row, column, msew, transpose, m_name, ms);
  }
}

static void exec_amu_release(void *amu_ctrl) {
  uint8_t msyncRd = check_mtok_idx(((amu_ctrl_event_t *)amu_ctrl)->mtilem);
  cpu.mtokr[msyncRd]++;
  mstore_queue_update_mrelease(msyncRd, cpu.mtokr[msyncRd]);
}

static void exec_amu_arith(void *amu_ctrl) {
  uint8_t md = ((amu_ctrl_event_t *)amu_ctrl)->md;
  if (md < 4) {
    memset(cpu.mtr[md], 0, TLEN / 8);
  } else {
    memset(cpu.macc[md - 4], 0, ALEN / 8);
  }
}

static bool normal_observed_tile_load(const amu_ctrl_event_t *event) {
  return event->op == 1 && event->sat == 0 && !event->isScale &&
    !event->isfp && !event->types1 && event->md < 4 &&
    !event->computeType && (event->typed == 0 || event->typed == 7);
}

static bool mx_load_extent(const amu_ctrl_event_t *event, unsigned *rows,
                            unsigned *bytes, uint8_t *last_mask) {
  const unsigned k = event->types2 ? event->mtilen : event->mtilem;
  *rows = event->types2 ? event->mtilem : event->mtilen;
  *bytes = event->typed == 7 ? (k + 1) / 2 : k;
  *last_mask = event->typed == 7 && (k & 1) ? 0x0f : 0xff;
  return *rows && *rows <= ROWNUM && k && *bytes <= TRLEN / 8;
}

static int compare_mx_load(const amu_ctrl_event_t *event, const void *res) {
  unsigned rows, bytes;
  uint8_t last_mask;
  if (!res || !mx_load_extent(event, &rows, &bytes, &last_mask)) return 1;
  const uint8_t *dut = res;
  const uint8_t *ref = (const uint8_t *)cpu.mtr[event->md];
  int different = 0, printed = 0;
  // Normal-load observations contain written bytes, not full register readback.
  // Derive the checked extent from the semantic event, never from DUT masks.
  for (unsigned row = 0; row < rows; ++row) {
    for (unsigned col = 0; col < bytes; ++col) {
      const size_t offset = (size_t)row * (TRLEN / 8) + col;
      const uint8_t mask = col + 1 == bytes ? last_mask : 0xff;
      if (((dut[offset] ^ ref[offset]) & mask) != 0) {
        different++;
        if (printed++ < MATRIX_DIFF_PRINT_CNT) {
          fprintf(stderr, "active MX load diff pc=%016lx md=%u row=%u byte=%u "
            "mask=%02x REF=%02x DUT=%02x\n", event->pc, event->md, row, col,
            mask, ref[offset], dut[offset]);
        }
      }
    }
  }
  return different != 0;
}

int exec_amu(void *amu_ctrl, void *res) {
  require_legacy_amu_verification((const amu_ctrl_event_t *)amu_ctrl);
  bool ret = 0;
  uint8_t op = ((amu_ctrl_event_t *)amu_ctrl)->op;
  uint8_t md = ((amu_ctrl_event_t *)amu_ctrl)->md;
  switch (op) {
    case 0: // case MMA
      panic("MMA should be executed by exec_amu_lazy");
      ret = 1;
      break;
    case 1: // case Matrix load/store
      if (normal_observed_tile_load(amu_ctrl)) {
        unsigned rows, bytes;
        uint8_t mask;
        if (!res || !mx_load_extent(amu_ctrl, &rows, &bytes, &mask)) return 1;
        exec_amu_load_store(amu_ctrl, res);
        return compare_mx_load(amu_ctrl, res);
      }
      exec_amu_load_store(amu_ctrl, res);
      // Compare the result with the expected result
      // When the result is not equal, set the return value to 1
      if (((amu_ctrl_event_t *)amu_ctrl)->sat == 0) {
        // only check matrix load
        if (md < 4) {
          ret = memcmp(res, cpu.mtr[md], TLEN / 8) != 0;
        } else {
          ret = memcmp(res, cpu.macc[md - 4], ALEN / 8) != 0;
        }
        if (ret) {
          uint64_t ref_val, dut_val;
          int total_diff = 0;
          int printed = 0;
          fprintf(stderr, "matrix diff md=%d @pc: %016lx\n", md, ((amu_ctrl_event_t *)amu_ctrl)->pc);
          if (md < 4) {
            const int cols = TRLEN / 64;
            for (int row = 0; row < ROWNUM; row++) {
              for (int idx = 0; idx < cols; idx++) {
                ref_val = cpu.mtr[md][row]._64[idx];
                dut_val = ((uint64_t *)res)[row * cols + idx];
                if (ref_val != dut_val) {
                  total_diff++;
                  if (printed < MATRIX_DIFF_PRINT_CNT) {
                    fprintf(stderr, "  [row=%d, idx=%d] REF=%016lx DUT=%016lx\n",
                            row, idx, ref_val, dut_val);
                    printed++;
                  }
                }
              }
            }
          } else {
            const int cols = ARLEN / 64;
            for (int row = 0; row < ROWNUM; row++) {
              for (int idx = 0; idx < cols; idx++) {
                ref_val = cpu.macc[md - 4][row]._64[idx];
                dut_val = ((uint64_t *)res)[row * cols + idx];
                if (ref_val != dut_val) {
                  total_diff++;
                  if (printed < MATRIX_DIFF_PRINT_CNT) {
                    fprintf(stderr, "  [row=%d, idx=%d] REF=%016lx DUT=%016lx\n",
                            row, idx, ref_val, dut_val);
                    printed++;
                  }
                }
              }
            }
          }
          fprintf(stderr, "diff summary: %d element(s) differ\n", total_diff);
          if (total_diff > MATRIX_DIFF_PRINT_CNT) {
            fprintf(stderr, "  ... and %d more (change MATRIX_DIFF_PRINT_CNT to show more)\n",
                    total_diff - MATRIX_DIFF_PRINT_CNT);
          }
        }
      }
      break;
    case 2: // case Mrelease
      exec_amu_release(amu_ctrl);
      // nothing to compare here.
      break;
    case 3: // case Marith, execute the operation
      exec_amu_arith(amu_ctrl);
      // Compare the result with the expected result
      // When the result is not equal, set the return value to 1
      if (md < 4) {
        ret = memcmp(res, cpu.mtr[md], TLEN / 8) != 0;
      } else {
        ret = memcmp(res, cpu.macc[md - 4], ALEN / 8) != 0;
      }
      break;
    default:
      panic("invalid AMU ctrl op");
      ret = 1;
      break;
  }
  return ret;
}

void exec_amu_lazy(void *amu_ctrl, void *res, void *src1, void *src2, void *src3) {
  require_legacy_amu_verification((const amu_ctrl_event_t *)amu_ctrl);
  Assert(((amu_ctrl_event_t *)amu_ctrl)->op == 0, "MMA should be executed by exec_amu_lazy");
  // 0. get md, ms1, and ms2 from amu_ctrl
  uint8_t md = ((amu_ctrl_event_t *)amu_ctrl)->md;
  uint8_t ms1 = ((amu_ctrl_event_t *)amu_ctrl)->ms1;
  uint8_t ms2 = ((amu_ctrl_event_t *)amu_ctrl)->ms2;

  // 1. cp acc[md] to src3
  memcpy(src3, cpu.macc[md - 4], ALEN / 8);
  // 2. cp tr[ms1] to src1
  memcpy(src1, cpu.mtr[ms1], TLEN / 8);
  // 3. cp tr[ms2] to src2
  memcpy(src2, cpu.mtr[ms2], TLEN / 8);
  // 4. cp res to acc[md]
  memcpy(cpu.macc[md - 4], res, ALEN / 8);
}

#endif // CONFIG_RV_AME
