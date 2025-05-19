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
  if (event->op == 0) {
    fprintf(stderr, "  md: %d, sat: %d, ms1: %d, ms2: %d\n"
                    "  mtilem: %d, mtilen: %d, mtilek: %d, types: %d, typed: %d\n",
                    event->md, event->sat, event->ms1, event->ms2,
                    event->mtilem, event->mtilen, event->mtilek, event->types, event->typed);
  } else if (event->op == 1) {
    fprintf(stderr, "  ms: %d, ls: %d, transpose: %d, isacc: %d\n"
                    "  base: %016lx, stride: %016lx, row: %d, column: %d, msew: %d\n",
                    event->md, event->sat, event->transpose, event->isacc,
                    event->base, event->stride, event->mtilem, event->mtilen, event->types);
  } else {
    fprintf(stderr, "  unknown op!\n");
  }
  
}

static bool cmp_amu_ctrl(amu_ctrl_event_t *l, amu_ctrl_event_t *r) {
  bool cmp_mma = l->op == 0 && r->op == 0 && l->md == r->md && l->sat == r->sat
                 && l->ms1 == r->ms1 && l->ms2 == r->ms2
                 && l->mtilem == r->mtilem && l->mtilen == r->mtilen && l->mtilek == r->mtilek
                 && l->types == r->types && l->typed == r->typed;
  bool cmp_mls = l->op == 1 && r->op == 1 && l->md == r->md && l->sat == r->sat
                 && l->transpose == r->transpose && l->isacc == r->isacc
                 && l->base == r->base && l->stride == r->stride 
                 && l->mtilem == r->mtilem && l->mtilen == r->mtilen
                 && l->types == r->types;
  return !(cmp_mma || cmp_mls);
}

int check_amu_ctrl(amu_ctrl_event_t *cmp) {
  int result = 0;
  if (amu_ctrl_queue_empty()) {
    printf("NEMU does not commit any AMU ctrl signals.\n");
    result = 1;
  } else {
    amu_ctrl_event_data = amu_ctrl_queue_front();
    amu_ctrl_queue_pop();

    if (cmp_amu_ctrl(&amu_ctrl_event_data, cmp)) {
      // There're differences between NEMU and DUT
      // replace them with NEMU's data
      if (amu_ctrl_event_data.op == 0) {
        // case MMA
        cmp->md = amu_ctrl_event_data.md;
        cmp->sat = amu_ctrl_event_data.sat;
        cmp->ms1 = amu_ctrl_event_data.ms1;
        cmp->ms2 = amu_ctrl_event_data.ms2;
        cmp->mtilem = amu_ctrl_event_data.mtilem;
        cmp->mtilen = amu_ctrl_event_data.mtilen;
        cmp->mtilek = amu_ctrl_event_data.mtilek;
        cmp->types = amu_ctrl_event_data.types;
        cmp->typed = amu_ctrl_event_data.typed;
      } else {
        // case Matrix load/store
        cmp->md = amu_ctrl_event_data.md;
        cmp->sat = amu_ctrl_event_data.sat;
        cmp->transpose = amu_ctrl_event_data.transpose;
        cmp->isacc = amu_ctrl_event_data.isacc;
        cmp->base = amu_ctrl_event_data.base;
        cmp->stride = amu_ctrl_event_data.stride;
        cmp->mtilem = amu_ctrl_event_data.mtilem;
        cmp->mtilen = amu_ctrl_event_data.mtilen;
        cmp->types = amu_ctrl_event_data.types;
      }
      result = 1;
    }
  }
  return result;
}

#endif // CONFIG_RVMATRIX
