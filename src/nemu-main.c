#include <common.h>
#ifndef CONFIG_SHARE
void init_monitor(int, char *[]);
void engine_start();
int is_exit_status_bad();

void clean_up_outputs() {
  extern void close_tracer();
  close_tracer();
}

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
  init_monitor(argc, argv);

  /* Start engine. */
  engine_start();

  clean_up_outputs();
  return is_exit_status_bad();
}
#endif