#include <ext/amuctrl.h>
#include <ext/amu_ctrl_queue_wrapper.h>

#ifdef CONFIG_RVMATRIX

amu_ctrl_event_t amu_ctrl_event_data;

amu_ctrl_event_t get_amu_ctrl_info() {
  return amu_ctrl_event_data;
}

__attribute__((unused))
static void print_amu_ctrl_event(amu_ctrl_event_t *event) {
  fprintf(stderr, "[NEMU] debug: amu_ctrl_event@pc: %016lx, op = %d\n", event->pc, event->op);
  switch (event->op) {
    case 0:
      fprintf(stderr, "  md: %d, sat: %d, ms1: %d, ms2: %d\n"
                      "  mtilem: %d, mtilen: %d, mtilek: %d, types: %d, typed: %d\n",
                      event->md, event->sat, event->ms1, event->ms2,
                      event->mtilem, event->mtilen, event->mtilek, event->types, event->typed);
      break;
    case 1:
      fprintf(stderr, "  ms: %d, ls: %d, transpose: %d, isacc: %d\n"
                      "  base: %016lx, stride: %016lx, row: %d, column: %d, msew: %d\n",
                      event->md, event->sat, event->isfp, event->isacc,
                      event->base, event->stride, event->mtilem, event->mtilen, event->types);
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
  bool cmp_mma = l->op == 0 && r->op == 0 && l->md == r->md && l->sat == r->sat
                 && l->isfp == r->isfp
                 && l->ms1 == r->ms1 && l->ms2 == r->ms2
                 && l->mtilem == r->mtilem && l->mtilen == r->mtilen && l->mtilek == r->mtilek
                 && l->types == r->types && l->typed == r->typed;
  bool cmp_mls = l->op == 1 && r->op == 1 && l->md == r->md && l->sat == r->sat
                 && l->isfp == r->isfp && l->isacc == r->isacc
                 && l->base == r->base && l->stride == r->stride 
                 && l->mtilem == r->mtilem && l->mtilen == r->mtilen
                 && l->types == r->types;
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
    cmp->md = 0;
    cmp->sat = 0;
    cmp->isfp = 0;
    cmp->ms1 = 0;
    cmp->ms2 = 0;
    cmp->mtilem = 0;
    cmp->mtilen = 0;
    cmp->mtilek = 0;
    cmp->types = 0;
    cmp->typed = 0;
    cmp->isacc = 0;
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
      cmp->types = amu_ctrl_event_data.types;
      cmp->pc = amu_ctrl_event_data.pc;
      switch (amu_ctrl_event_data.op) {
        case 0:
          // case MMA
          cmp->ms1 = amu_ctrl_event_data.ms1;
          cmp->ms2 = amu_ctrl_event_data.ms2;
          cmp->mtilek = amu_ctrl_event_data.mtilek;
          cmp->typed = amu_ctrl_event_data.typed;
          break;
        case 1:
          // case Matrix load/store
          cmp->isacc = amu_ctrl_event_data.isacc;
          cmp->base = amu_ctrl_event_data.base;
          cmp->stride = amu_ctrl_event_data.stride;
          break;
        case 2:
          // case Mrelease
          cmp->mtilem = amu_ctrl_event_data.mtilem;
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

#endif // CONFIG_RVMATRIX
