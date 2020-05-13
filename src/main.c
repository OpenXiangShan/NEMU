void init_monitor(int, char *[]);
void engine_start();
int goodtrap(void);
int is_batch_mode();

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
  init_monitor(argc, argv);

  /* Start engine. */
  engine_start();

  return (is_batch_mode() ? !goodtrap() : 0);
}
