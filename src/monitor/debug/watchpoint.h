#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include <common.h>

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

#ifndef __ICS_EXPORT
  char *expr;
  word_t new_val;
  word_t old_val;
#endif
} WP;

#ifndef __ICS_EXPORT
int set_watchpoint(char *e);
bool delete_watchpoint(int NO);
void list_watchpoint();
WP* scan_watchpoint();
#endif

#endif
