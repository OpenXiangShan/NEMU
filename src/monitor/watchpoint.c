#include <common.h>
#ifndef __ICS_EXPORT
#include <stdlib.h>
#endif

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

#ifndef __ICS_EXPORT
  char *expr;
  word_t old_val;
#endif
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

#ifndef __ICS_EXPORT
static WP* new_WP() {
  assert(free_ != NULL);
  WP *p = free_;
  free_ = free_->next;
  return p;
}

static void free_WP(WP *p) {
  assert(p >= wp_pool && p < wp_pool + NR_WP);
  free(p->expr);
  p->next = free_;
  free_ = p;
}

int set_watchpoint(char *e) {
  bool success;
  word_t val = expr(e, &success);
  if (!success) return -1;

  WP *p = new_WP();
  p->expr = strdup(e);
  p->old_val = val;

  p->next = head;
  head = p;

  return p->NO;
}

bool delete_watchpoint(int NO) {
  WP *p, *prev = NULL;
  for (p = head; p != NULL; prev = p, p = p->next) {
    if (p->NO == NO) { break; }
  }

  if (p == NULL) { return false; }
  if (prev == NULL) { head = p->next; }
  else { prev->next = p->next; }

  free_WP(p);
  return true;
}

void list_watchpoint() {
  if (head == NULL) {
    printf("No watchpoints\n");
    return;
  }

  printf("%8s\t%8s\t%8s\n", "NO", "Address", "Enable");
  WP *p;
  for (p = head; p != NULL; p = p->next) {
    printf("%8d\t%s\t" FMT_WORD "\n", p->NO, p->expr, p->old_val);
  }
}

void scan_watchpoint(vaddr_t pc) {
  WP *p;
  for (p = head; p != NULL; p = p->next) {
    bool success;
    word_t new_val = expr(p->expr, &success);
    if (p->old_val != new_val) {
      printf("\n\nHint watchpoint %d at address " FMT_WORD ", expr = %s\n", p->NO, pc, p->expr);
      printf("old value = " FMT_WORD "\nnew value = " FMT_WORD "\n", p->old_val, new_val);
      p->old_val = new_val;
      nemu_state.state = NEMU_STOP;
      return;
    }
  }
}
#endif
