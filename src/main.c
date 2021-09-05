void init_monitor(int, char *[]);
void init_keymap();
void engine_start();
int goodtrap(void);
int is_batch_mode();

void init_path_manger();
void init_simpoint();
void init_profiler();
void init_serializer();

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
  init_monitor(argc, argv);

  init_path_manger();
  init_simpoint();
  init_profiler();
  init_serializer();

  init_keymap();

  /* Start engine. */
  engine_start();


  return (is_batch_mode() ? !goodtrap() : 0);
}
