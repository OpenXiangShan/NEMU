#include <cpu/difftest/ame/amuctrl.h>
#include <cpu/difftest/ame/amu_ctrl_queue_wrapper.h>
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
#include <ame/mstore_queue_wrapper.h>
#endif // CONFIG_AME_MEM_ACCESS_CHECK
#include <isa.h>
#include <memory/paddr.h>
#include <ame/raw-fp4.h>
#include "../../../isa/riscv64/instr/ame/mcompute_impl.h"

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

#ifndef MATRIX_DIFF_PRINT_CNT
#define MATRIX_DIFF_PRINT_CNT 32
#endif

amu_ctrl_event_t amu_ctrl_event_data;
static bool snapshot_admitted;

bool amu_snapshot_ready(void) {
  return snapshot_admitted;
}

void amu_snapshot_reset(void) {
  snapshot_admitted = false;
}

int amu_snapshot_negotiate(uint32_t version, uint32_t ctrl_size, uint32_t mma_size,
                           uint32_t event_prefix_size, uint32_t tile_bytes,
                           uint32_t acc_bytes) {
#if defined(CONFIG_RV_AME_FP4) && defined(CONFIG_FPU_SOFT) && \
    defined(CONFIG_DIFFTEST_AMU_CTRL) && !defined(CONFIG_SHARE_CTRL)
  snapshot_admitted = version == AMU_CTRL_SNAPSHOT_VERSION &&
    version == AMU_MMA_SNAPSHOT_VERSION && ctrl_size == sizeof(amu_ctrl_snapshot_t) &&
    mma_size == sizeof(amu_mma_snapshot_t) &&
    event_prefix_size == sizeof(amu_ctrl_event_t) && tile_bytes == TLEN / 8 &&
    acc_bytes == ALEN / 8;
#else
  snapshot_admitted = false;
#endif
  return snapshot_admitted ? 0 : -2;
}

amu_ctrl_event_t get_amu_ctrl_info() {
  return amu_ctrl_event_data;
}

__attribute__((unused))
static void print_amu_ctrl_event(amu_ctrl_event_t *event) {
  fprintf(stderr, "[NEMU] debug: amu_ctrl_event@pc: %016lx, op = %d\n", event->pc, event->op);
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

static bool cmp_amu_ctrl(amu_ctrl_event_t *l, amu_ctrl_event_t *r, bool compare_metadata) {
  bool cmp_mma = l->op == 0 && r->op == 0 && l->rm == r->rm && l->md == r->md
                 && l->sat == r->sat && l->isfp == r->isfp
                 && l->ms1 == r->ms1 && l->ms2 == r->ms2
                 && l->mtilem == r->mtilem && l->mtilen == r->mtilen && l->mtilek == r->mtilek
                 && l->types1 == r->types1 && l->types2 == r->types2 && l->typed == r->typed;
  bool cmp_mls = l->op == 1 && r->op == 1 && l->md == r->md && l->sat == r->sat
                 && l->isfp == r->isfp && l->types1 == r->types1 && l->types2 == r->types2
                 && l->base == r->base && l->stride == r->stride 
                 && l->mtilem == r->mtilem && l->mtilen == r->mtilen
                 && l->typed == r->typed;
  bool cmp_mrelease = l->op == 2 && r->op == 2 && l->mtilem == r->mtilem;
  bool cmp_mzero = l->op == 3 && r->op == 3 && l->md == r->md && l->base == r->base;
  bool cmp_pc = l->pc == r->pc;
  bool cmp_metadata = !compare_metadata ||
    (l->computeType == r->computeType && l->isScale == r->isScale &&
     l->isConvertF32 == r->isConvertF32 && l->isDynamicMxfpQuant == r->isDynamicMxfpQuant);
  return !((cmp_mma || cmp_mls || cmp_mrelease || cmp_mzero) && cmp_pc && cmp_metadata);
}

static bool amu_ctrl_has_extended_metadata(const amu_ctrl_event_t *event) {
  return event->computeType || event->isScale || event->isConvertF32 || event->isDynamicMxfpQuant;
}

static int check_amu_ctrl_impl(amu_ctrl_event_t *cmp, bool full_event) {
  if (!cmp) return -2;
  int result = 0;
  if (amu_ctrl_queue_empty()) {
    Log("NEMU does not commit any AMU ctrl signals.");
    memset(cmp, 0, full_event ? sizeof(*cmp) : offsetof(amu_ctrl_event_t, computeType));
    cmp->op = UINT8_MAX;
    result = -1;
  } else {
    amu_ctrl_event_data = amu_ctrl_queue_front();
    if (!full_event && amu_ctrl_has_extended_metadata(&amu_ctrl_event_data)) return -2;
    if (cmp_amu_ctrl(&amu_ctrl_event_data, cmp, full_event)) {
      memcpy(cmp, &amu_ctrl_event_data,
        full_event ? sizeof(*cmp) : offsetof(amu_ctrl_event_t, computeType));
      // There're differences between NEMU and DUT
      // replace them with NEMU's data
      cmp->op = amu_ctrl_event_data.op;
      cmp->md = amu_ctrl_event_data.md;
      cmp->sat = amu_ctrl_event_data.sat;
      cmp->isfp = amu_ctrl_event_data.isfp;
      cmp->mtilem = amu_ctrl_event_data.mtilem;
      cmp->mtilen = amu_ctrl_event_data.mtilen;
      cmp->pc = amu_ctrl_event_data.pc;
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

int check_amu_ctrl(amu_ctrl_event_t *cmp) {
  return check_amu_ctrl_impl(cmp, false);
}

int check_amu_ctrl_snapshot(amu_ctrl_event_t *cmp, amu_ctrl_snapshot_t *snapshot) {
  if (!cmp || !snapshot) return -2;
  if (!amu_snapshot_ready()) return -2;
  if (amu_ctrl_queue_empty()) return -1;
  const int result = amu_ctrl_queue_copy_snapshot(snapshot);
  if (result != 0) return result;
  return check_amu_ctrl_impl(cmp, true);
}

#if defined(CONFIG_RV_AME_FP4) && defined(CONFIG_FPU_SOFT) && \
    defined(CONFIG_DIFFTEST_AMU_CTRL) && !defined(CONFIG_SHARE_CTRL)
static bool valid_mma_view(const amu_matrix_view_t *view, size_t rows,
                           size_t pitch, size_t *bytes) {
  if (!view || !view->bytes || !rows || !pitch || view->reserved ||
      view->row_pitch != pitch || rows > SIZE_MAX / pitch) return false;
  *bytes = rows * pitch;
  return view->capacity >= *bytes;
}

static bool disjoint_mma_views(const amu_matrix_view_t *views[4],
                               const size_t sizes[4]) {
  uintptr_t starts[4], ends[4];
  for (unsigned index = 0; index < 4; ++index) {
    starts[index] = (uintptr_t)views[index]->bytes;
    if (sizes[index] > UINTPTR_MAX - starts[index]) return false;
    ends[index] = starts[index] + sizes[index];
  }
  for (unsigned left = 0; left < 4; ++left) {
    for (unsigned right = left + 1; right < 4; ++right) {
      if (starts[left] < ends[right] && starts[right] < ends[left]) return false;
    }
  }
  return true;
}
#endif

int exec_amu_scaled_mma_snapshot(const amu_ctrl_event_t *event,
                                 const uint8_t *scale_a, size_t scale_a_size,
                                 const uint8_t *scale_b, size_t scale_b_size,
                                 amu_mma_snapshot_t *snapshot) {
#if defined(CONFIG_RV_AME_FP4) && defined(CONFIG_FPU_SOFT) && \
    defined(CONFIG_DIFFTEST_AMU_CTRL) && !defined(CONFIG_SHARE_CTRL)
  if (!amu_snapshot_ready() || !event || !snapshot || !scale_a || !scale_b ||
      !snapshot->scales || snapshot->version != AMU_MMA_SNAPSHOT_VERSION ||
      snapshot->struct_size != sizeof(*snapshot) || snapshot->reserved ||
      snapshot->scales->version != AMU_CTRL_SNAPSHOT_VERSION ||
      snapshot->scales->struct_size != sizeof(*snapshot->scales) ||
      snapshot->scales->reserved ||
      snapshot->scales->raised_flags || snapshot->scales->expected_tr.bytes ||
      snapshot->scales->expected_tr.capacity || snapshot->scales->expected_tr.row_pitch ||
      snapshot->scales->expected_tr.reserved || !event->valid || event->op != 0 ||
      event->md < 4 || event->md >= 8 || event->ms1 >= 4 || event->ms2 >= 4 ||
      !event->isfp || event->sat || event->rm != 0 || event->isScale ||
      event->isConvertF32 || event->isDynamicMxfpQuant || event->base || event->stride) return -2;

  const uint8_t mode = event->computeType;
  const uint8_t source_type = mode == 7 ? MTYPECODE_FP8E4M3 :
    mode == 8 ? MTYPECODE_FP8E5M2 : mode == 10 ? MTYPECODE_MXFP4 : UINT8_MAX;
  if (source_type == UINT8_MAX || event->types1 != get_mma_op_amuctrl(source_type) ||
      event->types2 != get_mma_op_amuctrl(source_type) ||
      event->typed != get_mma_op_amuctrl(MTYPECODE_FP32) ||
      !event->mtilem || event->mtilem > ROWNUM || !event->mtilen ||
      event->mtilen > ROWNUM || event->mtilen > ARENUM32 || !event->mtilek ||
      event->mtilek > TRENUM8 * (mode == 10 ? 2 : 1)) return -2;

  const size_t rows_a = event->mtilem;
  const size_t rows_b = event->mtilen;
  const size_t blocks = (event->mtilek + AME_MX_BLOCK_SIZE - 1) / AME_MX_BLOCK_SIZE;
  const size_t ab_pitch = mode == 10 ? (event->mtilek + 1) / 2 : event->mtilek;
  const size_t c_pitch = event->mtilen * sizeof(uint32_t);
  if (blocks > AME_MAX_SCALE_BLOCKS || rows_a > SIZE_MAX / blocks ||
      rows_b > SIZE_MAX / blocks || rows_a > SIZE_MAX / ab_pitch ||
      rows_b > SIZE_MAX / ab_pitch || rows_a > SIZE_MAX / c_pitch) return -2;
  const size_t scale_bytes_a = rows_a * blocks;
  const size_t scale_bytes_b = rows_b * blocks;
  if (scale_a_size != scale_bytes_a || scale_b_size != scale_bytes_b ||
      ab_pitch > TRENUM8 || c_pitch > ARENUM32 * sizeof(uint32_t)) return -2;
  const amu_scale_snapshot_t *scales = snapshot->scales->scales;
  const amu_scale_metadata_t *meta_a = &scales[0].metadata;
  const amu_scale_metadata_t *meta_b = &scales[1].metadata;
  if (!meta_a->generation || !meta_b->generation ||
      meta_a->rows != rows_a || meta_b->rows != rows_b ||
      meta_a->blocks != blocks || meta_b->blocks != blocks ||
      meta_a->row_pitch != blocks || meta_b->row_pitch != blocks ||
      meta_a->reg_id != event->ms1 || meta_b->reg_id != event->ms2 ||
      meta_a->side != 0 || meta_b->side != 1 || !meta_a->valid || !meta_b->valid ||
      meta_a->reserved || meta_b->reserved ||
      meta_a->source_stride < blocks || meta_b->source_stride < blocks ||
      scales[0].capacity < scale_bytes_a || scales[1].capacity < scale_bytes_b ||
      !scales[0].bytes || !scales[1].bytes) return -2;
  for (size_t row = 0; row < rows_a; ++row) {
    if (memcmp(scales[0].bytes + row * blocks, scale_a + row * blocks, blocks) != 0) return -2;
  }
  for (size_t row = 0; row < rows_b; ++row) {
    if (memcmp(scales[1].bytes + row * blocks, scale_b + row * blocks, blocks) != 0) return -2;
  }

  const size_t view_rows[4] = {rows_a, rows_b, rows_a, rows_a};
  const size_t view_pitch[4] = {ab_pitch, ab_pitch, c_pitch, c_pitch};
  size_t view_bytes[4];
  const amu_matrix_view_t *views[4];
  for (unsigned index = 0; index < 4; ++index) {
    views[index] = &snapshot->matrices[index];
    if (!valid_mma_view(views[index], view_rows[index], view_pitch[index],
                        &view_bytes[index])) return -2;
  }
  if (!disjoint_mma_views(views, view_bytes)) return -2;

  uint32_t output[ROWNUM][ARENUM32];
  uint8_t raised_flags = 0;
  for (size_t row = 0; row < rows_a; ++row) {
    for (size_t col = 0; col < rows_b; ++col) {
      const uint32_t acc = cpu.macc[event->md - 4][row]._32[col];
      output[row][col] = mxfp_mmacc_element(mode,
        acc, cpu.mtr[event->ms1][row]._8, cpu.mtr[event->ms2][col]._8,
        scale_a + row * blocks, scale_b + col * blocks,
        event->mtilek, &raised_flags);
    }
  }

  for (size_t row = 0; row < rows_a; ++row) {
    memcpy(snapshot->matrices[0].bytes + row * ab_pitch,
      cpu.mtr[event->ms1][row]._8, ab_pitch);
    memcpy(snapshot->matrices[2].bytes + row * c_pitch,
      cpu.macc[event->md - 4][row]._32, c_pitch);
    memcpy(snapshot->matrices[3].bytes + row * c_pitch,
      output[row], c_pitch);
    memcpy(cpu.macc[event->md - 4][row]._32, output[row], c_pitch);
  }
  for (size_t row = 0; row < rows_b; ++row) {
    memcpy(snapshot->matrices[1].bytes + row * ab_pitch,
      cpu.mtr[event->ms2][row]._8, ab_pitch);
  }
  mfflags->val |= raised_flags;
  mcsr->mfflags = mfflags->val;
  snapshot->raised_flags = raised_flags;
  return 0;
#else
  (void)event;
  (void)scale_a;
  (void)scale_a_size;
  (void)scale_b;
  (void)scale_b_size;
  (void)snapshot;
  return -2;
#endif
}

int exec_amu_lazy_snapshot(const amu_ctrl_event_t *event, amu_mma_snapshot_t *snapshot) {
  if (!amu_snapshot_ready()) return -2;
  if (event && event->isDynamicMxfpQuant) {
    return amu_ctrl_queue_apply_dynamic_snapshot(event, snapshot);
  }
  if (event && event->op == 0 && event->computeType) {
    return amu_ctrl_queue_apply_scaled_mma_snapshot(event, snapshot);
  }
  return -2;
}

static void exec_amu_load_store(void *amu_ctrl) {
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
  if (msew == AME_MXFP4_MLS_TYPED) {
#ifdef CONFIG_RV_AME_FP4
    Assert(ls == 0 && !transpose && m_name != 'c' &&
           raw_fp4_matrix_load_paddr(base, stride, row, column, false, ms),
           "Invalid packed MXFP4 AMU load");
#else
    Assert(false, "Packed MXFP4 AMU load without FP4 support");
#endif
  } else if (ls == 0) { // matrix load
    pmem_read_matrix(base, stride, row, column, msew, transpose, m_name, ms);
  } else { // matrix store
    Assert(ls == 1, "AmuCtrl.ls should be 0 or 1!");
    pmem_write_matrix(base, stride, row, column, msew, transpose, m_name, ms);
  }
}

static void exec_amu_release(void *amu_ctrl) {
  uint8_t msyncRd = check_mtok_idx(((amu_ctrl_event_t *)amu_ctrl)->mtilem);
  cpu.mtokr[msyncRd]++;
#ifdef CONFIG_AME_MEM_ACCESS_CHECK
  mstore_queue_update_mrelease(msyncRd, cpu.mtokr[msyncRd]);
#endif // CONFIG_AME_MEM_ACCESS_CHECK
}

static void exec_amu_arith(void *amu_ctrl) {
  uint8_t md = ((amu_ctrl_event_t *)amu_ctrl)->md;
  if (md < 4) {
    memset(cpu.mtr[md], 0, TLEN / 8);
  } else {
    memset(cpu.macc[md - 4], 0, ALEN / 8);
  }
}

static bool get_amu_write_region(const amu_ctrl_event_t *event, amu_write_region_t *region) {
  if (event == NULL || region == NULL || event->md >= 8) return false;

  const bool is_acc = event->md >= 4;
  region->reg_data = is_acc ? (const uint8_t *)cpu.macc[event->md - 4]
                            : (const uint8_t *)cpu.mtr[event->md];
  region->reg_size_bytes = is_acc ? ALEN / 8 : TLEN / 8;
  region->reg_row_bytes = is_acc ? ARLEN / 8 : TRLEN / 8;

  if (event->op == 3) {
    // mzero writes the whole register; use 64-bit units for diagnostics.
    region->active_rows = region->reg_size_bytes / region->reg_row_bytes;
    region->active_columns = region->reg_row_bytes / sizeof(uint64_t);
    region->element_bytes = sizeof(uint64_t);
#ifdef CONFIG_RV_AME_FP4
  } else if (event->op == 1 && event->sat == 0 && event->typed == AME_MXFP4_MLS_TYPED &&
             !event->isfp && !event->types1 && event->md < 4 &&
             ((event->types2 && event->mtilem == 8 && event->mtilen == 64) ||
              (!event->types2 && event->mtilem == 64 && event->mtilen == 8))) {
    region->active_rows = 8;
    region->active_columns = 32;
    region->element_bytes = 1;
#endif
  } else if (event->op == 1 && event->sat == 0 && event->typed <= 3) {
    const bool is_b = !event->types1 && !event->types2;
    region->active_rows = is_b ? event->mtilen : event->mtilem;
    region->active_columns = is_b ? event->mtilem : event->mtilen;
    region->element_bytes = (size_t)1 << event->typed;
  } else {
    return false;
  }

  return region->active_rows <= region->reg_size_bytes / region->reg_row_bytes &&
         region->active_columns <= region->reg_row_bytes / region->element_bytes;
}

static bool compare_amu_hash(const amu_ctrl_event_t *event, const amu_hash_t *dut) {
  amu_write_region_t region;
  Assert(get_amu_write_region(event, &region), "Invalid AMU write region");
  const amu_hash_t ref = hash_amu_region(&region);
  if (ref.lo == dut->lo && ref.hi == dut->hi && ref.bytes == dut->bytes) return false;

  fprintf(stderr, "Matrix hash mismatch: md=%d @pc: %016lx REF=%016lx:%016lx DUT=%016lx:%016lx "
                  "REF_bytes=%u DUT_bytes=%u\n",
          event->md, event->pc, ref.hi, ref.lo, dut->hi, dut->lo, ref.bytes, dut->bytes);
  return true;
}

static bool compare_amu_register(const amu_ctrl_event_t *event, const void *dut_result) {
  amu_write_region_t region;
  Assert(get_amu_write_region(event, &region), "Invalid AMU write region");
  if (region.active_rows == 0 || region.active_columns == 0) return false;
  Assert(dut_result != NULL, "Missing DUT result for AMU comparison");

  const uint8_t *dut = dut_result;
  const size_t active_row_bytes = region.active_columns * region.element_bytes;
  bool different = false;
  // Keep the common equal case in memcmp; full-width rows are contiguous.
  if (active_row_bytes == region.reg_row_bytes) {
    different = memcmp(region.reg_data, dut, region.active_rows * region.reg_row_bytes) != 0;
  } else {
    for (size_t row = 0; row < region.active_rows; ++row) {
      const size_t offset = row * region.reg_row_bytes;
      if (memcmp(region.reg_data + offset, dut + offset, active_row_bytes) != 0) {
        different = true;
        break;
      }
    }
  }
  if (!different) return false;

  // Scan individual elements only on mismatch, and only inside the write region.
  fprintf(stderr, "matrix diff md=%d @pc: %016lx\n", event->md, event->pc);
  int total_diff = 0;
  int printed = 0;
  for (size_t row = 0; row < region.active_rows; ++row) {
    for (size_t column = 0; column < region.active_columns; ++column) {
      const size_t offset = row * region.reg_row_bytes + column * region.element_bytes;
      uint64_t ref_val = 0;
      uint64_t dut_val = 0;
      memcpy(&ref_val, region.reg_data + offset, region.element_bytes);
      memcpy(&dut_val, dut + offset, region.element_bytes);
      if (ref_val != dut_val) {
        total_diff++;
        if (printed < MATRIX_DIFF_PRINT_CNT) {
          fprintf(stderr, "  [row=%zu, column=%zu] REF=%0*lx DUT=%0*lx\n",
                  row, column, (int)(region.element_bytes * 2), (unsigned long)ref_val,
                  (int)(region.element_bytes * 2), (unsigned long)dut_val);
          printed++;
        }
      }
    }
  }

  fprintf(stderr, "diff summary: %d element(s) differ\n", total_diff);
  if (total_diff > MATRIX_DIFF_PRINT_CNT) {
    fprintf(stderr, "  ... and %d more (change MATRIX_DIFF_PRINT_CNT to show more)\n",
            total_diff - MATRIX_DIFF_PRINT_CNT);
  }
  return true;
}

int exec_amu(void *amu_ctrl, void *res) {
  bool ret = 0;
  amu_ctrl_event_t *event = (amu_ctrl_event_t *)amu_ctrl;
  uint8_t op = event->op;
  switch (op) {
    case 0: // case MMA
      panic("MMA should be executed by exec_amu_lazy");
      ret = 1;
      break;
    case 1: // case Matrix load/store
      exec_amu_load_store(amu_ctrl);
      // Compare the result with the expected result
      // When the result is not equal, set the return value to 1
      if (event->sat == 0) {
        // only check matrix load
        ret = compare_amu_register(event, res);
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
      ret = compare_amu_register(event, res);
      break;
    default:
      panic("invalid AMU ctrl op");
      ret = 1;
      break;
  }
  return ret;
}

int exec_amu_hash(void *amu_ctrl, amu_hash_t *res) {
  bool ret = 0;
  amu_ctrl_event_t *event = (amu_ctrl_event_t *)amu_ctrl;
  uint8_t op = event->op;
  switch (op) {
    case 0: // case MMA
      panic("MMA should be executed by exec_amu_lazy");
      ret = 1;
      break;
    case 1: // case Matrix load/store
      exec_amu_load_store(amu_ctrl);
      if (event->sat == 0) {
        ret = compare_amu_hash(event, res);
      }
      break;
    case 2: // case Mrelease
      exec_amu_release(amu_ctrl);
      break;
    case 3: // case Marith, execute the operation
      exec_amu_arith(amu_ctrl);
      ret = compare_amu_hash(event, res);
      break;
    default:
      panic("invalid AMU ctrl op");
      ret = 1;
      break;
  }
  return ret;
}

void exec_amu_lazy(void *amu_ctrl, void *res, void *src1, void *src2, void *src3) {
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

#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
