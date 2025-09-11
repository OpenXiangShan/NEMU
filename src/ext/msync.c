#include <ext/msync.h>
#include <ext/msync_queue_wrapper.h>

#ifdef CONFIG_RVMATRIX

msync_event_t msync_event_data;

msync_event_t get_msync_info() {
  return msync_event_data;
}

__attribute__((unused))
static void print_msync_event(msync_event_t *event) {
  fprintf(stderr, "[NEMU] debug: msync_event@pc: %016lx, op = %d, token = %d\n", event->pc, event->op, event->token);
}

static bool cmp_msync(msync_event_t *l, msync_event_t *r) {
  return l->token != r->token || l->op != r->op || l->pc != r->pc;
}

int check_msync(msync_event_t *cmp) {
  int result = 0;
  if (msync_queue_empty()) {
    Log("NEMU does not commit any MSYNC signals.");
    cmp->op = -1;
    cmp->token = 0;
    cmp->pc = 0;
    result = -1;
  } else {
    msync_event_data = msync_queue_front();
    msync_queue_pop();
    if (cmp_msync(&msync_event_data, cmp)) {
      cmp->op = msync_event_data.op;
      cmp->token = msync_event_data.token;
      cmp->pc = msync_event_data.pc;
      result = 1;
    }
  }
  return result;
}

#endif // CONFIG_RVMATRIX