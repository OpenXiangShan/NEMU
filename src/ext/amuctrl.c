#include <ext/amuctrl.h>
#include <ext/amu_ctrl_queue_wrapper.h>
#include <ext/mstore_queue_wrapper.h>
#include <isa.h>
#include <memory/host.h>

#ifdef CONFIG_RVMATRIX

#ifndef MATRIX_DIFF_PRINT_CNT
#define MATRIX_DIFF_PRINT_CNT 32
#endif

amu_ctrl_event_t amu_ctrl_event_data;

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
      fprintf(stderr, "  tokenRd: %d\n", event->mtilem);
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
                 && l->base == r->base && l->stride == r->stride 
                 && l->mtilem == r->mtilem && l->mtilen == r->mtilen
                 && l->typed == r->typed;
  bool cmp_mrelease = l->op == 2 && r->op == 2 && l->mtilem == r->mtilem;
  bool cmp_mzero = l->op == 3 && r->op == 3 && l->md == r->md && l->base == r->base;
  bool cmp_pc = l->pc == r->pc;
  return !((cmp_mma || cmp_mls || cmp_mrelease || cmp_mzero) && cmp_pc);
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
    result = -1;
  } else {
    amu_ctrl_event_data = amu_ctrl_queue_front();
    amu_ctrl_queue_pop();
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
  }
  return result;
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
  if (ls == 0) { // matrix load
    host_read_matrix(base, stride, row, column, msew, transpose, m_name, ms);
  } else { // matrix store
    Assert(ls == 1, "AmuCtrl.ls should be 0 or 1!");
    host_write_matrix(base, stride, row, column, msew, transpose, m_name, ms);
  }
}

static void exec_amu_release(void *amu_ctrl) {
  uint8_t tokenRd = check_mtok_idx(((amu_ctrl_event_t *)amu_ctrl)->mtilem);
  cpu.mtokr[tokenRd]++;
  mstore_queue_update_mrelease(tokenRd, cpu.mtokr[tokenRd]);
}

static void exec_amu_arith(void *amu_ctrl) {
  uint8_t md = ((amu_ctrl_event_t *)amu_ctrl)->md;
  if (md < 4) {
    memset(cpu.mtr[md], 0, TLEN / 8);
  } else {
    memset(cpu.macc[md - 4], 0, ALEN / 8);
  }
}

int exec_amu(void *amu_ctrl, void *res) {
  bool ret = 0;
  uint8_t op = ((amu_ctrl_event_t *)amu_ctrl)->op;
  uint8_t md = ((amu_ctrl_event_t *)amu_ctrl)->md;
  switch (op) {
    case 0: // case MMA
      panic("MMA should be executed by exec_amu_lazy");
      ret = 1;
      break;
    case 1: // case Matrix load/store
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

#endif // CONFIG_RVMATRIX
