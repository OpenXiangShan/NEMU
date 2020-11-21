void init_monitor(int, char *[]);
void init_csr_exist();
void init_keymap();
void engine_start();
int goodtrap(void);
int is_batch_mode();

void init_path_manger();
void init_simpoint();

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
  init_monitor(argc, argv);

  init_path_manger();
  init_simpoint();

  init_csr_exist();
  init_keymap();

  /* Start engine. */
  engine_start();


  return (is_batch_mode() ? !goodtrap() : 0);
}
