#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

  char *expr;
  word_t new_val;
  word_t old_val;
} WP;

int set_watchpoint(char *e);
bool delete_watchpoint(int NO);
void list_watchpoint();
WP* scan_watchpoint();

#endif
