void init_monitor(int, char *[]);
void mainloop();
int goodtrap(void);
int is_batch_mode();

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
  init_monitor(argc, argv);

  /* Receive commands from user. */
  mainloop();

  return (is_batch_mode() ? !goodtrap() : 0);
}
